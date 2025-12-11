#include "HJNAPIPlayer.h"
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJTransferInfo.h"
#include "HJMediaUtils.h"
#include "HJNotify.h"
#include "HJRTMPUtils.h"
#include "HJComEvent.h"
#include "HJOGRenderEnv.h"
#include "HJPrioGraphProc.h"
#include "HJGraphLivePlayer.h"
#include "HJOGRenderWindowBridge.h"
#include "HJThreadPool.h"
#include "HJStatContext.h"
#include "HJEntryContext.h"
#include "HJNetManager.h"

#if defined (HarmonyOS)
    #include "HJGPUToRAM.h"
#endif

NS_HJ_BEGIN

std::mutex HJNAPIPlayer::s_playerMutex;
std::set<int> HJNAPIPlayer::s_playerIdxSet;
std::atomic<int> HJNAPIPlayer::s_insIdx{-1};

HJNAPIPlayer::HJNAPIPlayer()
{

}

HJNAPIPlayer::~HJNAPIPlayer()
{
    HJFLogi("{} playerexit ~HJNAPIPlayer", getInsName());
}

int HJNAPIPlayer::contextInit(const HJEntryContextInfo& i_contextInfo)
{
    int i_err = 0;
    do 
    {
        i_err = HJEntryContext::init(HJEntryType_Player, i_contextInfo);
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
void HJNAPIPlayer::preloadUrl(const std::string& url)
{
    HJNetManager::preloadUrl(url);
}
void HJNAPIPlayer::exitPlayer()
{
    HJFLogi("{} playerexit exitPlayer enter", getInsName());
    if (m_exitThread)
    {
        HJFLogi("{} playerexit exitPlayer done enter", getInsName());
        m_exitThread->done();
        m_exitThread = nullptr;
    }    
    HJFLogi("{} playerexit exitPlayer end", getInsName());
}
void HJNAPIPlayer::closePlayer()
{
    HJFLogi("{} closePlayer enter", getInsName());
    if (m_cacheWindow)
    {
        HJFLogi("{} close player enter, first destroy window", getInsName());
        priSetWindow(m_cacheWindow, 0, 0, HJTargetState_Destroy, HJOGEGLSurfaceType_Default, m_renderFps);
        m_cacheWindow = nullptr;
    }    
    
    closeNativeSource();
    
    HJFLogi("{} close player enter", getInsName());
    if (m_exitThread)
    {
        m_exitThread->done();
        m_exitThread = nullptr;
    }  
    HJFLogi("{} close player start thread async close", getInsName());
    m_exitThread = HJThreadPool::Create();
    m_exitThread->start();
    m_exitThread->async([this]()
    {
        int64_t t0 = HJCurrentSteadyMS();
        HJFLogi("{} async thread closePlayer enter", getInsName());
    
        if (m_player)
        {
            m_player->done();
            m_player = nullptr;
        }
        int64_t t1= HJCurrentSteadyMS();
        HJFLogi("{} graph proc done enter", getInsName());
        if (m_prioGraphProc)
        {
            m_prioGraphProc->done();
            m_prioGraphProc = nullptr;
        }
        int64_t t2= HJCurrentSteadyMS();
        //notify
        
        if (m_notify)
        {
            HJFLogi("{} playerexit HJ_PLAYER_NOTIFY_CLOSEDONE", getInsName());
            m_notify(HJ_PLAYER_NOTIFY_CLOSEDONE, "");
        }
        
        {
            HJ_LOCK(s_playerMutex);
            s_playerIdxSet.erase(m_curIdx);

            if (s_playerIdxSet.size() > 1)
            {
                for (auto& it : s_playerIdxSet)
                {
                    int diffIdx = abs(m_curIdx - it);
                    if (diffIdx > 3)
                    {
                         HJFLogi("{} playerstat after close the playerSet is not release idx:{} diff:{}", getInsName(), it, diffIdx);
                    }
                }
            }
        }
        HJFLogi("{} closePlayer done end total:{} closeplayer:{} closerender:{}", getInsName(), (t2 - t0), (t1-t0), (t2-t1));
        return HJ_OK;
    });
}
int HJNAPIPlayer::priCreateRenderGraph(const HJPlayerInfo &i_playerInfo)
{
    int i_err = HJ_OK;
    do 
    {
        HJBaseParam::Ptr param = HJBaseParam::Create();
        (*param)[HJBaseParam::s_paramFps] = i_playerInfo.m_fps;
        HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioComSourceType), (int)i_playerInfo.m_sourceType);
        
        m_renderFps = i_playerInfo.m_fps;
        
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
            HJFLogi("{} graph notify id:{}, msg:{}", getInsName(), HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID()), ntf->getMsg());
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
            case HJVIDEORENDERGRAPH_EVENT_NEED_SURFACE:
            {
                type = HJ_RENDER_NOTIFY_NEED_SURFACE;
                break;    
            }
            case HJVIDEORENDERGRAPH_EVENT_FACEU_ERROR:
            {
                type = HJ_RENDER_NOTIFY_FACEU_ERROR;
                break;
            }
            case HJVIDEORENDERGRAPH_EVENT_FACEU_COMPLETE:
            {
                type = HJ_RENDER_NOTIFY_FACEU_COMPLETE;
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
        HJ_CatchMapPlainSetVal(param, int, "InsIdx", (int)m_curIdx);
        HJ_CatchMapPlainSetVal(param, bool, "IsMirror", (bool)i_playerInfo.m_bSplitScreenMirror);  //video decode keyframe push
        i_err = m_prioGraphProc->init(param);
        if (i_err < 0)
        {
            HJLoge("m_graphVideoCapture init error:{}", i_err);
            break;
        }
    } while (false);
    return i_err;    
}
int HJNAPIPlayer::priCreatePlayerGraph(const HJPlayerInfo &i_playerInfo)
{
    int i_err = HJ_OK;
    do 
    {
        HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(i_playerInfo.m_url);
        mediaUrl->setDisableMFlag(i_playerInfo.m_disableMFlag);
        
        auto param = std::make_shared<HJKeyStorage>();
        
        HJDeviceType nHJDeviceType = HJDEVICE_TYPE_NONE;
        switch (i_playerInfo.m_videoCodecType)
        {
        case HJPlayerVideoCodecType_SoftDefault:
            nHJDeviceType = HJDEVICE_TYPE_NONE;
            break;
        case HJPlayerVideoCodecType_OHCODEC:
            nHJDeviceType = HJDEVICE_TYPE_OHCODEC;
            break;
        case HJPlayerVideoCodecType_VIDEOTOOLBOX:
            nHJDeviceType = HJDEVICE_TYPE_VIDEOTOOLBOX;
            break;
        case HJPlayerVideoCodecType_MEDIACODEC:
            nHJDeviceType = HJDEVICE_TYPE_MEDIACODEC;
            break;
        }
        
        (*param)["deviceType"] = nHJDeviceType;
        HJOGRenderWindowBridge::Ptr mainBridge = m_prioGraphProc->renderWindowBridgeAcquire();
        (*param)["mainBridge"] = mainBridge;
        (*param)["InsIdx"] = (int)m_curIdx;
        HJGraphType playerGraphType = HJGraphType_NONE;
        bool bUseKeyStrategy = false;
        if (i_playerInfo.m_playerType == HJPlayerType_LIVESTREAM)
        {
            bUseKeyStrategy = true;
            if (nHJDeviceType == HJDEVICE_TYPE_OHCODEC)
            {
                HJOGRenderWindowBridge::Ptr softBridge = m_prioGraphProc->renderWindowBridgeAcquireSoft();
                (*param)["softBridge"] = softBridge;
            }
            playerGraphType = HJGraphType_LIVESTREAM;
            
            //only livestream stat
            if(m_statCtx) 
            {
                std::weak_ptr<HJStatContext> statCtx = m_statCtx;
                (*param)["HJStatContext"] = statCtx;
            }
        }    
        else if (i_playerInfo.m_playerType == HJPlayerType_VOD)
        {
            playerGraphType = HJGraphType_VOD;
        }        
        (*param)["IsUseKeyStrategy"] = bUseKeyStrategy;  //video decode keyframe push
        
        m_player = HJGraphPlayer::createGraph(playerGraphType, m_curIdx);
        if (nullptr == m_player)
        {
            i_err = HJErrFatal;
            break;
        }    
        
        auto audioInfo = std::make_shared<HJAudioInfo>();
        audioInfo->m_samplesRate = 44100;
        audioInfo->setChannels(1);
        audioInfo->m_sampleFmt = 1;
        audioInfo->m_bytesPerSample = 2;
        (*param)["audioInfo"] = audioInfo;
        
        HJListener playerListener = [&](const HJNotification::Ptr ntf) -> int 
        {
            if (!ntf) {
                return HJ_OK;
            }
            //HJFLogi("{} notify id:{}, msg:{}", this->getInsName(), HJPluginNofityTypeToString((HJPluginNofityType)ntf->getID()), ntf->getMsg());
            int type = HJ_PLAYER_NOTIFY_NONE;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
            case HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME:
                type = HJ_PLAYER_NOTIFY_VIDEO_FIRST_RENDER;
                HJFLogi("{} player notify HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME id:{}, msg:{}", this->getInsName(), HJPluginNofityTypeToString((HJPluginNofityType)ntf->getID()), ntf->getMsg());
                break;
            case HJ_PLUGIN_NOTIFY_AUDIORENDER_FRAME:
                type = HJ_PLAYER_NOTIFY_AUDIO_FRAME;
                // HJFLogi("{} player notify id:{}, msg:{}", this->getInsName(), HJPluginNofityTypeToString((HJPluginNofityType)ntf->getID()), ntf->getMsg());
                break;
            case HJ_PLUGIN_NOTIFY_EOF:
                type = HJ_PLAYER_NOTIFY_EOF;
                HJFLogi("{} player notify HJ_PLAYER_NOTIFY_EOF id:{}, msg:{}", this->getInsName(), HJPluginNofityTypeToString((HJPluginNofityType)ntf->getID()), ntf->getMsg());
                break;
            default:
                break;
            }
            if (m_notify && (HJ_PLAYER_NOTIFY_NONE != type)) {
                m_notify(type, msg);
            }
            return HJ_OK;
        };
        
        (*param)["playerListener"] = playerListener;
        
        i_err = m_player->init(param);
        if (i_err < 0)
        {
            break;
        }
	    i_err = m_player->openURL(mediaUrl);
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}

int HJNAPIPlayer::openPlayer(const HJPlayerInfo &i_playerInfo, HJNAPIPlayerNotify i_notify, const HJEntryStatInfo& i_statInfo)
{
    int i_err = HJ_OK;
    do 
    {
        s_insIdx++;
        m_curIdx = s_insIdx;
        
        {
            HJ_LOCK(s_playerMutex);
            s_playerIdxSet.insert(m_curIdx);
        }
        
        setInsName(HJFMT("Player_{}", m_curIdx));
        
        m_notify = i_notify;
        
        HJFLogi("{} url:{} fps:{} codecType:{} sourceType:{} playerType:{} version:{}", getInsName(), i_playerInfo.m_url, i_playerInfo.m_fps, (int)i_playerInfo.m_videoCodecType, (int)i_playerInfo.m_sourceType, (int)i_playerInfo.m_playerType, HJ_VERSION);
        if (i_statInfo.bUseStat)
        {
            HJFLogi("{} stat start...", getInsName());
            m_statCtx = HJStatContext::Create();
            m_statCtx->setInsName(HJFMT("statContext_{}", m_curIdx));
            i_err = m_statCtx->init(i_statInfo.uid, i_statInfo.device, i_statInfo.sn, i_statInfo.interval, 0, [i_statInfo](const std::string & i_name, int i_nType, const std::string & i_info)
            {
                //HJFLogi("StatStart callback name:{}, type:{}, info:{}", i_name, i_nType, i_info);
                i_statInfo.statNotify(i_name, i_nType, i_info);
            });
            if (i_err < 0)
            {
                i_err = -1;
                HJFLoge("{} m_statCtx init error", getInsName());
                break;
            }
        }
        
        i_err = priCreateRenderGraph(i_playerInfo);
        if (i_err < 0)
        {
            HJFLoge("{} priCreateRenderGraph error:{}", getInsName(), i_err);
            break;
        }
        i_err = priCreatePlayerGraph(i_playerInfo);
        if (i_err < 0)
        {
            HJFLoge("{} priCreatePlayerGraph error:{}", getInsName(), i_err);
            break;
        }
    } while (false);
    return i_err;
}
int HJNAPIPlayer::setNativeWindow(void* window, int i_width, int i_height, int i_state)
{
    if (i_state == HJTargetState_Create || i_state == HJTargetState_Change)
    {
        m_cacheWindow = window;
    }
    else if (i_state == HJTargetState_Destroy)
    {
        m_cacheWindow = nullptr;
    }        
    return priSetWindow(window, i_width, i_height, i_state, HJOGEGLSurfaceType_Default, m_renderFps);
}
int HJNAPIPlayer::setMute(bool i_bMute)
{
    int i_err = HJ_OK;
    HJFLogi("setMute:{} enter", i_bMute);
    if (m_player)
    {
        i_err = m_player->setMute(i_bMute);
    }    
    HJFLogi("setMute:{} end", i_bMute);
    return i_err;
}
bool HJNAPIPlayer::isMute()
{
    if (m_player)
    {
        return m_player->isMuted();
    }    
    return false;
}
int64_t HJNAPIPlayer::getDuration()
{
    int64_t duration = 0;
    if (m_player)
    {
        //duration = ;
    }   
    return duration;
}
int64_t HJNAPIPlayer::getPosition()
{
    int64_t position = 0;
    if (m_player)
    {
        //position = ;
    }   
    return position;    
}
int HJNAPIPlayer::priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps)
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

void HJNAPIPlayer::openEffect(int i_effectType)
{
    HJBaseParam::Ptr param = HJBaseParam::Create();
    HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioEffectType), i_effectType);
    if (m_prioGraphProc)
    {
        HJFLogi("HJNAPIPlayer openeffect enter");
        m_prioGraphProc->openEffect(param);
    }
}
void HJNAPIPlayer::closeEffect(int i_effectType)
{
    HJBaseParam::Ptr param = HJBaseParam::Create();
    HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioEffectType), i_effectType);
    if (m_prioGraphProc)
    {
        m_prioGraphProc->closeEffect(param);
    }    
}
std::shared_ptr<HJRGBAMediaData> HJNAPIPlayer::acquireNativeSource()
{
    HJRGBAMediaData::Ptr data = nullptr;
    if (m_gpuToRamPtr)
    {
        data = m_gpuToRamPtr->getMediaRGBAData();
    }    
    return data;
}
int HJNAPIPlayer::openFaceu(const std::string &i_faceuUrl)
{
    int i_err = HJ_OK;
    do 
    {
        if (m_prioGraphProc)
        {
            HJBaseParam::Ptr param = HJBaseParam::Create();
            HJ_CatchMapPlainSetVal(param, std::string, "faceuUrl", i_faceuUrl);
            i_err = m_prioGraphProc->openFaceu(param);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;    
}
void HJNAPIPlayer::closeFaceu()
{
    if (m_prioGraphProc)
    {   
        m_prioGraphProc->closeFaceu();
    }   
}

int HJNAPIPlayer::openNativeSource(bool i_bUsePBO)
{
    int i_err = HJ_OK;
    do 
    {
        if (!m_prioGraphProc)
        {
            i_err = -1;
            break;
        }
        if (!m_gpuToRamPtr)
        {
            HJFLogi("openNativeSource enter i_bUsePBO:{}", i_bUsePBO);
            HJGPUToRAMType type = i_bUsePBO ? HJGPUToRAMType_PBO : HJGPUToRAMType_ImageReceiver;
            m_gpuToRamPtr = HJBaseGPUToRAM::CreateGPUToRAM(type);

            HJBaseParam::Ptr param = HJBaseParam::Create();
            if (!i_bUsePBO)
            {
                HJGPUToRAMImageReceiver::HOImageReceiverSurfaceCb surfaceCb = [this](void *i_window, int i_width, int i_height, bool i_bCreate)
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
                    return priSetWindow(i_window, i_width, i_height, state, HJOGEGLSurfaceType_FaceDetect, 30);
                };
                HJ_CatchMapSetVal(param, HJGPUToRAMImageReceiver::HOImageReceiverSurfaceCb, surfaceCb);
            }

            i_err = m_gpuToRamPtr->init(param);
            if (i_err < 0)
            {
                break;
            }    
    
            if (i_bUsePBO)
            {
                HJBaseGPUToRAM::Wtr wtr = m_gpuToRamPtr;
                m_prioGraphProc->openPBO([wtr](HJSPBuffer::Ptr i_buffer, int width, int height)
                {
                    HJBaseGPUToRAM::Ptr gpuToRamPtr = wtr.lock();
                    if (gpuToRamPtr)
                    {
                        gpuToRamPtr->setMediaData(i_buffer, width, height);
                    }
                    return HJ_OK;
                });
            }
                           
            m_prioGraphProc->openFaceMgr();
        }
    } while (false);
    HJFLogi("openNativeSource end i_err:{} i_bUsePBO:{}", i_err, i_bUsePBO);
    return i_err;
}
void HJNAPIPlayer::closeNativeSource()
{
    if (m_prioGraphProc)
    {
        m_prioGraphProc->closePBO();
        m_prioGraphProc->closeFaceMgr();
    }
    
    if (m_gpuToRamPtr)
    {
        m_gpuToRamPtr->done();
        m_gpuToRamPtr = nullptr;
    }   
}

void HJNAPIPlayer::setFaceInfo(HJFacePointsWrapper::Ptr faceInfo)
{
    if (m_prioGraphProc)
    {
        m_prioGraphProc->setFaceInfo(faceInfo);
    }    
}

NS_HJ_END