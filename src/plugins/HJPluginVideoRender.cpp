#include "HJPluginVideoRender.h"
#include "HJGraph.h"
#include "HJFLog.h"
#include "HJStatContext.h"

#if defined (HarmonyOS)
    #include "HJOGRenderWindowBridge.h"
    #include "HJFFHeaders.h"
#endif

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginVideoRender::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	MUST_GET_PARAMETER(HJTimeline::Ptr, timeline);
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	if (i_param && i_param->haveStorage("HJStatContext")) {
		m_statCtx = i_param->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
	}

#if defined (HarmonyOS)
	GET_PARAMETER(HJOGRenderWindowBridge::Ptr, bridge);
#elif defined (WINDOWS)
	GET_PARAMETER(HJSharedMemoryProducer::Ptr, sharedMemoryProducer);
#endif
	GET_PARAMETER(bool, onlyFirstFrame);
	GET_PARAMETER(HJDeviceType, deviceType);

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	m_timeline = timeline;
	Wtr wRender = SHARED_FROM_THIS;
	m_timeline->addListener(getName(), [wRender]() {
		auto render = wRender.lock();
		if (render) {
			render->onTimelineUpdated();
		}
	});
#if defined (HarmonyOS)
	m_bridge = bridge;
#elif defined (WINDOWS)
	m_sharedMemoryProducer = sharedMemoryProducer;
#endif
	m_onlyFirstFrame = onlyFirstFrame;
	m_deviceType = deviceType;
	return HJ_OK;
}

void HJPluginVideoRender::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	m_timeline = nullptr;
	m_currentFrame = nullptr;
#if defined(HarmonyOS)
	m_bridge = nullptr;
#endif

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

void HJPluginVideoRender::priStatCodecType()
{
	int decType = (int)m_deviceType;
	if (m_cacheCodecType != decType)
	{
		m_cacheCodecType = decType;
		auto statPtr = m_statCtx.lock();
		if (statPtr)
		{
			int notifyCodecType = (m_cacheCodecType == HJDEVICE_TYPE_NONE) ? 1 : 2; //1 soft; 2 hw
			statPtr->notify(getName(), JNotify_Play_DecodeStyle, notifyCodecType);
			HJFLogi("{} statplayer notify codec type:{} notifyCodecType:{}", getName(), m_cacheCodecType, notifyCodecType);
		}
	}
}
void HJPluginVideoRender::priStatDelay(const std::shared_ptr<HJMediaFrame>& i_frame)
{
	if (i_frame)
	{
		if (i_frame->haveStorage("passThroughDemuxSystemTime") && i_frame->haveStorage("passThroughIsKey"))
		{
			bool bKey = i_frame->getValue<bool>("passThroughIsKey");
			if (bKey)
			{
				int64_t demuxSystemTime = i_frame->getValue<int64_t>("passThroughDemuxSystemTime");
                int64_t currentTime = HJCurrentSteadyMS();
				int64_t diffTime = currentTime - demuxSystemTime;
				auto statPtr = m_statCtx.lock();
				if (statPtr)
				{
                    JStatPlayDelayClockInfo info;
                    info.m_recvTime = demuxSystemTime;
                    info.m_renderTime = currentTime;
					statPtr->notify(getName(), JNotify_Play_DelayClock, std::any(std::move(info)));
				}
				//HJFLogi("{} statplayer delayclock:{}", getName(), diffTime);
			}
		}
	}
}

int HJPluginVideoRender::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	size_t size = 0;
	if (!o_size) {
		o_size = &size;
	}
	auto ret = HJPlugin::deliver(i_srcKeyHash, i_mediaFrame, o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples);
	if (ret == HJ_OK) {
		setInfoFrameSize(*o_size);
	}

    if (i_mediaFrame->isEofFrame()) {
        HJFLogi("{} (i_mediaFrame->isEofFrame())", getName());
    }
	return ret;
}

int HJPluginVideoRender::runTask(int64_t* o_delay)
{
    addInIdx();
	int64_t enter = HJCurrentSteadyMS();
	bool log = false;
	if (m_lastEnterTimestamp < 0 || enter >= m_lastEnterTimestamp + LOG_INTERNAL) {
		m_lastEnterTimestamp = enter;
		log = true;
	}
	if (log) {
		RUNTASKLog("{}, enter", getName());
	}

	std::string route{};
	size_t size = -1;
	int ret;
	HJMediaFrame::Ptr currentFrame{};
	do {
		ret = SYNC_CONS_LOCK([&route, &currentFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_0";
				return HJErrAlreadyDone;
			}
			currentFrame = m_currentFrame;
			m_currentFrame = nullptr;	// m_currentFrame只在runTask()和interRelease()中修改和使用，所以可以在读锁中修改
			return HJ_OK;
		});
		if (ret == HJErrAlreadyDone) {
			break;
		}

		if (currentFrame == nullptr) {
			route += "_1";
			currentFrame = receive(m_inputKeyHash.load(), &size);
			if (currentFrame == nullptr) {
				route += "_2";
				ret = HJ_WOULD_BLOCK;
				break;
			}

			setInfoFrameSize(size);

			if (currentFrame->isEofFrame()) {
				route += "_3";
				if (m_pluginListener) {
					route += "_4";
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF)));
				}
				IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				ret = HJ_WOULD_BLOCK;
				break;
			}
		}

		HJGraphPtr graph = m_graph.lock();
		if (!graph) {
			route += "_5";
			return HJErrFatal;
		}
		bool hasAudio = graph->hasAudio();

		ret = SYNC_CONS_LOCK([&route, o_delay, &currentFrame, hasAudio, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_6";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Inited) {
				route += "_7";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_8";
				return HJ_WOULD_BLOCK;
			}

			bool setTimestamp = false;
			do {
				if (m_firstFrame) {
					// first frame, synchronization is not required
					route += "_9";
					break;
				}

				int64_t streamIndex = -1;
				int64_t timestamp = -1;
				double speed = 1.0;
				if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
					route += "_10";
					if (currentFrame->m_streamIndex < streamIndex) {
						route += "_11";
						break;
					}
					else if (currentFrame->m_streamIndex > streamIndex) {
						route += "_12";
						if (!hasAudio) {
							route += "_13";
							setTimestamp = true;
							break;
						}

						return HJ_WOULD_BLOCK;
					}
					else { // (currentFrame->m_stream_index == streamIndex)
						route += "_14";
						if (currentFrame->getPTS() <= timestamp) {
							route += "_15";
							route += HJFMT("_delay({})", timestamp - currentFrame->getPTS());
							break;
						}
						else { // (currentFrame->getPTS() > timestamp)
							route += "_16";
							auto delay = currentFrame->getPTS() - timestamp;
							if (speed > 0) {
								route += "_17";
								delay = ((double)delay) / speed;
							}

							if (o_delay) {
								route += HJFMT("_advance({})", delay);
								*o_delay = delay;
								m_currentFrame = currentFrame;
								currentFrame = nullptr;
								return HJ_OK;
							}
							else {
								route += "_18";
								return HJ_WOULD_BLOCK;
							}
						}
					}
				}
				else {
					route += "_19";
					if (!hasAudio) {
						// the first frame is video
						route += "_20";
						setTimestamp = true;
						break;
					}
					else { // (hasAudio)
						route += "_21";
						return HJ_WOULD_BLOCK;
					}
				}
			} while (false);

			// _TODO_GS_
/*
			if (!m_bFirstVideoFrame && !m_pOwnerPlayer->getPreviewDuration() && hasAudio && needToDropYUVFrame(item)) {
				// TODO: droping is needed for (!hasAudio)?
				releaseYUV420PFrame(item);
				goto retry;
			}
*/
			if (setTimestamp) {
				route += "_22";
				m_timeline->setTimestamp(currentFrame->m_streamIndex, currentFrame->getPTS(), 1.0);
			}

			if (m_deviceType == HJDEVICE_TYPE_NONE) {	// software
				route += "_23";
#if defined (HarmonyOS)
				if (m_bridge) {
					route += "_24";
					HJAVFrame::Ptr avFrame = currentFrame->getMFrame();
					AVFrame* frame = avFrame->getAVFrame();
					uint8_t* pData[3] = {};
					pData[0] = frame->data[0];
					pData[1] = frame->data[1];
					pData[2] = frame->data[2];
					int nLineSize[3] = {};
					nLineSize[0] = frame->linesize[0];
					nLineSize[1] = frame->linesize[1];
					nLineSize[2] = frame->linesize[2];
					m_bridge->produceFromPixel(HJTransferRenderModeInfo::Create(), pData, nLineSize, frame->width, frame->height);
				}
#elif defined (WINDOWS)
				if (m_sharedMemoryProducer) {
					m_sharedMemoryProducer->write2(currentFrame);
				}
#endif
			}

			return HJ_OK;
		});
		if (ret == HJ_OK) {
			route += "_25";
			if (currentFrame) {
				route += "_26";
				if (m_pluginListener) {
					route += "_27";
					auto notify = HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME);
					(*notify)["frame"] = currentFrame;
					m_pluginListener(std::move(notify));
				}

				if (m_firstFrame) {
					route += "_28";
					if (m_pluginListener) {
						route += "_29";
						std::string renderFlag = m_onlyFirstFrame ? "SecondSoftRender" : "MainRender";
						m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME, 0, renderFlag)));
					}
					m_firstFrame = false;
				}

				if (m_onlyFirstFrame) {
					route += "_30";
					IF_FALSE_BREAK(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);

					InputMap inputs;
					m_inputSync.prodLock([&inputs, this] {
						inputs = m_inputs;
						m_inputs.clear();
					});
					for (auto it : inputs) {
						auto input = it.second;
						input->mediaFrames.done();
						auto plugin = (input->plugin).lock();
						if (plugin != nullptr) {
							plugin->onOutputDisconnected(input->myKeyHash);
						}
					}
				}
				else
				{
					route += "_31";
					//only real render stat, the if use HW, the soft decoder、soft render not stat;
					priStatCodecType();
					priStatDelay(currentFrame);
				}
			}
		}
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}

void HJPluginVideoRender::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

void HJPluginVideoRender::setInfoFrameSize(size_t i_size)
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_VIDEORENDER_frameSize, i_size);
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

NS_HJ_END
