//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFetchDownloadJob.h"
#include "HJRangeAggregator.h"
#include "HJFileUtil.h"
#include "HJLocalFileManager.h"
#include "HJDataSourceKit.h"
#include "HJMediaUtils.h"
#include "HJBuffer.h"
#include "HJXIOBlobFile.h"
#include "HJFLog.h"
#include "HJException.h"
#include <algorithm>
#include <thread>
#include <chrono>

NS_HJ_BEGIN
//***********************************************************************************//
HJFetchDownloadJob::HJFetchDownloadJob(const HJXIOFileInfo& info, HJListener listener)
    : m_info(info)
    , m_listener(listener)
{
    auto fileManager = HJDataSourceKit::getInstance()->getFileManager();
    if (!fileManager) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, "file manager is null");
    }
    m_fileManager = fileManager;
    
    // 默认使用 HJRangeAggregator
    m_aggregator = HJCreates<HJRangeAggregator>();
}

HJFetchDownloadJob::~HJFetchDownloadJob()
{
    done();
}

void HJFetchDownloadJob::done()
{
    m_abort.store(true);
    HJNetFetch::Ptr fetch;
    {
        std::lock_guard<std::mutex> lock(m_fetch_mutex);
        fetch = m_fetch;
    }
    if (fetch) {
        fetch->done();
    }
}

int HJFetchDownloadJob::run()
{
    if (m_abort.load()) {
        return HJErrNetCanceled;
    }
    if (m_info.url.empty() || m_info.rid.empty() || m_info.local_url.empty()) {
        HJFLoge("download job: url or rid or local_url is empty");
        return HJErrInvalidParams;
    }
    HJFLogi("entry, url: {}, rid: {}, local_url: {}", m_info.url, m_info.rid, m_info.local_url);

    int res = HJ_OK;
    int64_t t0 = HJCurrentSteadyMS();
    m_hjtd_url = HJMediaUtils::makeHJTDFile(m_info.local_url);
    auto fileManager = HJLockWtr(m_fileManager);
    if (!fileManager) {
        HJFLoge("download job: file manager is null");
        return HJErrNotAlready;
    }
    auto db_file = fileManager->getDBFile(m_info.rid, m_info.category);
    if(db_file.has_value()) {
        m_db_info = db_file.value();
        //
        if (m_db_info.status == static_cast<int>(HJFileStatus::COMPLETED) &&
            m_info.local_url == m_db_info.local_url &&
            HJFileUtil::isFileExist(m_info.local_url)) {
            HJFLogi("download job: file already exists, rid: {}", m_info.rid, m_info.local_url);
            return HJ_OK;
        }
    }

    auto parent_dir = HJFileUtil::parentDir(m_info.local_url);
    if (!parent_dir.empty() && !HJFileUtil::isDirExist(parent_dir)) {
        if (!HJFileUtil::makeDir(parent_dir)) {
            HJFLoge("download job: failed to create dir: {}", parent_dir);
            return HJErrInvalidPath;
        }
    }

    // 初始化网络下载器
    const int64_t storage_block_size = static_cast<int64_t>(HJXIOBlobFile::getBlockSize());
    auto fetch = HJCreates<HJNetFetch>();
    res = fetch->init(m_info.url);
    if (HJ_OK != res) {
        HJFLoge("download job: fetch init failed: {}", res);
        return res;
    }
    
    // 获取 fetch block size 并验证
    const int64_t fetch_block_size = static_cast<int64_t>(fetch->getBlockSize());
    if (!HJBlockAggregator::validateBlockSizes(storage_block_size, fetch_block_size)) {
        HJFLoge("download job: invalid block sizes, storage: {}, fetch: {}", 
                storage_block_size, fetch_block_size);
        return HJErrInvalidParams;
    }
    
    // 校验远程文件大小
    int64_t remote_length = fetch->getLength();
    if (remote_length <= 0) {
        HJFLoge("download job: remote size <= 0");
        return HJErrInvalidParams;
    }
    {
        std::lock_guard<std::mutex> lock(m_fetch_mutex);
        m_fetch = fetch;
    }
    if (m_abort.load()) {
        return HJErrNetCanceled;
    }

    if (!db_file.has_value() ||
        (m_db_info.total_length > 0 && remote_length != m_db_info.total_length) ||
        m_info.local_url != m_db_info.local_url ||
        storage_block_size != m_db_info.block_size ||
        !HJFileUtil::isFileExist(m_hjtd_url)) 
    {
        HJFLogw("remote size {} != db size {}, re-downloading", remote_length, m_db_info.total_length);
        m_db_info = HJDBFileInfo(m_info.rid, m_info.url, m_info.local_url, remote_length, 
                                  static_cast<int>(storage_block_size));
        res = fileManager->updateFileInfo(m_db_info);
        if (HJ_OK != res) {
            HJFLoge("download job: updateFileInfo failed: {}", res);
            return res;
        }
    }
    
    // 扫描缺失块
    res = loadMissingBlocks();
    if (HJ_OK != res) {
        return res;
    }

    // 如果没有缺失块，说明已完成
    if (m_missing_blocks.empty()) {
        HJFLogi("download job: no missing blocks, already complete");
        fileManager->completedFile(m_hjtd_url, m_info.rid);
        return HJ_OK;
    }

    // 打开/创建blob文件
    auto murl = HJCreates<HJUrl>(m_hjtd_url, HJ_XIO_WRITE);
    (*murl)["size"] = m_db_info.total_length;

    m_blob = HJCreates<HJXIOBlobFile>(nullptr, [&](const HJNotification::Ptr ntf) -> int {
        if (!ntf) {
            return HJErrInvalidParams;
        }
        switch (ntf->getID()) {
            case (size_t)HJFileStatus::PENDING: {
                break;
            }
            case (size_t)HJFileStatus::COMPLETED: {
                notify(ntf);
                break;
            }
            default:
                break;
        }
        return HJ_OK;
    });
    m_blob->setName(m_info.rid);
    res = m_blob->open(murl);
    if (HJ_OK != res) {
        HJFLoge("download job: blob open failed: {}, path: {}", res, m_hjtd_url);
        return res;
    }

    // 计算进度基数
    m_total_blocks = m_db_info.blockCount();
    m_completed_blocks = m_total_blocks - static_cast<int64_t>(m_missing_blocks.size());

    {
        int percent = getProgress();
        auto ntf = HJMakeNotification(HJ_LOCALIO_NOTIFY_CACHE_START, percent, m_info.rid);
        (*ntf)["url"] = m_info.url;
        (*ntf)["rid"] = m_info.rid;
        (*ntf)["percent"] = percent;
        notify(ntf);
    }

    // 使用聚合器生成下载范围
    std::vector<HJRange64i> aggregated_ranges;
    if (m_aggregator) {
        aggregated_ranges = m_aggregator->aggregate(
            m_missing_blocks, 
            storage_block_size, 
            fetch_block_size, 
            m_db_info.total_length);
    } else {
        // 回退：每个块单独下载
        for (int64_t block_idx : m_missing_blocks) {
            aggregated_ranges.push_back(getBlockRange(block_idx));
        }
    }
    
    HJFLogi("download job: aggregated {} missing blocks into {} ranges", 
            m_missing_blocks.size(), aggregated_ranges.size());

    // 下载每个聚合范围
    for (size_t i = 0; i < aggregated_ranges.size(); ++i) {
        if (m_abort.load()) {
            m_blob->close();
            m_blob = nullptr;
            return HJErrNetCanceled;
        }

        const HJRange64i& range = aggregated_ranges[i];
        std::vector<int64_t> range_blocks = extractBlocksForRange(range);
        
        res = downloadAggregatedRange(range, range_blocks);
        if (HJ_OK != res) {
            HJFLoge("download job: downloadAggregatedRange {}-{} failed: {}", 
                    range.begin, range.end, res);
            m_blob->close();
            m_blob = nullptr;
            notifyError(res);
            return res;
        }
    }

    // 关闭blob文件
    m_blob->close();
    m_blob = nullptr;

    // 标记完成
    m_db_info.status = static_cast<int>(HJFileStatus::COMPLETED);
    m_db_info.modify_time = HJCurrentSteadyMS();
    fileManager = HJLockWtr(m_fileManager);
    if (fileManager) {
        fileManager->completedFile(m_hjtd_url, m_info.rid);
    }

    HJFLogi("end, time: {}ms, completed rid: {}, url: {}", HJCurrentSteadyMS() - t0, m_info.rid, m_info.url);
    return HJ_OK;
}

int HJFetchDownloadJob::loadMissingBlocks()
{
    m_missing_blocks.clear();
    const int64_t total_blocks = m_db_info.blockCount();

    for (int64_t i = 0; i < total_blocks; ++i) {
        if (!m_db_info.getBlockStatus(static_cast<int>(i))) {
            m_missing_blocks.push_back(i);
        }
    }

    HJFLogi("download job: {} missing blocks out of {}", m_missing_blocks.size(), total_blocks);
    return HJ_OK;
}


std::vector<int64_t> HJFetchDownloadJob::extractBlocksForRange(const HJRange64i& range) const
{
    std::vector<int64_t> blocks;
    const int64_t storage_block_size = m_db_info.block_size;
    if (storage_block_size <= 0) return blocks;
    
    int64_t start_block = range.begin / storage_block_size;
    int64_t end_block = range.end / storage_block_size;
    
    for (int64_t block_idx : m_missing_blocks) {
        if (block_idx >= start_block && block_idx <= end_block) {
            blocks.push_back(block_idx);
        }
    }
    return blocks;
}

int HJFetchDownloadJob::downloadAggregatedRange(const HJRange64i& range, const std::vector<int64_t>& range_blocks)
{
    if (!m_blob) {
        return HJErrNotAlready;
    }
    
    if (range_blocks.empty()) {
        return HJ_OK;
    }

    int64_t range_expected = range.end - range.begin + 1;
    if (range.end < range.begin || range_expected <= 0) {
        return HJErrInvalidParams;
    }
    
    HJFLogi("download job: downloading range {}-{}, {} blocks", 
            range.begin, range.end, range_blocks.size());

    for (m_retry_count = 0; m_retry_count < kMaxRetries; ++m_retry_count) {
        if (m_abort.load()) {
            return HJErrNetCanceled;
        }

        // 清空当前范围内已完成的块记录
        m_completed_in_range.clear();

        HJNetFetch::Ptr fetch;
        {
            std::lock_guard<std::mutex> lock(m_fetch_mutex);
            fetch = m_fetch;
        }
        if (!fetch) {
            return HJErrNotAlready;
        }

        int fetch_result = HJ_OK;
        int64_t total_received = 0;

        int res = fetch->fetch(range, [&](const HJNotification::Ptr ntf) -> int {
            if (!ntf) {
                return HJErrInvalidParams;
            }
            switch (ntf->getID()) {
            case HJNETFETCH_START:
                break;

            case HJNETFETCH_FETCHING: {
                if (m_abort.load()) {
                    return HJErrNetCanceled;
                }

                auto data = ntf->getValue<HJBuffer::Ptr>("data");
                if (!data || data->size() == 0) {
                    break;
                }
                HJRange64i data_range = ntf->getValue<HJRange64i>("range");
                if (data_range.end < data_range.begin ||
                    data_range.begin < range.begin || data_range.end > range.end) {
                    HJFLogw("download job: invalid data range, expect:{}-{}, got:{}-{}",
                            range.begin, range.end, data_range.begin, data_range.end);
                    return HJErrInvalidParams;
                }
                
                // 处理接收到的数据
                int proc_res = processReceivedData(data, data_range);
                if (proc_res != HJ_OK) {
                    fetch_result = proc_res;
                    return proc_res;
                }
                
                total_received += static_cast<int64_t>(data->size());
                break;
            }

            case HJNETFETCH_END:
                break;

            case HJNETFETCH_ERROR: {
                int result = static_cast<int>(ntf->getVal());
                HJFLogw("download job: fetch error: {}", result);
                fetch_result = result;
                break;
            }

            default:
                break;
            }
            return HJ_OK;
        });

        if (m_abort.load()) {
            // 中止前持久化已完成的块
            persistCompletedBlocks();
            return HJErrNetCanceled;
        }

        // 检查是否所有块都已完成
        bool all_blocks_complete = true;
        for (int64_t block_idx : range_blocks) {
            if (m_completed_in_range.find(block_idx) == m_completed_in_range.end() &&
                !m_db_info.getBlockStatus(static_cast<int>(block_idx))) {
                all_blocks_complete = false;
                break;
            }
        }

        if (res == HJ_OK && all_blocks_complete) {
            // 持久化所有已完成的块
            return persistCompletedBlocks();
        }

        // 部分成功：持久化已完成的块
        if (!m_completed_in_range.empty()) {
            persistCompletedBlocks();
        }

        HJFLogw("range:{}-{}, received:{}/{}, completed blocks:{}/{}, retry:{}/{}",
                range.begin, range.end, total_received, range_expected,
                m_completed_in_range.size(), range_blocks.size(),
                m_retry_count + 1, kMaxRetries);
    }

    HJFLoge("end: range {}-{} failed after {} retries", range.begin, range.end, kMaxRetries);
    return HJErrNetUnkown;
}

int HJFetchDownloadJob::processReceivedData(const HJBuffer::Ptr& data, const HJRange64i& data_range)
{
    if (!m_blob || !data || data->size() == 0) {
        return HJErrInvalidParams;
    }
    
    // 写入数据到 blob 文件
    if (m_blob->seek(data_range.begin, std::ios::beg) != HJ_OK) {
        return HJErrIOSeek;
    }
    int wrote = m_blob->write(data->data(), data->size());
    if (wrote != static_cast<int>(data->size())) {
        return HJErrIOWrite;
    }
    
    // 检查哪些 Storage_Block 已经完成
    const int64_t storage_block_size = m_db_info.block_size;
    if (storage_block_size <= 0) return HJ_OK;
    
    int64_t start_block = data_range.begin / storage_block_size;
    int64_t end_block = data_range.end / storage_block_size;
    
    for (int64_t block_idx = start_block; block_idx <= end_block; ++block_idx) {
        // 检查该块是否已经在完成集合中或数据库中标记为完成
        if (m_completed_in_range.find(block_idx) != m_completed_in_range.end()) {
            continue;
        }
        if (m_db_info.getBlockStatus(static_cast<int>(block_idx))) {
            continue;
        }
        
        // 计算该块的字节范围
        HJRange64i block_range = getBlockRange(block_idx);
        
        // 检查该块是否完全被当前数据覆盖
        // 注意：由于数据是流式到达的，我们需要检查累积写入
        // 这里简化处理：如果数据范围完全覆盖了块范围，则认为块完成
        // 实际上 HJXIOBlobFile 会在块完成时发送通知
        
        // 检查块是否完整（通过 blob 的块状态）
        if (m_blob->isBlockComplete(static_cast<size_t>(block_idx))) {
            m_completed_in_range.insert(block_idx);
            m_completed_blocks++;
            notifyProgress();
            
            HJFLogi("download job: block {} completed", block_idx);
        }
    }
    
    return HJ_OK;
}

int HJFetchDownloadJob::persistCompletedBlocks()
{
    if (!m_blob) {
        return HJErrInvalid;
    }
    
    auto t0 = HJCurrentSteadyMS();
    
    // 刷盘
    int res = m_blob->flush();
    if (HJ_OK != res) {
        HJFLoge("download job: blob flush failed: {}", res);
        return res;
    }
    
    auto t1 = HJCurrentSteadyMS();
    
    // 更新数据库中已完成块的状态
    for (int64_t block_idx : m_completed_in_range) {
        if (!m_db_info.getBlockStatus(static_cast<int>(block_idx))) {
            m_db_info.setBlockStatusAndUpdateSize(static_cast<int>(block_idx), true);
        }
    }
    
    m_db_info.status = static_cast<int>(HJFileStatus::PENDING);
    m_db_info.modify_time = HJCurrentSteadyMS();

    auto t2 = HJCurrentSteadyMS();
    
    auto fileManager = HJLockWtr(m_fileManager);
    if (fileManager) {
        res = fileManager->updateFileInfo(m_db_info);
        if (HJ_OK != res) {
            HJFLogw("download job: updateFileInfo failed: {}", res);
            // 不中止，继续下载
        }
    }
    
    HJFLogi("download job: persistCompletedBlocks, rid: {}, blocks: {}, time: {}/{}/{}ms",
            m_info.rid, m_completed_in_range.size(), 
            t1 - t0, t2 - t1, HJCurrentSteadyMS() - t0);

    // 清空已完成集合
    m_completed_in_range.clear();
    
    return HJ_OK;
}

int64_t HJFetchDownloadJob::getBlockIndex(int64_t byte_offset) const
{
    const int64_t block_size = m_db_info.block_size;
    if (block_size <= 0) return -1;
    return byte_offset / block_size;
}

HJRange64i HJFetchDownloadJob::getBlockRange(int64_t block_index) const
{
    const int64_t block_size = m_db_info.block_size;
    const int64_t start = block_index * block_size;
    const int64_t end = std::min(start + block_size, m_db_info.total_length) - 1;
    return {start, end};
}

int HJFetchDownloadJob::getProgress() const
{
    if (m_total_blocks <= 0) {
        return 0;
    }
    return static_cast<int>((m_completed_blocks * 100) / m_total_blocks);
}

bool HJFetchDownloadJob::isComplete() const
{
    return m_db_info.isComplete();
}

void HJFetchDownloadJob::notifyProgress()
{
    int percent = getProgress();
    auto ntf = HJMakeNotification(HJ_LOCALIO_NOTIFY_CACHE_PROGRESS, percent, m_info.rid);
    (*ntf)["url"] = m_info.url;
    (*ntf)["rid"] = m_info.rid;
    (*ntf)["percent"] = percent;
    notify(ntf);
}

void HJFetchDownloadJob::notifyError(int code)
{
    auto ntf = HJMakeNotification(HJ_LOCALIO_NOTIFY_CACHE_FAILED, code, m_info.rid);
    (*ntf)["url"] = m_info.url;
    (*ntf)["rid"] = m_info.rid;
    (*ntf)["code"] = code;
    notify(ntf);
}

NS_HJ_END
