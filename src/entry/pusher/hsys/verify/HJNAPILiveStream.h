#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJPluginSpeechRecognizer.h"
#include "HJPrioGraphProc.h"
#include "HJPusherInterface.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphPusher;
class HJStatContext;

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


private:
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
        
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJPrioGraphProc> m_prioGraphProc = nullptr;
    std::shared_ptr<HJGraphPusher> m_graphPusher = nullptr;
    HJNAPIEntryNotify m_notify = nullptr;
    std::shared_ptr<HJStatContext> m_statCtx = nullptr;
        
    int m_previewFps = 30;
    int m_encoderFps = 15;
    
    int m_previewWidth = 0;
    int m_previewHeight = 0;
};

NS_HJ_END



