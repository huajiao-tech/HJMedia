#include "HJPluginRTMPMuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJPluginRTMPMuxer::HJPluginRTMPMuxer(const std::string& i_name, size_t i_identify)
	: HJPluginMuxer(i_name, i_identify)
{
	m_muxer = std::make_shared<HJRTMPMuxer>([&](const HJNotification::Ptr ntf) -> int {
		if (m_listener) {
			m_listener(ntf);
		}
		return HJ_OK;
	});
	m_muxer->setName(getName());
    m_muxer->setTimestampZero(true);
}

int HJPluginRTMPMuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	// TODO 重复调用internalInit，失败的话m_listener有隐患；退出需要m_listener = nullptr吗？
	m_listener = i_param->getValue<HJListener>("rtmpListener");
	return HJPluginMuxer::internalInit(i_param);
}

void HJPluginRTMPMuxer::beforeDone()
{
	HJPluginMuxer::beforeDone();
	m_muxer->setQuit(true);
}

void HJPluginRTMPMuxer::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	auto now = HJCurrentSteadyMS();
	if (start_time > 0) {
		int rate{};
		if (i_mediaFrame->isAudio()) {
			auto info = i_mediaFrame->getAudioInfo();
			sample_size += info->getSampleCnt();
			if ((now - start_time) / 1000 >= duration_count * 5) {
				auto samples = duration_count * 5 * info->getSampleRate();
				HJFLogi("{}, ({})s, samples({}), sample_size diff({}), delay({})",
					getName(), duration_count * 5, samples, sample_size - samples, now - i_mediaFrame->getPTS());
				duration_count++;
			}
		}
		else if (i_mediaFrame->isVideo()) {
			auto info = i_mediaFrame->getVideoInfo();
			frame_size++;
			if ((now - start_time) / 1000 >= v_duration_count * 5) {
				auto frames = v_duration_count * 5 * info->m_frameRate;
				HJFLogi("{}, ({})s, frames({}), sample_size diff({}), delay({})",
					getName(), v_duration_count * 5, frames, frame_size - frames, now - i_mediaFrame->getPTS());
				v_duration_count++;
			}
		}
	}
	else {
		start_time  = now;
	}
}

int HJPluginRTMPMuxer::initMuxer(std::string i_url, int i_mediaTypes)
{
	if (m_handler->async([=] {
		SYNC_PROD_LOCK([=] {
			CHECK_DONE_STATUS();
			auto ret = HJPluginMuxer::initMuxer(i_url, i_mediaTypes);
			if (ret < 0) {
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT)));
				}
			}
			if (ret == HJ_OK) {
				m_status = HJSTATUS_Ready;
			}
		});
	})) {
		return HJ_OK;
	}
	return HJErrFatal;
}

NS_HJ_END
