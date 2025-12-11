#include "HJGraphLivePlayer.h"
#include "HJBaseUtils.h"
#include "HJStatContext.h"
#include "HJFLog.h"

#if defined (HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

std::string HJGraphLivePlayer::s_statPlaying = "STAT_PLAYING";
std::string HJGraphLivePlayer::s_statLoading = "STAT_LOADING";

#define DEMUXER             "demuxer"
#define DROPPING            "dropping"
#define AUDIODECODER        "audioDecoder"
#define AUDIORENDER         "audioRender"
#define VIDEOMAINDECODER    "videoMainDecoder"
#define VIDEOMAINRENDER     "videoMainRender"
#define VIDEOSOFTDECODER    "videoSoftDecoder"
#define VIDEOSOFTRENDER     "videoSoftRender"

HJGraphLivePlayer::HJGraphLivePlayer(const std::string& i_name, size_t i_identify)
    : HJGraphPlayer(i_name, i_identify)
{
    m_cacheObserver = HJCreates<HJCacheObserver>();
    m_cacheObserver->setName(i_name);
}

HJGraphLivePlayer::~HJGraphLivePlayer()
{
    HJGraphLivePlayer::done();
}

int HJGraphLivePlayer::internalInit(HJKeyStorage::Ptr i_param)
{
    m_entryTime = HJCurrentSteadyMS();

    MUST_HAVE_PARAMETERS;
    int ret = HJGraph::internalInit(i_param);
    if (ret < 0) {
        return ret;
    }

    do {
        int insIdx = 0;
        if (i_param && i_param->haveStorage("InsIdx"))
        {
            insIdx = i_param->getValue<int>("InsIdx");
        }
        if (i_param && i_param->haveStorage("HJStatContext")) {
            m_statCtx = i_param->getValueObj<HJStatContext::WPtr>("HJStatContext");

            auto statCtx = m_statCtx.lock();
            if (statCtx) {
                statCtx->notify("HJGraphLivePlayer", JNotify_Play_ScheduleUrlConsume, 0);
            }    
        }

        GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
        IF_FALSE_BREAK(audioInfo, HJErrInvalidParams);
        GET_PARAMETER(HJListener, playerListener);
        m_playerListener = playerListener;
        Wtr wPlayer = SHARED_FROM_THIS;
        HJListener pluginListener = [wPlayer](const HJNotification::Ptr ntf) -> int {
            auto player = wPlayer.lock();
            if (player) {
                return player->listener(ntf);
            }

            return HJ_OK;
        };
        GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
        GET_PARAMETER(HJDeviceType, deviceType);
//        m_deviceType = deviceType;
#if defined (HarmonyOS)
        GET_PARAMETER(HJOGRenderWindowBridge::Ptr, mainBridge);
        IF_FALSE_BREAK(mainBridge, HJErrInvalidParams);
        GET_PARAMETER(HJOGRenderWindowBridge::Ptr, softBridge);
        if (deviceType == HJDEVICE_TYPE_OHCODEC) {
            IF_FALSE_BREAK(softBridge, HJErrInvalidParams);
        }
#elif defined (WINDOWS)
        GET_PARAMETER(std::string, audioDeviceName);
#endif

        auto graphInfo = std::make_shared<HJKeyStorage>();
        (*graphInfo)["insIdx"] = insIdx;
        (*graphInfo)["graph"] = std::static_pointer_cast<HJGraph>(SHARED_FROM_THIS);

        IF_FALSE_BREAK(m_demuxer = HJPluginFFDemuxer::Create<HJPluginFFDemuxer>(DEMUXER, graphInfo), HJErrFatal);
        addPlugin(m_demuxer);
        m_pluginStatuses[DEMUXER].store(HJSTATUS_NONE);
        IF_FALSE_BREAK(m_dropping = HJPluginAVDropping::Create<HJPluginAVDropping>(DROPPING, graphInfo), HJErrFatal);
        addPlugin(m_dropping);
        m_videoKeyFrames[DROPPING].store(0);
        m_audioDurations[DROPPING].store(0);
        m_pluginStatuses[DROPPING].store(HJSTATUS_NONE);
        IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_dropping, HJMEDIA_TYPE_AV), ret);
        IF_FALSE_BREAK(m_videoMainRender = HJPluginVideoRender::Create<HJPluginVideoRender>(VIDEOMAINRENDER, graphInfo), HJErrFatal);
        addPlugin(m_videoMainRender);
        m_frameSizes[VIDEOMAINRENDER].store(0);
        m_pluginStatuses[VIDEOMAINRENDER].store(HJSTATUS_NONE);
        if (deviceType == HJDEVICE_TYPE_NONE) {
            IF_FALSE_BREAK(m_videoFFDecoder = HJPluginVideoFFDecoder::Create<HJPluginVideoFFDecoder>(VIDEOMAINDECODER, graphInfo), HJErrFatal);
            addPlugin(m_videoFFDecoder);
            m_frameSizes[VIDEOMAINDECODER].store(0);
            m_pluginStatuses[VIDEOMAINDECODER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_dropping, m_videoFFDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoFFDecoder, m_videoMainRender, HJMEDIA_TYPE_VIDEO), ret);
        }
        else if (deviceType == HJDEVICE_TYPE_OHCODEC) {
#if defined (HarmonyOS)
            IF_FALSE_BREAK(m_videoHWDecoder = HJPluginVideoOHDecoder::Create<HJPluginVideoOHDecoder>(VIDEOMAINDECODER, graphInfo), HJErrFatal);
            addPlugin(m_videoHWDecoder);
            m_frameSizes[VIDEOMAINDECODER].store(0);;
            m_pluginStatuses[VIDEOMAINDECODER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_dropping, m_videoHWDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoHWDecoder, m_videoMainRender, HJMEDIA_TYPE_VIDEO), ret);
#endif
            IF_FALSE_BREAK(m_videoFFDecoder = HJPluginVideoFFDecoder::Create<HJPluginVideoFFDecoder>(VIDEOSOFTDECODER, graphInfo), HJErrFatal);
            addPlugin(m_videoFFDecoder);
            m_frameSizes[VIDEOSOFTDECODER].store(0);
            m_pluginStatuses[VIDEOSOFTDECODER].store(HJSTATUS_NONE);
            IF_FALSE_BREAK(m_videoSoftRender = HJPluginVideoRender::Create<HJPluginVideoRender>(VIDEOSOFTRENDER, graphInfo), HJErrFatal);
            addPlugin(m_videoSoftRender);
            m_frameSizes[VIDEOSOFTRENDER].store(0);
            m_pluginStatuses[VIDEOSOFTRENDER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_videoFFDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoFFDecoder, m_videoSoftRender, HJMEDIA_TYPE_VIDEO), ret);
        }
        IF_FALSE_BREAK(m_renderThread = HJLooperThread::quickStart("renderThread" + HJFMT("_{}", insIdx)), HJErrFatal);
        addThread(m_renderThread);
        if (audioInfo != nullptr) {
            m_audioInfo = audioInfo;
            IF_FALSE_BREAK(m_audioDecoder = HJPluginAudioFFDecoder::Create<HJPluginAudioFFDecoder>(AUDIODECODER, graphInfo), HJErrFatal);
            addPlugin(m_audioDecoder);
            m_audioDurations[AUDIODECODER].store(0);
            m_pluginStatuses[AUDIODECODER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_dropping, m_audioDecoder, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_audioResampler = HJPluginAudioResampler::Create<HJPluginAudioResampler>("audioResampler", graphInfo), HJErrFatal);
            addPlugin(m_audioResampler);
            m_pluginStatuses["audioResampler"].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioDecoder, m_audioResampler, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_speedControl = HJPluginSpeedControl::Create<HJPluginSpeedControl>("speedControl", graphInfo), HJErrFatal);
            addPlugin(m_speedControl);
            m_pluginStatuses["speedControl"].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioResampler, m_speedControl, HJMEDIA_TYPE_AUDIO), ret);
#if defined (HarmonyOS)
            IF_FALSE_BREAK(m_audioRender = HJPluginAudioOHRender::Create<HJPluginAudioOHRender>(AUDIORENDER, graphInfo), HJErrFatal);
#elif defined (WINDOWS)
//            IF_FALSE_BREAK(m_audioRender = HJPluginAudioWORender::Create<HJPluginAudioWORender>(AUDIORENDER, graphInfo), HJErrFatal);
            IF_FALSE_BREAK(m_audioRender = HJPluginAudioWASRender::Create<HJPluginAudioWASRender>(AUDIORENDER, graphInfo), HJErrFatal);
#endif
            addPlugin(m_audioRender);
            m_audioDurations[AUDIORENDER].store(0);
            m_pluginStatuses[AUDIORENDER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_speedControl, m_audioRender, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_audioThread = HJLooperThread::quickStart("audioThread" + HJFMT("_{}", insIdx)), HJErrFatal);
            addThread(m_audioThread);
        }

        for (auto pln : m_plugins)
		{
			m_pluginQueueWtr.push_back(pln);
		}
        
        m_cacheObserver->setWatchStartTime();

        IF_FALSE_BREAK(m_timeline = HJTimeline::Create<HJTimeline>("HJTimeline" + HJFMT("_{}", insIdx)), HJErrFatal);
        IF_FAIL_BREAK(ret = m_timeline->init(nullptr), ret);

        auto param = std::make_shared<HJKeyStorage>();
        (*param)["HJStatContext"] = m_statCtx;
        (*param)["timeline"] = m_timeline;
        (*param)["thread"] = m_renderThread;
        (*param)["pluginListener"] = pluginListener;
        (*param)["deviceType"] = deviceType;
        if (deviceType == HJDEVICE_TYPE_NONE) {
#if defined (HarmonyOS)
            (*param)["bridge"] = mainBridge;
#elif defined (WINDOWS)
            IF_FALSE_BREAK(m_sharedMemoryProducer = HJSharedMemoryProducer::Create<HJSharedMemoryProducer>("HJSharedMemoryProducer" + HJFMT("_{}", insIdx)), HJErrFatal);
            m_sharedMemoryProducer->init(1920, 1080, 30);
            (*param)["sharedMemoryProducer"] = m_sharedMemoryProducer;
#endif
            IF_FAIL_BREAK(ret = m_videoMainRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_videoFFDecoder->init(param), ret);
        }
        else if (deviceType == HJDEVICE_TYPE_OHCODEC) {
            IF_FAIL_BREAK(ret = m_videoMainRender->init(param), ret);

#if defined (HarmonyOS)
            (*param)["bridge"] = softBridge;
#endif
            (*param)["onlyFirstFrame"] = true;
            (*param)["deviceType"] = HJDEVICE_TYPE_NONE;
            IF_FAIL_BREAK(ret = m_videoSoftRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["pluginListener"] = pluginListener;
#if defined (HarmonyOS)
            (*param)["bridge"] = mainBridge;
            IF_FAIL_BREAK(ret = m_videoHWDecoder->init(param), ret);
#endif

            (*param)["onlyFirstFrame"] = true;
            IF_FAIL_BREAK(ret = m_videoFFDecoder->init(param), ret);
        }

        if (audioInfo != nullptr) {
            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
#if defined (WINDOWS)
            if (!audioDeviceName.empty()) {
                (*param)["audioDeviceName"] = audioDeviceName;
            }
#else
            (*param)["thread"] = m_renderThread;
#endif
            (*param)["timeline"] = m_timeline;
            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_audioRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
            (*param)["thread"] = m_audioThread;
            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_speedControl->init(param), ret);
            IF_FAIL_BREAK(ret = m_audioResampler->init(param), ret);

            param->erase("audioInfo");
            IF_FAIL_BREAK(ret = m_audioDecoder->init(param), ret);
        }

        param = std::make_shared<HJKeyStorage>();
        (*param)["pluginListener"] = pluginListener;
        IF_FAIL_BREAK(ret = m_dropping->init(param), ret);

        if (mediaUrl) {
            (*param)["mediaUrl"] = mediaUrl;
        }
        
        std::function<void()> demuxNotify = [this]
        {
            int nSize = m_pluginQueueWtr.size();
            int i = 0;
            for (auto& pluginWtr : m_pluginQueueWtr)
            {
                HJPlugin::Ptr pln = pluginWtr.lock();
                if (pln)
                {
                    HJFLogi("{} pluginstat <{} {}>: inIdx:{} outIdx:{}", pln->getName(), i, nSize, pln->getInIdx(), pln->getOutIdx());
                }
                i++;
            }    
        };
        (*param)["demuxNotify"] = (std::function<void()>)demuxNotify;
        
        IF_FAIL_BREAK(ret = m_demuxer->init(param), ret);

        return HJ_OK;
    } while (false);

    HJGraphLivePlayer::internalRelease();
    return ret;
}

void HJGraphLivePlayer::internalRelease()
{
    HJFLogi("{}, internalRelease() begin", getName());

#if defined (HarmonyOS)
    m_videoHWDecoder = nullptr;
#endif
    m_videoSoftRender = nullptr;
    m_videoMainRender = nullptr;
    m_audioRender = nullptr;
    m_speedControl = nullptr;
    m_audioResampler = nullptr;
    m_videoFFDecoder = nullptr;
    m_audioDecoder = nullptr;
    m_dropping = nullptr;
    m_demuxer = nullptr;

    m_audioThread = nullptr;
    m_renderThread = nullptr;

    m_audioInfo = nullptr;
    m_playerListener = nullptr;

    if (m_timeline) {
        m_timeline->done();
        m_timeline = nullptr;
    }

    HJGraph::internalRelease();

#if defined (WINDOWS)
    if (m_sharedMemoryProducer) {
        m_sharedMemoryProducer->done();
        m_sharedMemoryProducer = nullptr;
    }
#endif

    HJFLogi("{}, internalRelease() end", getName());
}

void HJGraphLivePlayer::priHeartbeatRegist(const std::string& i_uniqueName, int i_interval)
{
    auto statCtx = m_statCtx.lock();
    if (statCtx)
    {
        if (i_uniqueName == s_statLoading)
        {
            statCtx->notify("HJGraphLivePlayer", JNotify_Play_Loading, 1);
        }

        statCtx->heartbeatRegist(i_uniqueName, 5000, [statCtxWtr = m_statCtx](const std::string& i_uniqueName, int i_heartState, int64_t i_curTime, int64_t i_duration)
        {
           if (i_heartState == TimerTask_Update)
           {
               JStatPlayStuckInfo info;
               if (i_uniqueName == s_statLoading)
               {
                   info.m_watchStuckDuration = i_duration;
               }
               else if (i_uniqueName == s_statPlaying)
               {
                   info.m_watchDuration = i_duration;
               }
               auto statCtx = statCtxWtr.lock();
               if (statCtx)
               {
                   statCtx->notify("HJGraphLivePlayer", JNotify_Play_Stutter, std::any(std::move(info)));
                   //HJFLogi("statplayer update uniqueName:{}", i_uniqueName); 
               }
           }
        });
    }
}
void HJGraphLivePlayer::priHeartbeatUnRegist(const std::string& i_uniqueName)
{
    auto statCtx = m_statCtx.lock();
    if (statCtx)
    {
        if (i_uniqueName == s_statLoading)
        {
            statCtx->notify("HJGraphLivePlayer", JNotify_Play_Loading, 2);
        }

        statCtx->heartbeatUnRegist(i_uniqueName);
    }
}

int HJGraphLivePlayer::setInfo(const Information i_info)
{
    if (!i_info) {
        return HJErrInvalidParams;
    }

    int ret = HJ_OK;
    int log = -1;
    switch (i_info->getID()) {
    case HJ_PLUGIN_SETINFO_DEMUXER_mediaType:
    {
        GET_INFO(HJMediaType, mediaType);
        m_mediaType.store(mediaType);
        break;
    }
    case HJ_PLUGIN_SETINFO_AUDIORENDER_audioDuration:
    {
        m_audioDurations[AUDIORENDER].store(i_info->getVal());
        log = 0;
        break;
    }
    case HJ_PLUGIN_SETINFO_AUDIODECODER_audioDuration:
    {
        m_audioDurations[AUDIODECODER].store(i_info->getVal());
        log = 0;
        break;
    }
    case HJ_PLUGIN_SETINFO_DROPPING_audioDuration:
    {
        auto val = i_info->getVal();
        //HJFLogi("HJ_PLUGIN_SETINFO_DROPPING_audioDuration audioDuration:{}", val);
        m_audioDurations[DROPPING].store(val);
        m_cacheObserver->addCacheDuration(val);
        log = 5;
        break;
    }
    case HJ_PLUGIN_SETINFO_VIDEORENDER_frameSize:
    {
        m_frameSizes[i_info->getName()].store(i_info->getVal());
        if (i_info->getName() == VIDEOMAINRENDER) {
            log = 1;
        }
        else {
            log = 2;
        }
        break;
    }
    case HJ_PLUGIN_SETINFO_VIDEODECODER_frameSize:
    {
        m_frameSizes[i_info->getName()].store(i_info->getVal());
        if (i_info->getName() == VIDEOMAINDECODER) {
            log = 1;
        }
        else {
            log = 2;
        }
        break;
    }
    case HJ_PLUGIN_SETINFO_PLUGIN_status:
    {
        m_pluginStatuses[i_info->getName()].store(static_cast<HJStatus>(i_info->getVal()));
        log = 3;
        break;
    }
    case HJ_PLUGIN_SETINFO_DROPPING_videoKeyFrames:
    {
        m_videoKeyFrames[DROPPING].store(i_info->getVal());
        log = 4;
        break;
    }
    default:
        ret = HJErrInvalidParams;
        break;
    }

    if (log == 0) {
        int64_t decoderAudioDuration = m_audioDurations[AUDIODECODER].load();
        int64_t renderAudioDuration = m_audioDurations[AUDIORENDER].load();
//        HJFLogi("{}, plugin({}), decoderAudioDuration({}), renderAudioDuration({}), totalAudioDuration({})", getName(), i_info->getName(), decoderAudioDuration, renderAudioDuration, decoderAudioDuration + renderAudioDuration);
    }
    else if (log == 1) {
        int64_t decoderMainFramsSize = m_frameSizes[VIDEOMAINDECODER].load();
        int64_t renderMainFramsSize = m_frameSizes[VIDEOMAINRENDER].load();
//        HJFLogi("{}, decoderMainFramsSize({}), renderMainFramsSize({}), totalFramsSize({})", getName(), decoderMainFramsSize, renderMainFramsSize, decoderMainFramsSize + renderMainFramsSize);
    }
    else if (log == 2) {
        int64_t decoderSoftFramsSize = m_frameSizes[VIDEOSOFTDECODER].load();
        int64_t renderSoftFramsSize = m_frameSizes[VIDEOSOFTRENDER].load();
//        HJFLogi("{}, decoderSoftFramsSize({}), renderSoftFramsSize({})", getName(), decoderSoftFramsSize, renderSoftFramsSize);
    }
    else if (log == 3) {
//        HJFLogi("{}, plugin({}), status({})", getName(), i_info->getName(), i_info->getVal());
    }
    else if (log == 4) {
//        HJFLogi("{}, droppingVideoKeyFrames({})", getName(), i_info->getVal());
    }
    else if (log == 5) {
//        HJFLogi("{}, droppingAudioDuration({})", getName(), i_info->getVal());
    }

    if (m_playerListener && getSettings().getPluginInfosEnable()) {
        auto ntfy = HJMakeNotification(HJ_PLUGIN_NOTIFY_PLUGIN_SETINFOS);
        auto audio_dropping = (int64_t)m_audioDurations[DROPPING].load();
		(*ntfy)["audio_dropping"] = audio_dropping;
        auto video_dropping = (int64_t)m_videoKeyFrames[DROPPING].load();
        (*ntfy)["video_dropping"] = video_dropping;
        (*ntfy)[AUDIODECODER] = (int64_t)m_audioDurations[AUDIODECODER].load();
        auto audioRender = (int64_t)m_audioDurations[AUDIORENDER].load();
        (*ntfy)[AUDIORENDER] = audioRender;
        (*ntfy)[VIDEOMAINDECODER] = (int64_t)m_frameSizes[VIDEOMAINDECODER].load();
        (*ntfy)[VIDEOMAINRENDER] = (int64_t)m_frameSizes[VIDEOMAINRENDER].load();
        (*ntfy)[VIDEOSOFTDECODER] = (int64_t)m_frameSizes[VIDEOSOFTDECODER].load();
        (*ntfy)[VIDEOSOFTRENDER] = (int64_t)m_frameSizes[VIDEOSOFTRENDER].load();
        //HJFLogi("setInfo audio_dropping:{}, video_dropping:{}, audioRender:{}", audio_dropping, video_dropping, audioRender);
        //
        m_playerListener(std::move(ntfy));
    }

    return ret;
}

int HJGraphLivePlayer::getInfo(Information io_info)
{
    if (!io_info) {
        return HJErrInvalidParams;
    }

    int ret;
    switch (io_info->getID()) {
    case HJ_PLUGIN_GETINFO_DEMUXER_canDeliverToOutputs:
    {
        ret = canDemuxerDeliverToOutputs(io_info);
        break;
    }
    case HJ_PLUGIN_GETINFO_DROPPING_canDeliverToOutputs:
    {
        ret = canDroppingDeliverToOutputs(io_info);
        break;
    }
    case HJ_PLUGIN_GETINFO_VIDEODECODER_canDeliverToOutputs:
    {
        ret = canVideoDecoderDeliverToOutputs(io_info);
        break;
    }
    default:
        ret = HJErrInvalidParams;
        break;
    }

    return ret;
}

int HJGraphLivePlayer::listener(const HJNotification::Ptr ntf)
{
    if (ntf == nullptr) {
        return HJ_OK;
    }

    switch (ntf->getID()) {
    case HJ_PLUGIN_NOTIFY_DEMUXER_EOF:
    case HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT:
    case HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN:
    case HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME:
    case HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT:
    case HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME:
    {
        SYNC_CONS_LOCK([this] {
            CHECK_DONE_STATUS();
            if (m_demuxer) {
                m_demuxer->reset(500);
            }
            });
        return HJ_WOULD_BLOCK;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING:
    {
        priHeartbeatRegist(s_statPlaying, 5000);
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING:
    {
        priHeartbeatUnRegist(s_statPlaying);
        m_cacheObserver->getWatchTime(true);
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING:
    {
        priHeartbeatRegist(s_statLoading, 5000);
        m_cacheObserver->setStutter(true);
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING:
    {
        priHeartbeatUnRegist(s_statLoading);
        m_cacheObserver->setStutter(false);
        break;
    }
    //case HJ_PLUGIN_NOTIFY_AUDIOCACHE_DURATION:
    //{
    //    m_cacheObserver->addCacheDuration(ntf->getVal());
    //    break;
    //}
    case HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME:
    {
        int64_t currentTime = HJCurrentSteadyMS();
        int64_t diffTime = currentTime - m_entryTime;
        HJFLogi("{} first frame time:{} flag:{}", getName(), diffTime, ntf->getMsg());

        m_firstRenderIdx++;

        if (m_firstRenderIdx == 1)
        {
            HJFLogi("{} first frame time:{} flag:{} m_firstRenderIdx==1", getName(), diffTime, ntf->getMsg());
            auto statCtx = m_statCtx.lock();
            if (statCtx)
            {
                statCtx->notify("HJGraphLivePlayer", JNotify_Play_SecondOnTime, diffTime);
                //HJFLogi("{} statplayer first frame time:{} flag:{}", getName(), diffTime, ntf->getMsg());
            }
            return m_playerListener ? m_playerListener(ntf) : HJ_OK;
        }
        else
        {
            return HJ_OK;
        }
        break;
    }
    default:
        break;
    }

    return m_playerListener ? m_playerListener(ntf) : HJ_OK;
}

int HJGraphLivePlayer::openURL(HJMediaUrl::Ptr i_url)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_demuxer == nullptr) {
            return HJErrFatal;
        }

        return m_demuxer->openURL(i_url);
    });
}

int HJGraphLivePlayer::close()
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        return HJ_OK;
    });
}

bool HJGraphLivePlayer::hasAudio()
{
    if (!(m_mediaType.load() & HJMEDIA_TYPE_AUDIO)) {
        return false;
    }

//    if (m_audioDurations[DROPPING].load() > 0) {
//        return true;
//    }
    if (m_audioDurations[AUDIODECODER].load() > 0) {
        return true;
    }
    if (m_audioDurations[AUDIORENDER].load() > 0) {
        return true;
    }

    return false;
}

int HJGraphLivePlayer::setMute(bool i_mute)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->setMute(i_mute);
    });
}

bool HJGraphLivePlayer::isMuted()
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(false);

        if (!m_audioRender) {
            return false;
        }

        return m_audioRender->isMuted();
    });
}

#if defined (WINDOWS)
int HJGraphLivePlayer::resetAudioDevice(const std::string& i_deviceName)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->resetDevice(i_deviceName);
    });
}
#endif

int HJGraphLivePlayer::canDemuxerDeliverToOutputs(Information io_info)
{
//    if (m_audioDurations[DROPPING].load() > 1000) {
//        return HJ_WOULD_BLOCK;
//    }

    return HJ_OK;
}

int HJGraphLivePlayer::canDroppingDeliverToOutputs(Information io_info)
{
    const std::any* anyObj = io_info->getStorage("mediaFrame");
    if (!anyObj) {
        return HJErrInvalidParams;
    }
    auto mediaFrame = std::any_cast<HJMediaFrame::Ptr>(*anyObj);
    if (!mediaFrame) {
        return HJErrInvalidParams;
    }

    if (mediaFrame->isAudio()) {
        if ((m_audioDurations[AUDIODECODER].load() + m_audioDurations[AUDIORENDER].load()) > 300) {
            return HJ_WOULD_BLOCK;
        }
    }
    else if (mediaFrame->isVideo()) {
        if ((m_frameSizes[VIDEOMAINDECODER].load() + m_frameSizes[VIDEOMAINRENDER].load()) > 30) {
            return HJ_WOULD_BLOCK;
        }
    }

    return HJ_OK;
}

int HJGraphLivePlayer::canVideoDecoderDeliverToOutputs(Information io_info)
{
    if (io_info->getName() == VIDEOMAINDECODER) {
        if (m_frameSizes[VIDEOMAINRENDER].load() >= 2) {
            return HJ_WOULD_BLOCK;
        }
    }

    return HJ_OK;
}

NS_HJ_END
