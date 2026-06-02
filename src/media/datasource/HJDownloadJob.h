//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJDataSource.h"
#include <atomic>
#include <mutex>
#include <vector>

NS_HJ_BEGIN
//***********************************************************************************//
class HJDownloadJob : public HJXIOInterrupt, public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJDownloadJob);

    explicit HJDownloadJob(const HJXIOFileInfo& info, HJListener listener = nullptr);
    virtual ~HJDownloadJob();

    int init();
    int run();
    void done();
    int getProgress() const;
    bool isComplete() const;

    int notify(const HJNotification::Ptr& ntf) {
        if (m_listener) {
            return m_listener(ntf);
        }
        return HJ_OK;
    }
    int64_t getValidSize() const {
        return m_missingInfo.validSize;
    }
    int64_t getTotalLength() const {
        return m_missingInfo.totalLength;
    }
private:
    // size_t computeTargetStart() const;
    // size_t computeTargetEndExclusive(size_t start_offset) const;
    // std::vector<size_t> buildTargetBlocks(size_t start_offset, size_t end_exclusive) const;
    size_t blockLength(size_t block_idx) const;

    int seekTo(int64_t offset);
    int readData(uint8_t* buffer, size_t cnt);
    int readBlock(size_t block_idx, uint8_t* buffer, size_t buffer_size);
    void closeDataSource();
    void notifyProgress(bool force);
    void finalizeRun();
protected:
    HJXIOFileInfo           m_info;
    HJListener              m_listener{};

    HJDataSource::Utr       m_dataSource{};
    HJBlobMissingInfo       m_missingInfo;

    std::atomic<size_t>     m_totalBlocks{0};
    std::atomic<size_t>     m_completedBlocks{0};
    std::atomic<bool>       m_abort{false};
    std::atomic<bool>       m_running{false};
    std::atomic<bool>       m_isComplete{false};
    int                     m_lastPercent{-1};
    mutable std::mutex      m_dataSourceMutex;
};

NS_HJ_END
