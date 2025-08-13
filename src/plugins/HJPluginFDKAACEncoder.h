#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

class HJPluginFDKAACEncoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginFDKAACEncoder);

	HJPluginFDKAACEncoder(const std::string& i_name = "HJPluginFDKAACEncoder", size_t i_identify = -1)
		: HJPluginCodec(i_name, i_identify) {
		m_mediaType = HJMEDIA_TYPE_AUDIO;
	}

	virtual void onOutputUpdated() override {}

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual int runTask() override;
	virtual HJBaseCodec::Ptr createCodec() override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
};

NS_HJ_END
