#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

class HJPluginFDKAACEncoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginFDKAACEncoder);

	HJPluginFDKAACEncoder(const std::string& i_name = "HJPluginFDKAACEncoder", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginFDKAACEncoder() { done(); }

	virtual void onOutputUpdated() override {}

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJMediaType getType() override { return HJMEDIA_TYPE_AUDIO; }
	virtual HJBaseCodec::Ptr createCodec() override;
//	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
};

NS_HJ_END
