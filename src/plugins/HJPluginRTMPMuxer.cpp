#include "HJPluginRTMPMuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN
/*
HJPluginRTMPMuxer::HJPluginRTMPMuxer(const std::string& i_name, HJKeyStorage::Ptr i_graphInfo)
	: HJPluginMuxer(i_name, i_graphInfo)
{
	Wtr wMuxer = SHARED_FROM_THIS;
	m_muxer = std::make_shared<HJRTMPMuxer>([wMuxer](const HJNotification::Ptr ntf) -> int {
		auto muxer = wMuxer.lock();
		if (muxer) {
			muxer->listener(ntf);
		}
		return HJ_OK;
	});
	m_muxer->setName(getName());
    m_muxer->setTimestampZero(true);
}

int HJPluginRTMPMuxer::done()
{
	m_quitting.store(true);
	m_muxer->setQuit(true);
	return HJPluginMuxer::done();
}
*/
int HJPluginRTMPMuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	// TODO 重复调用internalInit，失败的话m_listener有隐患；退出需要m_listener = nullptr吗？
	m_listener = i_param->getValue<HJListener>("rtmpListener");
	i_param->erase("rtmpListener");
	return HJPluginMuxer::internalInit(i_param);
}
/*
int HJPluginRTMPMuxer::beforeDone()
{
	m_quitting.store(true);
	auto muxer = m_muxerSync.consLock([this] {
		return m_muxer;
	});
	if (muxer) {
		muxer->setQuit(true);
	}

	return HJPluginMuxer::beforeDone();
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
*/
HJBaseMuxer::Ptr HJPluginRTMPMuxer::createMuxer()
{
	Wtr wMuxer = SHARED_FROM_THIS;
	auto baseMuxer = std::make_shared<HJRTMPMuxer>([wMuxer](const HJNotification::Ptr ntf) -> int {
		auto muxer = wMuxer.lock();
		if (muxer) {
			muxer->listener(ntf);
		}
		return HJ_OK;
	});
	baseMuxer->setTimestampZero(true);
	return baseMuxer;
}
/*
int HJPluginRTMPMuxer::initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx)
{
	auto muxer = SHARED_FROM_THIS;
	if (m_handler->async([muxer, i_url, i_mediaTypes, statCtx] {
		muxer->asyncInitMuxer(i_url, i_mediaTypes, statCtx);
	})) {
		return HJ_OK;
	}
	return HJErrFatal;
}

void HJPluginRTMPMuxer::asyncInitMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx)
{
	auto ret = SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		return HJPluginMuxer::initMuxer(i_url, i_mediaTypes, statCtx);
	});
	if (ret < 0) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception));
		//if (m_pluginListener) {
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT, getID());
	}
	else if (ret == HJ_STOP) {
		setStatus(HJSTATUS_Stoped);
	}
	else if (ret == HJ_OK) {
		setStatus(HJSTATUS_Ready);
	}
}
*/
void HJPluginRTMPMuxer::listener(const HJNotification::Ptr ntf)
{
	if (m_listener) {
		m_listener(ntf);
	}
}

NS_HJ_END
