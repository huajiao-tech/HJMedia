#pragma once

#include "HJPluginMuxer.h"
#include "HJRTMPMuxer.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

class HJPluginRTMPMuxer : public HJPluginMuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginRTMPMuxer);

	HJPluginRTMPMuxer(const std::string& i_name = "HJPluginRTMPMuxer", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginMuxer(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginRTMPMuxer() { done(); }

protected:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJListener rtmpListener
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
//	virtual int beforeDone() override;
//	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
	virtual HJBaseMuxer::Ptr createMuxer() override;
//	virtual int initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx) override;

//	virtual void asyncInitMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx);
	virtual void listener(const HJNotification::Ptr ntf);

	HJListener m_listener{};
};

NS_HJ_END
