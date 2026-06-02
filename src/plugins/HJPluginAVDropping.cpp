#include "HJPluginAVDropping.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginAVDropping::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	GET_PARAMETER(HJLooperThread::Ptr, thread);

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	return HJPlugin::internalInit(param);
}

int HJPluginAVDropping::setDropPerameter(int64_t i_maxAudioDuration, int64_t i_minAudioDuration)
{
	if (i_maxAudioDuration <= 0 || i_minAudioDuration <= 0 || i_maxAudioDuration <= i_minAudioDuration) {
		return HJErrInvalidParams;
	}

	return m_inputSync.prodLock([this, i_maxAudioDuration, i_minAudioDuration] {
		if (m_quitting.load()) {
			return HJErrAlreadyDone;
		}

		m_maxAudioDuration = i_maxAudioDuration;
		m_minAudioDuration = i_minAudioDuration;

		return HJ_OK;
	});
}

void HJPluginAVDropping::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	HJPlugin::onInputAdded(i_srcKeyHash, i_type);

	m_inputKeyHash.store(i_srcKeyHash);

	auto input = m_inputs[i_srcKeyHash];
	input->eventFlags |= (EVENT_FLAG_AUDIO_DURATION | EVENT_FLAG_VIDEO_KEY_FRAMES);
}

void HJPluginAVDropping::onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type)
{
	if (i_type == HJMEDIA_TYPE_AUDIO) {
		m_outputAudioKeyHash.store(i_dstKeyHash);
	}
	else if (i_type == HJMEDIA_TYPE_VIDEO) {
		m_outputVideoKeyHash.store(i_dstKeyHash);
	}
}

int HJPluginAVDropping::runTask(int64_t* o_delay)
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
		HJMediaFrame::Ptr currentFrame = nullptr;
		auto err = SYNC_CONS_LOCK([&route, &currentFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_0";
				return HJErrAlreadyDone;
			}

			currentFrame = m_currentFrame;
			m_currentFrame = nullptr;
			return HJ_OK;
		});
		if (err == HJErrAlreadyDone) {
			ret = HJErrAlreadyDone;
			break;
		}

		if (!currentFrame) {
			route += "_1";

			auto srcKeyHash = m_inputKeyHash.load();
			auto input = getInput(srcKeyHash);
			if (input == nullptr) {
				break;
			}

			if (input->mediaFrames.audioDuration() > m_maxAudioDuration) {
				dropFrames(srcKeyHash, m_minAudioDuration, &size);
			}


			currentFrame = receive(m_inputKeyHash.load(), &size);
		}

		if (currentFrame) {
			route += "_6";
			err = canDeliverToOutputs(currentFrame);
			if (err != HJ_OK) {
				route += "_7";
				err = SYNC_CONS_LOCK([&route, currentFrame, this] {
					if (m_status == HJSTATUS_Done) {
						route += "_8";
						return HJErrAlreadyDone;
					}

					m_currentFrame = currentFrame;
					return HJ_OK;
				});
				if (err == HJErrAlreadyDone) {
					ret = HJErrAlreadyDone;
				}

				break;
			}

			route += "_9";
			deliverToOutputs(currentFrame);
			ret = HJ_OK;
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

		if (!inFrame->isEofFrame()) {
			route += "_4";
#if 0
			ret = canDeliverToOutputs(inFrame);
			if (ret != HJ_OK) {
				route += "_5";
				if (ret == HJ_WOULD_BLOCK) {
					route += "_6";
					store(inputKeyHash, inFrame);

					if (tryDropFrames(route, inputKeyHash, size)) {
						route += "_7";
						HJFLogi("{}, tryDropFrames() ok, size{}", getName(), size);
					}
				}
				break;
			}
#else
			auto res = query(QUERY_CAN_DELIVER_TO_OUTPUTS_ID, getID(), inFrame);
			if (!res.isOk()) {
				route += "_5";
				ret = res.code;
				break;
			}
			if (!res.value) {
				route += "_6";
				store(inputKeyHash, inFrame);

				if (tryDropFrames(route, inputKeyHash, size)) {
					route += "_7";
					HJFLogi("{}, tryDropFrames() ok, size{}", getName(), size);
				}
				ret = HJ_WOULD_BLOCK;
				break;
			}
#endif
		}

		deliverToOutputs(inFrame);
#endif
		route += "_8";
	} while (false);
//    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), task duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
	}
	return ret;
}

void HJPluginAVDropping::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	if (i_mediaFrame->isVideo()) {
//		HJFLogi("{}, videoFrame({})", getName(), i_mediaFrame->getPTS());
	}
	else if (i_mediaFrame->isAudio()) {
//		HJFLogi("{}, audioFrame({})", getName(), i_mediaFrame->getPTS());
	}
	else if (i_mediaFrame->isEofFrame()) {
		HJFLogi("{}, EofFrame({})", getName(), i_mediaFrame->getPTS());
	}
	else {
		HJFLogi("{}, unknownFrame({})", getName(), i_mediaFrame->getPTS());
	}

	OutputMap outputs;
	m_outputSync.consLock([&outputs, this] {
		outputs = m_outputs;
	});
	for (auto it = outputs.begin(); it != outputs.end(); ++it) {
		auto output = it->second;
		auto plugin = output->plugin.lock();
		if (plugin != nullptr) {
			if (i_mediaFrame->isEofFrame() ||
				(i_mediaFrame->isVideo() && output->type & HJMEDIA_TYPE_VIDEO) ||
				(i_mediaFrame->isAudio() && output->type & HJMEDIA_TYPE_AUDIO)) {
				auto err = plugin->deliver(output->myKeyHash, i_mediaFrame);
				if (err < 0) {
					HJFLoge("{}, plugin->deliver() error({})", getName(), err);
				}
			}
		}
	}
}
/*
int HJPluginAVDropping::canDeliverToOutputs(HJMediaFrame::Ptr i_mediaFrame)
{
	auto graph = m_graph.lock();
	if (!graph) {
		HJFLoge("{}, (!graph)", getName());
		return HJErrExcep;
	}

	auto param = HJMakeNotification(HJ_PLUGIN_GETINFO_DROPPING_canDeliverToOutputs);
	(*param)["mediaFrame"] = i_mediaFrame;
	return graph->getInfo(param);
}
*/
bool HJPluginAVDropping::tryDropFrames(std::string& route, size_t inputKeyHash, int64_t& size)
{
	bool ret{ false };
	do {
		Input::Ptr input{};
		int64_t maxAudioDuration{};
		int64_t minAudioDuration{};
		ret = m_inputSync.consLock([this, inputKeyHash, &input, &maxAudioDuration, &minAudioDuration] {
			if (m_quitting.load()) {
				return false;
			}

			auto it = m_inputs.find(inputKeyHash);
			if (it == m_inputs.end()) {
				return false;
			}

			input = it->second;
			maxAudioDuration = m_maxAudioDuration;
			minAudioDuration = m_minAudioDuration;
			return true;
		});
		if (!ret) {
			route += "_10";
			break;
		}

		if (input->mediaFrames.audioDuration() > maxAudioDuration) {
			route += "_11";
			HJMediaFrameDeque::FrameDequeInfo info;
			ret = input->mediaFrames.dropFrames(minAudioDuration, &info);
			if (ret) {
				route += "_12";
				reportFrameDequeInfo(input->eventFlags, info);

				size = info.dequeSize;
			}
		}

		route += "_13";
	} while (false);

	return ret;
}

NS_HJ_END
