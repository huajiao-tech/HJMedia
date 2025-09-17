#pragma once

#include "HJPluginCodec.h"
#include "HJOGRenderWindowBridge.h"

NS_HJ_BEGIN

class HJPluginVideoOHDecoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoOHDecoder);

	HJPluginVideoOHDecoder(const std::string& i_name = "HJPluginVideoOHDecoder", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_graphInfo) { }
	virtual ~HJPluginVideoOHDecoder() {
		HJPluginVideoOHDecoder::done();
	}
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr) override;

protected:
	// HJVideoInfo::Ptr videoInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// HJOGRenderWindowBridge::Ptr bridge
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJBaseCodec::Ptr createCodec() override;
    virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo) override;

	virtual void setInfoFrameSize(size_t i_size);
    virtual int canDeliverToOutputs();

	std::atomic<int> m_firstFrame{ 15 };
	HJOGRenderWindowBridge::Ptr m_bridge{};
    HJMediaFrame::Ptr m_currentFrame{};
};

NS_HJ_END
