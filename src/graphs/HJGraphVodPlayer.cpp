#include "HJGraphVodPlayer.h"
#include "HJStatContext.h"
#include "HJFLog.h"
#if defined (HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

HJ_NAME_DECLARE(demuxer)
HJ_NAME_DECLARE(audioDecoder)
HJ_NAME_DECLARE(audioResampler)
HJ_NAME_DECLARE(audioRender)
HJ_NAME_DECLARE(videoDecoder)
HJ_NAME_DECLARE(videoRender)

HJGraphVodPlayer::HJGraphVodPlayer(const std::string& i_name, size_t i_identify)
    : HJGraphPlayer(i_name, i_identify)
{
    m_cacheObserver = HJCreates<HJCacheObserver>();
    m_cacheObserver->setName(i_name);
}

int HJGraphVodPlayer::internalInit(HJKeyStorage::Ptr i_param)
{
    MUST_HAVE_PARAMETERS;
    int ret = HJGraph::internalInit(i_param);
    if (ret < 0) {
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
        m_canDemuxerDeliverVideoToOutput = [this]() {
            return (m_videoFrames[videoDecoder_HASH64].load() + m_videoFrames[videoRender_HASH64].load()) <= 30;
        };
        m_canVideoDecoderDeliverToOutput = [this]() {
            return m_videoFrames[videoRender_HASH64].load() < 2;
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
//        IF_FALSE_BREAK(audioInfo, HJErrInvalidParams);
        GET_PARAMETER(HJListener, playerListener);
        m_playerListener = playerListener;
        GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
        GET_PARAMETER(HJDeviceType, deviceType);
        GET_PARAMETER2(int64_t, prerollDurationMs, 120);
        GET_PARAMETER2(int64_t, pcmCallbackIntervalMs, 20);
#if defined (HarmonyOS)
        GET_PARAMETER(HJOGRenderWindowBridge::Ptr, mainBridge);
        IF_FALSE_BREAK(mainBridge, HJErrInvalidParams);
#elif defined (WINDOWS)
        GET_PARAMETER(std::string, audioDeviceName);
#endif
        GET_PARAMETER2(int, repeats, 1);
        m_repeats = repeats;

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
//        m_pluginStatuses[DEMUXER].store(HJSTATUS_NONE);
        IF_FALSE_BREAK(m_videoRender = HJPluginVideoRender::Create<HJPluginVideoRender>(videoRender_STRING, videoRender_HASH64, graphInfo), HJErrFatal);
        addPlugin(m_videoRender);
//        m_videoFrames[VIDEORENDER] = 0;
//        m_pluginStatuses[VIDEORENDER].store(HJSTATUS_NONE);
        m_videoFrames[videoRender_HASH64].store(0);
        if (deviceType == HJDEVICE_TYPE_NONE) {
            IF_FALSE_BREAK(m_videoSFDecoder = HJPluginVideoFFDecoder::Create<HJPluginVideoFFDecoder>(videoDecoder_STRING, videoDecoder_HASH64, graphInfo), HJErrFatal);
            addPlugin(m_videoSFDecoder);
            HJPlugin::Wtr wPlugin = m_videoSFDecoder;
            m_eventBus->subscribe(EVENT_VIDEO_DECODER_ON_OUTPUT_UPDATED_ID, [wPlugin]() -> HJRet {
                auto plugin = wPlugin.lock();
                if (!plugin) {
                    return HJErrAlreadyDone;
                }
                plugin->onOutputUpdated();
                return HJ_OK;
            });
//            m_videoFrames[VIDEODECODER] = 0;
//            m_pluginStatuses[VIDEODECODER].store(HJSTATUS_NONE);
            m_videoFrames[videoDecoder_HASH64].store(0);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_videoSFDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoSFDecoder, m_videoRender, HJMEDIA_TYPE_VIDEO), ret);
        }
        else if (deviceType == HJDEVICE_TYPE_OHCODEC) {
#if defined (HarmonyOS)
            IF_FALSE_BREAK(m_videoHWDecoder = HJPluginVideoOHDecoder::Create<HJPluginVideoOHDecoder>(videoDecoder_STRING, videoDecoder_HASH64, graphInfo), HJErrFatal);
            addPlugin(m_videoHWDecoder);
            HJPlugin::Wtr wPlugin = m_videoHWDecoder;
            m_eventBus->subscribe(EVENT_VIDEO_DECODER_ON_OUTPUT_UPDATED_ID, [wPlugin]() -> HJRet {
                auto plugin = wPlugin.lock();
                if (!plugin) {
                    return HJErrAlreadyDone;
                }
                plugin->onOutputUpdated();
                return HJ_OK;
            });
//            m_videoFrames[VIDEODECODER] = 0;
//            m_pluginStatuses[VIDEODECODER].store(HJSTATUS_NONE);
            m_videoFrames[videoDecoder_HASH64].store(0);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_videoHWDecoder, HJMEDIA_TYPE_VIDEO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoHWDecoder, m_videoRender, HJMEDIA_TYPE_VIDEO), ret);
#endif
        }
        IF_FALSE_BREAK(m_renderThread = HJLooperThread::quickStart("renderThread" + HJFMT("_{}", insIdx)), HJErrFatal);
        addThread(m_renderThread);
        if (audioInfo != nullptr) {
            m_audioInfo = audioInfo;
            IF_FALSE_BREAK(m_audioDecoder = HJPluginAudioFFDecoder::Create<HJPluginAudioFFDecoder>(audioDecoder_STRING, audioDecoder_HASH64, graphInfo), HJErrFatal);
            addPlugin(m_audioDecoder);
//            m_audioDuration[AUDIODECODER] = 0;
//            m_pluginStatuses[AUDIODECODER].store(HJSTATUS_NONE);
            m_audioDuration[audioDecoder_HASH64].store(0);
            IF_FAIL_BREAK(ret = connectPlugins(m_demuxer, m_audioDecoder, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_audioResampler = HJPluginAudioResampler::Create<HJPluginAudioResampler>(audioResampler_STRING, audioResampler_HASH64, graphInfo), HJErrFatal);
            addPlugin(m_audioResampler);
//            m_pluginStatuses["audioResampler"].store(HJSTATUS_NONE);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioDecoder, m_audioResampler, HJMEDIA_TYPE_AUDIO), ret);
#if defined (HarmonyOS)
            IF_FALSE_BREAK(m_audioRender = HJPluginAudioOHRender::Create<HJPluginAudioOHRender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
#elif defined (WINDOWS)
//            IF_FALSE_BREAK(m_audioRender = HJPluginAudioWORender::Create<HJPluginAudioWORender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
            IF_FALSE_BREAK(m_audioRender = HJPluginAudioWASRender::Create<HJPluginAudioWASRender>(audioRender_STRING, audioRender_HASH64, graphInfo), HJErrFatal);
#endif
            addPlugin(m_audioRender);
//            m_audioDuration[AUDIORENDER] = 0;
//            m_pluginStatuses[AUDIORENDER].store(HJSTATUS_NONE);
            m_audioDuration[audioRender_HASH64].store(0);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioResampler, m_audioRender, HJMEDIA_TYPE_AUDIO), ret);
            IF_FALSE_BREAK(m_audioThread = HJLooperThread::quickStart("audioThread" + HJFMT("_{}", insIdx)), HJErrFatal);
            addThread(m_audioThread);
        }

        m_cacheObserver->setWatchStartTime();

        IF_FALSE_BREAK(m_timeline = HJTimeline::Create<HJTimeline>("HJTimeline" + HJFMT("_{}", insIdx)), HJErrFatal);
        IF_FAIL_BREAK(ret = m_timeline->init(nullptr), ret);
/*
        m_canVideoDecoderDeliverToOutput = [this]() {
            return m_videoFrames[VIDEORENDER].getData() < 2;
        };
*/
        auto param = std::make_shared<HJKeyStorage>();
        (*param)["HJStatContext"] = statCtx;
        (*param)["timeline"] = m_timeline;
        (*param)["thread"] = m_renderThread;
//        (*param)["pluginListener"] = pluginListener;
        (*param)["deviceType"] = deviceType;
        if (deviceType == HJDEVICE_TYPE_NONE) {
#if defined (HarmonyOS)
            (*param)["bridge"] = mainBridge;
#elif defined (WINDOWS)
            IF_FALSE_BREAK(m_sharedMemoryProducer = HJSharedMemoryProducer::Create<HJSharedMemoryProducer>("HJSharedMemoryProducer" + HJFMT("_{}", insIdx)), HJErrFatal);
            m_sharedMemoryProducer->init(1920, 1080, 30);
            (*param)["sharedMemoryProducer"] = m_sharedMemoryProducer;
#endif
            IF_FAIL_BREAK(ret = m_videoRender->init(param), ret);
/*
            HJPluginVideoFFDecoder::Wtr wVideoDecoder = m_videoSFDecoder;
            HJConditionTrigger::Callback callback = [wVideoDecoder]() {
                auto videoDecoder = wVideoDecoder.lock();
                if (!videoDecoder) {
                    return HJErrAlreadyDone;
                }

                videoDecoder->onOutputUpdated();
                return HJ_OK;
            };
            addConditionTrigger(m_canVideoDecoderDeliverToOutput, callback,
                &m_videoFrames[VIDEORENDER]);
*/
            param = std::make_shared<HJKeyStorage>();
//            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_videoSFDecoder->init(param), ret);
        }
        else if (deviceType == HJDEVICE_TYPE_OHCODEC) {
            IF_FAIL_BREAK(ret = m_videoRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
//            (*param)["pluginListener"] = pluginListener;
#if defined (HarmonyOS)
/*
            HJPluginVideoOHDecoder::Wtr wVideoDecoder = m_videoHWDecoder;
            HJConditionTrigger::Callback callback = [wVideoDecoder]() {
                auto videoDecoder = wVideoDecoder.lock();
                if (!videoDecoder) {
                    return HJErrAlreadyDone;
                }

                videoDecoder->onOutputUpdated();
                return HJ_OK;
            };
            addConditionTrigger(m_canVideoDecoderDeliverToOutput, callback,
                &m_videoFrames[VIDEORENDER]);
*/
            (*param)["bridge"] = mainBridge;
            IF_FAIL_BREAK(ret = m_videoHWDecoder->init(param), ret);
#endif
        }

        if (audioInfo != nullptr) {
            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
            (*param)["timeline"] = m_timeline;
            (*param)["prerollDurationMs"] = prerollDurationMs;
            (*param)["pcmCallbackIntervalMs"] = pcmCallbackIntervalMs;
#if defined (WINDOWS)
            if (!audioDeviceName.empty()) {
                (*param)["audioDeviceName"] = audioDeviceName;
            }
#else
            (*param)["thread"] = m_renderThread;
#endif
//            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_audioRender->init(param), ret);

            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
            (*param)["thread"] = m_audioThread;
//            (*param)["pluginListener"] = pluginListener;
            IF_FAIL_BREAK(ret = m_audioResampler->init(param), ret);

            param->erase("audioInfo");
            IF_FAIL_BREAK(ret = m_audioDecoder->init(param), ret);
        }
/*
        m_canDemuxerDeliverAudioToOutput = [this]() {
            return (m_audioDuration[AUDIODECODER].getData() + m_audioDuration[AUDIORENDER].getData()) <= 1600;
        };
        m_canDemuxerDeliverVideoToOutput = [this]() {
            return (m_videoFrames[VIDEODECODER].getData() + m_videoFrames[VIDEORENDER].getData()) <= 30;
        };
        HJPluginFFDemuxer::Wtr wDemuxer = m_demuxer;
        HJConditionTrigger::Callback callback = [wDemuxer]() {
            auto demuxer = wDemuxer.lock();
            if (!demuxer) {
                return HJErrAlreadyDone;
            }

            demuxer->onOutputUpdated();
            return HJ_OK;
        };
        addConditionTrigger(m_canDemuxerDeliverAudioToOutput, callback,
            &m_audioDuration[AUDIODECODER], &m_audioDuration[AUDIORENDER]);
        addConditionTrigger(m_canDemuxerDeliverVideoToOutput, callback,
            &m_videoFrames[VIDEODECODER], &m_videoFrames[VIDEORENDER]);
*/
        param = std::make_shared<HJKeyStorage>();
//        (*param)["pluginListener"] = pluginListener;
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
    m_paused.store(false);

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

    if (m_thread) {
        m_thread->done();
        m_thread = nullptr;
    }
    m_handler = nullptr;
}
/*
int HJGraphVodPlayer::setInfo(const Information i_info)
{
    if (!i_info) {
        return HJErrInvalidParams;
    }

    int ret = HJ_OK;
    switch (i_info->getID()) {
    case HJ_PLUGIN_SETINFO_DEMUXER_mediaType:
    {
        HJMediaType mediaType = i_info->getValue<HJMediaType>("mediaType");
        m_mediaType.store(mediaType);
        break;
    }
    case HJ_PLUGIN_SETINFO_PLUGIN_status:
    {
        m_pluginStatuses[i_info->getName()].store(static_cast<HJStatus>(i_info->getVal()));
        break;
    }
    case HJ_PLUGIN_SETINFO_PLUGIN_audioDuration:
    {
        m_audioDuration[i_info->getName()] = i_info->getVal();
        break;
    }
    case HJ_PLUGIN_SETINFO_PLUGIN_videoFrames:
    {
        if (i_info->getName() == VIDEORENDER) {
            HJFLogi("{}, HJ_PLUGIN_SETINFO_PLUGIN_videoFrames(VIDEORENDER) : frames({})", getName(), static_cast<size_t>(i_info->getVal()));
        }
        m_videoFrames[i_info->getName()] = static_cast<size_t>(i_info->getVal());
        break;
    }
    case HJ_PLUGIN_SETINFO_PLUGIN_videoKeyFrames:
    {
        m_videoKeyFrames[i_info->getName()].store(i_info->getVal());
        break;
    }
    default:
        ret = HJErrInvalidParams;
        break;
    }

    return ret;
}

int HJGraphVodPlayer::getInfo(Information io_info)
{
    if (!io_info) {
        return HJErrInvalidParams;
    }

    int ret{ HJ_OK };
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

    case HJ_PLUGIN_GETINFO_hasAudio:
    {
        (*io_info)["hasAudio"] = hasAudio();
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
    case HJ_PLUGIN_NOTIFY_DEMUXER_EOF:
    {
        m_currentRepeatNumber++;
        if (m_repeats == 0 || m_currentRepeatNumber < m_repeats) {
            SYNC_CONS_LOCK([this] {
                CHECK_DONE_STATUS();
                if (m_demuxer) {
                    m_demuxer->reset(0);
                }
            });
        }
        else if (m_currentRepeatNumber == m_repeats) {
            m_lastStreamIndex = ntf->getValue<int>("streamIndex");
        }
        return HJ_OK;
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
//        break;
        return HJ_WOULD_BLOCK;
    }
    case HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF:
    {
        m_videoStreamIndex = ntf->getValue<int>("streamIndex");
        // TODO：如果多段视频，各段之间的type不同，此处获取的type可能不是对应段的
        auto type = m_mediaType.load();
        if (!(type & HJMEDIA_TYPE_AUDIO) ||
            m_videoStreamIndex <= m_audioStreamIndex) {
            HJFLogi("{}, eof(video))", getName());
            if (m_videoStreamIndex == m_lastStreamIndex) {
                ntf->setID(HJ_PLUGIN_NOTIFY_EOF);
            }
        }
        break;
    }
    case HJ_PLUGIN_NOTIFY_AUDIORENDER_EOF:
    {
        m_audioStreamIndex = ntf->getValue<int>("streamIndex");
        // TODO：同上
        auto type = m_mediaType.load();
        if (!(type & HJMEDIA_TYPE_VIDEO) ||
            m_audioStreamIndex <= m_videoStreamIndex) {
            HJFLogi("{}, eof(audio))", getName());
            if (m_audioStreamIndex == m_lastStreamIndex) {
                ntf->setID(HJ_PLUGIN_NOTIFY_EOF);
            }
        }
        break;
    }
    default:
        break;
    }

    return m_playerListener ? m_playerListener(ntf) : HJ_OK;
}
*/
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
/*
bool HJGraphVodPlayer::hasAudio()
{
    if (!(m_mediaType.load() & HJMEDIA_TYPE_AUDIO)) {
        return false;
    }

    if (m_audioDuration[AUDIODECODER].getData() > 0) {
        return true;
    }
    if (m_audioDuration[AUDIORENDER].getData() > 0) {
        return true;
    }

    return false;
}
*/
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

int HJGraphVodPlayer::setVolume(float i_volume)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->setVolume(i_volume);
    });
}

float HJGraphVodPlayer::getVolume()
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(1.0f);

        if (!m_audioRender) {
            return 1.0f;
        }

        return m_audioRender->getVolume();
    });
}


int HJGraphVodPlayer::pause()
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

        m_audioRender->setPause(true);

        return HJ_OK;
    });
}

int HJGraphVodPlayer::resume()
{
    return SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (!m_paused.load()) {
            return HJ_OK;
        }

        m_paused.store(false);

        m_audioRender->setPause(false);

        if (m_timeline) {
            m_timeline->play();
        }

        return HJ_OK;
    });
}

#if defined (WINDOWS)
int HJGraphVodPlayer::resetAudioDevice(const std::string& i_deviceName)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (!m_audioRender) {
            return HJErrNotInited;
        }

        return m_audioRender->resetDevice(i_deviceName);
        return HJ_OK;
    });
}
#endif

int64_t HJGraphVodPlayer::getCurrentTimestamp()
{
    int64_t ts{};
    SYNC_CONS_LOCK([&ts, this] {
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

    auto maxTs = m_maxTimestamp.load();
    if (maxTs > 0) {
        ts = std::min<int64_t>(ts, maxTs);
    }
    return ts;
}

int64_t HJGraphVodPlayer::getDuration()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(int64_t{ 0 });
        if (!m_demuxer) {
            return int64_t{ 0 };
        }
        return m_demuxer->getDuration();
    });
}

int HJGraphVodPlayer::seek(int64_t i_timestamp)
{
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

int HJGraphVodPlayer::switchAudioTrack(int i_trackId)
{
    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_demuxer == nullptr) {
            return HJErrFatal;
        }
        return m_demuxer->switchAudioTrack(i_trackId);
    });
}
/*
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
        if (!m_canDemuxerDeliverAudioToOutput()) {
            return HJ_WOULD_BLOCK;
        }
    }
    else if (mediaFrame->isVideo()) {
        if (!m_canDemuxerDeliverVideoToOutput()) {
            return HJ_WOULD_BLOCK;
        }
    }

    return HJ_OK;
}

int HJGraphVodPlayer::canVideoDecoderDeliverToOutputs(Information io_info)
{
    if (!m_canVideoDecoderDeliverToOutput()) {
        return HJ_WOULD_BLOCK;
    }

    return HJ_OK;
}
*/
int HJGraphVodPlayer::registerHandlers()
{
    using RegFn = HJRet(HJGraphVodPlayer::*)();
    static const RegFn regs[] = {
        & HJGraphVodPlayer::registerQueryHandler_hasAudio,
        & HJGraphVodPlayer::registerQueryHandler_canDeliverToOutputs,
        & HJGraphVodPlayer::registerQueryHandler_canPluginEof,
        & HJGraphVodPlayer::registerEventHandler_pluginNotify,
        & HJGraphVodPlayer::registerEventHandler_mediaType,
        & HJGraphVodPlayer::registerEventHandler_statusUpdated,
        & HJGraphVodPlayer::registerEventHandler_audioDuration,
        & HJGraphVodPlayer::registerEventHandler_videoFrames,
        & HJGraphVodPlayer::registerEventHandler_videoKeyFrames,
    };

    for (auto fn : regs) {
        HJRet ret = (this->*fn)();
        if (ret != HJ_OK) {
            return ret;
        }
    }
    return HJ_OK;
}

int HJGraphVodPlayer::registerQueryHandler_hasAudio()
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

int HJGraphVodPlayer::registerQueryHandler_canDeliverToOutputs()
{
    return m_queryBus->registerHandler(QUERY_CAN_DELIVER_TO_OUTPUTS_ID, [this](size_t pluginId, const HJMediaFrame::Ptr& frame) -> bool {
        if (pluginId == demuxer_HASH64) {
            if (frame->isAudio()) {
                return m_canDemuxerDeliverAudioToOutput();
            }
            if (frame->isVideo()) {
                return m_canDemuxerDeliverVideoToOutput();
            }
            return true;
        }
        if (pluginId == videoDecoder_HASH64) {
            return m_canVideoDecoderDeliverToOutput();
        }
        return false;
    });
}

int HJGraphVodPlayer::registerQueryHandler_canPluginEof()
{
    return m_queryBus->registerHandler(QUERY_CAN_PLUGIN_EOF_ID, [this](size_t pluginId, int streamIndex) -> bool {
        if (pluginId == demuxer_HASH64) {
            m_currentRepeatNumber++;
            if (m_repeats == 0 || m_currentRepeatNumber < m_repeats) {
                SYNC_CONS_LOCK([this] {
                    CHECK_DONE_STATUS();
                    if (m_demuxer) {
                        m_demuxer->reset(0);
                    }
                });
            }
            else if (m_currentRepeatNumber == m_repeats) {
                m_lastStreamIndex = streamIndex;
            }

            return true;
        }
        else if (pluginId == videoRender_HASH64) {
            m_videoStreamIndex = streamIndex;
            if (m_videoStreamIndex == m_lastStreamIndex) {
                auto type = m_mediaType.fetch_and(~HJMEDIA_TYPE_VIDEO);
                if (type == HJMEDIA_TYPE_VIDEO) {
                    HJFLogi("{}, eof(video))", getName());
                    
                    if (m_timeline) {
                        int64_t streamIndex;
                        int64_t timestamp;
                        double speed;
                        if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
                            m_maxTimestamp.store(timestamp);
                        }
                    }

                    m_eventBus->report(EVENT_GRAPH_EOF_ID);
                }

                return true;
            }
        }
        else if (pluginId == audioRender_HASH64) {
            m_audioStreamIndex = streamIndex;
            if (m_audioStreamIndex == m_lastStreamIndex) {
                auto type = m_mediaType.fetch_and(~HJMEDIA_TYPE_AUDIO);
                if (type == HJMEDIA_TYPE_AUDIO) {
                    HJFLogi("{}, eof(audio))", getName());
                    
                    if (m_timeline) {
                        int64_t streamIndex;
                        int64_t timestamp;
                        double speed;
                        if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
                            m_maxTimestamp.store(timestamp);
                        }
                    }

                    m_eventBus->report(EVENT_GRAPH_EOF_ID);
                }

                return true;
            }
        }

        return false;
    });
}

int HJGraphVodPlayer::registerEventHandler_pluginNotify()
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
        case HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME:
            break;
        default:
            break;
        }
    });
}

int HJGraphVodPlayer::registerEventHandler_statusUpdated()
{
    return m_eventBus->registerHandler(EVENT_STATUS_UPDATED_ID, [this](size_t pluginId) {

    });
}

int HJGraphVodPlayer::registerEventHandler_audioDuration()
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

int HJGraphVodPlayer::registerEventHandler_videoFrames()
{
    return m_eventBus->registerHandler(EVENT_VIDEO_FRAMES_ID, [this](size_t pluginId, int64_t videoFrames, bool reduce) {
        m_videoFrames[pluginId].store(videoFrames);

        if (reduce) {
            if (pluginId == videoRender_HASH64 && m_canVideoDecoderDeliverToOutput()) {
                m_eventBus->inform(EVENT_VIDEO_DECODER_ON_OUTPUT_UPDATED_ID);
            }

            if ((pluginId == videoDecoder_HASH64 || pluginId == videoRender_HASH64) && m_canDemuxerDeliverVideoToOutput()) {
                m_eventBus->inform(EVENT_DEMUXER_ON_OUTPUT_UPDATED_ID);
            }
        }
    });
}

int HJGraphVodPlayer::registerEventHandler_videoKeyFrames()
{
    return m_eventBus->registerHandler(EVENT_VIDEO_KEY_FRAMES_ID, [this](size_t pluginId, int64_t videoKeyFrames, bool reduce) {
        m_videoKeyFrames[pluginId].store(videoKeyFrames);
    });
}

// HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_SEEK
// HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT
// HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME
// EVENT_MEDIA_TYPE_ID
// QUERY_CAN_PLUGIN_EOF_ID
// EVENT_GRAPH_DURATION_ID
// EVENT_GRAPH_SEI_INFOS_ID

// HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA
// HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT
// HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN
// HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME

// HJ_PLUGIN_NOTIFY_ERROR_AUDIOCONVERTER_CONVERT
// HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME

// HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING
// HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING
// HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING
// HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING
// HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT
// HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK
// QUERY_CAN_PLUGIN_EOF_ID
// EVENT_GRAPH_AUDIO_FRAME_ID

// HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME
// QUERY_CAN_PLUGIN_EOF_ID
// EVENT_GRAPH_VIDEO_FRAME_ID
// EVENT_GRAPH_FIRST_FRAME_RENDERED_ID

NS_HJ_END
