#include "HJPluginFFDemuxer.h"
#include "HJDemuxers.h"
#include "HJFLog.h"

NS_HJ_BEGIN

int HJPluginFFDemuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	auto ret = HJPluginDemuxer::internalInit(i_param);
	if (ret == HJ_OK) {
		m_initId = m_handler->genMsgId();
	}
	return ret;
}

HJBaseDemuxer::Ptr HJPluginFFDemuxer::createDemuxer()
{
	return std::make_shared<HJFFDemuxer>();
}

int HJPluginFFDemuxer::initDemuxer(const HJMediaUrl::Ptr& i_url, uint64_t i_delay)
{
	Wtr wDemuxer = SHARED_FROM_THIS;
	if (m_handler->asyncAndClear([wDemuxer, i_url] {
		auto demuxer = wDemuxer.lock();
		if (demuxer) {
			demuxer->asyncInitDemuxer(i_url);
		}
	}, m_initId, i_delay)) {
		return HJ_OK;
	}
	return HJErrFatal;
}

void HJPluginFFDemuxer::asyncInitDemuxer(const HJMediaUrl::Ptr& i_url)
{
	auto ret = SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		auto err = HJPluginDemuxer::initDemuxer(i_url);
		if (err < 0) {
			setStatus(HJSTATUS_Exception, false);
		}
		else if (err == HJ_STOP) {
			setStatus(HJSTATUS_Stoped, false);
		}
		else if (err == HJ_OK) {
			setStatus(HJSTATUS_Ready, false);
			postTask();
		}
		return err;
	});
    if (ret < 0 && ret != HJErrAlreadyDone) {
		if (m_pluginListener) {
			m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT)));
		}
    }
}

NS_HJ_END
