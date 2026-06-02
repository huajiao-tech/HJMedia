#include "HJGraphMusicPlayer.h"
#include "HJFLog.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

HJ_NAME_DECLARE(demuxer)
HJ_NAME_DECLARE(audioDecoder)
HJ_NAME_DECLARE(audioResampler)
HJ_NAME_DECLARE(audioRender)

// temp
HJ_NAME_DECLARE(ffMuxer)

HJGraphMusicPlayer::HJGraphMusicPlayer(const std::string& i_name, size_t i_identify)
    : HJGraphPlayer(i_name, i_identify)
{
    m_cacheObserver = HJCreates<HJCacheObserver>();
    m_cacheObserver->setName(i_name);
}

int HJGraphMusicPlayer::internalInit(HJKeyStorage::Ptr i_param)
{
    MUST_HAVE_PARAMETERS;
    int ret = HJGraph::internalInit(i_param);
    if (ret != HJ_OK) {
        return ret;
    }

    do {
        ret = registerHandlers();
        if (ret != HJ_OK) {
            break;
        }

        m_canDemuxerDeliverAudioToOutput = [this]() {
            return (m_audioDuration[audioDecoder_HASH64].load() + m_audioDuration[audioRender_HASH64].load()) <= 600;
        };

        m_thread = HJLooperThread::quickStart(getName());
        if (m_thread == nullptr) {
            ret = HJErrFatal;
            break;
        }

        m_handler = m_thread->createHandler();
        if (m_handler == nullptr) {
            ret = HJErrFatal;
            break;
        }

        m_seekId = m_handler->genMsgId();

        int insIdx = 0;
        if (i_param && i_param->haveStorage("InsIdx")) {
            insIdx = i_param->getValue<int>("InsIdx");
        }

        HJStatContext::WPtr statCtx;
        if (i_param) {
            statCtx = i_param->getValueObj<HJStatContext::WPtr>("HJStatContext");
        }

        GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
        IF_FALSE_BREAK(audioInfo, HJErrInvalidParams);
        GET_PARAMETER(HJListener, playerListener);
        m_playerListener = playerListener;
        GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
        GET_PARAMETER2(int64_t, prerollDurationMs, 120);
        GET_PARAMETER2(int64_t, pcmCallbackIntervalMs, 20);
#if defined (WINDOWS)
        GET_PARAMETER(std::string, audioDeviceName);
#endif
        GET_PARAMETER2(int, repeats, 1);
        m_repeats = std::max<int>(0, repeats);

        auto graphInfo = std::make_shared<HJKeyStorage>();
        (*graphInfo)["insIdx"] = insIdx;
        (*graphInfo)["graph"] = std::static_pointer_cast<HJGraph>(SHARED_FROM_THIS);

        IF_FALSE_BREAK(m_demuxer = HJPluginFFDemuxer::Create<HJPluginFFDemuxer>(demuxer_STRING, demuxer_HASH64, graphInfo), HJErrFatal);
        addPlugin(m_demuxer);
        HJPlugin::Wtr wPlugin = m_demuxer;
        m_eventBus->subscribe(EVENT_DEMUXER_ON_OUTPUT_UPDATED_ID, [wPlugin]() -> HJRet {
            auto plugin = wPlugin.lock();
            if (!plugin) {
                return HJErrAlreadyDone;
            }
            plugin->onOutputUpdated();
            return HJ_OK;
        });

        IF_FALSE_BREAK(m_renderThread = HJLooperThread::quickStart("renderThread" + HJFMT("_{}", insIdx)), HJErrFatal);
        addThread(m_renderThread);

        m_audioInfo = audioInfo;
        IF_FALSE_BREAK(m_audioDecoder = HJPluginAudioFFDecoder::Create<HJPluginAudioFFDecoder>(audioDecoder_STRING, audioDecoder_HASH64, graphInfo), HJErrFatal);
        addPlugin(m_audioDecoder);
        m_audioDuration[audioDecoder_HASH64].store(0);
        IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_audioDecoder, HJMEDIA_TYPE_AUDIO), ret);

        IF_FALSE_BREAK(m_audioResampler = HJPluginAudioResampler::Create<HJPluginAudioResampler>(audioResampler_STRING, audioResampler_HASH64, graphInfo), HJErrFatal);
        addPlugin(m_audioResampler);
        IF_FAIL_BREAK(ret = connectPlugins(m_audioDecoder, m_audioResampler, HJMEDIA_TYPE_AUDIO), ret);

#if defined (HarmonyOS)
        IF_FALSE_BREAK(m_audioRender = HJPluginAudioOHRender::Create<HJPluginAudioOHRender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
#elif defined (WINDOWS)
        IF_FALSE_BREAK(m_audioRender = HJPluginAudioWASRender::Create<HJPluginAudioWASRender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
#elif defined (HJ_OS_IOS)
        IF_FALSE_BREAK(m_audioRender = HJPluginAudioIOSRender::Create<HJPluginAudioIOSRender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
#elif defined (HJ_OS_ANDROID)
        IF_FALSE_BREAK(m_audioRender = HJPluginAudioAARender::Create<HJPluginAudioAARender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
#else
        ret = HJErrNotSupport;
        break;
#endif
        addPlugin(m_audioRender);
        m_audioDuration[audioRender_HASH64].store(0);
        IF_FAIL_BREAK(ret = connectPlugins(m_audioResampler, m_audioRender, HJMEDIA_TYPE_AUDIO), ret);

        IF_FALSE_BREAK(m_audioThread = HJLooperThread::quickStart("audioThread" + HJFMT("_{}", insIdx)), HJErrFatal);
        addThread(m_audioThread);

        m_cacheObserver->setWatchStartTime();

        IF_FALSE_BREAK(m_timeline = HJTimeline::Create<HJTimeline>("HJTimeline" + HJFMT("_{}", insIdx)), HJErrFatal);
        IF_FAIL_BREAK(ret = m_timeline->init(nullptr), ret);

        auto param = std::make_shared<HJKeyStorage>();
        (*param)["HJStatContext"] = statCtx;
        (*param)["timeline"] = m_timeline;
#if defined (WINDOWS)
        if (!audioDeviceName.empty()) {
            (*param)["audioDeviceName"] = audioDeviceName;
        }
#else
        (*param)["thread"] = m_renderThread;
#endif
        (*param)["audioInfo"] = audioInfo;
        (*param)["prerollDurationMs"] = prerollDurationMs;
        (*param)["pcmCallbackIntervalMs"] = pcmCallbackIntervalMs;
        IF_FAIL_BREAK(ret = m_audioRender->init(param), ret);

        param = std::make_shared<HJKeyStorage>();
        (*param)["audioInfo"] = audioInfo;
        (*param)["thread"] = m_audioThread;
        IF_FAIL_BREAK(ret = m_audioResampler->init(param), ret);

        param->erase("audioInfo");
        IF_FAIL_BREAK(ret = m_audioDecoder->init(param), ret);

        param = std::make_shared<HJKeyStorage>();
        if (mediaUrl) {
            (*param)["mediaUrl"] = mediaUrl;
        }
        IF_FAIL_BREAK(ret = m_demuxer->init(param), ret);

        return HJ_OK;
    } while (false);

    HJGraphMusicPlayer::internalRelease();
    return ret;
}

void HJGraphMusicPlayer::internalRelease()
{
    m_audioRender = nullptr;
    m_audioResampler = nullptr;
    m_audioDecoder = nullptr;
    m_demuxer = nullptr;

    m_audioThread = nullptr;
    m_renderThread = nullptr;

    m_audioInfo = nullptr;
    m_playerListener = nullptr;
    m_paused.store(false);

    if (m_timeline) {
        m_timeline->done();
        m_timeline = nullptr;
    }

    HJGraph::internalRelease();

    if (m_thread) {
        m_thread->done();
        m_thread = nullptr;
    }
    m_handler = nullptr;
}

int HJGraphMusicPlayer::openURL(HJMediaUrl::Ptr i_url)
{
    return SYNC_CONS_LOCK([this, i_url] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_demuxer == nullptr) {
            return HJErrFatal;
        }

        return m_demuxer->openURL(i_url);
    });
}

int HJGraphMusicPlayer::close()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        return HJ_OK;
    });
}

int HJGraphMusicPlayer::setMute(bool i_mute)
{
    return SYNC_CONS_LOCK([this, i_mute] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->setMute(i_mute);
    });
}

bool HJGraphMusicPlayer::isMuted()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(false);

        if (!m_audioRender) {
            return false;
        }

        return m_audioRender->isMuted();
    });
}

int HJGraphMusicPlayer::setVolume(float i_volume)
{
    return SYNC_CONS_LOCK([this, i_volume] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->setVolume(i_volume);
    });
}

float HJGraphMusicPlayer::getVolume()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(1.0f);

        if (!m_audioRender) {
            return 1.0f;
        }

        return m_audioRender->getVolume();
    });
}

int HJGraphMusicPlayer::pause()
{
    return SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_paused.load()) {
            return HJ_OK;
        }

        m_paused.store(true);

        if (m_timeline) {
            m_timeline->pause();
        }

        if (m_audioRender) {
            m_audioRender->setPause(true);
        }

        return HJ_OK;
    });
}

int HJGraphMusicPlayer::resume()
{
    return SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (!m_paused.load()) {
            return HJ_OK;
        }

        m_paused.store(false);

        if (m_audioRender) {
            m_audioRender->setPause(false);
        }

        if (m_timeline) {
            m_timeline->play();
        }

        return HJ_OK;
    });
}

#if defined (WINDOWS)
int HJGraphMusicPlayer::resetAudioDevice(const std::string& i_deviceName)
{
    return SYNC_CONS_LOCK([this, i_deviceName] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->resetDevice(i_deviceName);
    });
}
#endif

int64_t HJGraphMusicPlayer::getCurrentTimestamp()
{
    int64_t ts{};
    SYNC_CONS_LOCK([this, &ts] {
        CHECK_DONE_STATUS();
        if (m_timeline) {
            int64_t streamIndex;
            int64_t timestamp;
            double speed;
            if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
                ts = timestamp;
            }
        }
    });

    int64_t maxTs = -1;
    {
        std::lock_guard<std::mutex> lock(m_playbackStateMutex);
        maxTs = m_maxTimestamp;
    }
    if (maxTs > 0) {
        ts = std::min<int64_t>(ts, maxTs);
    }
    return ts;
}

int64_t HJGraphMusicPlayer::getDuration()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(int64_t{ 0 });
        if (!m_demuxer) {
            return int64_t{ 0 };
        }
        return m_demuxer->getDuration();
    });
}

std::vector<int> HJGraphMusicPlayer::getAudioTrackIds()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(std::vector<int>{});
        if (!m_demuxer) {
            return std::vector<int>{};
        }
        auto mediaInfo = m_demuxer->getMediaInfo();
        if (!mediaInfo) {
            return std::vector<int>{};
        }

        std::vector<int> audioTrackIds;
        for (const auto& audioInfo : mediaInfo->getAudioInfos()) {
            if (!audioInfo) {
                continue;
            }
            audioTrackIds.push_back(static_cast<int>(audioInfo->m_trackID));
        }
        return audioTrackIds;
    });
}

HJAudioTrackDisplayInfoVector HJGraphMusicPlayer::getAudioTrackDisplayInfos()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(HJAudioTrackDisplayInfoVector{});
        if (!m_demuxer) {
            return HJAudioTrackDisplayInfoVector{};
        }
        auto mediaInfo = m_demuxer->getMediaInfo();
        if (!mediaInfo) {
            return HJAudioTrackDisplayInfoVector{};
        }

        HJAudioTrackDisplayInfoVector infos;
        for (const auto& info : mediaInfo->getAudioTrackDisplayInfos()) {
            infos.push_back(info ? info->dup() : nullptr);
        }
        return infos;
    });
}

int HJGraphMusicPlayer::switchAudioTrack(int i_trackId)
{
    return SYNC_PROD_LOCK([this, i_trackId] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (!m_demuxer || !m_audioRender) {
            return HJErrNotInited;
        }

        auto mediaInfo = m_demuxer->getMediaInfo();
        if (!mediaInfo) {
            return HJErrNotInited;
        }

        if (mediaInfo->m_astIdx == i_trackId) {
            return HJ_OK;
        }
#if 0
        bool wasPaused = m_paused.load();
        if (!wasPaused) {
            m_paused.store(true);
            if (m_timeline) {
                m_timeline->pause();
            }
            m_audioRender->setPause(true);
        }

        int ret = m_demuxer->switchAudioTrack(i_trackId);
        if (ret != HJ_OK) {
            if (!wasPaused) {
                m_audioRender->setPause(false);
                if (m_timeline) {
                    m_timeline->play();
                }
                m_paused.store(false);
            }
            return ret;
        }

        int64_t ts = 0;
        if (m_timeline) {
            int64_t streamIndex{};
            double speed{};
            int64_t timestamp{};
            if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
                ts = timestamp;
            }
        }
        if (ts < 0) {
            ts = 0;
        }
        m_demuxer->seek(ts);

        if (!wasPaused) {
            m_audioRender->setPause(false);
            if (m_timeline) {
                m_timeline->play();
            }
            m_paused.store(false);
        }

        return HJ_OK;
#else
        return m_demuxer->switchAudioTrack(i_trackId);
#endif
    });
}

int HJGraphMusicPlayer::seek(int64_t i_timestamp)
{
    HJFLogi("i_timestamp:{}", i_timestamp);
    return SYNC_CONS_LOCK([this, i_timestamp] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_handler == nullptr) {
            return HJErrFatal;
        }

        HJPluginFFDemuxer::Wtr wDemuxer = m_demuxer;
        m_handler->asyncAndClear([wDemuxer, i_timestamp] {
            auto demuxer = wDemuxer.lock();
            if (demuxer) {
                demuxer->seek(i_timestamp);
            }
        }, m_seekId);

        return HJ_OK;
    });
}

int HJGraphMusicPlayer::setRepeats(int i_repeats)
{
    return SYNC_CONS_LOCK([this, i_repeats] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (i_repeats < 0) {
            return HJErrInvalidParams;
        }
        if (m_demuxer == nullptr) {
            return HJErrNotInited;
        }

        bool shouldReset = false;
        {
            std::lock_guard<std::mutex> lock(m_playbackStateMutex);
            m_repeats = i_repeats;

            if (!m_audioPlaybackCompleted) {
                if (m_pendingDemuxerFinalEof) {
                    m_pendingDemuxerFinalEof = false;
                    m_mediaType.store(m_demuxer->getMediaType());
                    shouldReset = true;
                }
            }
        }

        if (shouldReset) {
            m_demuxer->reset(0);
        }

        return HJ_OK;
    });
}

int HJGraphMusicPlayer::registerHandlers()
{
    using RegFn = HJRet(HJGraphMusicPlayer::*)();
    static const RegFn regs[] = {
        & HJGraphMusicPlayer::registerQueryHandler_hasAudio,
        & HJGraphMusicPlayer::registerQueryHandler_canDeliverToOutputs,
        & HJGraphMusicPlayer::registerQueryHandler_canPluginEof,
        & HJGraphMusicPlayer::registerEventHandler_pluginNotify,
        & HJGraphMusicPlayer::registerEventHandler_mediaType,
        & HJGraphMusicPlayer::registerQueryHandler_seekSucceeded,
        & HJGraphMusicPlayer::registerQueryHandler_streamOpened,
        & HJGraphMusicPlayer::registerEventHandler_statusUpdated,
        & HJGraphMusicPlayer::registerEventHandler_audioDuration,
    };

    for (auto fn : regs) {
        HJRet ret = (this->*fn)();
        if (ret != HJ_OK) {
            return ret;
        }
    }
    return HJ_OK;
}

int HJGraphMusicPlayer::registerQueryHandler_hasAudio()
{
    return m_queryBus->registerHandler(QUERY_HAS_AUDIO_ID, [this]() -> bool {
        if (!(m_mediaType.load() & HJMEDIA_TYPE_AUDIO)) {
            return false;
        }
        if (m_audioDuration[audioDecoder_HASH64].load() > 0) {
            return true;
        }
        if (m_audioDuration[audioRender_HASH64].load() > 0) {
            return true;
        }
        return false;
    });
}

int HJGraphMusicPlayer::registerEventHandler_mediaType()
{
    return m_eventBus->registerHandler(EVENT_MEDIA_TYPE_ID, [this](uint32_t mediaType) {
        //m_mediaType.store(mediaType);
    });
}

int HJGraphMusicPlayer::registerQueryHandler_canDeliverToOutputs()
{
    return m_queryBus->registerHandler(QUERY_CAN_DELIVER_TO_OUTPUTS_ID, [this](size_t pluginId, const HJMediaFrame::Ptr& frame) -> bool {
        if (pluginId == demuxer_HASH64) {
            if (frame->isAudio()) {
                return m_canDemuxerDeliverAudioToOutput();
            }
            return true;
        }
        return false;
    });
}

int HJGraphMusicPlayer::registerQueryHandler_canPluginEof()
{
    return m_queryBus->registerHandler(QUERY_CAN_PLUGIN_EOF_ID, [this](size_t pluginId, int streamIndex) -> bool {
        if (pluginId == demuxer_HASH64) {
            bool shouldReset = false;
            {
                std::lock_guard<std::mutex> lock(m_playbackStateMutex);
                m_currentRepeatNumber++;
                m_lastStreamIndex = streamIndex;
                if (m_repeats == 0 || m_currentRepeatNumber < m_repeats) {
                    shouldReset = true;
                } else if (m_currentRepeatNumber >= m_repeats) {
                    m_pendingDemuxerFinalEof = true;
                }
            }
            if (shouldReset) {
                SYNC_CONS_LOCK([this] {
                    CHECK_DONE_STATUS();
                    if (m_demuxer) {
                        m_demuxer->reset(0);
                    }
                });
            }
            return true;
        }
        if (pluginId == audioRender_HASH64) {
            int64_t timestamp = -1;
            if (m_timeline) {
                int64_t timelineStreamIndex{};
                double speed{};
                if (m_timeline->getTimestamp(timelineStreamIndex, timestamp, speed)) {
                    (void)timelineStreamIndex;
                } else {
                    timestamp = -1;
                }
            }

            bool matchedFinalEof = false;
            bool shouldReportEof = false;
            {
                std::lock_guard<std::mutex> lock(m_playbackStateMutex);
                matchedFinalEof = (m_pendingDemuxerFinalEof && streamIndex == m_lastStreamIndex);
                auto type = m_mediaType.fetch_and(~HJMEDIA_TYPE_AUDIO);
                if (matchedFinalEof && !m_audioPlaybackCompleted && m_mediaType.load() == 0) {
                    m_audioPlaybackCompleted = true;
                    m_maxTimestamp = timestamp;
                    shouldReportEof = true;
                }
            }

            if (shouldReportEof) {
                HJFLogi("{}, eof(audio))", getName());
                m_eventBus->report(EVENT_GRAPH_EOF_ID);
            }
            return matchedFinalEof;
        }

        return false;
    });
}

int HJGraphMusicPlayer::registerEventHandler_pluginNotify()
{
    return m_eventBus->registerHandler(EVENT_PLUGIN_NOTIFY_ID, [this](HJPluginNotifyType notifyType, size_t pluginID) {
        switch (notifyType) {
        case HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_SEEK:
        case HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT:
        case HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME:
        case HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA:
        case HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT:
        case HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN:
        case HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME:
        {
            HJFLogi("{}, {}, notifyType({})", getName(), "EVENT_PLUGIN_NOTIFY_ID",
                    HJPluginNotifyTypeToString(notifyType));
            SYNC_CONS_LOCK([this] {
                CHECK_DONE_STATUS();
                if (m_demuxer) {
                    m_demuxer->reset(500);
                }
            });
            break;
        }
        case HJ_PLUGIN_NOTIFY_ERROR_AUDIOCONVERTER_CONVERT:
            break;
        case HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME:
            break;
        case HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT:
            break;
        case HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK:
            break;
        case HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING:
            break;
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
        default:
            break;
        }
    });
}

int HJGraphMusicPlayer::registerQueryHandler_seekSucceeded()
{
    return m_eventBus->registerHandler(EVENT_SEEK_SUCCEEDED_ID, [this](size_t pluginId) {
        if (pluginId != demuxer_HASH64) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_playbackStateMutex);
        if (m_pendingDemuxerFinalEof) {
            m_pendingDemuxerFinalEof = false;
            m_currentRepeatNumber = m_repeats - 1;
            m_mediaType.store(m_demuxer->getMediaType());
        }
        if (m_audioPlaybackCompleted) {
            m_audioPlaybackCompleted = false;
            m_maxTimestamp = -1;
        }
    });
}

int HJGraphMusicPlayer::registerQueryHandler_streamOpened()
{
    return m_eventBus->registerHandler(EVENT_STREAM_OPENED_ID, [this](size_t pluginId) {
        (void)pluginId;

        {
            std::lock_guard<std::mutex> lock(m_playbackStateMutex);
            m_currentRepeatNumber = 0;
            m_mediaType.store(m_demuxer->getMediaType());
            m_audioPlaybackCompleted = false;
            m_pendingDemuxerFinalEof = false;
            m_maxTimestamp = -1;
        }

        m_eventBus->report(EVENT_GRAPH_STREAM_OPENED_ID);
    });
}

int HJGraphMusicPlayer::registerEventHandler_statusUpdated()
{
    return m_eventBus->registerHandler(EVENT_STATUS_UPDATED_ID, [this](size_t pluginId) {
        (void)pluginId;
    });
}

int HJGraphMusicPlayer::registerEventHandler_audioDuration()
{
    return m_eventBus->registerHandler(EVENT_AUDIO_DURATION_ID, [this](size_t pluginId, int64_t audioDuration, bool reduce) {
        m_audioDuration[pluginId].store(audioDuration);

        if (reduce) {
            if ((pluginId == audioDecoder_HASH64 || pluginId == audioRender_HASH64) && m_canDemuxerDeliverAudioToOutput()) {
                m_eventBus->inform(EVENT_DEMUXER_ON_OUTPUT_UPDATED_ID);
            }
        }
    });
}

int HJGraphMusicPlayer::openRecorder(HJKeyStorage::Ptr i_param)
{
    GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
    if (mediaUrl == nullptr) {
        return HJErrInvalidParams;
    }

    return SYNC_PROD_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_inRecording) {
            return HJErrAlreadyExist;
        }

        int ret;
        do {
            IF_FALSE_BREAK(m_muxer = HJPluginFFMuxer::Create<HJPluginFFMuxer>(ffMuxer_STRING, ffMuxer_HASH64), HJErrFatal);
            addPlugin(m_muxer);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_muxer, HJMEDIA_TYPE_AV), ret);

            auto param = std::make_shared<HJKeyStorage>();
            (*param)["mediaUrl"] = mediaUrl;
            auto mediaInfo = m_demuxer->getMediaInfo();
            if (!mediaInfo) {
                return HJErrNotInited;
            }

            auto audioInfo = mediaInfo->getAudioInfo();
            if (audioInfo != nullptr) {
                (*param)["audioInfo"] = audioInfo;
            }
            auto videoInfo = mediaInfo->getVideoInfo();
            if (videoInfo != nullptr) {
                (*param)["videoInfo"] = videoInfo;
            }
            if (m_playerListener) {
                (*param)["pluginListener"] = m_playerListener;
            }
            IF_FAIL_BREAK(ret = m_muxer->init(param), ret);

            m_inRecording = true;
            return HJ_OK;
        } while (false);

        return ret;
        });
}

void HJGraphMusicPlayer::closeRecorder()
{
    SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS();

        if (m_muxer != nullptr) {
            removePlugin(m_muxer);
            m_muxer = nullptr;
            m_inRecording = false;
        }
        });
}

NS_HJ_END
