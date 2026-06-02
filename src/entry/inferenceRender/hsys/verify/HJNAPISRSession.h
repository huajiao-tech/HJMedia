#pragma once

#include "HJError.h"
#include "HJFaceSRWrapper.h"
#include "HJImageSeqWrapper.h"
#include "HJTransferMediaData.h"
#include "HJThreadPool.h"
#include <mutex>

NS_HJ_BEGIN

struct HJNAPISRStat
{
    int lastRet = HJ_OK;
    HJFaceSRProcessResult result;
};

class HJNAPISRSession
{
public:
    HJ_DEFINE_CREATE(HJNAPISRSession);
    HJNAPISRSession();
    virtual ~HJNAPISRSession();

    int init(const std::string& modelDir, const std::string& seqDir, int fps, bool bLoop);
    int setOptions(bool bFullSR, bool bFaceScale, int faceScaleWidth);

    int start();
    void stop();

    int onWindowChanged(void* window, int width, int height, int state);

    HJNAPISRStat getStat();

private:
    int priProcessOneFrame();

    std::mutex m_mutex;
    std::unique_ptr<HJFaceSRWrapper> m_faceSR = nullptr;
    std::unique_ptr<HJImageSeqWrapper> m_imgSeq = nullptr;
    HJTimerThreadPool::Ptr m_timer = nullptr;

    HJTransferMediaData::Ptr m_latestFrame = nullptr;
    HJNAPISRStat m_stat;

    int m_fps = 15;
    bool m_bLoop = true;
    bool m_bFullSR = false;
    bool m_bFaceScale = true;
    int m_faceScaleWidth = 200;

    bool m_inited = false;
    bool m_renderInited = false;
    void* m_window = nullptr;
    int m_windowW = 0;
    int m_windowH = 0;
};

NS_HJ_END
