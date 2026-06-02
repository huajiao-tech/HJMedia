#include "HJNAPISRSession.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJFaceDetectExport.h"
#include "HJVideoSRExport.h"
#include "HJRteGraphSetupInfo.h"
#include "HJTransferInfo.h"
#include <algorithm>
#include "HJDataConvert.h"

NS_HJ_BEGIN

HJNAPISRSession::HJNAPISRSession()
{
}

HJNAPISRSession::~HJNAPISRSession()
{
    stop();
}

int HJNAPISRSession::init(const std::string& modelDir, const std::string& seqDir, int fps, bool bLoop)
{
    if (modelDir.empty() || seqDir.empty() || fps <= 0)
    {
        return HJErrInvalidParams;
    }

    stop();

    std::lock_guard<std::mutex> guard(m_mutex);

    m_faceSR = std::make_unique<HJFaceSRWrapper>();
    m_imgSeq = std::make_unique<HJImageSeqWrapper>();
    if (!m_faceSR || !m_imgSeq)
    {
        return HJErrNotInited;
    }

    HJFaceDetectWrapperOption faceOpt;
    faceOpt.ncnnRetinaFaceThreadNums = 1;
    faceOpt.retinaFaceTargetSize = 200;
    faceOpt.ncnnRetinaFaceUseGPU = false;
    faceOpt.ncnnScrfdUseGPU = false;
    faceOpt.ncnnScrfdThreadNums = 1;
    
    HJVideoSRWrapperOption srOpt;
    
    
    srOpt.ncnnThreadNums = 4;
    srOpt.ncnnUseGPU = true;
    srOpt.ncnnScale = 4;
    HJFLogi("HJNAPISRSession MindSpore verify request accelerator threadNum:{} useGPU:{}",
        srOpt.ncnnThreadNums, srOpt.ncnnUseGPU);

    int ret = m_faceSR->init(
        modelDir,
        HJFaceDetectWrapperType_NCNNRETINAFACE,
        HJVideoSRWrapperType_MINDSPOREREALESRGAN,//HJVideoSRWrapperType_NCNNREALESRGAN,
        faceOpt,
        srOpt);
    if (ret < 0)
    {
        HJFLoge("HJNAPISRSession init faceSR failed ret:{}", ret);
        return ret;
    }
    HJFLogi("m_faceSR init ok");
    ret = m_imgSeq->init(seqDir, fps, HJConvertDataType_RGB, bLoop);
    if (ret < 0)
    {
        HJFLoge("HJNAPISRSession init image seq failed ret:{}", ret);
        m_faceSR->done();
        return ret;
    }

    m_fps = fps;
    m_bLoop = bLoop;
    m_latestFrame = nullptr;
    m_stat = HJNAPISRStat();
    m_inited = true;
    return HJ_OK;
}

int HJNAPISRSession::setOptions(bool bFullSR, bool bFaceScale, int faceScaleWidth)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    (void)bFullSR;
    (void)bFaceScale;
    (void)faceScaleWidth;
    m_bFullSR = true;
    m_bFaceScale = false;
    m_faceScaleWidth = 180;
    HJFLogi("HJNAPISRSession force options FullScale 180x320 for MindSpore verify");
    return HJ_OK;
}

int HJNAPISRSession::start()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (!m_inited || !m_faceSR || !m_imgSeq)
    {
        return HJErrNotInited;
    }
    if (m_timer)
    {
        return HJ_OK;
    }
    
    m_timer = HJTimerThreadPool::Create();
    if (!m_timer)
    {
        return HJErrNotInited;
    }

    int64_t intervalMs = std::max<int64_t>(1, 1000 / m_fps);
    int ret = m_timer->startTimer(intervalMs, [this]()
    {
        return priProcessOneFrame();
    }, false);
    if (ret < 0)
    {
        m_timer = nullptr;
        return ret;
    }
    return HJ_OK;
}

void HJNAPISRSession::stop()
{
    HJTimerThreadPool::Ptr timer = nullptr;
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        timer = m_timer;
        m_timer = nullptr;
    }
    if (timer)
    {
        timer->stopTimer();
    }

    std::lock_guard<std::mutex> guard(m_mutex);

    if (m_faceSR)
    {
        m_faceSR->done();
    }
    m_faceSR.reset();
    m_imgSeq.reset();
    m_latestFrame = nullptr;
    m_stat = HJNAPISRStat();
    m_renderInited = false;
    m_inited = false;
}

int HJNAPISRSession::onWindowChanged(void* window, int width, int height, int state)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_window = window;
    m_windowW = width;
    m_windowH = height;
    if (!m_renderInited)
    {
        return HJ_OK;
    }
    return 0;
}

HJNAPISRStat HJNAPISRSession::getStat()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_stat;
}

int HJNAPISRSession::priProcessOneFrame()
{
    static constexpr int kMindSporeFullScaleWidth = 180;
    static constexpr int kMindSporeFullScaleHeight = 320;

    HJImageSeqWrapper* imgSeq = nullptr;
    HJFaceSRWrapper* faceSR = nullptr;
    bool bFull = false;
    bool bFaceScale = false;
    int faceScaleWidth = 200;

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (!m_imgSeq || !m_faceSR)
        {
            return HJ_OK;
        }
        imgSeq = m_imgSeq.get();
        faceSR = m_faceSR.get();
        bFull = m_bFullSR;
        bFaceScale = m_bFaceScale;
        faceScaleWidth = m_faceScaleWidth;
    }

    HJTransferMediaData::Ptr input = imgSeq->acquire();
    if (!input)
    {
        return HJ_OK;
    }

    HJFaceSRProcessResult processRet;
    HJFaceSRProcessOption processOption;
    processOption.mode = bFull ? HJFaceSRProcessMode_FullScale : (bFaceScale ? HJFaceSRProcessMode_FaceScale : HJFaceSRProcessMode_FaceOrigin);
    processOption.preScaleWidth = bFull ? kMindSporeFullScaleWidth : 0;
    processOption.preScaleHeight = bFull ? kMindSporeFullScaleHeight : 0;
    processOption.bFeather = true;
    processOption.mixAlphaRatio = 1.0f;
    processOption.policy = HJFaceSRProcessPolicy_Mipmap;
    int ret = faceSR->process(input, processOption, processRet);
    HJFLogi("m_faceSR process bFull:{} bFaceScale:{} fsw:{} prescale:{}x{} facedetect:{} sr:{}",
        bFull, bFaceScale, faceScaleWidth, processOption.preScaleWidth, processOption.preScaleHeight,
        processRet.faceDetectElapsedMs, processRet.srElapsedMs);
    HJTransferMediaData::Ptr renderFrame = input;
    if (ret == HJ_OK)
    {
        HJTransferMediaData::Ptr srOut = faceSR->takeLastOutput();
        if (srOut)
        {
            renderFrame = srOut;
        }
        
        if (renderFrame)
        {
            
            static int iii = 0;
            iii++;
            if ((iii % 10) == 0)
            {
                int pitches[4] = { renderFrame->getWidth() * 4, 0, 0, 0 };
                unsigned char* planes[4] = { renderFrame->getBuffer()->getBuf(), nullptr, nullptr, nullptr };
                std::string imageUrl = "/data/storage/el2/base/haps/entry/files/log_sr/" + std::to_string(iii) + ".jpg";
                HJDataConvert::cvtSaveToImage(HJConvertDataType_RGBA, planes, pitches, renderFrame->getWidth(), renderFrame->getHeight(), imageUrl);
            }
        }
    }
    if (renderFrame && renderFrame->getConvertType() != HJConvertDataType_RGBA)
    {
        HJTransferMediaData::Ptr rgba = HJTransferMediaData::create(renderFrame, HJConvertDataType_RGBA);
        if (rgba)
        {
            renderFrame = rgba;
        }
    }

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_latestFrame = renderFrame;
        m_stat.lastRet = ret;
        m_stat.result = processRet;
    }

    imgSeq->recovery(input);
    return HJ_OK;
}

NS_HJ_END
