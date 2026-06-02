//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJLocalIOUtils.h"
#include "HJExecutor.h"
#include "HJLocalFileManager.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
class HJDownloadJob;

// class HJDownloadsDelegateMock : public HJExecutorPoolDelegate {
// public:
//     HJ_DECLARE_PUWTR(HJDownloadsDelegateMock);

// };

class HJDownloads : public HJExecutorPoolDelegate
{
public:
    HJ_DECLARE_PUWTR(HJDownloads);

    static constexpr size_t kDefaultWorkerCount = 2;
    static constexpr size_t kMaxRetryCount = 10;

    explicit HJDownloads(HJListener listener = nullptr, size_t worker_count = kDefaultWorkerCount);
    virtual ~HJDownloads();

    int start();
    void done();

    int download(const HJXIOFileInfo& info);
    int cancel(const std::string& rid, bool deleteFile = false);

    size_t pendingCount() const;
    size_t activeCount() const;
    bool isRunning() const;
    void setListener(HJListener listener) { m_listener = std::move(listener); }
    void setRetryCount(size_t retry_count) { m_retry_count = retry_count; }
protected:
    HJTask::Ptr onAcquireTask() override;
    void onStop() override;
private:
    struct HJDownloadItem {
        HJXIOFileInfo info;
        size_t sequence{0};
        int64_t valid_size{0};
        int64_t total_size{0};
        size_t retry_count{0};
    };

    void insertByPriorityLocked(HJDownloadItem item);
    void runTask(const HJDownloadItem& i_item);

    int doDownloadJob(HJDownloadItem& item);
    void cancelDownloadJobs();

    int notify(const HJNotification::Ptr& ntf) {
        if (m_listener) {
            return m_listener(ntf);
        }
        return HJ_OK;
    }
private:
    mutable std::mutex              m_mutex;
    std::condition_variable         m_cv;
    HJExecutorPool::Ptr             m_pool;
    std::deque<HJDownloadItem>      m_queue;
    std::unordered_set<std::string> m_pending_rids;
    std::unordered_set<std::string> m_inflight_rids;
    std::unordered_set<std::string> m_canceled_rids;
    std::unordered_map<std::string, std::shared_ptr<HJDownloadJob>> m_active_jobs;
    size_t                          m_sequence{0};
    std::atomic<bool>               m_running{false};
    std::atomic<bool>               m_stop{false};
    HJListener                      m_listener{};
    HJLocalFileManager::Wtr         m_fileManager;
    size_t                          m_worker_count{ kDefaultWorkerCount };
    size_t                          m_retry_count{ kMaxRetryCount };
};

NS_HJ_END
