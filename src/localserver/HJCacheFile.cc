//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJCacheFile.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJXIOFile.h"
#include "HJFileUtil.h"
#include "HJMediaUtils.h"
#include "HJServerUtils.h"
#include "HJMediaStorageManager.h"
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//
HJCacheFile::HJCacheFile()
{
	m_executor = HJCreateExecutor(HJMakeGlobalName("HJCacheFile"));
    if(!m_executor) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, "error, create executor");
    }
}

HJCacheFile::~HJCacheFile()
{
	close();
}

/**
 * @brief 
 * @param url 
 * @param rid
 * @param cacheDir 
 * @param preCacheSize
 * @return int 
 */
int HJCacheFile::open(HJUrl::Ptr murl)
{
	if (!murl || murl->getUrl().empty()) {
		return HJErrInvalidParams;
	}
    int res = HJXIOBase::open(murl);
	if (HJ_OK != res) {
		return res;
	}
    getOptions(murl);
    if (m_remoteUrl.empty() || m_localUrl.empty()) {
        HJFLoge("invalid cache file option. rid:{}, remote:{}, local:{}", m_rid, m_remoteUrl, m_localUrl);
        return HJErrInvalidParams;
    }
    if ((res = ensureLocalFile()) != HJ_OK) {
        HJFLoge("ensure local file failed, rid:{}, res:{}", m_rid, res);
        return res;
    }
    if ((res = ensureFetcher()) != HJ_OK) {
        HJFLoge("ensure remote metadata failed, rid:{}, res:{}", m_rid, res);
        return res;
    }
    schedulePrefetch();
    HJFLogi("open cache file rid:{}, remote:{}, local:{}, status:{}, preCache:{}, total_length:{}",
        m_rid, m_remoteUrl, m_localUrl, (int)m_fileStatus, m_preCacheSize, m_fileLength);
    return HJ_OK;
}

void HJCacheFile::close()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
        m_blockCv.notify_all();
    }
    if (m_fetch) {
        m_fetch->done();
        m_fetch = nullptr;
    }
    {
        std::lock_guard<std::mutex> fileLock(m_fileMutex);
        if (m_localFile) {
            m_localFile->flush();
            m_localFile->close();
            m_localFile = nullptr;
        }
    }
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        persistDBInfoLocked();
    }
}
int HJCacheFile::read(void* buffer, size_t cnt)
{
    if (!buffer || cnt == 0) {
        return HJErrInvalidParams;
    }
    if (!m_localFile) {
        return HJErrNotAlready;
    }
    size_t total = 0;
    while (total < cnt) {
        int targetBlock = -1;
        size_t expect = cnt - total;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_fileLength > 0 && m_pos >= static_cast<size_t>(m_fileLength) && m_fileStatus == HJFileStatus::COMPLETED) {
                break;
            }
            targetBlock = blockIndexByOffset(static_cast<int64_t>(m_pos));
            if (targetBlock < 0 || targetBlock >= blockCount()) {
                break;
            }
            if (!isBlockReadyLocked(targetBlock)) {
                enqueueBlockLocked(targetBlock, true);
                lock.unlock();
                scheduleFetchLoop();
                lock.lock();
                m_blockCv.wait(lock, [this, targetBlock]() {
                    return m_stop || isBlockReadyLocked(targetBlock);
                });
                if (m_stop) {
                    break;
                }
                if (!isBlockReadyLocked(targetBlock)) {
                    continue;
                }
            }
        }
        const int64_t blockStart = blockStartOffset(targetBlock);
        const size_t blockLen = blockLength(targetBlock);
        if (blockStart < 0 || blockLen == 0) {
            break;
        }
        size_t offsetInside = 0;
        if (static_cast<int64_t>(m_pos) > blockStart) {
            offsetInside = static_cast<size_t>(static_cast<int64_t>(m_pos) - blockStart);
        }
        const size_t readable = std::min(blockLen - offsetInside, expect);
        int rcnt = 0;
        {
            std::lock_guard<std::mutex> fileLock(m_fileMutex);
            if (HJ_OK != m_localFile->seek(static_cast<int64_t>(m_pos), std::ios::beg)) {
                return HJErrIOSeek;
            }
            rcnt = m_localFile->read(static_cast<uint8_t*>(buffer) + total, readable);
        }
        if (rcnt <= 0) {
            break;
        }
        total += static_cast<size_t>(rcnt);
        m_pos += static_cast<size_t>(rcnt);
        if (static_cast<size_t>(rcnt) < readable) {
            break;
        }
    }
    if (total > 0) {
        return static_cast<int>(total);
    }
    return 0;
}
 int HJCacheFile::write(const void* buffer, size_t cnt)
 {
     HJ_UNUSED(buffer);
     HJ_UNUSED(cnt);
     return HJErrNotSupport;
 }
int HJCacheFile::seek(int64_t offset, int whence)
{
    if (!m_localFile) {
        return HJErrNotAlready;
    }
    std::lock_guard<std::mutex> fileLock(m_fileMutex);
    int ret = m_localFile->seek(offset, whence);
    if (HJ_OK == ret) {
        m_pos = static_cast<size_t>(m_localFile->tell());
    }
    return ret;
}

 int HJCacheFile::flush()
 {
     std::lock_guard<std::mutex> fileLock(m_fileMutex);
     if (!m_localFile) {
         return HJErrNotAlready;
     }
     return m_localFile->flush();
 }

int64_t HJCacheFile::tell()
{
    return static_cast<int64_t>(m_pos);
}

int64_t HJCacheFile::size()
{
    if (m_fileLength > 0) {
        return m_fileLength;
    }
    std::lock_guard<std::mutex> fileLock(m_fileMutex);
    if (m_localFile) {
        return m_localFile->size();
    }
    return 0;
}

bool HJCacheFile::eof()
{
    if (m_fileStatus != HJFileStatus::COMPLETED) {
        return false;
    }
    return m_pos >= static_cast<size_t>(m_fileLength);
}

void HJCacheFile::getOptions(const HJUrl::Ptr& murl)
{
    m_rid = murl->getString("rid");
    m_cacheDir = murl->getString("cacheDir");
    m_preCacheSize = murl->getInt("preCacheSize");
    m_category = murl->getString("category");
    if(murl->haveValue("dbInfo")) {
        m_dbinfo = murl->getValue<HJDBFileInfo>("dbInfo");
    }
    m_remoteUrl = murl->getUrl();
    m_localUrl = HJMediaUtils::getLocalUrl(m_cacheDir, m_remoteUrl, m_rid);
    if (!m_dbinfo.has_value()) 
    {
        HJFLogi("create dbInfo for {}", m_remoteUrl);
        HJDBFileInfo dbInfo;
        dbInfo.rid = m_rid;
        dbInfo.url = m_remoteUrl;
        dbInfo.local_url = m_localUrl;
        dbInfo.status = static_cast<int>(HJFileStatus::NONE);
        dbInfo.size = 0;
        // dbInfo.total_length = 0;
        dbInfo.category = m_category;
        dbInfo.create_time = HJCurrentEpochMS();
        dbInfo.modify_time = dbInfo.create_time;
        // dbInfo.use_count = 0;
        dbInfo.block_size = HJ_FILE_BLOCK_SIZE_DEFAULT;
        // dbInfo.block_status.resize(dbInfo.blockCount(), false);
        //
        m_dbinfo = dbInfo;
    }

    m_blockSize = static_cast<size_t>((*m_dbinfo).block_size);
    m_fileStatus = static_cast<HJFileStatus>((*m_dbinfo).status);
    if (m_fileStatus == HJFileStatus::NONE && HJFileUtil::fileExist(m_localUrl)) {
        m_fileStatus = HJFileStatus::PENDING;
    }
    if(HJFileStatus::COMPLETED == m_fileStatus) {
        m_fileLength = (*m_dbinfo).total_length;
    }
    return;
}

int HJCacheFile::ensureLocalFile()
{
    if (m_localFile) {
        return HJ_OK;
    }
    if (m_localUrl.empty()) {
        HJFLoge("invalid local url, {}", m_localUrl);
        return HJErrInvalidParams;
    }
    m_localFile = HJCreateu<HJXIOFile>();
    auto url = HJCreates<HJUrl>(m_localUrl, HJ_XIO_RW);
    int res = m_localFile->open(url);
    if (HJ_OK != res) {
        m_localFile = nullptr;
        HJFLoge("open local file failed, {}", res);
        return res;
    }
    if(HJFileStatus::COMPLETED == m_fileStatus && m_fileLength != m_localFile->size()) {
        HJFLogw("local file size not match, rid:{}, local:{}, remote:{}", m_rid, m_localFile->size(), m_fileLength);
    }
    return HJ_OK;
}

int HJCacheFile::ensureFetcher()
{
    if(HJFileStatus::COMPLETED == m_fileStatus || m_fetch) {
        return HJ_OK;
    }
    m_fetch = createFetch();
    if (!m_fetch) {
        HJFLoge("create net fetch failed, rid:{}", m_rid);
        return HJErrNewObj;
    }
    persistDBInfoLocked();

    return HJ_OK;
}

void HJCacheFile::schedulePrefetch()
{
    if (m_fileLength <= 0 || !m_dbinfo.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_fileStatus == HJFileStatus::COMPLETED) {
        return;
    }
    if (m_preCacheSize > 0) {
        prefetchHeadLocked(static_cast<size_t>(m_preCacheSize));
    }
    prefetchTailLocked();
    lock.unlock();

    scheduleFetchLoop();
}

void HJCacheFile::prefetchHeadLocked(size_t bytes)
{
    if (bytes == 0) {
        return;
    }
    size_t scheduled = 0;
    const int totalBlocks = blockCount();
    for (int idx = 0; idx < totalBlocks && scheduled < bytes; ++idx) {
        if (isBlockReadyLocked(idx)) {
            scheduled += blockLength(idx);
            continue;
        }
        enqueueBlockLocked(idx, false);
        scheduled += blockLength(idx);
    }
}

void HJCacheFile::prefetchTailLocked()
{
    if (m_fileLength <= 0) {
        return;
    }
    const int totalBlocks = blockCount();
    if (totalBlocks <= 0) {
        return;
    }
    int64_t tailBytes = m_fileLength * m_tailPercentage;
    // if (tailBytes < static_cast<int64_t>(m_blockSize)) {
    //     tailBytes = static_cast<int64_t>(m_blockSize);
    // }
    int64_t tailStart = std::max<int64_t>(0, m_fileLength - tailBytes);
    const int startIndex = blockIndexByOffset(tailStart);
    for (int idx = startIndex; idx < totalBlocks; ++idx) {
        if (isBlockReadyLocked(idx)) {
            continue;
        }
        enqueueBlockLocked(idx, false);
    }
}

void HJCacheFile::requestBlock(int blockIndex, bool highPriority)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    enqueueBlockLocked(blockIndex, highPriority);
    scheduleFetchLoop();
}

void HJCacheFile::enqueueBlockLocked(int blockIndex, bool highPriority)
{
    if (blockIndex < 0 || blockIndex >= blockCount()) {
        return;
    }
    if (isBlockReadyLocked(blockIndex)) {
        return;
    }
    if (m_blockPending.find(blockIndex) != m_blockPending.end()) {
        return;
    }
    m_blockPending.insert(blockIndex);
    if (highPriority) {
        m_blockQueue.emplace_front(blockIndex);
    } else {
        m_blockQueue.emplace_back(blockIndex);
    }
}

void HJCacheFile::scheduleFetchLoop()
{
    HJExecutor::Ptr executor = m_executor;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_fetching || m_blockQueue.empty()) {
        return;
    }
    m_fetching = true;
    lock.unlock();
    //
    auto self = HJSharedFromThis();
    if (!executor) {
        fetchLoop();
        return;
    }
    executor->async([this, self]() { fetchLoop(); });
}

void HJCacheFile::fetchLoop()
{
    while (true) {
        int blockIndex = -1;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_blockQueue.empty()) {
                m_fetching = false;
                return;
            }
            blockIndex = m_blockQueue.front();
            m_blockQueue.pop_front();
        }
        int ret = fetchBlock(blockIndex);
        if (ret != HJ_OK) {
            HJFLogw("fetch block {} failed, ret:{}, rid:{}", blockIndex, ret, m_rid);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_blockPending.erase(blockIndex);
            if (m_blockQueue.empty()) {
                m_fetching = false;
                return;
            }
            continue;
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_blockPending.erase(blockIndex);
            if (m_blockQueue.empty()) {
                m_fetching = false;
                return;
            }
        }
    }
}

int HJCacheFile::fetchBlock(int blockIndex)
{
    int res = ensureFetcher();
    if (HJ_OK != res) {
        return res;
    }
    const int64_t start = blockStartOffset(blockIndex);
    const size_t length = blockLength(blockIndex);
    if (start < 0 || length == 0) {
        return HJErrInvalidParams;
    }
    const int64_t end = start + static_cast<int64_t>(length) - 1;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_blockProgress[blockIndex] = 0;
    }
    HJFLogi("fetch block rid:{}, idx:{}, range:{}-{}", m_rid, blockIndex, start, end);
    res = m_fetch->fetch({start, end});
    if (res != HJ_OK) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_blockProgress.erase(blockIndex);
        m_blockCv.notify_all();
    }
    return res;
}

bool HJCacheFile::writeToLocal(int64_t offset, const HJBuffer::Ptr& data)
{
    if (!m_localFile || !data || data->size() == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_fileMutex);
    if (HJ_OK != m_localFile->seek(offset, std::ios::beg)) {
        return false;
    }
    int wrote = m_localFile->write(data->data(), data->size());
    return wrote == static_cast<int>(data->size());
}

void HJCacheFile::handleDownloadedChunk(int64_t offset, const uint8_t* data, size_t length)
{
    HJ_UNUSED(data);
    if (length == 0) {
        return;
    }
    size_t consumed = 0;
    while (consumed < length) {
        const int blockIndex = blockIndexByOffset(offset + static_cast<int64_t>(consumed));
        if (blockIndex < 0) {
            break;
        }
        const int64_t blockStart = blockStartOffset(blockIndex);
        const size_t blockLen = blockLength(blockIndex);
        if (blockLen == 0) {
            break;
        }
        const size_t inside = static_cast<size_t>((offset + static_cast<int64_t>(consumed)) - blockStart);
        const size_t chunk = std::min(blockLen - inside, length - consumed);
        bool completed = false;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto it = m_blockProgress.find(blockIndex);
            if (it != m_blockProgress.end()) {
                it->second += chunk;
                if (it->second >= blockLen) {
                    m_blockProgress.erase(it);
                    completed = true;
                }
            } else if (!isBlockReadyLocked(blockIndex)) {
                completed = true;
            }
            if (completed) {
                markBlockCompleteLocked(blockIndex);
            }
        }
        consumed += chunk;
    }
}

void HJCacheFile::markBlockCompleteLocked(int blockIndex)
{
    if (!m_dbinfo.has_value() || blockIndex < 0 || blockIndex >= blockCount()) {
        return;
    }
    if (m_dbinfo->getBlockStatus(blockIndex)) {
        m_blockCv.notify_all();
        return;
    }
    m_dbinfo->setBlockStatusAndUpdateSize(blockIndex, true);
    updateFileStatusLocked();
    persistDBInfoLocked();
    m_blockCv.notify_all();
}

void HJCacheFile::persistDBInfoLocked()
{
    if (!m_dbinfo.has_value() || m_category.empty()) {
        return;
    }
    auto manager = m_storageManager.lock();
    if (!manager) {
        return;
    }
    m_dbinfo->modify_time = HJCurrentEpochMS();
    manager->AddOrUpdateFile(m_category, *m_dbinfo);
}

void HJCacheFile::updateFileStatusLocked()
{
    if (!m_dbinfo.has_value()) {
        return;
    }
    if (m_dbinfo->total_length > 0 && m_dbinfo->size >= m_dbinfo->total_length) {
        m_dbinfo->status = static_cast<int>(HJFileStatus::COMPLETED);
        m_fileStatus = HJFileStatus::COMPLETED;
    } else {
        m_dbinfo->status = static_cast<int>(HJFileStatus::PENDING);
        m_fileStatus = HJFileStatus::PENDING;
    }
}

bool HJCacheFile::isBlockReadyLocked(int blockIndex) const
{
    if (m_fileStatus == HJFileStatus::COMPLETED) {
        return true;
    }
    if (!m_dbinfo.has_value() || blockIndex < 0 || blockIndex >= blockCount()) {
        return false;
    }
    return m_dbinfo->getBlockStatus(blockIndex);
}

int HJCacheFile::blockCount() const
{
    if (m_blockSize == 0 || m_fileLength <= 0) {
        return 0;
    }
    return static_cast<int>((m_fileLength + static_cast<int64_t>(m_blockSize) - 1) / static_cast<int64_t>(m_blockSize));
}

int HJCacheFile::blockIndexByOffset(int64_t offset) const
{
    if (offset <= 0 || m_blockSize == 0) {
        return 0;
    }
    return static_cast<int>(offset / static_cast<int64_t>(m_blockSize));
}

int64_t HJCacheFile::blockStartOffset(int blockIndex) const
{
    if (blockIndex < 0 || m_blockSize == 0) {
        return -1;
    }
    return static_cast<int64_t>(blockIndex) * static_cast<int64_t>(m_blockSize);
}

size_t HJCacheFile::blockLength(int blockIndex) const
{
    if (blockIndex < 0 || m_blockSize == 0 || m_fileLength <= 0) {
        return 0;
    }
    const int64_t start = blockStartOffset(blockIndex);
    if (start >= m_fileLength) {
        return 0;
    }
    const int64_t end = std::min<int64_t>(m_fileLength, start + static_cast<int64_t>(m_blockSize));
    return static_cast<size_t>(end - start);
}
HJNetFetch::Ptr HJCacheFile::createFetch()
{
    HJFLogi("entry, create fetch rid:{}, url:{}, m_blockSize:{}", m_rid, m_remoteUrl, m_blockSize);

    auto fetch = HJCreates<HJNetFetch>(m_blockSize, HJSharedFromThis());
    int res = m_fetch->init(m_remoteUrl);
    if (HJ_OK != res) {
        HJFLoge("fetch init failed, url:{}, res:{}", m_remoteUrl, res);
        return nullptr;
    }
    m_fileLength = m_fetch->getLength();
    //
    m_dbinfo->total_length = m_fileLength;
    m_dbinfo->status = static_cast<int>(HJFileStatus::PENDING);

    HJFLogi("end, file length:{}", m_fileLength);

    return fetch;
}
int HJCacheFile::fetch(std::tuple<size_t, size_t, size_t> range)
{
    if(!m_fetch) {
        return HJErrNotAlready;
    }
    HJFLogi("fetch id:{}, range:{}-{}", std::get<0>(range), std::get<1>(range), std::get<2>(range));

    int64_t start = std::get<1>(range);
    int64_t end = std::get<2>(range);
    return m_fetch->fetch({start, end});
}

// HJNetFetchDelegate
int HJCacheFile::onNetFetchNotify(HJNotification::Ptr ntfy)
{
    if (!ntfy) {
        return HJErrInvalidParams;
    }
    switch (ntfy->getID())
    {
    case HJNETFETCH_INIT: {
        HJFLogi("fetch init rid:{}, msg:{}", m_rid, ntfy->getMsg());
        break;
    }
    case HJNETFETCH_FETCHING: {
        auto data = ntfy->getValue<HJBuffer::Ptr>("data");
        HJRange64i range = ntfy->getValue<HJRange64i>("range");
        if (data && data->size() > 0) {
            if (!writeToLocal(range.begin, data)) {
                HJFLogw("write local failed rid:{}, offset:{}, size:{}", m_rid, range.begin, data->size());
            }
            handleDownloadedChunk(range.begin, data->data(), data->size());
        }
        break;
    }
    case HJNETFETCH_DONE: {
        HJFLogi("fetch done rid:{}, msg:{}", m_rid, ntfy->getMsg());
        break;
    }
    case HJNETFETCH_ERROR: {
        HJFLoge("fetch error rid:{}, msg:{}", m_rid, ntfy->getMsg());
        break;
    }
    default:
        break;
    }

    return HJ_OK;
}


NS_HJ_END
