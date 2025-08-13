#include "HJNAPITestLive.h"
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJTransferInfo.h"
#include "HJMediaUtils.h"
#include "HJNotify.h"
#include "HJRTMPUtils.h"
#include "HJComEvent.h"
#include "HJOGRenderEnv.h"
#include "HJGraphComVideoCapture.h"
#include "HJGraphComPusher.h"

NS_HJ_BEGIN
    
bool HJNAPITestLive::s_bContextInit = false;

HJNAPITestLive::HJNAPITestLive()
{

}

HJNAPITestLive::~HJNAPITestLive()
{
   
}
int HJNAPITestLive::contextInit(const HJPusherContextInfo& i_contextInfo)
{
    int i_err = 0;
    do 
    {
        if (!s_bContextInit)
        {
            i_err = HJ::HJLog::Instance().init(i_contextInfo.logIsValid, i_contextInfo.logDir, i_contextInfo.logLevel, i_contextInfo.logMode, true, i_contextInfo.logMaxFileSize, i_contextInfo.logMaxFileNum);
            if (i_err == HJ_OK)
            {
                s_bContextInit = true;
                HJFLogi("log create success");
            }    
        }
    } while (false);
    return i_err;
}
int HJNAPITestLive::openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo)
{
    int i_err = 0;
    do 
    {
        if (m_graphVideoCapture)
        {
            HJ::HJBaseParam::Ptr param = HJ::HJBaseParam::Create();
            (*param)["pngsequrl"] = std::string(i_pngseqInfo.pngSeqUrl);
            i_err = m_graphVideoCapture->openPNGSeq(param);
            if (i_err < 0)
            {
                break;
            }    
        }
    } while (false);
    return i_err;
}

void HJNAPITestLive::closePusher()
{
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("closePush enter");
    if (m_graphPusher)
    {
        m_graphPusher->done();
        m_graphPusher = nullptr;
    }    
    HJFLogi("closePush done end time:{}", (HJCurrentSteadyMS() - t0));
}
void HJNAPITestLive::setMute(bool i_mute)
{
    if (m_graphPusher)
    {
        m_graphPusher->setMute(i_mute);
    }    
}

void HJNAPITestLive::removeInterleave()
{
    if (m_graphPusher)
    {
        m_graphPusher->removeInterleave();
    }    
}
int HJNAPITestLive::openPusher(const HJPusherVideoInfo& i_videoInfo, const HJPusherAudioInfo& i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo)
{
    int i_err = 0;
    do 
    {
        if (m_graphPusher)
        {
            i_err = -1;
            HJFLoge("pusher has already created, not open again");
            break;
        }    
        
        m_graphPusher = HJGraphComPusher::Create();
        
        auto surfaceCb = [this](void *i_window, int i_width, int i_height, bool i_bCreate)
        {
            int state = 0;
            if (i_bCreate)
            {
                state = HJTargetState_Create;
            } 
            else 
            {
                state = HJTargetState_Destroy;
            }
            int64_t t0 = HJCurrentSteadyMS();
            int ret =  priSetWindow(i_window, i_width, i_height, state, HJOGEGLSurfaceType_Encoder, m_encoderFps);
            HJFLogi("setwindow state:{} timediff:{}", state, (HJCurrentSteadyMS() - t0));
            return ret;
        };
        
        HJBaseParam::Ptr param = HJBaseParam::Create();
        (*param)[HJ_CatchName(HOVideoSurfaceCb)] = (HOVideoSurfaceCb)surfaceCb;
        
        HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>();
        videoInfo->m_codecID = i_videoInfo.videoCodecId;
        videoInfo->m_width = i_videoInfo.videoWidth;
        videoInfo->m_height = i_videoInfo.videoHeight;
        videoInfo->m_bitrate = i_videoInfo.videoBitrateBit;
        videoInfo->m_frameRate = i_videoInfo.videoFramerate;
        videoInfo->m_gopSize = i_videoInfo.videoGopSize;
        
        m_encoderFps = i_videoInfo.videoFramerate;
        
        (*param)[HJ_CatchName(HJVideoInfo::Ptr)] = videoInfo;
        
        HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
        audioInfo->m_codecID = i_audioInfo.audioCodecId;
        audioInfo->m_bitrate = i_audioInfo.audioBitrateBit;
        audioInfo->m_sampleFmt = i_audioInfo.audioSampleFormat;
        audioInfo->m_samplesRate = i_audioInfo.audioCaptureSampleRate;
        audioInfo->setChannels(i_audioInfo.audioCaptureChannels);
        (*param)[HJ_CatchName(HJAudioInfo::Ptr)] = audioInfo;
        
        HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(i_rtmpInfo.rtmpUrl);
        (*param)[HJ_CatchName(HJMediaUrl::Ptr)] = mediaUrl;
        HJListener rtmpListener = [&](const HJNotification::Ptr ntf) -> int { 
            if (!ntf) {
                return HJ_OK;
            }
            HJFLogi("rtmp notify id:{}, msg:{}", HJRTMPEEventToString((HJRTMPEEvent)ntf->getID()), ntf->getMsg());
            int type = HJ_PUSHER_NOTIFY_NONE;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
                case HJRTMP_EVENT_STREAM_CONNECTED: {
                    type = HJ_PUSHER_NOTIFY_CONNECT_SUCCESS;
                    break;
                }
                case HJRTMP_EVENT_CONNECT_FAILED:
                case HJRTMP_EVENT_STREAM_CONNECT_FAIL: { 
                    type = HJ_PUSHER_NOTIFY_CONNECT_FAILED;
                    break;
                }
                case HJRTMP_EVENT_DISCONNECTED: { 
                    type = HJ_PUSHER_NOTIFY_CONNECT_CLOSED;
                    break;
                }
                case HJRTMP_EVENT_RETRY:{
                    type = HJ_PUSHER_NOTIFY_CONNECT_RETRY;
                    break;
                }
                case HJRTMP_EVENT_RECV_Error:
                case HJRTMP_EVENT_SEND_Error: { 
                    type = HJ_PUSHER_NOTIFY_ERROR;
                    break;
                }
                default: {
                    break;
                }
            }
            if(m_notify && (HJ_PUSHER_NOTIFY_NONE != type)) {
                m_notify(type, msg);
            }
			return HJ_OK;
		};
        (*param)["rtmpListener"] = rtmpListener;
         
        i_err = m_graphPusher->init(param);
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
void HJNAPITestLive::closePreview()
{
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("closePreview enter");
    if (m_graphPusher)
    {
        HJFLogi("closePreview m_graphPusher enter");
        m_graphPusher->done();
        HJFLogi("closePreview m_graphPusher end");
        m_graphPusher = nullptr;
    }    
    
    HJFLogi("closePreview m_graphVideoCapture done enter");
    if (m_graphVideoCapture)
    {
        HJFLogi("closePreview m_graphVideoCapture-> done enter");
        m_graphVideoCapture->done();
        HJFLogi("closePreview m_graphVideoCapture-> done end");
        m_graphVideoCapture = nullptr;
    }     
    HJFLogi("closePreview m_graphVideoCapture done end time:{}", (HJCurrentSteadyMS() - t0));
}

int HJNAPITestLive::openRecorder(const HJPusherRecordInfo &i_recorderInfo)
{
    int i_err = 0;
    do 
    {
        if (m_graphPusher)
        {
            HJBaseParam::Ptr param = HJBaseParam::Create();
            (*param)["localRecordUrl"] = (std::string)(i_recorderInfo.recordUrl);
            i_err = m_graphPusher->openRecorder(param);
            if (i_err < 0)
            {
                break;
            }    
        }    
    } while (false);
    return i_err;
}
void HJNAPITestLive::closeRecorder()
{
    if (m_graphPusher)
    {
        m_graphPusher->closeRecorder();
    }   
}
int HJNAPITestLive::openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIPusherNotify i_notify, uint64_t &o_surfaceId)
{
    int i_err = 0;
	do
	{
        if (i_previewInfo.videoWidth <= 0 || i_previewInfo.videoHeight <= 0 || i_previewInfo.videoFps <= 0)
        {
            i_err = -1;
            HJLoge("param error");
            break;
        }
        
        if (m_graphVideoCapture)
        {
            i_err = -1;
            HJFLoge("pusher has already created, not open again");
            break;
        } 
        
        m_notify = i_notify;
        
        HJBaseParam::Ptr param = HJBaseParam::Create();
        (*param)[HJBaseParam::s_paramFps] = i_previewInfo.videoFps;
        m_previewFps = i_previewInfo.videoFps;
        
        HJTransferRenderModeInfo renderModeInfo;
        renderModeInfo.color = "BT601";
        renderModeInfo.cropMode = "clip";
        renderModeInfo.viewOffx = 0.0;
        renderModeInfo.viewOffy = 0.0;
        renderModeInfo.viewWidth = 1.0;
        renderModeInfo.viewHeight = 1.0;
        std::string renderModeStr = renderModeInfo.initSerial();
        if (renderModeStr.empty())
        {
            i_err = -1;
            HJFLoge("initSerial error");
            break;
        }    
        (*param)[HJBaseParam::s_paramRenderBridge] = (std::string)renderModeStr;

         m_graphVideoCapture = HJGraphComVideoCapture::Create();
        if (!m_graphVideoCapture)
        {
            i_err = -1;
            HJFLoge("HJGraphComVideoCapture create error");
            break;
        }
        
        HJListener renderListener = [&](const HJNotification::Ptr ntf) -> int
        {
            if (!ntf)
            {
                return HJ_OK;
            }
            HJFLogi("graph video capture notify id:{}, msg:{}", HJRenderVideoCaptureEventToString((HJRenderVideoCaptureEvent)ntf->getID()), ntf->getMsg());
            int type = HJVIDEOCAPTURE_EVENT_NONE;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
            case HJVIDEOCAPTURE_EVENT_ERROR_DEFAULT:
            {
                type = HJ_RENDER_NOTIFY_ERROR_DEFAULT;
                break;
            }
            case HJVIDEOCAPTURE_EVENT_ERROR_UPDATE:
            {
                type = HJ_RENDER_NOTIFY_ERROR_UPDATE;
                break;
            }
            case HJVIDEOCAPTURE_EVENT_ERROR_DRAW:
            {
                type = HJ_RENDER_NOTIFY_ERROR_DRAW;
                break;
            }
            case HJVIDEOCAPTURE_EVENT_ERROR_VIDEOCAPTURE_INIT:
            {
                type = HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT;
                break;
            }
            case HJVIDEOCAPTURE_EVENT_ERROR_PNGSEQ_INIT:
            {
                type = HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT;
                break;
            }
            case HJVIDEOCAPTURE_EVENT_PNGSEQ_COMPLETE:
            {
                type = HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE;
                break;
            }
            default:
            {
                break;
            }
            }
            if (m_notify && (HJVIDEOCAPTURE_EVENT_NONE != type))
            {
                m_notify(type, msg);
            }
            return HJ_OK;
        };
        (*param)["renderListener"] = (HJListener)renderListener;
        
        i_err = m_graphVideoCapture->init(param);
        if (i_err < 0)
        {
            HJLoge("m_graphVideoCapture init error:{}", i_err);
            break;
        }
        if (m_graphVideoCapture)
        {
            m_bridge = m_graphVideoCapture->renderWindowBridgeAcquire();
        }    
        
        if (!m_bridge)
        {
            i_err = -1;
            HJFLoge("acquire brideg error");
            break;
        }
        
        i_err = m_bridge->getSurfaceId(o_surfaceId);
        if (i_err < 0)
        {
            i_err = -1;
            HJFLoge("get SurfaceId error");
            break;
        }  
        
        NativeWindow *nativeWindow = m_bridge->getNativeWindow();
        i_err = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, i_previewInfo.videoWidth, i_previewInfo.videoHeight);
        if (i_err != 0)
        {
            i_err = -1;
            HJFLoge("OH_NativeWindow_NativeWindowHandleOpt set wh error w:{} h:{}", i_previewInfo.videoWidth, i_previewInfo.videoHeight);
            break;
        }

	} while (false);
	return i_err;    
}
int HJNAPITestLive::setNativeWindow(void* window, int i_width, int i_height, int i_state)
{
    return priSetWindow(window, i_width, i_height, i_state, HJOGEGLSurfaceType_Default, m_previewFps);
}
        
int HJNAPITestLive::priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps)
{
    int i_err = 0;
    do 
    {
        HJTransferRenderTargetInfo renderTargetInfo;
        renderTargetInfo.nativeWindow = (int64_t)i_window;
        renderTargetInfo.width = i_width;
        renderTargetInfo.height = i_height;
        renderTargetInfo.state = i_state;
        renderTargetInfo.type = i_type;
        renderTargetInfo.fps = i_fps;
        std::string renderTargetStr = renderTargetInfo.initSerial();
        if (renderTargetStr.empty())
        {
            HJFLoge("initSerial error");
            i_err = -1;
            break;
        }    
        if (m_graphVideoCapture)
        {
            i_err = m_graphVideoCapture->eglSurfaceProc(renderTargetStr);
            if (i_err < 0)
            {
                HJFLoge("eglSurfaceProc error i_err:{} w:{} h:{} state:{}", i_err, i_width, i_height, i_state);
                break;
            }   
        }
    } while (false);
    return i_err;
}

void HJNAPITestLive::setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_tsf = std::move(i_tsf);
}

std::unique_ptr<ThreadSafeFunctionWrapper> HJNAPITestLive::getThreadSafeFunction() {
    return std::move(m_tsf);
}

NS_HJ_END