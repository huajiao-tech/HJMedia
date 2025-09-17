#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

#include "HJPusherInterface.h"
#include "ThreadSafeFunctionWrapper.h"

NS_HJ_BEGIN
   
class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphComPusher;

class HJNAPITestLive
{
public:
    
    HJ_DEFINE_CREATE(HJNAPITestLive);
    HJNAPITestLive();
    virtual ~HJNAPITestLive();

    static int contextInit(const HJEntryContextInfo& i_contextInfo);
    
    int openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIEntryNotify i_notify, uint64_t &o_surfaceId);
    int setNativeWindow(void* window, int i_width, int i_height, int i_state);
    void closePreview();
    
    int openPusher(const HJPusherVideoInfo& i_videoInfo, const HJPusherAudioInfo& i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo);
    void closePusher();  
    
    int openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo);

    int openRecorder(const HJPusherRecordInfo &i_recorderInfo);
    void closeRecorder();
    
    void setMute(bool i_mute);
        
    void removeInterleave();

    void setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf);

    std::unique_ptr<ThreadSafeFunctionWrapper> getThreadSafeFunction();
private:
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
        
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJPrioGraphProc> m_prioGraphProc = nullptr;
    std::shared_ptr<HJGraphComPusher> m_graphPusher = nullptr;
    HJNAPIEntryNotify m_notify = nullptr;
    static bool s_bContextInit;
    std::unique_ptr<ThreadSafeFunctionWrapper> m_tsf = nullptr;
    
    int m_previewFps = 30;
    int m_encoderFps = 15;
};

NS_HJ_END



