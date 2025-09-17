#pragma once

#include "HJPluginCapturer.h"

NS_HJ_BEGIN

class HJPluginAudioOHCapturer : public HJPluginCapturer
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioOHCapturer);

	HJPluginAudioOHCapturer(const std::string& i_name = "HJPluginAudioOHCapturer", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCapturer(i_name, i_graphInfo) { }
	virtual ~HJPluginAudioOHCapturer() {
		HJPluginAudioOHCapturer::done();
	}

	void setMute(bool i_mute);

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
	virtual HJBaseCapture::Ptr createCapturer() override;
};

NS_HJ_END
