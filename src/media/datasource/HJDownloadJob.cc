//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJDownloadJob.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//
HJDownloadJob::HJDownloadJob(const HJXIOFileInfo& info, HJListener listener)
    : m_info(info)
    , m_listener(listener)
{
}

HJDownloadJob::~HJDownloadJob()
{
    done();
}

int HJDownloadJob::init()
{
    if (m_info.url.empty() || m_info.rid.empty() || m_info.local_url.empty()) {
        HJFLoge("download job: url or rid or local_url is empty");
        return HJErrInvalidParams;
    }
    HJFLogi("entry, url: {}, rid: {}, local_url: {}", m_info.url, m_info.rid, m_info.local_url);

    int res = HJ_OK;
    HJUrl::Ptr murl = HJCreates<HJUrl>(m_info.url);
    (*murl)["local_dir"] = HJFileUtil::parentDir(m_info.local_url);
    (*murl)["rid"] = m_info.rid;
    (*murl)["timeout"] = int64_t(300000);
    (*murl)["interrupt_callback"] = *m_interruptCB;

    m_dataSource = HJCreateu<HJDataSource>();
    res = m_dataSource->open(murl);
    if (HJ_OK != res) {
        HJFLoge("error, open data source failed");
        return res;
    }
    m_missingInfo = m_dataSource->getMissingInfo();
    if (m_missingInfo.blockSize <= 0 || m_missingInfo.totalLength <= 0) {
        HJFLoge("download job: invalid missing info, block_size:{}, total_length:{}",
                m_missingInfo.blockSize, m_missingInfo.totalLength);
        return HJErrInvalidData;
    }
    if(m_missingInfo.incompleteBlocks.empty() || m_missingInfo.status == static_cast<int>(HJFileStatus::COMPLETED)) {
        m_isComplete.store(true);
        HJFLogw("file is completed");
        return HJ_EOF;
    }
    HJFLogi("end, url: {}, rid:{}, block_size:{}, total_length:{}, valid_size:{}, incomplete_blocks:{}", m_info.url, m_info.rid,
            m_missingInfo.blockSize, m_missingInfo.totalLength, m_missingInfo.validSize, m_missingInfo.incompleteBlocks.size());

    return HJ_OK;
}

int HJDownloadJob::run()
{
    if (m_abort.load() || getIsQuit()) {
        return HJErrNetCanceled;
    }
    HJFLogi("entry, rid:{}", m_info.rid);
    m_running.store(true);
    struct RunGuard {
        HJDownloadJob* job{nullptr};
        ~RunGuard() {
            if (job) {
                job->finalizeRun();
            }
        }
    } guard{this};

    int res = init();
    if (res != HJ_OK) {
        return res;
    }

    const size_t block_size = static_cast<size_t>(m_missingInfo.blockSize);
    const int64_t block_count = (m_missingInfo.totalLength + block_size - 1) / block_size;
    int64_t validSize = HJ_MIN(m_missingInfo.validSize, (block_count - m_missingInfo.incompleteBlocks.size()) * block_size);
    std::vector<size_t> target_blocks;
    if (m_info.preCacheSize < 0) {
        target_blocks = m_missingInfo.incompleteBlocks;
    } else {
        int64_t target_size = m_info.preCacheSize - m_missingInfo.validSize;
        if(target_size > 0) {
            int64_t toReadSize = 0;
            for (size_t block_idx : m_missingInfo.incompleteBlocks) {
                toReadSize += blockLength(block_idx);               
                if (toReadSize > target_size) {
                    break;
                }
                target_blocks.push_back(block_idx);
            }
        }
    }
    if (target_blocks.empty()) {
        m_isComplete.store(true);
        notifyProgress(true);
        return HJ_OK;
    }
    m_totalBlocks.store(target_blocks.size());
    m_completedBlocks.store(0);

    std::vector<uint8_t> buffer;
    buffer.resize(block_size);
    for (size_t i = 0; i < target_blocks.size(); ++i) {
        if (m_abort.load() || getIsQuit()) {
            return HJErrNetCanceled;
        }
        res = readBlock(target_blocks[i], buffer.data(), buffer.size());
        if (res != HJ_OK) {
            return res;
        }
        m_completedBlocks.fetch_add(1);
        notifyProgress(false);
    }

    m_isComplete.store(true);
    notifyProgress(true);
    return HJ_OK;
}

void HJDownloadJob::done()
{
    HJFLogi("entry");
    setQuit(true);
    m_abort.store(true);
    if (!m_running.load()) {
        closeDataSource();
    }
    HJFLogi("end");
}

int HJDownloadJob::getProgress() const
{
    const size_t total = m_totalBlocks.load();
    if (total == 0) {
        return 0;
    }
    const size_t completed = m_completedBlocks.load();
    return static_cast<int>((completed * 100) / total);
}

bool HJDownloadJob::isComplete() const
{
    return m_isComplete.load();
}

// size_t HJDownloadJob::computeTargetStart() const
// {
//     if (m_info.preCacheSize < 0) {
//         return 0;
//     }
//     if (m_missingInfo.incompleteBlocks.empty()) {
//         return 0;
//     }
//     const size_t block_size = static_cast<size_t>(m_missingInfo.blockSize);
//     return m_missingInfo.incompleteBlocks.front() * block_size;
// }

// size_t HJDownloadJob::computeTargetEndExclusive(size_t start_offset) const
// {
//     if (m_info.preCacheSize == 0) {
//         return 0;
//     }
//     const int64_t total_length = m_missingInfo.totalLength;
//     if (total_length <= 0) {
//         return 0;
//     }
//     if (m_info.preCacheSize < 0) {
//         return static_cast<size_t>(total_length);
//     }
//     const int64_t end_value = /*static_cast<int64_t>(start_offset) +*/m_info.preCacheSize;
//     const int64_t end_clamped = (std::min)(end_value, total_length);
//     if (end_clamped <= 0) {
//         return 0;
//     }
//     return static_cast<size_t>(end_clamped);
// }

// std::vector<size_t> HJDownloadJob::buildTargetBlocks(size_t start_offset, size_t end_exclusive) const
// {
//     std::vector<size_t> blocks;
//     if (end_exclusive <= start_offset) {
//         return blocks;
//     }
//     blocks.reserve(m_missingInfo.incompleteBlocks.size());

//     const size_t block_size = static_cast<size_t>(m_missingInfo.blockSize);
//     for (size_t block_idx : m_missingInfo.incompleteBlocks) {
//         const size_t block_start = block_idx * block_size;
//         if (block_start < start_offset) {
//             continue;
//         }
//         if (block_start >= end_exclusive) {
//             break;
//         }
//         blocks.push_back(block_idx);
//     }
//     return blocks;
// }

size_t HJDownloadJob::blockLength(size_t block_idx) const
{
    const size_t block_size = static_cast<size_t>(m_missingInfo.blockSize);
    const int64_t total_length = m_missingInfo.totalLength;
    if (block_size == 0 || total_length <= 0) {
        HJFLoge("error, Invalid block size or total length");
        return 0;
    }
    const size_t start = block_idx * block_size;
    if (start >= static_cast<size_t>(total_length)) {
        return 0;
    }
    const size_t remaining = static_cast<size_t>(total_length) - start;
    return (std::min)(block_size, remaining);
}

int HJDownloadJob::seekTo(int64_t offset)
{
    std::lock_guard<std::mutex> lock(m_dataSourceMutex);
    if (!m_dataSource) {
        return HJErrNotAlready;
    }
    return m_dataSource->seek(offset, std::ios::beg);
}

int HJDownloadJob::readData(uint8_t* buffer, size_t cnt)
{
    std::lock_guard<std::mutex> lock(m_dataSourceMutex);
    if (!m_dataSource) {
        return HJErrNotAlready;
    }
    return m_dataSource->read(buffer, cnt);
}

int HJDownloadJob::readBlock(size_t block_idx, uint8_t* buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return HJErrInvalidParams;
    }
    const size_t block_size = static_cast<size_t>(m_missingInfo.blockSize);
    const size_t block_len = blockLength(block_idx);
    if (block_size == 0 || block_len == 0) {
        return HJErrInvalidData;
    }

    const int64_t block_start = static_cast<int64_t>(block_idx * block_size);
    int res = seekTo(block_start);
    if (res != HJ_OK) {
        return HJErrIOSeek;
    }

    size_t read_total = 0;
    while (read_total < block_len) {
        if (m_abort.load() || getIsQuit()) {
            return HJErrNetCanceled;
        }
        const size_t to_read = (std::min)(buffer_size, block_len - read_total);
        int bytes_read = readData(buffer, to_read);
        if (bytes_read > 0) {
            read_total += static_cast<size_t>(bytes_read);
            //
            m_missingInfo.validSize += bytes_read;
            continue;
        }
        return (bytes_read == 0) ? HJErrNetRecv : bytes_read;
    }
    return HJ_OK;
}

void HJDownloadJob::closeDataSource()
{
    std::lock_guard<std::mutex> lock(m_dataSourceMutex);
    if (!m_dataSource) {
        return;
    }
    m_dataSource->close();
    m_dataSource.reset();
}

void HJDownloadJob::notifyProgress(bool force)
{
    int percent = getProgress();
    if (!force && percent == m_lastPercent) {
        return;
    }
    HJFLogi("HJ_LOCALIO_NOTIFY_CACHE PROGRESS, rid:{}, valid_size:{}, total_size:{}", m_info.rid, m_missingInfo.validSize, m_missingInfo.totalLength);
    m_lastPercent = percent;
    auto ntf = HJMakeNotification(HJ_LOCALIO_NOTIFY_CACHE_PROGRESS, percent, m_info.rid);
    (*ntf)["url"] = m_info.url;
    (*ntf)["rid"] = m_info.rid;
    (*ntf)["percent"] = percent;
    (*ntf)["total"] = m_missingInfo.totalLength;
    (*ntf)["valid"] = m_missingInfo.validSize;
    notify(ntf);
}

void HJDownloadJob::finalizeRun()
{
    m_running.store(false);
    closeDataSource();
}

NS_HJ_END
