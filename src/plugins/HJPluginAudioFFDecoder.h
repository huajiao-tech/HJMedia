#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

class HJPluginAudioFFDecoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioFFDecoder);

	HJPluginAudioFFDecoder(const std::string& i_name = "HJPluginAudioFFDecoder", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_graphInfo) { }
	virtual ~HJPluginAudioFFDecoder() {
		HJPluginAudioFFDecoder::done();
	}
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr) override;

	virtual void onOutputUpdated() override;

protected:
	// HJAudioInfo::Ptr audioInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJBaseCodec::Ptr createCodec() override;

	virtual void setInfoAudioDuration(int64_t i_audioDuration);
};

NS_HJ_END
