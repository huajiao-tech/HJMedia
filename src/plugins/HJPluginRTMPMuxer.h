#pragma once

#include "HJPluginMuxer.h"
#include "HJRTMPMuxer.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

class HJPluginRTMPMuxer : public HJPluginMuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginRTMPMuxer);

	HJPluginRTMPMuxer(const std::string& i_name = "HJPluginRTMPMuxer", HJKeyStorage::Ptr i_graphInfo = nullptr);
	virtual ~HJPluginRTMPMuxer() {
		HJPluginRTMPMuxer::done();
	}
	virtual int done() override;

protected:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJListener rtmpListener
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
	virtual int initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx) override;

	virtual void asyncInitMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx);

	HJListener m_listener{};
};

NS_HJ_END
