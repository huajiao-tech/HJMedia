#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJPusherInterface.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJGraphComVideoCapture;
class HJGraphPusher;

class HJNAPILiveStream
{
public:
    using HJNAPILiveStreamCb = std::function<void(int i_type, int i_ret, const std::string& i_msgInfo)>;

    HJ_DEFINE_CREATE(HJNAPILiveStream);
    HJNAPILiveStream();
    virtual ~HJNAPILiveStream();

    static int contextInit(const HJPusherContextInfo& i_contextInfo);
    
    int openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIPusherNotify i_notify, uint64_t &o_surfaceId);
    int setNativeWindow(void* window, int i_width, int i_height, int i_state);
    void closePreview();
    
    int openPusher(const HJPusherVideoInfo& i_videoInfo, const HJPusherAudioInfo& i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo);
    void closePusher();  
    
    int openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo);

    int openRecorder(const HJPusherRecordInfo &i_recorderInfo);
    void closeRecorder();
    
    void setMute(bool i_mute);
private:
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
        
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJGraphComVideoCapture> m_graphVideoCapture = nullptr;
    std::shared_ptr<HJGraphPusher> m_graphPusher = nullptr;
    HJNAPIPusherNotify m_notify = nullptr;
    
    static bool s_bContextInit;
    
    int m_previewFps = 30;
    int m_encoderFps = 15;
};

NS_HJ_END



