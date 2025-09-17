#pragma once

#include "HJPlugin.h"
#include "HJTimeline.h"

NS_HJ_BEGIN

class HJPluginAudioRender : public HJPlugin
{
public:
	using Ptr = std::shared_ptr<HJPluginAudioRender>;
	using Wtr = std::weak_ptr<HJPluginAudioRender>;

	HJPluginAudioRender(const std::string& i_name = "HJPluginAudioRender", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginAudioRender() {
		HJPluginAudioRender::done();
	}
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr) override;
/*
	virtual int start();
	virtual void stop();
	virtual void pause();
*/
	virtual int setMute(bool i_mute);
	virtual bool isMuted();

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJTimeline::Ptr timeline
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int runTask(int64_t* o_delay = nullptr) override {
    	return HJ_WOULD_BLOCK;
    }
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;

	virtual void asyncInitRender(const HJAudioInfo::Ptr& i_audioInfo);
	virtual void internalSetMute() = 0;
	virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) = 0;
	virtual bool releaseRender() = 0;
	virtual void setInfoAudioDuration(int64_t i_audioSamples);

	std::atomic<size_t> m_inputKeyHash{};
	HJTimeline::Ptr m_timeline{};
	HJAudioInfo::Ptr m_audioInfo{};
	int m_maxCache{};
	bool m_buffering{};
	bool m_muted{};
    std::atomic<bool> m_eof{};
};

NS_HJ_END
