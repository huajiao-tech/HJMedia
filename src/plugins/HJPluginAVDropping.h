#pragma once

#include "HJPlugin.h"

NS_HJ_BEGIN

class HJPluginAVDropping : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginAVDropping);

	HJPluginAVDropping(const std::string& i_name = "HJPluginAVDropping", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginAVDropping() {
		HJPluginAVDropping::done();
	}
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr) override;

	virtual int setDropPerameter(int64_t i_maxAudioDuration, int64_t i_minAudioDuration);

protected:
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// HJGraph graph
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

	virtual void setInfoAudioDuration(int64_t i_audioDuration);
	virtual void setInfoVideoKeyFrames(int64_t i_videoKeyFrames);
	virtual int canDeliverToOutputs(HJMediaFrame::Ptr i_mediaFrame, int a);
	virtual void dropFrames(int64_t i_audioDutation, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_audioSamples = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_videoFrames = nullptr);

	std::atomic<size_t> m_inputKeyHash{};
	std::atomic<size_t> m_outputAudioKeyHash{};
	std::atomic<size_t> m_outputVideoKeyHash{};
	int64_t m_maxAudioDuration{ 10000 };
	int64_t m_minAudioDuration{ 5000 };

	HJMediaFrame::Ptr m_currentFrame{};
};

NS_HJ_END
