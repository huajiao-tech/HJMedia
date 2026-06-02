#include "HJPluginAudioRender.h"
#include "HJGraph.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

#include <algorithm>
#include <climits>

NS_HJ_BEGIN

int HJPluginAudioRender::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	MUST_GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	MUST_GET_PARAMETER(HJTimeline::Ptr, timeline);
	GET_PARAMETER(HJLooperThread::Ptr, thread);
    GET_PARAMETER2(int64_t, prerollDurationMs, 120);
    GET_PARAMETER2(int64_t, pcmCallbackIntervalMs, 20);
#if defined (WINDOWS)
	GET_PARAMETER(std::string, audioDeviceName);
#endif

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPlugin::internalInit(param);
	if (ret != HJ_OK) {
		return ret;
	}

	Wtr wRender = SHARED_FROM_THIS;
	if (!m_handler->async([wRender, audioInfo] {
		auto render = wRender.lock();
		if (render) {
			render->runInit(audioInfo);
		}
	})) {
		return HJErrFatal;
	}

    m_runOpMsgId = m_handler->genMsgId();
    m_reinitMsgId = m_handler->genMsgId();

    m_audioInfo = audioInfo;
    m_timeline = timeline;
    if (audioInfo != nullptr) {
        int bytesPerSample = audioInfo->m_bytesPerSample;
        if (bytesPerSample <= 0) {
            bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(audioInfo->m_sampleFmt));
            if (bytesPerSample <= 0) {
                bytesPerSample = 2;
            }
        }
        const size_t bytesPerFrame = std::max<size_t>(1, audioInfo->m_channels * bytesPerSample);
        const size_t sampleRate = std::max<size_t>(1, audioInfo->m_samplesRate);
        m_pcmCallbackIntervalMs = std::max<int64_t>(1, pcmCallbackIntervalMs);
        const size_t callbackFrames = std::max<size_t>(1, static_cast<size_t>((sampleRate * static_cast<size_t>(m_pcmCallbackIntervalMs)) / 1000));
        // Keep enough submitted PCM to bridge render callback jitter and the configured polling timer.
        const size_t framesPerCallback = audioInfo->m_samplesPerFrame > 0
            ? static_cast<size_t>(audioInfo->m_samplesPerFrame)
            : callbackFrames;
        const size_t timerFrames = callbackFrames;
        const size_t reserveFrames = std::max<size_t>(sampleRate / 2,
                                                       std::max<size_t>(framesPerCallback * 8, timerFrames * 8));
        m_pcmCallbackCapacityBytes = std::max<size_t>(bytesPerFrame, reserveFrames * bytesPerFrame);
    } else {
        m_pcmCallbackCapacityBytes = 0;
        m_pcmCallbackIntervalMs = std::max<int64_t>(1, pcmCallbackIntervalMs);
    }
    m_prerollDurationMs = std::max<int64_t>(0, prerollDurationMs);
    m_prerolling = true;
    clearPCMCallbackData(0);
#if defined (WINDOWS)
	m_audioDeviceName = audioDeviceName;
#endif

    if (m_pcmCallback && m_handler) {
        m_handler->openTimer([wRender](uint64_t now, uint64_t next) {
            auto render = wRender.lock();
            if (render) {
                render->runPCMCallback(now, next);
            }
        }, 1000, m_pcmCallbackIntervalMs);
    }

	return HJ_OK;
}

void HJPluginAudioRender::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

    m_handlerSync.consLock([this] {
        if (m_handler != nullptr) {
            m_handler->sync([this] {
                releaseRender();
                return HJ_OK;
            });
        }
    });

	HJPlugin::internalRelease();

//	releaseRender();

	if (m_inited.load()) {
    // Note: Audio render uses its own thread. Stop the thread before releasing resources
    // to keep HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING and HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING ordered.
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING, getID());
		m_inited.store(false);
	}

	m_timeline = nullptr;
	m_audioInfo = nullptr;
    m_prerolling = true;
    clearPCMCallbackData(0);

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPluginAudioRender::setMute(bool i_mute)
{
	return SYNC_PROD_LOCK([this, i_mute] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		m_muted.store(i_mute);

		return HJ_OK;
	});
}

bool HJPluginAudioRender::isMuted()
{
	return SYNC_CONS_LOCK([this] {
		CHECK_DONE_STATUS(false);

		return m_muted.load();
	});
}

int HJPluginAudioRender::setVolume(float i_volume)
{
    return SYNC_PROD_LOCK([this, i_volume] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        const float volume = std::max<float>(0.0f, std::min<float>(1.0f, i_volume));
        m_volume.store(volume);

        return HJ_OK;
    });
}

float HJPluginAudioRender::getVolume()
{
    return SYNC_CONS_LOCK([this] {
        CHECK_DONE_STATUS(1.0f);

        return m_volume.load();
    });
}

void HJPluginAudioRender::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	HJPlugin::onInputAdded(i_srcKeyHash, i_type);

	m_inputKeyHash.store(i_srcKeyHash);

	auto input = m_inputs[i_srcKeyHash];
	input->eventFlags = EVENT_FLAG_AUDIO_DURATION;
}

int HJPluginAudioRender::runFlush()
{
    return SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        if (m_timeline != nullptr) {
            m_timeline->flush();
        }
//        discardPendingPCMCallbackData();
//		HJFLogi("{}, m_timeline->flush()", getName());
        return HJ_OK;
    });
}

int HJPluginAudioRender::runInit(const HJAudioInfo::Ptr& i_audioInfo)
{
	auto ret = SYNC_PROD_LOCK([this, i_audioInfo] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		auto ret = initRender(i_audioInfo);
        if (ret == HJ_OK) {
            m_audioInfo->m_blockAlign = m_blockAlign;
        }
        return ret;
	});
	if (ret == HJErrAlreadyDone) {
		return HJErrAlreadyDone;
	}
	else if (ret < 0) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT, getID());
	}
	else if (ret == HJ_OK) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Ready), HJErrAlreadyDone);
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING, getID());
		m_inited.store(true);
	}

    return ret;
}

int HJPluginAudioRender::onPlaybackCompleted()
{
    bool shouldPost = false;
    auto ret = SYNC_PROD_LOCK([this, &shouldPost] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_running) {
            m_running = false;
            shouldPost = true;
        }
        discardPendingPCMCallbackData();
        return HJ_OK;
    });
    if (ret == HJ_OK && shouldPost) {
        postRunOp(RunOp::StopEof);
    }
    return ret;
}

void HJPluginAudioRender::postReinit()
{
    HJLooperThread::Handler::Ptr handler{};
    int reinitMsgId{};
    const int ret = SYNC_CONS_LOCK([this, &handler, &reinitMsgId] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        handler = m_handler;
        reinitMsgId = m_reinitMsgId;
        return HJ_OK;
    });
    if (ret != HJ_OK || handler == nullptr || reinitMsgId == 0) {
        return;
    }

    Wtr wRender = SHARED_FROM_THIS;
    handler->asyncAndClear([wRender] {
        auto render = wRender.lock();
        if (render) {
            render->runReinit();
        }
    }, reinitMsgId);
}

int HJPluginAudioRender::runReinit()
{
#if 0
    HJAudioInfo::Ptr audioInfo{};
    int ret = SYNC_PROD_LOCK([this, &audioInfo] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        audioInfo = m_audioInfo;
        if (!audioInfo) {
            return HJErrInvalidParams;
        }
        return HJ_OK;
    });
    if (ret == HJErrAlreadyDone) {
        return HJErrAlreadyDone;
    }
    if (ret < 0) {
        IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT, getID());
        return ret;
    }

    // Do not hold the plugin state lock across AudioUnit teardown/startup.
    releaseRender();
    clearPCMCallbackData(0);

    ret = initRender(audioInfo);
    if (ret == HJ_OK) {
        bool shouldRestart = false;
        ret = SYNC_PROD_LOCK([this, &shouldRestart] {
            CHECK_DONE_STATUS(HJErrAlreadyDone);
            shouldRestart = (!m_paused && m_running);
            return HJ_OK;
        });
        if (ret == HJ_OK && shouldRestart) {
            ret = setStreamRunning(true);
            if (ret != HJ_OK && ret != HJErrAlreadyDone) {
                SYNC_PROD_LOCK([this] {
                    CHECK_DONE_STATUS();
                    m_running = false;
                });
            }
        }
    }
#else
//    IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
    releaseRender();
    clearPCMCallbackData(0);

    bool shouldRestart{};
    int ret = SYNC_PROD_LOCK([this, &shouldRestart] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        auto audioInfo = m_audioInfo;
        if (!audioInfo) {
            return HJErrInvalidParams;
        }

        shouldRestart = (!m_paused && m_running);

        auto err = initRender(audioInfo);
        if (err != HJ_OK) {
            return err;
        }

        return HJ_OK;
    });
    if (ret == HJ_OK && shouldRestart) {
        ret = setStreamRunning(true);
        if (ret != HJ_OK) {
            SYNC_PROD_LOCK([this] {
                CHECK_DONE_STATUS();
                m_running = false;
            });
        }
    }
#endif

    if (ret == HJErrAlreadyDone) {
        return HJErrAlreadyDone;
    }
    if (ret < 0) {
        IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT, getID());
        return ret;
    }
    else if (ret == HJ_OK) {
        IF_FALSE_RETURN(setStatus(HJSTATUS_Ready), HJErrAlreadyDone);
    }

    return ret;
}

void HJPluginAudioRender::onInputUpdated()
{
    bool shouldPost = false;
    SYNC_PROD_LOCK([this, &shouldPost] {
        CHECK_DONE_STATUS();
        if (m_paused || m_running) {
            return;
        }
        m_running = true;
        shouldPost = true;
    });
    if (shouldPost) {
        postRunOp(RunOp::Start);
    }
}

void HJPluginAudioRender::onPauseStateChanged(bool i_pause)
{
    bool shouldPost = false;
    RunOp op = RunOp::StopPause;
    bool hasPendingFrames = false;
    if (!i_pause) {
        hasPendingFrames = getQueuedSize() > 0;
    }
    SYNC_PROD_LOCK([this, i_pause, hasPendingFrames, &shouldPost, &op] {
        CHECK_DONE_STATUS();
        if (i_pause) {
            if (m_running) {
                m_running = false;
                shouldPost = true;
                op = RunOp::StopPause;
            }
            return;
        }

        if (!m_running && hasPendingFrames) {
            m_running = true;
            shouldPost = true;
            op = RunOp::Start;
        }
    });
    if (shouldPost) {
        postRunOp(op);
    }
}

void HJPluginAudioRender::postRunOp(RunOp op)
{
    HJLooperThread::Handler::Ptr handler{};
    int runOpMsgId{};
    const int ret = SYNC_CONS_LOCK([this, &handler, &runOpMsgId] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        handler = m_handler;
        runOpMsgId = m_runOpMsgId;
        return HJ_OK;
    });
    if (ret != HJ_OK || handler == nullptr || runOpMsgId == 0) {
        return;
    }

    Wtr wRender = SHARED_FROM_THIS;
    handler->asyncAndClear([wRender, op] {
        auto render = wRender.lock();
        if (render) {
            render->runRunOp(op);
        }
    }, runOpMsgId);
}

void HJPluginAudioRender::runRunOp(RunOp op)
{
    bool shouldApply = false;
    bool running = false;
    bool eofStop = false;
    int ret = SYNC_PROD_LOCK([this, op, &shouldApply, &running, &eofStop] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        switch (op) {
        case RunOp::Start:
            if (m_paused || !m_running) {
                return HJ_OK;
            }
            shouldApply = true;
            running = true;
            return HJ_OK;
        case RunOp::StopPause:
            if (!m_paused && m_running) {
                return HJ_OK;
            }
            shouldApply = true;
            running = false;
            eofStop = false;
            return HJ_OK;
        case RunOp::StopEof:
            if (m_running) {
                return HJ_OK;
            }
            shouldApply = true;
            running = false;
            eofStop = true;
            return HJ_OK;
        }

        return HJ_OK;
    });

    if (ret == HJ_OK && shouldApply) {
        // RemoteIO stop can wait for its callback thread; keep plugin locks out of that path.
        ret = setStreamRunning(running, eofStop);
        if (ret != HJ_OK && ret != HJErrAlreadyDone && running) {
            SYNC_PROD_LOCK([this] {
                CHECK_DONE_STATUS();
                m_running = false;
            });
        }
    }

    if (ret != HJ_OK && ret != HJErrAlreadyDone) {
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK, getID());
    }
}

void HJPluginAudioRender::runPCMCallback(uint64_t now, uint64_t next)
{
    (void)now;
    (void)next;
    HJAudioInfo::Ptr audioInfo{};
    int64_t renderTimestamp{};
    SYNC_CONS_LOCK([this, &audioInfo, &renderTimestamp] {
        CHECK_DONE_STATUS();

        audioInfo = m_audioInfo;
        renderTimestamp = getRenderTimestamp();
    });

    if (!m_inited.load() || !audioInfo || audioInfo->m_samplesRate <= 0 || audioInfo->m_blockAlign <= 0 || renderTimestamp <= 0) {
        return;
    }

    const uint64_t renderedFrames = static_cast<uint64_t>((renderTimestamp * audioInfo->m_samplesRate) / 1000);
    std::shared_ptr<std::vector<uint8_t>> pcmData = std::make_shared<std::vector<uint8_t>>();
    {
        std::lock_guard<std::mutex> lock(m_pcmCallbackMutex);
        if (renderedFrames < m_pcmLastRenderedFrames) {
            m_pcmCallbackReadPos = 0;
            m_pcmCallbackWritePos = 0;
            m_pcmCallbackBufferedBytes = 0;
            m_pcmWrittenFrames = renderedFrames;
            m_pcmDrainedFrames = renderedFrames;
        }
        m_pcmLastRenderedFrames = renderedFrames;

        if (renderedFrames <= m_pcmDrainedFrames || m_pcmCallbackBufferedBytes == 0 || m_pcmCallbackCache.empty()) {
            return;
        }

        uint64_t framesToDrain = std::min<uint64_t>(renderedFrames - m_pcmDrainedFrames,
                                                    m_pcmCallbackBufferedBytes / static_cast<size_t>(audioInfo->m_blockAlign));
        if (framesToDrain == 0) {
            return;
        }

        const size_t bytesToDrain = static_cast<size_t>(framesToDrain) * static_cast<size_t>(audioInfo->m_blockAlign);
        pcmData->resize(bytesToDrain);
        const size_t firstCopySize = std::min<size_t>(bytesToDrain, m_pcmCallbackCache.size() - m_pcmCallbackReadPos);
        memcpy(pcmData->data(), m_pcmCallbackCache.data() + m_pcmCallbackReadPos, firstCopySize);
        if (bytesToDrain > firstCopySize) {
            memcpy(pcmData->data() + firstCopySize, m_pcmCallbackCache.data(), bytesToDrain - firstCopySize);
        }
        m_pcmCallbackReadPos = (m_pcmCallbackReadPos + bytesToDrain) % m_pcmCallbackCache.size();
        m_pcmCallbackBufferedBytes -= bytesToDrain;
        m_pcmDrainedFrames += framesToDrain;
    }

    if (!pcmData->empty()) {
        report(EVENT_GRAPH_RENDERED_PCM_ID, HJGraphRenderedPCM{ audioInfo, pcmData });
    }
}

std::tuple<int, int32_t, HJMediaFrame::Ptr> HJPluginAudioRender::fillAudioBuffer(std::string& route, void* data, int32_t dataSize, int64_t& size)
{
    int ret{ HJ_OK };
    int32_t validSize = dataSize;
    HJMediaFrame::Ptr kernalFrame{};
    while (dataSize > 0) {
        route += "_30";
        if (kernalFrame == nullptr) {
            if (m_prerolling) {
                const int64_t queuedDurationMs = getQueuedAudioDurationMs();
                const bool hasControlFrame = hasQueuedControlFrame();
                if (queuedDurationMs < m_prerollDurationMs && !hasControlFrame) {
                    route += "_3P";
                    if (!m_buffering) {
                        route += "_33";
                        m_buffering = true;
                        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING, getID());
                    }

                    memset(data, 0, dataSize);
                    validSize -= dataSize;
                    break;
                }
                route += "_3Q";
                m_prerolling = false;
            }

            route += "_31";
            std::tie(ret, kernalFrame) = receiveInputFrame(route, m_inputKeyHash.load(), size);
            if (ret != HJ_OK) {
                break;
            }
            if (kernalFrame == nullptr) {
                route += "_32";
                m_prerolling = true;
                if (!m_buffering) {
                    route += "_33";
                    m_buffering = true;
                    report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING, getID());
                }

                memset(data, 0, dataSize);
                validSize -= dataSize;
                break;
            }

            if (kernalFrame->isClearFrame()) {
                ret = runFlush();
                validSize = 0;
                kernalFrame = nullptr;
                break;
            }

            if (kernalFrame->isEofFrame()) {
                route += "_38";
                auto res = query(QUERY_CAN_PLUGIN_EOF_ID, getID(), kernalFrame->m_streamIndex);
                if (!res.isOk()) {
                    ret = res.code;
                    break;
                }
                if (res.value) {
//                    IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
                    ret = onPlaybackCompleted();
                    if (ret != HJ_OK) {
                        break;
                    }

                    memset(data, 0, dataSize);
                    validSize -= dataSize;
                    kernalFrame = nullptr;
                    break;
                }

                kernalFrame = nullptr;
                continue;
            }

//            report(EVENT_GRAPH_AUDIO_FRAME_ID, kernalFrame);

            if (m_buffering) {
                route += "_3B";
                m_buffering = false;
                report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING, getID());
            }
        }

        HJAVFrame::Ptr avFrame = kernalFrame->getMFrame();
        if (avFrame == nullptr) {
            HJFLoge("{}, (avFrame == nullptr)", getName());
            kernalFrame = nullptr;
            continue;
        }
        AVFrame* frame = avFrame->getAVFrame();
        if (frame == nullptr || frame->data[0] == nullptr) {
            HJFLoge("{}, (frame == nullptr || frame->data[0] == nullptr)", getName());
            kernalFrame = nullptr;
            continue;
        }

        int32_t bufferSize = static_cast<int32_t>(frame->nb_samples * m_blockAlign);
        if (bufferSize <= 0 || bufferSize <= kernalFrame->m_bufferPos) {
            HJFLoge("{}, (bufferSize <= 0 || bufferSize <= kernalFrame->m_bufferPos)", getName());
            kernalFrame = nullptr;
            continue;
        }
        int32_t copySize = std::min<int32_t>(dataSize, bufferSize - kernalFrame->m_bufferPos);
        memcpy(data, frame->data[0] + kernalFrame->m_bufferPos, copySize);
        data = static_cast<int8_t*>(data) + copySize;
        dataSize -= copySize;
        kernalFrame->m_bufferPos += copySize;

        if (kernalFrame->m_bufferPos >= bufferSize) {
            route += "_3D";
            HJTimeline::Ptr timeline{};
            ret = SYNC_CONS_LOCK([this, &route, &timeline] {
                if (m_status == HJSTATUS_Done) {
                    route += "_3E";
                    return HJErrAlreadyDone;
                }

                timeline = m_timeline;
                return HJ_OK;
            });
            if (ret != HJ_OK) {
                break;
            }
            if (timeline != nullptr) {
                route += "_3E";
                timeline->setTimestamp(kernalFrame->m_streamIndex, kernalFrame->getPTS(), kernalFrame->getSpeed());
//                HJFLogi("{}, timeline->setTimestamp({}, {}, {})", getName(), kernalFrame->m_streamIndex, kernalFrame->getPTS(), kernalFrame->getSpeed());
            }

            kernalFrame = nullptr;
        }
    }

    return std::make_tuple(ret, validSize, kernalFrame);
}

int64_t HJPluginAudioRender::getQueuedAudioDurationMs() const
{
    auto input = const_cast<HJPluginAudioRender*>(this)->getInput(m_inputKeyHash.load());
    if (input == nullptr) {
        return 0;
    }
    return input->mediaFrames.audioDuration();
}

bool HJPluginAudioRender::hasQueuedControlFrame() const
{
    auto input = const_cast<HJPluginAudioRender*>(this)->getInput(m_inputKeyHash.load());
    if (input == nullptr) {
        return false;
    }
    return input->mediaFrames.hasControlFrame();
}

size_t HJPluginAudioRender::getQueuedSize() const
{
    auto input = const_cast<HJPluginAudioRender*>(this)->getInput(m_inputKeyHash.load());
    if (input == nullptr) {
        return 0;
    }
    return input->mediaFrames.size();
}

void HJPluginAudioRender::appendPCMCallbackData(const uint8_t* data, int32_t dataSize)
{
    if (!data || dataSize <= 0 || m_blockAlign <= 0 || m_pcmCallbackCapacityBytes == 0) {
        return;
    }

    size_t bytesToWrite = static_cast<size_t>(dataSize);
    bytesToWrite -= bytesToWrite % static_cast<size_t>(m_blockAlign);
    if (bytesToWrite == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_pcmCallbackMutex);
    if (m_pcmCallbackCache.size() != m_pcmCallbackCapacityBytes) {
        m_pcmCallbackCache.assign(m_pcmCallbackCapacityBytes, 0);
        m_pcmCallbackReadPos = 0;
        m_pcmCallbackWritePos = 0;
        m_pcmCallbackBufferedBytes = 0;
    }
    if (m_pcmCallbackCache.empty()) {
        return;
    }

    if (bytesToWrite >= m_pcmCallbackCapacityBytes) {
        const size_t keptBytes = m_pcmCallbackCapacityBytes - (m_pcmCallbackCapacityBytes % static_cast<size_t>(m_blockAlign));
        const uint8_t* tail = data + (bytesToWrite - keptBytes);
        memcpy(m_pcmCallbackCache.data(), tail, keptBytes);
        m_pcmCallbackReadPos = 0;
        m_pcmCallbackWritePos = keptBytes % m_pcmCallbackCache.size();
        m_pcmCallbackBufferedBytes = keptBytes;
        m_pcmWrittenFrames += static_cast<uint64_t>(bytesToWrite / static_cast<size_t>(m_blockAlign));
        m_pcmDrainedFrames = m_pcmWrittenFrames - static_cast<uint64_t>(keptBytes / static_cast<size_t>(m_blockAlign));
        return;
    }

    const size_t freeBytes = m_pcmCallbackCapacityBytes - m_pcmCallbackBufferedBytes;
    if (bytesToWrite > freeBytes) {
        size_t bytesToDrop = bytesToWrite - freeBytes;
        bytesToDrop -= bytesToDrop % static_cast<size_t>(m_blockAlign);
        if (bytesToDrop > m_pcmCallbackBufferedBytes) {
            bytesToDrop = m_pcmCallbackBufferedBytes - (m_pcmCallbackBufferedBytes % static_cast<size_t>(m_blockAlign));
        }
        m_pcmCallbackReadPos = (m_pcmCallbackReadPos + bytesToDrop) % m_pcmCallbackCache.size();
        m_pcmCallbackBufferedBytes -= bytesToDrop;
        m_pcmDrainedFrames += static_cast<uint64_t>(bytesToDrop / static_cast<size_t>(m_blockAlign));
    }

    const size_t firstCopySize = std::min<size_t>(bytesToWrite, m_pcmCallbackCache.size() - m_pcmCallbackWritePos);
    memcpy(m_pcmCallbackCache.data() + m_pcmCallbackWritePos, data, firstCopySize);
    if (bytesToWrite > firstCopySize) {
        memcpy(m_pcmCallbackCache.data(), data + firstCopySize, bytesToWrite - firstCopySize);
    }
    m_pcmCallbackWritePos = (m_pcmCallbackWritePos + bytesToWrite) % m_pcmCallbackCache.size();
    m_pcmCallbackBufferedBytes += bytesToWrite;
    m_pcmWrittenFrames += static_cast<uint64_t>(bytesToWrite / static_cast<size_t>(m_blockAlign));
}

void HJPluginAudioRender::clearPCMCallbackData(uint64_t renderedFrames)
{
    std::lock_guard<std::mutex> lock(m_pcmCallbackMutex);
    m_pcmCallbackCache.assign(m_pcmCallbackCapacityBytes, 0);
    m_pcmCallbackReadPos = 0;
    m_pcmCallbackWritePos = 0;
    m_pcmCallbackBufferedBytes = 0;
    m_pcmWrittenFrames = renderedFrames;
    m_pcmDrainedFrames = renderedFrames;
    m_pcmLastRenderedFrames = renderedFrames;
}

void HJPluginAudioRender::discardPendingPCMCallbackData()
{
    std::lock_guard<std::mutex> lock(m_pcmCallbackMutex);
    if (m_pcmCallbackCache.size() != m_pcmCallbackCapacityBytes) {
        m_pcmCallbackCache.assign(m_pcmCallbackCapacityBytes, 0);
    }
    m_pcmCallbackReadPos = 0;
    m_pcmCallbackWritePos = 0;
    m_pcmCallbackBufferedBytes = 0;
    m_pcmDrainedFrames = m_pcmWrittenFrames;
    m_pcmLastRenderedFrames = m_pcmDrainedFrames;
}

void HJPluginAudioRender::applyOutputVolume(void* data, int32_t dataSize)
{
    auto audioInfo = SYNC_CONS_LOCK([this] {
        return m_audioInfo;
    });

    if (!data || dataSize <= 0 || !audioInfo) {
        return;
    }

    const float volume = m_muted.load() ? 0.0f : m_volume.load();
    if (volume >= 0.9999f && volume <= 1.0001f) {
        return;
    }

    switch (audioInfo->m_sampleFmt) {
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
    {
        const size_t sampleCount = static_cast<size_t>(dataSize) / sizeof(float);
        auto* samples = static_cast<float*>(data);
        for (size_t i = 0; i < sampleCount; ++i) {
            float scaled = samples[i] * volume;
            samples[i] = std::max<float>(-1.0f, std::min <float>(1.0f, scaled));
        }
        break;
    }
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
    {
        const size_t sampleCount = static_cast<size_t>(dataSize) / sizeof(int32_t);
        auto* samples = static_cast<int32_t*>(data);
        for (size_t i = 0; i < sampleCount; ++i) {
            const int64_t scaled = static_cast<int64_t>(samples[i] * volume);
            samples[i] = static_cast<int32_t>(std::max<int64_t>(INT32_MIN, std::min<int64_t>(INT32_MAX, scaled)));
        }
        break;
    }
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
    default:
    {
        const size_t sampleCount = static_cast<size_t>(dataSize) / sizeof(int16_t);
        auto* samples = static_cast<int16_t*>(data);
        for (size_t i = 0; i < sampleCount; ++i) {
            const int32_t scaled = static_cast<int32_t>(samples[i] * volume);
            samples[i] = static_cast<int16_t>(std::max<int32_t>(INT16_MIN, std::min<int32_t>(INT16_MAX, scaled)));
        }
        break;
    }
    }
}

NS_HJ_END
