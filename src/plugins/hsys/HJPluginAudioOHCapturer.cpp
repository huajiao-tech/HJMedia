#include "HJPluginAudioOHCapturer.h"
#include "HJACaptureOH.h"
#include "HJFLog.h"

NS_HJ_BEGIN

int HJPluginAudioOHCapturer::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	if (!audioInfo) {
		return HJErrInvalidParams;
	}
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(audioInfo);
	(*param)["thread"] = thread;
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	return HJPluginCapturer::internalInit(param);
}

void HJPluginAudioOHCapturer::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	auto info = i_mediaFrame->getAudioInfo();
	sample_size += info->getSampleCnt();
	auto now = HJCurrentSteadyMS();
	if (start_time > 0) {
		if ((now - start_time) / 1000 >= duration_count * 5) {
			auto samples = duration_count * 5 * info->getSampleRate();
			HJFLogi("{}, ({})s, samples({}), sample_size diff({}), delay({})",
				getName(), duration_count * 5, samples, sample_size - samples, now - i_mediaFrame->getPTS());
			duration_count++;
		}
	}
	else {
		start_time  = now;
	}

	HJPluginCapturer::deliverToOutputs(i_mediaFrame);
}

HJBaseCapture::Ptr HJPluginAudioOHCapturer::createCapturer()
{
	return HJBaseCapture::createCapture(HJMEDIA_TYPE_AUDIO, HJCAPTURE_TYPE_OH);
}

void HJPluginAudioOHCapturer::setMute(bool i_mute)
{
	SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS();

		auto capturer = std::dynamic_pointer_cast<HJACaptureOH>(m_capturer);
		if (capturer != nullptr) {
			capturer->setMute(i_mute);
		}
	});
}

NS_HJ_END
