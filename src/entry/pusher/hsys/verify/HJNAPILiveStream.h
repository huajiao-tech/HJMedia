#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJPluginSpeechRecognizer.h"
#include "HJPrioGraphProc.h"
#include "HJPusherInterface.h"
#include "HJAsyncCache.h"
#include "HJFacePointMgr.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphPusher;
class HJStatContext;
class HJBaseGPUToRAM;
class HJFacePointsReal;

//class HJRoiCom : public HJPrioCom
//{
//public:
//    HJ_DEFINE_CREATE(HJRoiCom);
//
//    HJRoiCom() = default;
//    virtual ~HJRoiCom() = default;
//    virtual int sendMessage(HJBaseMessage::Ptr i_msg) override; 
//
//    std::shared_ptr<HJFacePointsReal> acquire();
//    void recovery(std::shared_ptr<HJFacePointsReal> i_data);
//
//private:
//    HJAsyncCache<std::shared_ptr<HJFacePointsReal>> m_cache;
//};

class HJNAPILiveStream
{
public:
    using HJNAPILiveStreamCb = std::function<void(int i_type, int i_ret, const std::string& i_msgInfo)>;

    HJ_DEFINE_CREATE(HJNAPILiveStream);
    HJNAPILiveStream();
    virtual ~HJNAPILiveStream();

    static int contextInit(const HJEntryContextInfo& i_contextInfo);
    
    int openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIEntryNotify i_notify, const HJEntryStatInfo& i_statInfo, uint64_t &o_surfaceId);
    int setNativeWindow(void* window, int i_width, int i_height, int i_state);
    void closePreview();
    
    int openPusher(const HJPusherVideoInfo& i_videoInfo, const HJPusherAudioInfo& i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo, const HJEntryStatInfo& i_statInfo);
    void closePusher();  
    
    int openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo);

    int openRecorder(const HJPusherRecordInfo &i_recorderInfo);
    void closeRecorder();
    
    void setMute(bool i_mute);

    void setDoubleScreen(bool i_bDoubleScreen);
    void setGiftPusher(bool i_bGiftPusher);

    int openSpeechRecognizer(HJPluginSpeechRecognizer::Call i_call);
    void closeSpeechRecognizer();

    void openEffect(int i_effectType);
    void closeEffect(int i_effectType);
    
    int openNativeSource(bool i_bUsePBO = true);
    void closeNativeSource();
    
    int openFaceu(const std::string &i_faceuUrl, bool i_bDebugPoint = false);
    void closeFaceu();
    
    std::shared_ptr<HJRGBAMediaData> acquireNativeSource();
    void setFaceInfo(HJFacePointsWrapper::Ptr faceInfo);

    void setVideoEncQuantOffset(int i_quantOffset);
    void setFaceProtected(bool i_bProtect);
private:
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
    void priROIPrepare(const HJKeyStorage::Ptr& i_param, int i_width, int i_height);    
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJPrioGraphProc> m_prioGraphProc = nullptr;
    std::shared_ptr<HJGraphPusher> m_graphPusher = nullptr;
    HJNAPIEntryNotify m_notify = nullptr;
    std::shared_ptr<HJStatContext> m_statCtx = nullptr;
        
    //HJRoiCom::Ptr m_roiCom = nullptr;
    int m_previewFps = 30;
    int m_encoderFps = 15;
    
    int m_previewWidth = 0;
    int m_previewHeight = 0;
        
    std::shared_ptr<HJBaseGPUToRAM> m_gpuToRamPtr = nullptr;

    std::mutex m_nativeSourceLock;
    bool m_bNativeSourceOpen = false;
    HJFaceSubscribeFuncPtr m_pointSubscriberPtr = nullptr;
    HJAsyncCache<std::shared_ptr<HJFacePointsReal>> m_cache;

    std::atomic<int> m_quantOffset{-3};
};

NS_HJ_END



