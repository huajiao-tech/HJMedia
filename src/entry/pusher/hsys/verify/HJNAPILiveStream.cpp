#include "HJNAPILiveStream.h"
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJTransferInfo.h"
#include "HJPrioGraphProc.h"
#include "HJMediaUtils.h"
#include "HJComEvent.h"
#include "HJGraphPusher.h"
#include "HJOGRenderWindowBridge.h"
#include "HJStatContext.h"
#include "HJPrioUtils.h"
#include "HJEntryContext.h"

NS_HJ_BEGIN

HJNAPILiveStream::HJNAPILiveStream()
{
}

HJNAPILiveStream::~HJNAPILiveStream()
{
    HJFLogi("~HJNAPILiveStream {}", size_t(this));
}
int HJNAPILiveStream::contextInit(const HJEntryContextInfo &i_contextInfo)
{
    int i_err = 0;
    do
    {
        i_err = HJ::HJEntryContext::init(HJEntryType_Pusher, i_contextInfo);
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
int HJNAPILiveStream::openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo)
{
    int i_err = 0;
    do
    {
        if (m_prioGraphProc)
        {
            HJ::HJBaseParam::Ptr param = HJ::HJBaseParam::Create();
            (*param)["pngsequrl"] = std::string(i_pngseqInfo.pngSeqUrl);
            i_err = m_prioGraphProc->openPNGSeq(param);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
}

void HJNAPILiveStream::closePusher()
{
    HJFLogi("closePush enter");
    if (m_graphPusher)
    {
        m_graphPusher->done();
        m_graphPusher = nullptr;
    }
    HJFLogi("closePush end");
}
void HJNAPILiveStream::setMute(bool i_mute)
{
    if (m_graphPusher)
    {
        m_graphPusher->setMute(i_mute);
    }
}

int HJNAPILiveStream::openPusher(const HJPusherVideoInfo &i_videoInfo, const HJPusherAudioInfo &i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo, const HJEntryStatInfo& i_statInfo)
{
    int i_err = 0;
    HJFLogi("openPush enter");
    do
    {
        if (i_statInfo.bUseStat)
        {
            m_statCtx = HJStatContext::Create();
            i_err = m_statCtx->init(i_statInfo.uid, i_statInfo.device, i_statInfo.sn, i_statInfo.interval, 0, [i_statInfo](const std::string & i_name, int i_nType, const std::string & i_info)
            {
                HJFLogi("StatStart callback name:{}, type:{}, info:{}", i_name, i_nType, i_info);
                i_statInfo.statNotify(i_name, i_nType, i_info);
            });
            if (i_err < 0)
            {
                i_err = -1;
                HJFLoge("m_statCtx init error");
                break;
            }
        }

        if (m_graphPusher)
        {
            i_err = -1;
            HJFLoge("pusher has already created, not open again");
            break;
        }
        m_graphPusher = HJGraphPusher::Create();

        auto param = std::make_shared<HJKeyStorage>();

        HJOHSurfaceCb surfaceCb = [this](void *i_window, int i_width, int i_height, bool i_bCreate)
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
            HJFLogi("setwindow enter {}", i_bCreate ? "create" : "detroy");
            int ret = priSetWindow(i_window, i_width, i_height, state, HJOGEGLSurfaceType_EncoderPusher, m_encoderFps);
            HJFLogi("setwindow state:{} timediff:{}", state, (HJCurrentSteadyMS() - t0));
            return ret;
        };
        (*param)["surfaceCb"] = surfaceCb;

        HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>();
        videoInfo->m_codecID = i_videoInfo.videoCodecId;
        videoInfo->m_width = i_videoInfo.videoWidth;
        videoInfo->m_height = i_videoInfo.videoHeight;
        videoInfo->m_bitrate = i_videoInfo.videoBitrateBit;
        videoInfo->m_frameRate = i_videoInfo.videoFramerate;
        videoInfo->m_gopSize = i_videoInfo.videoGopSize;
        
        m_encoderFps = i_videoInfo.videoFramerate;
        
        (*param)["videoInfo"] = videoInfo;

        HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
        audioInfo->m_codecID = i_audioInfo.audioCodecId;
        audioInfo->m_bitrate = i_audioInfo.audioBitrateBit;
        audioInfo->m_sampleFmt = i_audioInfo.audioSampleFormat;
        audioInfo->m_samplesRate = i_audioInfo.audioCaptureSampleRate;
        audioInfo->setChannels(i_audioInfo.audioCaptureChannels);

        // fixme set audio encoder
        // i_audioInfo.audioEncoderSampleRate
        // i_audioInfo.audioEncoderChannels

        (*param)["audioInfo"] = audioInfo;

        HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(i_rtmpInfo.rtmpUrl);
        (*param)["mediaUrl"] = mediaUrl;
        HJListener rtmpListener = [&](const HJNotification::Ptr ntf) -> int
        {
            if (!ntf)
            {
                return HJ_OK;
            }
            HJFLogi("rtmp notify id:{}, msg:{}", HJRTMPEEventToString((HJRTMPEEvent)ntf->getID()), ntf->getMsg());
            int type = HJ_PUSHER_NOTIFY_NONE;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
            case HJRTMP_EVENT_STREAM_CONNECTED:
            {
                type = HJ_PUSHER_NOTIFY_CONNECT_SUCCESS;
                break;
            }
            case HJRTMP_EVENT_CONNECT_FAILED:
            case HJRTMP_EVENT_STREAM_CONNECT_FAIL:
            {
                type = HJ_PUSHER_NOTIFY_CONNECT_FAILED;
                break;
            }
            case HJRTMP_EVENT_DISCONNECTED:
            {
                type = HJ_PUSHER_NOTIFY_CONNECT_CLOSED;
                break;
            }
            case HJRTMP_EVENT_DROP_FRAME:
            {
                type = HJ_PUSHER_NOTIFY_DROP_FRAME;
                break;
            }
            case HJRTMP_EVENT_AUTOADJUST_BITRATE:
            {
                type = HJ_PUSHER_NOTIFY_AUTOADJUST_BITRATE;
                if(m_graphPusher) {
                    int bps = ntf->getVal() * 1000;
                    m_graphPusher->adjustBitrate(bps);
                }
                break;
            }
            case HJRTMP_EVENT_RETRY:
            {
                type = HJ_PUSHER_NOTIFY_CONNECT_RETRY;
                break;
            }
            case HJRTMP_EVENT_RECV_Error:
            case HJRTMP_EVENT_SEND_Error:
            {
                type = HJ_PUSHER_NOTIFY_ERROR;
                break;
            }
            case HJRTMP_EVENT_LIVE_INFO:
            {
                type = HJ_PUSHER_NOTIFY_LIVE_INFO;
                auto kbps = ntf->getInt("kbps");
                auto fps = ntf->getInt("fps");
                auto delay = ntf->getInt("delay");
                HJFLogi("live stream HJRTMP_EVENT_LIVE_INFO -- kbps:{}, fps:{}, delay:{}", kbps, fps, delay);
                //
                if(fps > 0 && kbps > 0) {
                    auto json = HJYJsonDocument::create();
                    (*json)["kbps"] = kbps;
                    (*json)["fps"] = fps;
                    (*json)["delay"] = delay;
                    msg = json->getSerialInfo();
                    HJFLogi("live stream HJRTMP_EVENT_LIVE_INFO -- msg{}:", msg);
                } else {
                    type = HJ_PUSHER_NOTIFY_NONE;
                    HJFLogi("live stream HJRTMP_EVENT_LIVE_INFO -- msg[] {}:", msg);
                }
                break;
            }
            default:
            {
                break;
            }
            }
            if (m_notify && (HJ_PUSHER_NOTIFY_NONE != type))
            {
                m_notify(type, msg);
            }
            return HJ_OK;
        };
        (*param)["rtmpListener"] = rtmpListener;

        HJListener pusherListener = [&](const HJNotification::Ptr ntf) -> int {
            if (!ntf) {
                return HJ_OK;
            }
            HJFLogi("pusher notify id:{}, msg:{}", HJPluginNofityTypeToString((HJPluginNofityType)ntf->getID()), ntf->getMsg());
            int type = HJ_PUSHER_NOTIFY_NONE;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
            case HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT:
                type = HJ_PUSHER_NOTIFY_ERROR_MUXER_INIT;
                break;
            case HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME:
                type = HJ_PUSHER_NOTIFY_ERROR_MUXER_WRITEFRAME;
                break;
            case HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN:
                type = HJ_PUSHER_NOTIFY_ERROR_CODEC_RUN;
                break;
            case HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME:
                type = HJ_PUSHER_NOTIFY_ERROR_CODEC_GETFRAME;
                break;
            case HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME:
                type = HJ_PUSHER_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME;
                break;
            case HJ_PLUGIN_NOTIFY_ERROR_CAPTURER_GETFRAME:
                type = HJ_PUSHER_NOTIFY_ERROR_CAPTURER_GETFRAME;
                break;
            default:
                break;
            }
            if (m_notify && (HJ_PUSHER_NOTIFY_NONE != type)) {
                m_notify(type, msg);
            }
            return HJ_OK;
        };
        (*param)["pusherListener"] = pusherListener;

        //
        if(m_statCtx) {
            std::weak_ptr<HJStatContext> statCtx = m_statCtx;
            (*param)["HJStatContext"] = statCtx;
        }
        i_err = m_graphPusher->init(param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    HJFLogi("openPush end i_err:{}", i_err);
    return i_err;
}
void HJNAPILiveStream::closePreview()
{
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("closePreview enter");
    if (m_graphPusher)
    {
        HJFLogi("m_graphPusher done enter");
        m_graphPusher->done();
        HJFLogi("m_graphPusher done end");
        m_graphPusher = nullptr;
    }
    HJFLogi("m_graphVideoCapture done enter");
    if (m_prioGraphProc)
    {
        m_prioGraphProc->done();
        m_prioGraphProc = nullptr;
    }
    HJFLogi("m_graphVideoCapture done end time:{}", (HJCurrentSteadyMS() - t0));
}

int HJNAPILiveStream::openRecorder(const HJPusherRecordInfo &i_recorderInfo)
{
    int i_err = 0;
    do
    {
        HJFLogi("openRecorder enter");
        if (m_graphPusher)
        {
            auto param = std::make_shared<HJKeyStorage>();
            HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(i_recorderInfo.recordUrl);
            (*param)["mediaUrl"] = mediaUrl;
            i_err = m_graphPusher->openRecorder(param);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    HJFLogi("openRecorder end i_err:{}", i_err);
    return i_err;
}
void HJNAPILiveStream::closeRecorder()
{
    HJFLogi("closeRecorder enter");
    if (m_graphPusher)
    {
        m_graphPusher->closeRecorder();
    }
    HJFLogi("closeRecorder end");
}
void HJNAPILiveStream::setDoubleScreen(bool i_bDoubleScreen)
{
    HJFLogi("setDoubleScreen");
    do
    {
        if (m_prioGraphProc)
        {
            bool bLandscape = m_previewWidth > m_previewHeight;
            m_prioGraphProc->setDoubleScreen(i_bDoubleScreen, bLandscape);
        }
    } while (false);
}
void HJNAPILiveStream::setGiftPusher(bool i_bGiftPusher)
{
    HJFLogi("setGiftPusher");
    if (m_prioGraphProc)
    {
        m_prioGraphProc->setGiftPusher(i_bGiftPusher);
    }
}
int HJNAPILiveStream::openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIEntryNotify i_notify, const HJEntryStatInfo& i_statInfo, uint64_t &o_surfaceId)
{
    int i_err = 0;
    do
    {
        HJFLogi("openPreview enter");
        if (i_previewInfo.videoWidth <= 0 || i_previewInfo.videoHeight <= 0 || i_previewInfo.videoFps <= 0)
        {
            i_err = -1;
            HJLoge("param error");
            break;
        }

        if (m_prioGraphProc)
        {
            i_err = -1;
            HJFLoge("pusher has already created, not open again");
            break;
        }
        m_previewWidth = i_previewInfo.videoWidth;
        m_previewHeight = i_previewInfo.videoHeight;
        m_notify = i_notify;

        if (i_statInfo.bUseStat)
        {
            m_statCtx = HJStatContext::Create();
            i_err = m_statCtx->init(i_statInfo.uid, i_statInfo.device, i_statInfo.sn, i_statInfo.interval, 0, [](const std::string & i_name, int i_nType, const std::string & i_info)
	        {
		        HJFLogi("StatStart callback name:{}, type:{}, info:{}", i_name, i_nType, i_info);
	        });
            if (i_err < 0)
            {
                i_err = -1;
                HJFLoge("m_statCtx init error");
                break;
            }    
        }    
        
        HJBaseParam::Ptr param = HJBaseParam::Create();
        (*param)[HJBaseParam::s_paramFps] = i_previewInfo.videoFps;
        m_previewFps = i_previewInfo.videoFps;

        m_prioGraphProc = HJPrioGraphProc::Create();
        if (!m_prioGraphProc)
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
            HJFLogi("graph video capture notify id:{}, msg:{}", HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID()), ntf->getMsg());
            int type = HJVIDEORENDERGRAPH_EVENT_NONE;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
            case HJVIDEORENDERGRAPH_EVENT_ERROR_DEFAULT:
            {
                type = HJ_RENDER_NOTIFY_ERROR_DEFAULT;
                break;
            }
            case HJVIDEORENDERGRAPH_EVENT_ERROR_UPDATE:
            {
                type = HJ_RENDER_NOTIFY_ERROR_UPDATE;
                break;
            }
            case HJVIDEORENDERGRAPH_EVENT_ERROR_DRAW:
            {
                type = HJ_RENDER_NOTIFY_ERROR_DRAW;
                break;
            }
            case HJVIDEORENDERGRAPH_EVENT_ERROR_INIT:
            {
                type = HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT;
                break;
            }
            case HJVIDEORENDERGRAPH_EVENT_ERROR_PNGSEQ_INIT:
            {
                type = HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT;
                break;
            }
            case HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE:
            {
                type = HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE;
                break;
            }
            default:
            {
                break;
            }
            }
            if (m_notify && (HJVIDEORENDERGRAPH_EVENT_NONE != type))
            {
                m_notify(type, msg);
            }
            return HJ_OK;
        };
        (*param)["renderListener"] = (HJListener)renderListener;
        HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioComSourceType), (int)HJPrioComSourceType_SERIES);
        i_err = m_prioGraphProc->init(param);
        if (i_err < 0)
        {
            HJLoge("m_graphVideoCapture init error:{}", i_err);
            break;
        }

        if (m_prioGraphProc)
        {
            m_bridge = m_prioGraphProc->renderWindowBridgeAcquire();
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
    HJFLogi("openPreview end i_err:{}", i_err);
    return i_err;
}
int HJNAPILiveStream::setNativeWindow(void *window, int i_width, int i_height, int i_state)
{
    return priSetWindow(window, i_width, i_height, i_state, HJOGEGLSurfaceType_UI, m_previewFps);
}

int HJNAPILiveStream::priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps)
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
            i_err = -1;
            break;
        }
        if (m_prioGraphProc)
        {
            i_err = m_prioGraphProc->eglSurfaceProc(renderTargetStr);
            if (i_err < 0)
            {
                HJFLoge("eglSurfaceProc error i_err:{} w:{} h:{} state:{}", i_err, i_width, i_height, i_state);
                break;
            }
        }
    } while (false);
    return i_err;
}

int HJNAPILiveStream::openSpeechRecognizer(HJPluginSpeechRecognizer::Call i_call) {
    int i_err = 0;
    do {
        HJFLogi("openSpeechRecognizer enter");
        if (m_graphPusher) {
            auto param = std::make_shared<HJKeyStorage>();
            (*param)["speechRecognizerCall"] = i_call;
            i_err = m_graphPusher->openSpeechRecognizer(param);
            if (i_err < 0) {
                break;
            }
        }
    } while (false);
    HJFLogi("openSpeechRecognizer end i_err:{}", i_err);
    return i_err;
}

void HJNAPILiveStream::closeSpeechRecognizer() {
    HJFLogi("closeSpeechRecognizer enter");
    if (m_graphPusher) {
        m_graphPusher->closeSpeechRecognizer();
    }
    HJFLogi("closeSpeechRecognizer end");
}

NS_HJ_END