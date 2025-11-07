#include "HJNAPITestPlayer.h"
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
#include "HJRteGraphProc.h"
#include "HJOGEGLSurface.h"
#include "HJRteComDraw.h"
#include "HJOGBaseShader.h"

NS_HJ_BEGIN
    
bool HJNAPITestPlayer::s_bContextInit = false;

HJNAPITestPlayer::HJNAPITestPlayer()
{

}

HJNAPITestPlayer::~HJNAPITestPlayer()
{
   
}
int HJNAPITestPlayer::contextInit(const HJEntryContextInfo& i_contextInfo)
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

void HJNAPITestPlayer::closePlayer()
{
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("closePlayer enter");
    
    if (m_player)
    {
        m_player->done();
        m_player = nullptr;
    }
        
    HJFLogi("m_graphVideoCapture enter");
    if (m_prioGraphProc)
    {
        m_prioGraphProc->done();
        m_prioGraphProc = nullptr;
    }
    
    HJFLogi("closePlayer done end time:{}", (HJCurrentSteadyMS() - t0));
}
int HJNAPITestPlayer::priCreateRenderGraph(const HJPlayerInfo &i_playerInfo)
{
    int i_err = HJ_OK;
    do 
    {
        HJBaseParam::Ptr param = HJBaseParam::Create();
        (*param)[HJBaseParam::s_paramFps] = i_playerInfo.m_fps;
        HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioComSourceType), (int)i_playerInfo.m_sourceType);
        
        m_renderFps = i_playerInfo.m_fps;
        
#if USE_RTE
        m_prioGraphProc = HJRteGraphProc::Create();  
#else
        m_prioGraphProc = HJPrioGraphProc::Create();
#endif
        if (!m_prioGraphProc)
        {
            i_err = -1;
            HJFLoge("HJGraphComVideoCapture create error");
            break;
        }
        i_err = m_prioGraphProc->init(param);
        if (i_err < 0)
        {
            HJLoge("m_graphVideoCapture init error:{}", i_err);
            break;
        }
    } while (false);
    return i_err;    
}
int HJNAPITestPlayer::priCreatePlayerGraph(const HJPlayerInfo &i_playerInfo)
{
    int i_err = HJ_OK;
    do 
    {
        HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(i_playerInfo.m_url);
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
//            if(m_statCtx) 
//            {
//                std::weak_ptr<HJStatContext> statCtx = m_statCtx;
//                (*param)["HJStatContext"] = statCtx;
//            }
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
//            if (m_notify && (HJ_PLAYER_NOTIFY_NONE != type)) {
//                m_notify(type, msg);
//            }
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

int HJNAPITestPlayer::openPlayer(const HJPlayerInfo &i_playerInfo, HJNAPIPlayerNotify i_notify)
{
    int i_err = HJ_OK;
    do 
    {
        i_err = priCreateRenderGraph(i_playerInfo);
        if (i_err < 0)
        {
            HJFLoge("priCreateRenderGraph error:{}", i_err);
            break;
        }
        i_err = priCreatePlayerGraph(i_playerInfo);
        if (i_err < 0)
        {
            HJFLoge("priCreatePlayerGraph error:{}", i_err);
            break;
        }
    } while (false);
    return i_err;
}
int HJNAPITestPlayer::setNativeWindow(void* window, int i_width, int i_height, int i_state)
{
    if (i_state == HJTargetState_Create || i_state == HJTargetState_Change)
    {
        m_cacheWindow = window;
    }
    else if (i_state == HJTargetState_Destroy)
    {
        m_cacheWindow = nullptr;
    }        

    std::shared_ptr<HJOGEGLSurface> eglSurface = nullptr;
    int i_err = priSetWindow(window, i_width, i_height, i_state, HJOGEGLSurfaceType_Default, m_renderFps, eglSurface);
    if (i_err == HJ_OK)
    {
        if (i_state == HJTargetState_Create)
        {
             if (m_prioGraphProc)
             {
                std::weak_ptr<HJRteGraphProc> wgraphProc = m_prioGraphProc;
                std::weak_ptr<HJOGEGLSurface> weglSurface = eglSurface;
                 m_prioGraphProc->graphProcAsync([wgraphProc, weglSurface]()
                  {
                    HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
                    HJOGEGLSurface::Ptr eglSurface = weglSurface.lock();
                    if (!graphProc || !eglSurface)
                    {
                        return HJ_OK;
                    }

                    HJRteComDrawEGL::Ptr uicom = HJRteComDrawEGL::Create();
                    uicom->setInsName("uicom");
                    //uicom->setPriority(HJRteComPriority_Target);
                    uicom->setSurface(eglSurface);
                    graphProc->addTarget(uicom);
                    
        ////1. test: source->ui            
        //            HJOGBaseShader::Ptr shaderOES = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_OES);
        //            m_links.push_back(m_videoSource->addTarget(uicom, shaderOES, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 1.0, 0.5)));
                
        ////2. test: source->fbo->ui            
                    HJRteComDrawFBO::Ptr video2D = HJRteComDrawFBO::Create();
                    video2D->setInsName("video2D");
                    video2D->setPriority(HJRteComPriority_VideoSource);
                    graphProc->addFilter(video2D);  
                    
                    HJOGBaseShader::Ptr shaderOES = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_OES);
                    //m_links.push_back(m_videoSource->addTarget(video2D, shaderOES));

                    //fixme
                    graphProc->connectCom(*graphProc->getSources().begin(), video2D, shaderOES);
                    


                    HJRteComDrawBlurFBO::Ptr blur = HJRteComDrawBlurFBO::Create();
                    blur->setInsName("blur");
                    blur->setPriority(HJRteComPriority_VideoBlur);
                    blur->init(nullptr);
                    graphProc->addFilter(blur);

                    //not use m_videoSource to blur, use video2D to blur, blur shader is not implement OES, so video2D OES->2D is important
                    graphProc->connectCom(video2D, blur);

                    HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
                    graphProc->connectCom(blur, uicom, shader2D); 

 #if 0                   
                    HJRteComDrawGrayFBO::Ptr gray = HJRteComDrawGrayFBO::Create();
                    gray->setInsName("gray");
                    gray->setPriority(HJRteComPriority_VideoGray);
                    gray->init(nullptr);
                    graphProc->addFilter(gray);
                    //m_links.push_back(video2D->addTarget(gray));
                    graphProc->connectCom(video2D, gray);

                    HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
                    //m_links.push_back(gray->addTarget(uicom, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 1.0, 0.5))); 
                    graphProc->connectCom(gray, uicom, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 1.0, 0.5));


                    HJRteComDrawBlurFBO::Ptr blur = HJRteComDrawBlurFBO::Create();
                    blur->setInsName("blur");
                    blur->setPriority(HJRteComPriority_VideoBlur);
                    blur->init(nullptr);
                    graphProc->addFilter(blur);

                    //not use m_videoSource to blur, use video2D to blur, blur shader is not implement OES, so video2D OES->2D is important
                    graphProc->connectCom(video2D, blur);
                    graphProc->connectCom(blur, uicom, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.5, 1.0, 0.5, true, false)); 
#endif
                    return 0;
                 });
             }
        }
        else if (i_state == HJTargetState_Destroy)
        {

        }
    }
    //if (i_err == HJ_OK i_state == HJTargetState_Create)

    return i_err;
}
        
int HJNAPITestPlayer::priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps, std::shared_ptr<HJOGEGLSurface> & o_eglSurface)
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
            i_err = m_prioGraphProc->eglSurfaceProc(renderTargetStr, o_eglSurface);
            if (i_err < 0)
            {
                HJFLoge("eglSurfaceProc error i_err:{} w:{} h:{} state:{}", i_err, i_width, i_height, i_state);
                break;
            }   
        }
    } while (false);
    return i_err;
}

void HJNAPITestPlayer::openEffect(int i_effectType)
{
    if (m_prioGraphProc)
    {
        HJFLogi("HJNAPITestPlayer openeffect enter");
        //m_prioGraphProc->openEffect(param);

        if (m_prioGraphProc)
        {
            std::weak_ptr<HJRteGraphProc> wgraphProc = m_prioGraphProc;
            m_prioGraphProc->graphProcAsync([wgraphProc, i_effectType]()
            {
                HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
                if (!graphProc)
                {
                    return HJ_OK;
                } 

                if (HJRteEffect_Gray == i_effectType)
                {
                    
                }
                else if (HJRteEffect_Gray == i_effectType)
                {
                    
                }
                return HJ_OK;
            });
        }
    }
}
void HJNAPITestPlayer::closeEffect(int i_effectType)
{
    if (m_prioGraphProc)
    {
        HJFLogi("HJNAPITestPlayer closeEffect enter");
        //m_prioGraphProc->openEffect(param);

        if (m_prioGraphProc)
        {
            std::weak_ptr<HJRteGraphProc> wgraphProc = m_prioGraphProc;
            m_prioGraphProc->graphProcAsync([wgraphProc, i_effectType]()
            {
                HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
                if (!graphProc)
                {
                    return HJ_OK;
                } 

                if (HJPrioEffect_Gray == i_effectType)
                {
                    
                }
                else if (HJPrioEffect_Blur == i_effectType)
                {
                    graphProc->foreachUseType<HJRteComDrawBlurFBO>([graphProc](HJRteCom::Ptr i_com) 
	                { 
		                graphProc->removeCom(i_com);
		                return HJ_OK;
	                });
    
                }
                return HJ_OK;
            });
        }
    }   
}

NS_HJ_END