#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

class HJPluginVideoFFDecoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoFFDecoder);

	HJPluginVideoFFDecoder(const std::string& i_name = "HJPluginVideoFFDecoder", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_graphInfo) { }
	virtual ~HJPluginVideoFFDecoder() {
		HJPluginVideoFFDecoder::done();
	}
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr) override;

protected:
	// HJVideoInfo::Ptr videoInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// bool onlyFirstFrame
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJBaseCodec::Ptr createCodec() override;

	virtual void setInfoFrameSize(size_t i_size);
	virtual int canDeliverToOutputs();

	bool m_firstFrame{ true };
	bool m_onlyFirstFrame{};
};

NS_HJ_END
