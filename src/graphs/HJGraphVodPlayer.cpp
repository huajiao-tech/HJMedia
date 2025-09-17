#include "HJGraphVodPlayer.h"
#include "HJStatContext.h"
#include "HJFLog.h"
#if defined (HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

#define DEMUXER         "demuxer"
#define AUDIODECODER    "audioDecoder"
#define AUDIORENDER     "audioRender"
#define VIDEODECODER    "videoDecoder"
#define VIDEORENDER     "videoRender"

HJGraphVodPlayer::HJGraphVodPlayer(const std::string& i_name, size_t i_identify)
    : HJGraphPlayer(i_name, i_identify)
{
    m_cacheObserver = HJCreates<HJCacheObserver>();
}

HJGraphVodPlayer::~HJGraphVodPlayer()
{
    HJGraphVodPlayer::done();
}

int HJGraphVodPlayer::internalInit(HJKeyStorage::Ptr i_param)
{
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
        // _TODO_GS_
        HJStatContext::WPtr statCtx;
        if (i_param) {
            statCtx = i_param->getValueObj<HJStatContext::WPtr>("HJStatContext");
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
#if defined (HarmonyOS)
        GET_PARAMETER(HJOGRenderWindowBridge::Ptr, mainBridge);
        IF_FALSE_BREAK(mainBridge, HJErrInvalidParams);
#endif

        auto graphInfo = std::make_shared<HJKeyStorage>();
        (*graphInfo)["insIdx"] = insIdx;
        (*graphInfo)["graph"] = std::static_pointer_cast<HJGraph>(SHARED_FROM_THIS);

        IF_FALSE_BREAK(m_demuxer = HJPluginFFDemuxer::Create<HJPluginFFDemuxer>(DEMUXER, graphInfo), HJErrFatal);
        addPlugin(m_demuxer);
        m_pluginStatuses[DEMUXER].store(HJSTATUS_NONE);
        IF_FALSE_BREAK(m_videoRender = HJPluginVideoRender::Create<HJPluginVideoRender>(VIDEORENDER, graphInfo), HJErrFatal);
        addPlugin(m_videoRender);
        m_frameSizes[VIDEORENDER].store(0);
        m_pluginStatuses[VIDEORENDER].store(HJSTATUS_NONE);
        if (deviceType == HJDEVICE_TYPE_NONE) {
            IF_FALSE_BREAK(m_videoSFDecoder = HJPluginVideoFFDecoder::Create<HJPluginVideoFFDecoder>(VIDEODECODER, graphInfo), HJErrFatal);
            addPlugin(m_videoSFDecoder);
            m_frameSizes[VIDEODECODER].store(0);
            m_pluginStatuses[VIDEODECODER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_videoSFDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoSFDecoder, m_videoRender, HJMEDIA_TYPE_VIDEO), ret);
        }
        else if (deviceType == HJDEVICE_TYPE_OHCODEC) {
#if defined (HarmonyOS)
            IF_FALSE_BREAK(m_videoHWDecoder = HJPluginVideoOHDecoder::Create<HJPluginVideoOHDecoder>(VIDEODECODER, graphInfo), HJErrFatal);
            addPlugin(m_videoHWDecoder);
            m_frameSizes[VIDEODECODER].store(0);
            m_pluginStatuses[VIDEODECODER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_videoHWDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoHWDecoder, m_videoRender, HJMEDIA_TYPE_VIDEO), ret);
#endif
        }
        IF_FALSE_BREAK(m_renderThread = HJLooperThread::quickStart("renderThread" + HJFMT("_{}", insIdx)), HJErrFatal);
        addThread(m_renderThread);
        if (audioInfo != nullptr) {
            m_audioInfo = audioInfo;
            IF_FALSE_BREAK(m_audioDecoder = HJPluginAudioFFDecoder::Create<HJPluginAudioFFDecoder>(AUDIODECODER, graphInfo), HJErrFatal);
            addPlugin(m_audioDecoder);
            m_audioDurations[AUDIODECODER].store(0);
            m_pluginStatuses[AUDIODECODER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_audioDecoder, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_audioResampler = HJPluginAudioResampler::Create<HJPluginAudioResampler>("HJPluginAudioResampler", graphInfo), HJErrFatal);
            addPlugin(m_audioResampler);
            m_pluginStatuses["HJPluginAudioResampler"].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioDecoder, m_audioResampler, HJMEDIA_TYPE_AUDIO), ret);
#if defined (WINDOWS)
            IF_FALSE_BREAK(m_audioRender = HJPluginAudioWORender::Create<HJPluginAudioWORender>(AUDIORENDER, graphInfo), HJErrFatal);
#endif
#if defined (HarmonyOS)
            IF_FALSE_BREAK(m_audioRender = HJPluginAudioOHRender::Create<HJPluginAudioOHRender>(AUDIORENDER, graphInfo), HJErrFatal);
#endif
            addPlugin(m_audioRender);
            m_audioDurations[AUDIORENDER].store(0);
            m_pluginStatuses[AUDIORENDER].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioResampler, m_audioRender, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_audioThread = HJLooperThread::quickStart("audioThread" + HJFMT("_{}", insIdx)), HJErrFatal);
            addThread(m_audioThread);
        }

        m_cacheObserver->setWatchStartTime();

        IF_FALSE_BREAK(m_timeline = HJTimeline::Create<HJTimeline>("HJTimeline" + HJFMT("_{}", insIdx)), HJErrFatal);
        IF_FAIL_BREAK(ret = m_timeline->init(nullptr), ret);

        auto param = std::make_shared<HJKeyStorage>();
        (*param)["HJStatContext"] = statCtx;
        (*param)["timeline"] = m_timeline;
        (*param)["thread"] = m_renderThread;
        (*param)["pluginListener"] = pluginListener;
        (*param)["deviceType"] = deviceType;
        if (deviceType == HJDEVICE_TYPE_NONE) {
#if defined (HarmonyOS)
            (*param)["bridge"] = mainBridge;
#endif
            IF_FAIL_BREAK(ret = m_videoRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_videoSFDecoder->init(param), ret);
        }
        else if (deviceType == HJDEVICE_TYPE_OHCODEC) {
            IF_FAIL_BREAK(ret = m_videoRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["pluginListener"] = pluginListener;
#if defined (HarmonyOS)
            (*param)["bridge"] = mainBridge;
            IF_FAIL_BREAK(ret = m_videoHWDecoder->init(param), ret);
#endif
        }

        if (audioInfo != nullptr) {
            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
            (*param)["timeline"] = m_timeline;
            (*param)["thread"] = m_renderThread;
            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_audioRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
            (*param)["thread"] = m_audioThread;
            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_audioResampler->init(param), ret);

            param->erase("audioInfo");
            IF_FAIL_BREAK(ret = m_audioDecoder->init(param), ret);
        }

        param = std::make_shared<HJKeyStorage>();
        (*param)["pluginListener"] = pluginListener;
        if (mediaUrl) {
            (*param)["mediaUrl"] = mediaUrl;
        }
        IF_FAIL_BREAK(ret = m_demuxer->init(param), ret);

        return HJ_OK;
    } while (false);

    HJGraphVodPlayer::internalRelease();
    return ret;
}

void HJGraphVodPlayer::internalRelease()
{
#if defined (HarmonyOS)
    m_videoHWDecoder = nullptr;
#endif
    m_videoRender = nullptr;
    m_audioRender = nullptr;
    m_audioResampler = nullptr;
    m_videoSFDecoder = nullptr;
    m_audioDecoder = nullptr;
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
}

int HJGraphVodPlayer::setInfo(const Information i_info)
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
    case HJ_PLUGIN_SETINFO_VIDEORENDER_frameSize:
    {
        m_frameSizes[VIDEORENDER].store(i_info->getVal());
        log = 1;
        break;
    }
    case HJ_PLUGIN_SETINFO_VIDEODECODER_frameSize:
    {
        m_frameSizes[VIDEODECODER].store(i_info->getVal());
        log = 1;
        break;
    }
    case HJ_PLUGIN_SETINFO_PLUGIN_status:
    {
        m_pluginStatuses[i_info->getName()].store(static_cast<HJStatus>(i_info->getVal()));
        log = 2;
        break;
    }
    default:
        ret = HJErrInvalidParams;
        break;
    }

    if (log == 0) { 
        int64_t decoderAudioDuration = m_audioDurations[AUDIODECODER];
        int64_t renderAudioDuration = m_audioDurations[AUDIORENDER];
//        HJFLogi("{}, decoderAudioDuration({}), renderAudioDuration({}), totalAudioDuration({})", getName(), decoderAudioDuration, renderAudioDuration, decoderAudioDuration + renderAudioDuration);
    }
    else if (log == 1) { 
        int64_t decoderFramsSize = m_frameSizes[VIDEODECODER];
        int64_t renderFramsSize = m_frameSizes[VIDEORENDER];
//        HJFLogi("{}, decoderFramsSize({}), renderFramsSize({}), totalFramsSize({})", getName(), decoderFramsSize, renderFramsSize, decoderFramsSize + renderFramsSize);
    }
    else if (log == 2) {
//        HJFLogi("{}, plugin({}), status({})", getName(), i_info->getName(), i_info->getVal());
    }

    return ret;
}

int HJGraphVodPlayer::getInfo(Information io_info)
{
    if (!io_info) {
        return HJErrInvalidParams;
    }

    int ret;
    switch (io_info->getID()) {
    case HJ_PLUGIN_GETINFO_VIDEODECODER_canDeliverToOutputs:
    {
        ret = canVideoDecoderDeliverToOutputs(io_info);
        break;
    }
    case HJ_PLUGIN_GETINFO_DEMUXER_canDeliverToOutputs:
    {
        ret = canDemuxerDeliverToOutputs(io_info);
        break;
    }
    default:
        ret = HJErrInvalidParams;
        break;
    }

    return ret;
}

int HJGraphVodPlayer::listener(const HJNotification::Ptr ntf)
{
    if (ntf == nullptr) {
        return HJ_OK;
    }

    switch (ntf->getID()) {
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING:
    {
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING:
    {
        m_cacheObserver->getWatchTime(true);
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING:
    {
        m_cacheObserver->setStutter(true);
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING:
    {
        m_cacheObserver->setStutter(false);
        break;
    }
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
        break;
    }
    case HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF:
    {
        SYNCHRONIZED_SYNC(m_mediaTypeSync);
        int type = static_cast<int>(m_mediaType.load());
        type &= ~HJMEDIA_TYPE_VIDEO;
        HJFLogi("{}, video eof, type({})", getName(), type);
        m_mediaType.store(static_cast<HJMediaType>(type));
        if (type == 0) {
            ntf->setID(HJ_PLUGIN_NOTIFY_EOF);
        }
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_EOF:
    {
        SYNCHRONIZED_SYNC(m_mediaTypeSync);
        int type = static_cast<int>(m_mediaType.load());
        type &= ~HJMEDIA_TYPE_AUDIO;
        HJFLogi("{}, audio eof, type({})", getName(), type);
        m_mediaType.store(static_cast<HJMediaType>(type));
        if (type == 0) {
            ntf->setID(HJ_PLUGIN_NOTIFY_EOF);
        }
        break;
    }
    default:
        break;
    }

    return m_playerListener ? m_playerListener(ntf) : HJ_OK;
}

int HJGraphVodPlayer::openURL(HJMediaUrl::Ptr i_url)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_demuxer == nullptr) {
            return HJErrFatal;
        }

        return m_demuxer->openURL(i_url);
    });
}

int HJGraphVodPlayer::close()
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        return HJ_OK;
    });
}

bool HJGraphVodPlayer::hasAudio()
{
    if (!(m_mediaType.load() & HJMEDIA_TYPE_AUDIO)) {
        return false;
    }

    if (m_audioDurations[AUDIODECODER].load() > 0) {
        return true;
    }
    if (m_audioDurations[AUDIORENDER].load() > 0) {
        return true;
    }

    return false;
}

int HJGraphVodPlayer::setMute(bool i_mute)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->setMute(i_mute);
    });
}

bool HJGraphVodPlayer::isMuted()
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(false);

        if (!m_audioRender) {
            return false;
        }

        return m_audioRender->isMuted();
    });
}

int HJGraphVodPlayer::canDemuxerDeliverToOutputs(Information io_info)
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
        if ((m_audioDurations[AUDIODECODER].load() + m_audioDurations[AUDIORENDER].load()) > 600) {
            return HJ_WOULD_BLOCK;
        }
    }
    else if (mediaFrame->isVideo()) {
        if ((m_frameSizes[VIDEODECODER].load() + m_frameSizes[VIDEORENDER].load()) > 30) {
            return HJ_WOULD_BLOCK;
        }
    }

    return HJ_OK;
}

int HJGraphVodPlayer::canVideoDecoderDeliverToOutputs(Information io_info)
{
    if (m_frameSizes[VIDEORENDER].load() >= 2) {
        return HJ_WOULD_BLOCK;
    }

    return HJ_OK;
}

NS_HJ_END
