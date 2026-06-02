#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

// Audio decoder plugin based on FFmpeg. See doc/HJPluginAudioFFDecoder.md for details.

class HJPluginAudioFFDecoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioFFDecoder);

	HJPluginAudioFFDecoder(const std::string& i_name = "HJPluginAudioFFDecoder", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginAudioFFDecoder() { done(); }

protected:
	// HJAudioInfo::Ptr audioInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJMediaType getType() override { return HJMEDIA_TYPE_AUDIO; }
	virtual HJBaseCodec::Ptr createCodec() override;
};

NS_HJ_END
