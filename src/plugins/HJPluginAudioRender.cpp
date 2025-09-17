#include "HJPluginAudioRender.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

int HJPluginAudioRender::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	MUST_GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	MUST_GET_PARAMETER(HJTimeline::Ptr, timeline);
	GET_PARAMETER(HJLooperThread::Ptr, thread);

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	HJPluginAudioRender::Wtr wRender = SHARED_FROM_THIS;
	if (m_handler->async([wRender, audioInfo] {
		auto render = wRender.lock();
		if (render) {
			render->asyncInitRender(audioInfo);
		}
	})) {
		m_audioInfo = audioInfo;
		m_timeline = timeline;

		return HJ_OK;
	}

	HJPluginAudioRender::internalRelease();
	return HJErrFatal;
}

void HJPluginAudioRender::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	if (releaseRender()) {
		if (m_pluginListener) {
			m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING)));
		}
	}
	m_timeline = nullptr;
	m_audioInfo = nullptr;

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPluginAudioRender::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	int64_t audioDuration = 0;
	if (!o_audioDuration) {
		o_audioDuration = &audioDuration;
	}
	int64_t audioSamples = 0;
    if (!o_audioSamples) {
		o_audioSamples = &audioSamples;
	}
	auto ret = HJPlugin::deliver(i_srcKeyHash, i_mediaFrame, o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples);
	if (ret == HJ_OK) {
//		HJFLogi("{}, runTask(), audioDuration({}), audioSamples({}), i_mediaFrame->getDuration({})", getName(), audioDuration, audioSamples, i_mediaFrame->getDuration());
		setInfoAudioDuration(*o_audioSamples);
        
        if (i_mediaFrame->isEofFrame()) {
            m_eof.store(true);
        }
	}

	return ret;
}

int HJPluginAudioRender::setMute(bool i_mute)
{
	return SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		m_muted = i_mute;
		internalSetMute();

		return HJ_OK;
	});
}

bool HJPluginAudioRender::isMuted()
{
	return SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS(false);

		return m_muted;
	});
}

void HJPluginAudioRender::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

void HJPluginAudioRender::asyncInitRender(const HJAudioInfo::Ptr& i_audioInfo)
{
	auto ret = SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		auto err = initRender(i_audioInfo);
		if (err < 0) {
			setStatus(HJSTATUS_Exception, false);
		}
		else if (err == HJ_OK) {
			setStatus(HJSTATUS_Ready, false);
		}
		return err;
	});
	if (ret == HJ_OK) {
		if (m_pluginListener) {
			m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING)));
		}
	}
	else if (ret < 0 && ret != HJErrAlreadyDone) {
		if (m_pluginListener) {
			m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT)));
		}
	}
}

void HJPluginAudioRender::setInfoAudioDuration(int64_t i_audioSamples)
{
	auto graph = m_graph.lock();
	if (graph) {
		auto audioInfo = SYNC_CONS_LOCK([this] {
			CHECK_DONE_STATUS((HJAudioInfo::Ptr(nullptr)));
			return m_audioInfo;
		});
		if (audioInfo && audioInfo->m_samplesRate > 0) {
			int64_t audioDuration = i_audioSamples * 1000 / audioInfo->m_samplesRate;
			HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_AUDIORENDER_audioDuration, audioDuration);
			info->setName(HJSyncObject::getName());
			graph->setInfo(std::move(info));
		}
		else {
			HJFLoge("{}, (!audioInfo || audioInfo->m_samplesRate <= 0)", getName());
		}
	}
}

NS_HJ_END
