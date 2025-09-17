//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJRTMPWrapper.h"
#include "HJExecutor.h"
#include "HJStatisticalTools.h"

//#define HJ_HAVE_NETWORK_LIMITED 1

NS_HJ_BEGIN
//***********************************************************************************//
class HJRTMPAsyncWrapper : public HJRTMPWrapperDelegate
{
public:
    HJ_DECLARE_PUWTR(HJRTMPAsyncWrapper)
    HJRTMPAsyncWrapper(HJRTMPWrapperDelegate::Wtr delegate);
    virtual ~HJRTMPAsyncWrapper();

    virtual int init(const std::string& url, HJOptions::Ptr opts = nullptr);
    virtual void done();
    virtual int start();
    //
    virtual void setQuit(const bool isQuit = true) {
        HJLogi("entry, isQuit:" + HJ2STR(isQuit));
        m_isQuit = isQuit;
    }
    void tryWait();

    int64_t getRetryCount() const {
        return m_retryCount;
    }

    void setNetSimulBitrate(int bitrate) {
        if (bitrate != m_simulaBitrate) {
            m_newSimulator = true;
            m_simulaBitrate = bitrate;
        }
    }
private:
    static const int64_t ACQU_TIMEOUT_DEFAULT;
    static const int64_t RETRY_INTERVAL_DEFAULT;
    static const int64_t LOW_BITRATE_LIMITED;
private:
    virtual int run();
    //
    int createAVIO();
    void destroyAVIO();
    int retryAVIO();
    //
    virtual int notify(HJNotification::Ptr ntf) {
        auto delegate = HJLockWtr<HJRTMPWrapperDelegate>(m_delegate);
        if (delegate) {
            return delegate->onRTMPWrapperNotify(std::move(ntf));
        }
        return HJ_OK;
    }
    /**
    * HJRTMPWrapperDelegate
    */
    virtual int onRTMPWrapperNotify(HJNotification::Ptr ntf) override;
    virtual HJBuffer::Ptr onAcquireMediaTag(int64_t timeout, bool isHeader) override;
    //
    double getRetryInterval(int n);

    //
    int checkNetBitrate(int netkbps);
    void tryNetSimulate(int size);
private:
    std::string         m_url{""};
    HJOptions::Ptr      m_opts{ nullptr };
    HJRTMPWrapperDelegate::Wtr m_delegate;
    HJRTMPWrapper::Ptr  m_wrapper{nullptr};
    HJExecutor::Ptr     m_executor{nullptr};
    bool                m_acquireHeader{ true };
    //
    int64_t             m_retryTimeLimited{60 * 1000};
    int64_t             m_retryTime{HJ_NOTS_VALUE};
    int64_t             m_retryCount{0};
    int64_t             m_retryInterval{RETRY_INTERVAL_DEFAULT};
    bool                m_lowBRTimeoutEnable = false;
    int64_t             m_lowBRTimeoutLimited = { LOW_BITRATE_LIMITED };
    int64_t             m_lowBRLimited = 500;
    //
    std::atomic<bool>   m_isQuit = { false };
    std::mutex          m_wrapperMutex;
    int64_t             m_runTime = HJ_NOTS_VALUE;
    int64_t             m_lowBitrateTime = HJ_NOTS_VALUE;
    HJNetBitrateTracker::Ptr m_netBitTracker;
    HJValueStatisticsInt64   m_valueStatistics;
    //
    int                 m_simulaBitrate{ 0 };
    bool                m_newSimulator{false};
#if defined(HJ_HAVE_NETWORK_LIMITED)
    HJNetworkSimulator::Utr m_simulator{ nullptr };
#endif
};

NS_HJ_END
