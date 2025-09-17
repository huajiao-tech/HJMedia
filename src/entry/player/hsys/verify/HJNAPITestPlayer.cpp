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
#include "HJGraphComPlayer.h"
#include "HJOGRenderWindowBridge.h"
#include "HJRteGraphProc.h"

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
    
    if (m_graphPlayer)
    {
        m_graphPlayer->done();
        m_graphPlayer = nullptr;
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
        HJBaseParam::Ptr param = HJBaseParam::Create();
        HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(i_playerInfo.m_url);
	    HJ_CatchMapSetVal(param, HJMediaUrl::Ptr, mediaUrl);
        
        int HJDeviceType = HJDEVICE_TYPE_NONE;
        switch (i_playerInfo.m_videoCodecType)
        {
        case HJPlayerVideoCodecType_SoftDefault:
            HJDeviceType = HJDEVICE_TYPE_NONE;
            break;
        case HJPlayerVideoCodecType_OHCODEC:
            HJDeviceType = HJDEVICE_TYPE_OHCODEC;
            break;
        case HJPlayerVideoCodecType_VIDEOTOOLBOX:
            HJDeviceType = HJDEVICE_TYPE_VIDEOTOOLBOX;
            break;
        case HJPlayerVideoCodecType_MEDIACODEC:
            HJDeviceType = HJDEVICE_TYPE_MEDIACODEC;
            break;
        }
        
        (*param)["HJDeviceType"] = (int)HJDeviceType;

        if (m_prioGraphProc)
        {
            std::shared_ptr<HJOGRenderWindowBridge> mainBridge = m_prioGraphProc->renderWindowBridgeAcquire();
            HJ_CatchMapSetVal(param, HJOGRenderWindowBridge::Ptr, mainBridge);
            
            //fixme lfs now only test, formal versio soft bridge is only use hw+
            //if (HJDeviceType == HJDEVICE_TYPE_OHCODEC)   
            {
                HJOGRenderWindowBridge::Ptr softBridge = m_prioGraphProc->renderWindowBridgeAcquireSoft();
                HJ_CatchMapPlainSetVal(param, HJOGRenderWindowBridge::Ptr, "HJOGRenderWindowBridgeSoft", softBridge);
            }
        } 
        m_graphPlayer = HJGraphComPlayer::Create();
        i_err = m_graphPlayer->init(param);
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
    return priSetWindow(window, i_width, i_height, i_state, HJOGEGLSurfaceType_Default, m_renderFps);
}
        
int HJNAPITestPlayer::priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps)
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

void HJNAPITestPlayer::openEffect(int i_effectType)
{
    HJBaseParam::Ptr param = HJBaseParam::Create();
    HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioEffectType), i_effectType);
    if (m_prioGraphProc)
    {
        HJFLogi("HJNAPITestPlayer openeffect enter");
        m_prioGraphProc->openEffect(param);
    }
}
void HJNAPITestPlayer::closeEffect(int i_effectType)
{
    HJBaseParam::Ptr param = HJBaseParam::Create();
    HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJPrioEffectType), i_effectType);
    if (m_prioGraphProc)
    {
        m_prioGraphProc->closeEffect(param);
    }    
}

NS_HJ_END