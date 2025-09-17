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

void HJPluginAVDropping::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPluginAVDropping::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	size_t size = 0;
	if (!o_size) {
		o_size = &size;
	}
	int64_t audioDuration = 0;
	if (!o_audioDuration) {
		o_audioDuration = &audioDuration;
	}
	int64_t videoKeyFrames = 0;
	if (!o_videoKeyFrames) {
		o_videoKeyFrames = &videoKeyFrames;
	}
	int64_t audioSamples = 0;
	if (!o_audioSamples) {
		o_audioSamples = &audioSamples;
	}
	auto ret = HJPlugin::deliver(i_srcKeyHash, i_mediaFrame, o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples);
	if (ret == HJ_OK) {
		if (i_mediaFrame->isAudio()) {
			setInfoAudioDuration(*o_audioDuration);
		}
		else if (i_mediaFrame->isVideo()) {
			setInfoVideoKeyFrames(*o_videoKeyFrames);
		}
	}

	return ret;
}

int HJPluginAVDropping::setDropPerameter(int64_t i_maxAudioDuration, int64_t i_minAudioDuration)
{
	if (i_maxAudioDuration <= 0 || i_minAudioDuration <= 0 || i_maxAudioDuration <= i_minAudioDuration) {
		return HJErrInvalidParams;
	}

	return m_inputSync.consLock([=] {
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
	m_inputKeyHash.store(i_srcKeyHash);
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
	int64_t audioDuration = 0;
	int64_t videoKeyFrames = 0;
	int ret = HJ_WOULD_BLOCK;
	do {
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
			currentFrame = receive(m_inputKeyHash.load(), &size, &audioDuration, &videoKeyFrames);
			if (currentFrame) {
				route += "_2";
				if (currentFrame->isAudio()) {
					route += "_3";
					setInfoAudioDuration(audioDuration);
				}
				else if (currentFrame->isVideo()) {
					route += "_4";
					setInfoVideoKeyFrames(videoKeyFrames);
				}

				if (!currentFrame->isFlushFrame() && !currentFrame->isEofFrame() &&
					audioDuration > m_maxAudioDuration) {
					route += "_5";
					dropFrames(m_minAudioDuration, &size, &audioDuration, nullptr, &videoKeyFrames);
					setInfoAudioDuration(audioDuration);
					setInfoVideoKeyFrames(videoKeyFrames);
					ret = HJ_OK;
					break;
				}
			}
		}

		if (currentFrame) {
			route += "_6";
			err = canDeliverToOutputs(currentFrame, 0);
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
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), task duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - enter), ret);
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
		HJFLogi("{}, EOFFrame({})", getName(), i_mediaFrame->getPTS());
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

void HJPluginAVDropping::setInfoAudioDuration(int64_t i_audioDuration)
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_DROPPING_audioDuration, i_audioDuration);
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

void HJPluginAVDropping::setInfoVideoKeyFrames(int64_t i_videoKeyFrames)
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_DROPPING_videoKeyFrames, i_videoKeyFrames);
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

int HJPluginAVDropping::canDeliverToOutputs(HJMediaFrame::Ptr i_mediaFrame, int)
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

void HJPluginAVDropping::dropFrames(int64_t i_audioDutation, size_t* o_size, int64_t* o_audioDuration, int64_t* o_audioSamples, int64_t* o_videoKeyFrames, int64_t* o_videoFrames)
{
	auto input = getInput(m_inputKeyHash.load());
	if (input != nullptr) {
		input->mediaFrames.dropFrames(i_audioDutation, o_size, o_audioDuration, o_audioSamples, o_videoKeyFrames, o_videoFrames);
	}
}

NS_HJ_END
