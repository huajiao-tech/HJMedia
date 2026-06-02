//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJDownloads.h"
#include "HJDownloadJob.h"
#include "HJFileUtil.h"
#include "HJDataSourceKit.h"
#include "HJMediaDBUtils.h"
#include "HJMediaUtils.h"
#include "HJFLog.h"
#include "HJException.h"
#include <algorithm>

NS_HJ_BEGIN
//***********************************************************************************//
HJDownloads::HJDownloads(HJListener listener, size_t worker_count)
    : m_listener(listener)
    , m_worker_count(worker_count)
{
    if (m_worker_count <= 0) {
        m_worker_count = kDefaultWorkerCount;
    }

    auto fileManager = HJDataSourceKit::getInstance()->getFileManager();
    if(!fileManager) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, "file manager is null");
    }
    m_fileManager = fileManager;
}

HJDownloads::~HJDownloads()
{
    done();
}

int HJDownloads::start()
{
    m_pool = HJCreates<HJExecutorPool>(m_worker_count, HJSharedFromThis());

    m_stop.store(false);
    m_running.store(true);
    m_cv.notify_all();

    return HJ_OK;
}

void HJDownloads::done()
{
    HJFLogi("entry");
    m_stop.exchange(true);
    m_running.store(false);

    cancelDownloadJobs();
    
    if (m_pool) {
        m_pool->stop();
        m_pool = nullptr;
    }
    m_cv.notify_all();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
    m_pending_rids.clear();
    m_inflight_rids.clear();
    m_canceled_rids.clear();
    m_active_jobs.clear();
    HJFLogi("end");
}

bool HJDownloads::isRunning() const
{
    return m_running.load();
}

size_t HJDownloads::pendingCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

size_t HJDownloads::activeCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active_jobs.size();
}

int HJDownloads::download(const HJXIOFileInfo& info)
{
    if (info.url.empty()) {
        return HJErrInvalidParams;
    }

    HJXIOFileInfo xinfo = info;
    if (xinfo.rid.empty()) {
        xinfo.rid = HJMediaUtils::makeUrlRid(xinfo.url);
    }
    if (xinfo.rid.empty() || xinfo.local_url.empty()) {
        HJFLoge("xinfo rid:{} or local_url:{} is null", xinfo.rid, xinfo.local_url);
        return HJErrInvalidParams;
    }

    auto fileManager = HJLockWtr(m_fileManager);
    if(!fileManager) {
        return HJErrNotAlready;
    }
    auto dbFile = fileManager->getCompletedFile(xinfo.rid, xinfo.category);
    if (dbFile.has_value() && HJFileUtil::isFileExist(dbFile->local_url)) {
        HJFLogi("HJ_LOCALIO_NOTIFY_CACHE COMPLETED, file already exist, rid:{}", xinfo.rid);
        {
            auto ntf = HJMakeNotification(HJ_LOCALIO_NOTIFY_CACHE_COMPLETED, xinfo.rid);
            (*ntf)["url"] = xinfo.url;
            (*ntf)["rid"] = xinfo.rid;
            (*ntf)["total"] = dbFile->total_length;
            (*ntf)["valid"] = dbFile->size;

            if (info.listener) {
                info.listener(ntf);
            } else {
                notify(ntf);
            }
        }
        return HJ_EOF;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop.load()) {
            return HJErrAlreadyDone;
        }
        if (m_pending_rids.count(xinfo.rid) > 0 || m_inflight_rids.count(xinfo.rid) > 0 ||
            m_active_jobs.count(xinfo.rid) > 0) {
            return HJErrAlreadyExist;
        }
        HJDownloadItem item{xinfo, m_sequence++};
        if(dbFile.has_value()) {
            item.valid_size = dbFile->size;
            item.total_size = dbFile->total_length;
        }
        insertByPriorityLocked(std::move(item));
        m_pending_rids.insert(xinfo.rid);
        HJFLogi("end, insert item, url:{}, local_url:{}, rid:{}, priority:{}", info.url, info.local_url, info.rid, info.priority);
    }
    m_cv.notify_one();
    return HJ_OK;
}

int HJDownloads::cancel(const std::string& rid, bool deleteFile)
{
    HJ_UNUSED(deleteFile);
    if (rid.empty()) {
        return HJErrInvalidParams;
    }
    std::shared_ptr<HJDownloadJob> job;
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_active_jobs.find(rid);
        if (it != m_active_jobs.end()) {
            job = it->second;
            m_canceled_rids.insert(rid);
            removed = true;
        } else if (m_inflight_rids.count(rid) > 0) {
            m_canceled_rids.insert(rid);
            removed = true;
        } else if (m_pending_rids.count(rid) > 0) {
            for (auto queue_it = m_queue.begin(); queue_it != m_queue.end(); ++queue_it) {
                if (queue_it->info.rid == rid) {
                    m_queue.erase(queue_it);
                    removed = true;
                    break;
                }
            }
            m_pending_rids.erase(rid);
        } else {
            return HJErrNotAlready;
        }
    }
    if (job) {
        job->done();
    }
    return removed ? HJ_OK : HJErrNotAlready;
}

void HJDownloads::insertByPriorityLocked(HJDownloadItem item)
{
    auto insert_pos = m_queue.end();
    for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
        if (item.info.priority > it->info.priority) {
            insert_pos = it;
            break;
        }
    }
    if (insert_pos == m_queue.end()) {
        m_queue.emplace_back(std::move(item));
    } else {
        m_queue.insert(insert_pos, std::move(item));
    }
}

HJTask::Ptr HJDownloads::onAcquireTask()
{
    HJFLogi("entry");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() {
        return m_stop.load() || !m_queue.empty();
    });
    if (m_stop.load()) {
        HJFLogw("waning, stop");
        return nullptr;
    }
    if (m_queue.empty()) {
        HJFLogw("waning, queue is empty");
        return nullptr;
    }
    
    HJDownloadItem item = m_queue.front();
    m_queue.pop_front();
    m_pending_rids.erase(item.info.rid);
    m_inflight_rids.insert(item.info.rid);
    lock.unlock();
    HJFLogi("start, rid:{}, url:{}, local_url:{}, priority:{}", item.info.rid, item.info.url, item.info.local_url, item.info.priority);
    auto task = HJCreates<HJTask>([this, item]() {
        runTask(item);
    });
    task->setName(item.info.rid);
    task->setClsID(item.info.priority);
    return task;
}

void HJDownloads::onStop()
{
    HJFLogi("entry");
    m_stop.store(true);
    m_running.store(false);
    m_cv.notify_all();
    HJFLogi("end");
}

void HJDownloads::runTask(const HJDownloadItem& i_item)
{
    HJDownloadItem item = i_item;
    bool canceled_before_start = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inflight_rids.erase(item.info.rid);
        auto it = m_canceled_rids.find(item.info.rid);
        if (it != m_canceled_rids.end()) {
            m_canceled_rids.erase(it);
            canceled_before_start = true;
        }
    }
    if (canceled_before_start || m_stop.load()) {
        if (canceled_before_start) {
            HJFLogi("download canceled before start, rid:{}", item.info.rid);
        } else {
            HJFLogw("download aborted before start, rid:{}", item.info.rid);
        }
        m_cv.notify_one();
        return;
    }
    {
        HJFLogi("HJ_LOCALIO_NOTIFY_CACHE START, rid:{}, valid_size:{}, total_size:{}", item.info.rid, item.valid_size, item.total_size);
        auto ntf = HJMakeNotification(HJ_LOCALIO_NOTIFY_CACHE_START, item.info.rid);
        (*ntf)["url"] = item.info.url;
        (*ntf)["rid"] = item.info.rid;
        (*ntf)["total"] = item.total_size;
        (*ntf)["valid"] = item.valid_size;

        if(item.info.listener) {
            item.info.listener(ntf);
        } else {
            notify(ntf);
        }
    }
    int res = doDownloadJob(item);
    bool was_canceled = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_canceled_rids.find(item.info.rid);
        if (it != m_canceled_rids.end()) {
            m_canceled_rids.erase(it);
            was_canceled = true;
        }
    }
    if (res != HJ_OK) {
        bool retry_queued = false;
        if (!was_canceled && res != HJErrNetCanceled && !m_stop.load() && item.retry_count < m_retry_count) {
            ++item.retry_count;
            const size_t retry_count = item.retry_count;
            const std::string rid = item.info.rid;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!m_stop.load() && m_pending_rids.count(rid) == 0 && m_active_jobs.count(rid) == 0) {
                    insertByPriorityLocked(std::move(item));
                    m_pending_rids.insert(rid);
                    retry_queued = true;
                }
            }
            if (retry_queued) {
                HJFLogw("download failed, retry {}/{}, rid:{}, res:{}", retry_count, m_retry_count, rid, res);
                m_cv.notify_one();
                return;
            }
        }
        HJFLogw("download failed, rid:{}, res:{}", item.info.rid, res);
    }
    {
        HJFLogi("HJ_LOCALIO_NOTIFY_CACHE COMPLETED, rid:{}, valid_size:{}, total_size:{}", item.info.rid, item.valid_size, item.total_size);
        auto notify_id = (res == HJ_OK) ? HJ_LOCALIO_NOTIFY_CACHE_COMPLETED : HJ_LOCALIO_NOTIFY_CACHE_FAILED;
        auto ntf = HJMakeNotification(notify_id, res, item.info.rid);
        (*ntf)["url"] = item.info.url;
        (*ntf)["rid"] = item.info.rid;
        (*ntf)["code"] = res;
        (*ntf)["total"] = item.total_size;
        (*ntf)["valid"] = item.valid_size;

        if(item.info.listener) {
            item.info.listener(ntf);
        } else {
            notify(ntf);
        }
    }
    m_cv.notify_one();
}

int HJDownloads::doDownloadJob(HJDownloadItem& item)
{
    if (m_stop.load()) {
        HJFLogw("stop");
        return HJErrNetCanceled;
    }
    int t0 = HJCurrentSteadyMS();
    const HJXIOFileInfo& info = item.info;
    auto job = HJCreates<HJDownloadJob>(info, info.listener ? info.listener : m_listener);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop.load()) {
            return HJErrNetCanceled;
        }
        m_active_jobs[info.rid] = job;
    }
    int res = job->run();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_active_jobs.erase(info.rid);
    }
    item.valid_size = job->getValidSize();
    item.total_size = job->getTotalLength();
    HJFLogi("end, rid:{}, time:{}, res:{}", info.rid, HJCurrentSteadyMS() - t0, res);
    
    if (res != HJ_OK) {
        return res;
    }
    return HJ_OK;
}

void HJDownloads::cancelDownloadJobs()
{
    std::vector<std::shared_ptr<HJDownloadJob>> jobs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        jobs.reserve(m_active_jobs.size());
        for (const auto& it : m_active_jobs) {
            jobs.push_back(it.second);
        }
    }
    for (const auto& job : jobs) {
        if (job) {
            job->done();
        }
    }
}

NS_HJ_END
