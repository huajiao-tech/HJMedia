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
	if (i_param) {
		if (i_param->haveValue("HJStatContext")) {
			m_statCtx = i_param->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
		}
		if (i_param->haveValue("enableSEIUUids")) {
			std::lock_guard<std::mutex> lock(m_seiMutex);
			m_enableSEIUUids = i_param->getValue<std::map<std::string, bool>>("enableSEIUUids");
		}
		for (const auto& seiuuid : m_enableSEIUUids) {
			HJFLogi("enable SEI Uuids:{}, force:{}", seiuuid.first, seiuuid.second);
		}
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
	m_timeline->addListener(getName(), [wRender](const HJNotification::Ptr i_ntf) {
		auto render = wRender.lock();
		if (render) {
			switch (i_ntf->getID()) {
			case HJ_TIMELINE_NOTIFY_PLAY:
			case HJ_TIMELINE_NOTIFY_UPDATED:
				render->postTask();
				break;
			}
			return HJ_OK;
		}

		return HJErrAlreadyDone;
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

	if (m_timeline) {
		m_timeline->removeListener(getName());
		m_timeline = nullptr;
	}
//	m_currentFrame = nullptr;
#if defined(HarmonyOS)
	m_bridge = nullptr;
#elif defined (WINDOWS)
	m_sharedMemoryProducer = nullptr;
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
				auto statPtr = m_statCtx.lock();
				if (statPtr)
				{
                    JStatPlayDelayClockInfo info;
                    info.m_recvTime = demuxSystemTime;
                    info.m_renderTime = currentTime;
					statPtr->notify(getName(), JNotify_Play_DelayClock, std::any(std::move(info)));
				}
				//HJFLogi("{} statplayer delayclock:{}", getName(), currentTime - demuxSystemTime);
			}
		}
	}
}

int HJPluginVideoRender::runTask(int64_t* o_delay)
{
//    addInIdx();
	auto log = logRunTask();
	if (log) {
		RUNTASKLog("{}, enter", getName());
	}

	std::string route{};
	int64_t size{ -1 };
	int ret{ HJ_OK };
	do {
#if 0
		auto next = preview(m_inputKeyHash.load());
		auto flush = (next != nullptr) && next->isFlushFrame();
		HJMediaFrame::Ptr currentFrame{};
		ret = SYNC_PROD_LOCK([this, &route, &currentFrame, flush] {
			if (m_status == HJSTATUS_Done) {
				route += "_0";
				return HJErrAlreadyDone;
			}

			if (!flush) {
				currentFrame = m_currentFrame;
			}
			m_currentFrame = nullptr;
			return HJ_OK;
		});
		if (ret != HJ_OK) {
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

			if (currentFrame->isFlushFrame()) {
				ret = runFlush();
				break;
			}

			if (currentFrame->isEofFrame()) {
				route += "_3";
				//if (m_pluginListener) {
				//	route += "_4";
				//	auto ntf = HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF);
				//	(*ntf)["streamIndex"] = currentFrame->m_streamIndex;
				//	m_pluginListener(std::move(ntf));
				//}
//				IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				auto res = query(QUERY_CAN_PLUGIN_EOF_ID, getID(), currentFrame->m_streamIndex);
				if (!res.isOk()) {
					ret = res.code;
					break;
				}
				if (res.value) {
					IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				}

				ret = HJ_OK;
				break;
			}
		}

		auto graph = m_graph.lock();
		if (!graph) {
			route += "_5";
			ret = HJErrFatal;
			break;
		}
		bool hasAudio = graph->hasAudio();

		ret = SYNC_PROD_LOCK([&route, o_delay, &currentFrame, hasAudio, this] {
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
				int64_t streamIndex = -1;
				int64_t timestamp = -1;
				double speed = 1.0;
				if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
					if (m_firstFrame) {
						break;
					}

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
						if (m_firstFrame) {
							break;
						}

						route += "_21";
						return HJ_WOULD_BLOCK;
					}
				}
			} while (false);

			// TODO: How to drop frames for video only?

			if (setTimestamp) {
				route += "_22";
				m_timeline->setTimestamp(currentFrame->m_streamIndex, currentFrame->getPTS(), 1.0);
//				HJFLogi("{}, m_timeline->setTimestamp({}, {})", getName(), currentFrame->m_streamIndex, currentFrame->getPTS());
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
				//if (m_pluginListener) {
				//	route += "_27";
				//	auto notify = HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME);
				//	(*notify)["frame"] = currentFrame;
				//	m_pluginListener(std::move(notify));
				//}
				report(EVENT_GRAPH_VIDEO_FRAME_ID, currentFrame);

				if (m_firstFrame) {
					route += "_28";
					//if (m_pluginListener) {
					//	route += "_29";
					//	std::string renderFlag = m_onlyFirstFrame ? "SecondSoftRender" : "MainRender";
					//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME, 0, renderFlag)));
					//}
					report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME, getID());
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
				else {
					route += "_31";
					// Only report render stats for software render; if using HW, soft decode/soft render stats are not reported.
					priStatCodecType();
					priStatDelay(currentFrame);
				}
			}
		}
#else
		auto inputKeyHash = m_inputKeyHash.load();
		HJMediaFrame::Ptr inFrame{};
		std::tie(ret, inFrame) = receiveInputFrame(route, inputKeyHash, size);
		if (ret != HJ_OK) { 
            route += "_0";
			break;
		}
		if (inFrame == nullptr) {
            route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		if (inFrame->isClearFrame()) {
            route += "_2";
			auto err = runFlush();
			if (err < 0) {
                route += "_3";
				ret = err;
			}
			break;
		}

		if (inFrame->isEofFrame()) {
            route += "_4";
			//if (m_pluginListener) {
			//	route += "_5";
			//	auto ntf = HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF);
			//	(*ntf)["streamIndex"] = inFrame->m_streamIndex;
			//	m_pluginListener(std::move(ntf));
			//}
//			IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
			auto res = query(QUERY_CAN_PLUGIN_EOF_ID, getID(), inFrame->m_streamIndex);
			if (!res.isOk()) {
				route += "_5";
				ret = res.code;
				break;
			}
			if (res.value) {
				route += "_6";
				IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
			}

			break;
		}

		int64_t delay{ -1 };
		std::tie(ret, delay) = syncVideoFrame(route, inFrame);
		if (ret < 0) {
            route += "_7";
			break;
		}
		else if (ret == HJ_WOULD_BLOCK) {
            route += "_8";
			store(inputKeyHash, inFrame);
			if (delay >= 0 && o_delay != nullptr) {
                route += "_9";
				*o_delay = delay;
				ret = HJ_OK;
			}
			break;
		}

		ret = postprocessVideoFrame(route, inFrame);
#endif
        route += "_A";
	} while (false);
//    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
	}
	return ret;
}

int HJPluginVideoRender::runFlush()
{
	HJTimeline::Ptr timeline{};
	auto ret = SYNC_CONS_LOCK([this, &timeline] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		timeline = m_timeline;
		return HJ_OK;
	});

    if (timeline) { 
		timeline->flush();
		HJFLogi("{}, timeline->flush()", getName());
	}

	return ret;
}

void HJPluginVideoRender::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	HJPlugin::onInputAdded(i_srcKeyHash, i_type);

	m_inputKeyHash.store(i_srcKeyHash);

	auto input = m_inputs[i_srcKeyHash];
	input->eventFlags = EVENT_FLAG_VIDEO_FRAMES;
}

std::tuple<int, int64_t> HJPluginVideoRender::syncVideoFrame(std::string& route, HJMediaFrame::Ptr& currentFrame)
{
    int64_t delay{ -1 };
	HJTimeline::Ptr timeline{};
#if defined (HarmonyOS)
	HJOGRenderWindowBridge::Ptr producer{};
#elif defined (WINDOWS)
	HJSharedMemoryProducer::Ptr producer{};
#else
	HJObject::Ptr producer{};
#endif
	auto ret = SYNC_CONS_LOCK([this, &route, &currentFrame, &delay, &timeline, &producer] {
		if (m_status == HJSTATUS_Done) {
			route += "_6";
			return HJErrAlreadyDone;
		}
		if (m_status >= HJSTATUS_EOF) {
			route += "_8";
			return HJErrInvalidData;
		}
#if 0
		auto graph = m_graph.lock();
		if (!graph) {
			return HJErrFatal;
		}
		bool hasAudio = graph->hasAudio();
#else
		auto res = query(QUERY_HAS_AUDIO_ID);
		if (!res.isOk()) {
			return res.code;
		}
		auto hasAudio = res.value;
#endif
		bool setTimestamp{ false };
		do {
			int64_t streamIndex{ -1 };
			int64_t timestamp{ -1 };
			double speed{ 1.0 };
			if (m_timeline->getTimestamp(streamIndex, timestamp, speed)) {
				if (m_firstFrame) {
					break;
				}

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
				else { // (videoFrame->m_stream_index == streamIndex)
					route += "_14";
					if (currentFrame->getPTS() <= timestamp) {
						route += "_15";
						route += HJFMT("_delay({})", timestamp - currentFrame->getPTS());
						break;
					}
					else { // (videoFrame->getPTS() > timestamp)
						route += "_16";
						delay = currentFrame->getPTS() - timestamp;
						if (speed > 0) {
							route += "_17";
							delay = ((double)delay) / speed;
						}

						route += HJFMT("_advance({})", delay);
						return HJ_WOULD_BLOCK;
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
					if (m_firstFrame) {
						break;
					}

					route += "_21";
					return HJ_WOULD_BLOCK;
				}
			}
		} while (false);

		// TODO: How to drop frames for video only?

		if (setTimestamp) {
			route += "_22";
			timeline = m_timeline;
//			m_timeline->setTimestamp(currentFrame->m_streamIndex, currentFrame->getPTS(), 1.0);
		}

		if (m_deviceType == HJDEVICE_TYPE_NONE) {	// software
			route += "_23";
#if defined (HarmonyOS)
			producer = m_bridge;
/*
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
*/
#elif defined (WINDOWS)
			producer = m_sharedMemoryProducer;
/*
			if (m_sharedMemoryProducer) {
				m_sharedMemoryProducer->write2(currentFrame);
			}
*/
#endif
		}

		return HJ_OK;
	});
	if (timeline) {
		timeline->setTimestamp(currentFrame->m_streamIndex, currentFrame->getPTS(), 1.0);
//		HJFLogi("{}, timeline->setTimestamp({}, {})", getName(), currentFrame->m_streamIndex, currentFrame->getPTS());
	}
	if (producer != nullptr) {
#if defined (HarmonyOS)
		route += "_24";
		HJAVFrame::Ptr avFrame = currentFrame->getMFrame();
		if (avFrame != nullptr) {
			AVFrame* frame = avFrame->getAVFrame();
			if (frame != nullptr) {
				uint8_t* pData[3] = {};
				pData[0] = frame->data[0];
				pData[1] = frame->data[1];
				pData[2] = frame->data[2];
				int nLineSize[3] = {};
				nLineSize[0] = frame->linesize[0];
				nLineSize[1] = frame->linesize[1];
				nLineSize[2] = frame->linesize[2];
				producer->produceFromPixel(HJTransferRenderModeInfo::Create(), pData, nLineSize, frame->width, frame->height);
			}
		}
#elif defined (WINDOWS)
		producer->write2(currentFrame);
#endif
	}

	return std::make_tuple(ret, delay);
}

int HJPluginVideoRender::postprocessVideoFrame(std::string& route, HJMediaFrame::Ptr& currentFrame)
{
	//if (m_pluginListener) {
	//	route += "_27";
	//	auto notify = HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME);
	//	(*notify)["frame"] = currentFrame;
	//	m_pluginListener(std::move(notify));
	//}
	procSEI(currentFrame);

	report(EVENT_GRAPH_VIDEO_FRAME_ID, currentFrame);

	if (m_firstFrame) {
		route += "_28";
		//if (m_pluginListener) {
		//	route += "_29";
		//	std::string renderFlag = m_onlyFirstFrame ? "SecondSoftRender" : "MainRender";
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME, 0, renderFlag)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME, getID());
		m_firstFrame = false;
	}

	if (m_onlyFirstFrame) {
		route += "_30";
		IF_FALSE_RETURN(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);

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
	else {
		route += "_31";
		// Only report render stats for software render; if using HW, soft decode/soft render stats are not reported.
		priStatCodecType();
		priStatDelay(currentFrame);
	}

	return HJ_OK;
}

void HJPluginVideoRender::setEnableSEIUUids(const std::string& i_uuid, bool bKeyMustCb)
{
	if (i_uuid.empty()) {
		return;
	}
	std::lock_guard<std::mutex> lock(m_seiMutex);
	m_enableSEIUUids[i_uuid] = bKeyMustCb;
}

void HJPluginVideoRender::procSEI(const HJMediaFrame::Ptr& mvf)
{
	bool keyFrame = mvf->getValue<bool>("passThroughIsKey");
	std::vector<HJSEIData> userSEIDatas{};
	auto seiNals = mvf->getSEINals();
	if (seiNals && seiNals->size() > 0)
	{
		const auto& nals = seiNals->getDatas();
		for (const auto& nal : nals) {
			auto userDatas = HJSEIWrapper::parseSEINals(nal);
			if (userDatas.size() > 0) {
				userSEIDatas.insert(userSEIDatas.end(), userDatas.begin(), userDatas.end());
			}
		}
	}
	{
		std::lock_guard<std::mutex> lock(m_seiMutex);
		if (!m_enableSEIUUids.empty())
		{
			std::vector<HJSEIData> filteredSEIDatas{};
			for (const auto& kv : m_enableSEIUUids) {
				const std::string& uuid = kv.first;
				bool always_on_key = kv.second;

				bool found = false;
				for (const auto& sei : userSEIDatas) {
					if (sei.uuid == uuid) {
						filteredSEIDatas.push_back(sei);
						found = true;
					}
				}

				if (!found && always_on_key && keyFrame) {
					HJSEIData empty_sei;
					empty_sei.uuid = uuid;
					filteredSEIDatas.push_back(empty_sei);
				}
			}
			userSEIDatas = std::move(filteredSEIDatas);
		}
	}
	//
	if (userSEIDatas.size() > 0) {
		report(EVENT_GRAPH_SEI_INFOS_ID, userSEIDatas, keyFrame);
	}

	return;
}

NS_HJ_END
