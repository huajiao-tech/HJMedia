#include "HJNAPIPlayer.h"
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJTransferInfo.h"
#include "HJMediaUtils.h"
#include "HJNotify.h"
#include "HJRTMPUtils.h"
#include "HJComEvent.h"
#include "HJOGRenderEnv.h"
#include "HJRteGraphProc.h"
#include "HJGraphLivePlayer.h"
#include "HJOGRenderWindowBridge.h"
#include "HJThreadPool.h"
#include "HJStatContext.h"
#include "HJEntryContext.h"
#include "HJNetManager.h"
#include "HJRteGraphSetupInfo.h"
#include "HJDataSourceKit.h"

#if defined (HarmonyOS)
    #include "HJGPUToRAM.h"
    #include "deviceinfo.h"
#endif

#include "HJJson.h"

NS_HJ_BEGIN

std::mutex HJNAPIPlayer::s_playerMutex;
std::set<int> HJNAPIPlayer::s_playerIdxSet;
std::atomic<int> HJNAPIPlayer::s_insIdx{-1};

class HJJsonSeiData final : public HJInterpreter
{
public:
    std::string uuid;
    std::string data;

    HJ_JSON_REFLECT_BEGIN(HJJsonSeiData)
        HJ_JSON_REFLECT_MEMBER(uuid)
        HJ_JSON_REFLECT_MEMBER(data)
    HJ_JSON_REFLECT_END(HJJsonSeiData)
};
HJ_JSON_REFLECT_IMPLEMENT(HJJsonSeiData)

class HJJsonSeiInfo final : public HJInterpreter
{
public:
    std::vector<HJJsonSeiData> seiDatas;
    bool isKeyFrame;

    HJ_JSON_REFLECT_BEGIN(HJJsonSeiInfo)
        HJ_JSON_REFLECT_MEMBER(seiDatas)
        HJ_JSON_REFLECT_MEMBER(isKeyFrame)
    HJ_JSON_REFLECT_END(HJJsonSeiInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(HJJsonSeiInfo)

HJNAPIPlayer::HJNAPIPlayer()
{

}

HJNAPIPlayer::~HJNAPIPlayer()
{
    HJFLogi("{} playerexit ~HJNAPIPlayer", getInsName());
}

void HJNAPIPlayer::setSeiNotify(const std::function<void(const std::vector<HJSEIData>&, bool)>& i_notify)
{
    m_seiNotify = i_notify;
}

int HJNAPIPlayer::contextInit(const HJEntryContextPlayerInfo& i_contextInfo)
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

std::string HJNAPIPlayer::download(const std::string& url, const std::string& dir, const std::string& rid, int preCacheSize, int priority, HJListener listener)
{
    return HJDataSourceKit::getInstance()->download(url, dir, rid, preCacheSize, priority, listener);
}
int HJNAPIPlayer::cancelDownload(const std::string& rid)
{
    return HJDataSourceKit::getInstance()->cancel(rid);
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
        HJFLogi("{} close player enter, destroy window enter", getInsName());
        HJEntryBaseRender::setBaseNativeWindow(m_cacheClassStyle, m_cacheInsName, m_cacheWindow, 0, 0, HJTargetState_Destroy, m_renderFps);
        m_cacheWindow = nullptr;
    }
    for (const auto & native_window_map : m_nativeWindowMap) {
        HJFLogi("{} close player enter, destroy window enter", getInsName());
        HJEntryBaseRender::setBaseNativeWindow(native_window_map.second.classStyle, native_window_map.second.insName, native_window_map.second.nativeWindow, 0, 0, HJTargetState_Destroy, m_renderFps);
        HJFLogi("{} OH_NativeWindow_DestroyNativeWindow enter", getInsName());
        OH_NativeWindow_DestroyNativeWindow(native_window_map.second.nativeWindow);
        HJFLogi("{} OH_NativeWindow_DestroyNativeWindow end", getInsName());
    }
    m_nativeWindowMap.clear();
    
    HJEntryBaseRender::closeNativeSource();
    
    if (m_exitThread)
    {
        HJFLogi("{} exithread done enter", getInsName());
        m_exitThread->done();
        m_exitThread = nullptr;
    }  
    HJFLogi("{} exit thread create", getInsName());
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
        HJEntryBaseRender::doneRender();
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

        m_renderFps = i_playerInfo.m_fps;

        if (i_playerInfo.m_fps == 0) 
        {
            m_renderFps = 30;
            m_bManulDrive = true;
            HJ_CatchMapPlainSetVal(param, bool, HJBaseParam::s_paramIsManulDrive, m_bManulDrive);
            HJFLogi("{} fps is 0, use manulDrive", getInsName());
        }
        else
        {
            m_bManulDrive = false;
            HJFLogi("{} fps is {}, not use manulDrive", getInsName(), i_playerInfo.m_fps);
        }
    
        (*param)[HJBaseParam::s_paramFps] = i_playerInfo.m_fps;
        HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteGraphProcType), HJRteGraphProcType_CONFIG_SETUP);

        std::string configInfo = i_playerInfo.m_graphConfig;
        if (configInfo.empty())
        {
            if (i_playerInfo.m_sourceType == HJPrioComSourceType_SPLITSCREEN)
            {
                HJ_CatchMapPlainSetVal(param, bool, "IsMirror", (bool)i_playerInfo.m_bSplitScreenMirror);
                HJ_CatchMapSetVal(param, HJRteGraphConstructorType, HJRteGraphConstructorType_SplictScreen);
            }
            else
            {
                HJ_CatchMapSetVal(param, HJRteGraphConstructorType, HJRteGraphConstructorType_PlaceHolder);
            }
            configInfo = HJRteGraphConfigConstructor::constructGraph(param);
        }
        
        HJ_CatchMapPlainSetVal(param, std::string, "graphConfigInfo", configInfo);                 
       

        HJ_CatchMapPlainSetVal(param, int, "InsIdx", (int)m_curIdx);
        HJ_CatchMapPlainSetVal(param, bool, "IsMirror", (bool)i_playerInfo.m_bSplitScreenMirror);  //video decode keyframe push
        i_err = HJEntryBaseRender::initRender(param);
        if (i_err < 0)
        {
            HJLoge("HJEntryBaseRender initRender error:{}", i_err);
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
        if(!i_playerInfo.m_localDir.empty()) {
            (*mediaUrl)["local_dir"] = i_playerInfo.m_localDir;
        }
        if(!i_playerInfo.m_rid.empty()) {
            (*mediaUrl)["url_rid"] = i_playerInfo.m_rid;
        }
        
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
        HJOGRenderWindowBridge::Ptr mainBridge = m_graphProc->renderWindowBridgeAcquire();


        HJManulDriveCb manulDriveCb = [this]()
        {
            HJEntryBaseRender::manualDrive();          
        };
        if (m_bManulDrive && mainBridge)
        {
            mainBridge->setManulDriveCb(manulDriveCb);
        }

        (*param)["mainBridge"] = mainBridge;
        (*param)["InsIdx"] = (int)m_curIdx;
        HJGraphType playerGraphType = HJGraphType_NONE;
        bool bUseKeyStrategy = false;
        if (i_playerInfo.m_playerType == HJPlayerType_LIVESTREAM)
        {
            bUseKeyStrategy = true;
            if (nHJDeviceType == HJDEVICE_TYPE_OHCODEC)
            {
                HJOGRenderWindowBridge::Ptr softBridge = m_graphProc->renderWindowBridgeAcquireSoft();
                (*param)["softBridge"] = softBridge;

                if (m_bManulDrive && softBridge)
                {
                    softBridge->setManulDriveCb(manulDriveCb);
                }
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
            (*param)["repeats"] = i_playerInfo.m_repeats;
        }        
        (*param)["IsUseKeyStrategy"] = bUseKeyStrategy;  //video decode keyframe push
        (*param)["enableSEIUUids"] = i_playerInfo.m_enableSEIUUids;  //enable SEI UUIDs
        
        m_player = HJGraphPlayer::createGraph(playerGraphType, m_curIdx);
        if (nullptr == m_player)
        {
            i_err = HJErrFatal;
            break;
        }    

        registerHandlers(m_player->eventBus());

        auto audioInfo = std::make_shared<HJAudioInfo>();
        audioInfo->m_samplesRate = 44100;
        audioInfo->setChannels(1);
        audioInfo->m_sampleFmt = 1;
        audioInfo->m_bytesPerSample = 2;
        (*param)["audioInfo"] = audioInfo;
/*        
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
*/        
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
        
        HJFLogi("{} url:{} localDir:{} rid:{} fps:{} codecType:{} sourceType:{} playerType:{} version:{}", getInsName(), i_playerInfo.m_url, i_playerInfo.m_localDir, i_playerInfo.m_rid, i_playerInfo.m_fps, (int)i_playerInfo.m_videoCodecType, (int)i_playerInfo.m_sourceType, (int)i_playerInfo.m_playerType, HJ_VERSION);
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
int HJNAPIPlayer::setNativeWindow(const std::string &i_classStyle, const std::string &i_insName, void* window, int i_width, int i_height, int i_state)
{
     HJFLogi("{} setNativeWindow classStyle:{} insName:{} width:{} height:{} state:{}", this->getInsName(), i_classStyle, i_insName, i_width, i_height, i_state);
    if (i_state == HJTargetState_Create || i_state == HJTargetState_Change)
    {
        m_cacheWindow = window;
        m_cacheClassStyle = i_classStyle;
        m_cacheInsName = i_insName;
    }
    else if (i_state == HJTargetState_Destroy)
    {
        m_cacheWindow = nullptr;
    }      
    HJFLogi("{} HJEntryBaseRender setNativeWindow enter", this->getInsName());
    int i_err = HJEntryBaseRender::setBaseNativeWindow(i_classStyle, i_insName, window, i_width, i_height, i_state,  m_renderFps);
    HJFLogi("{} HJEntryBaseRender setNativeWindow end i_err:{}", this->getInsName(), i_err);
    return i_err;
}

int HJNAPIPlayer::setNativeWindow(const std::string &i_classStyle, const std::string &i_insName, int64_t i_surfaceId, int i_width, int i_height, int i_state)
{
    int i_err = HJ_OK;
    
    HJFLogi("{} player setNativeWindow wh:{} x {} state:{}", this->getInsName(), i_width, i_height, i_state);
    
    NativeWindow* window = nullptr;
    auto winIt = m_nativeWindowMap.find(i_surfaceId);
    if (winIt != m_nativeWindowMap.end())
    {
        window = winIt->second.nativeWindow;
    }

    if (i_state == HJTargetState_Create) {
        OH_NativeWindow_CreateNativeWindowFromSurfaceId(i_surfaceId, &window);
        PriSurfaceBindInfo bind{};
        bind.nativeWindow = window;
        bind.classStyle = i_classStyle;
        bind.insName = i_insName;
        m_nativeWindowMap.emplace(i_surfaceId, std::move(bind));
    } else if (i_state == HJTargetState_Change) {
        m_nativeWindowMap[i_surfaceId].classStyle = i_classStyle;
        m_nativeWindowMap[i_surfaceId].insName = i_insName;
    }
    if (window != nullptr) {
        i_err = HJEntryBaseRender::setBaseNativeWindow(i_classStyle, i_insName, window, i_width, i_height, i_state, m_renderFps);
        if (i_state == HJTargetState_Destroy) {
            HJFLogi("{} player OH_NativeWindow_DestroyNativeWindow enter", this->getInsName());
            OH_NativeWindow_DestroyNativeWindow(window);
            HJFLogi("{} player OH_NativeWindow_DestroyNativeWindow end", this->getInsName());
            if (winIt != m_nativeWindowMap.end()) {
                m_nativeWindowMap.erase(winIt);
            }
        }
    }
    HJFLogi("{} player setNativeWindow wh:{} x {} state:{} end i_err:{}", this->getInsName(), i_width, i_height, i_state, i_err);
    return i_err;
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
        duration = m_player->getDuration();
    }   
    return duration;
}
int64_t HJNAPIPlayer::getCurrentTimestamp()
{
    int64_t currentTimestamp = 0;
    if (m_player)
    {
        currentTimestamp = m_player->getCurrentTimestamp();
    }
    return currentTimestamp;
}
int64_t HJNAPIPlayer::getPosition()
{
    int64_t position = 0;
    if (m_player)
    {
        position = m_player->getCurrentTimestamp();
    }   
    return position;    
}
int HJNAPIPlayer::setPause(bool i_pause)
{
    int i_err = HJ_OK;
    if (m_player)
    {
        if (i_pause) {
            i_err = m_player->pause();
        } else {
            i_err = m_player->resume();
        }
    }
    return i_err;
}

void HJNAPIPlayer::registerHandlers(const std::shared_ptr<HJEventBus> i_bus)
{
    i_bus->registerHandler(EVENT_GRAPH_STREAM_OPENED_ID, [this]() {
        HJFLogi("{} player notify EVENT_GRAPH_STREAM_OPENED_ID id:{}", this->getInsName(), "EVENT_GRAPH_STREAM_OPENED_ID");
        auto duration = m_player->getDuration();
        if (m_notify) {
            m_notify(HJ_PLAYER_NOTIFY_DURATION, std::to_string(duration));
        }
//        m_trackIds = m_player->getAudioTrackIds();
    });

    i_bus->registerHandler(EVENT_GRAPH_EOF_ID, [this]() {
        HJFLogi("{} player notify HJ_PLAYER_NOTIFY_EOF id:{}", this->getInsName(), "EVENT_GRAPH_EOF_ID");
        if (m_notify) {
            m_notify(HJ_PLAYER_NOTIFY_EOF, "");
        }
    });

    i_bus->registerHandler(EVENT_GRAPH_VIDEO_FRAME_ID, [this](const HJMediaFrame::Ptr& frame) {

    });

    i_bus->registerHandler(EVENT_GRAPH_RENDERED_PCM_ID, [this](const HJGraphRenderedPCM& pcm) {
        auto audioInfo = pcm.m_audioInfo;
        auto pcmData = pcm.m_pcmData;
        HJFLogi("EVENT_GRAPH_RENDERED_PCM channels:{}, bytesPerSample:{}, sampleRate:{}, PCM data size:{}", audioInfo->m_channels, audioInfo->m_bytesPerSample, audioInfo->m_samplesRate, pcmData->size());
    });

    i_bus->registerHandler(EVENT_GRAPH_SEI_INFOS_ID, [this](const std::vector<HJSEIData>& userSEIDatas, bool keyFrame) {
        HJFLogi("{} player notify EVENT_GRAPH_SEI_INFOS_ID, msg:{}, keyFrame:{}", this->getInsName(), userSEIDatas.size(), keyFrame);
        if (m_seiNotify) {
            for (const auto& sei : userSEIDatas) {
                HJFLogi("{} 202603101102 sei uuid:{}, size:{}, data(hex):{}",
                    this->getInsName(),
                    sei.uuid,
                    sei.data.size(),
                    HJUtilitys::hexMem(sei.data.data(), sei.data.size()));
            }
            m_seiNotify(userSEIDatas, keyFrame);
            return;
        }
        if (m_notify) {
            // HJFLogi("sei infos: {}, keyFrame:{}", userSEIDatas.size(), keyFrame);
            HJJsonSeiInfo jsonSeiInfos;
            jsonSeiInfos.seiDatas.reserve(userSEIDatas.size());
            for (const auto& sei : userSEIDatas) {
                HJFLogi("{} 202603101102 sei uuid:{}, size:{}, data(hex):{}",
                    this->getInsName(),
                    sei.uuid,
                    sei.data.size(),
                    HJUtilitys::hexMem(sei.data.data(), sei.data.size()));
                HJJsonSeiData data;
                data.uuid = sei.uuid;
                data.data = HJUtilitys::base64Encode(sei.data);
                jsonSeiInfos.seiDatas.push_back(std::move(data));
            }
            jsonSeiInfos.isKeyFrame = keyFrame;

            const std::string seiInfos = jsonSeiInfos.getSerialInfo();
            HJFLogi("seiInfos:{}", seiInfos);

            m_notify(HJ_PLAYER_NOTIFY_SEI_INFOS, seiInfos);
        }
    });

    i_bus->registerHandler(EVENT_GRAPH_FIRST_FRAME_RENDERED_ID, [this](const std::string& pluginName) {
        HJFLogi("{} player notify HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME id:{}, msg:{}", this->getInsName(), "EVENT_GRAPH_FIRST_FRAME_RENDERED_ID", pluginName);
        if (m_notify) {
            m_notify(HJ_PLAYER_NOTIFY_VIDEO_FIRST_RENDER, pluginName);
        }
    });

    i_bus->registerHandler(EVENT_GRAPH_AUTODELAY_PARAMS_ID, [this](const std::shared_ptr<HJParams>& params) {

    });
}

int HJNAPIPlayer::setEnableSEIUUids(const std::string& i_uuid, bool bKeyMustCb)
{
    int i_err = HJ_OK;
    if (m_player)
    {
        i_err = m_player->setEnableSEIUUids(i_uuid, bKeyMustCb);
    }
    return i_err;
}


NS_HJ_END

