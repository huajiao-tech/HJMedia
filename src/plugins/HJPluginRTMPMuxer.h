#pragma once

#include "HJPluginMuxer.h"
#include "HJRTMPMuxer.h"

NS_HJ_BEGIN

class HJPluginRTMPMuxer : public HJPluginMuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginRTMPMuxer);

	HJPluginRTMPMuxer(const std::string& i_name = "HJPluginRTMPMuxer", size_t i_identify = -1);

protected:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJListener rtmpListener
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void beforeDone() override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
	virtual int initMuxer(std::string i_url, int i_mediaTypes) override;

	HJListener m_listener{};
};

NS_HJ_END
