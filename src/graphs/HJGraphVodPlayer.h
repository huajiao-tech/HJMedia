#pragma once

#include <atomic>
#include "HJGraph.h"
#include "HJPlugins.h"

NS_HJ_BEGIN

class HJGraphVodPlayer : public HJGraphPlayer
{
public:
	HJ_DEFINE_CREATE(HJGraphVodPlayer);

	HJGraphVodPlayer(const std::string& i_name = "HJGraphVodPlayer", size_t i_identify = 0);
	virtual ~HJGraphVodPlayer() { done(); }
//	virtual int setInfo(const Information i_info) override;
//	virtual int getInfo(Information io_info) override;

//	virtual int listener(const HJNotification::Ptr);
	virtual int openURL(HJMediaUrl::Ptr i_url) override;
	virtual int close();
//	virtual bool hasAudio() override;
	virtual int setMute(bool i_mute) override;
	virtual bool isMuted() override;
	virtual int setVolume(float i_volume) override;
	virtual float getVolume() override;
	virtual int pause() override;
	virtual int resume() override;
	virtual bool isPaused() const override {
		return m_paused.load();
	}
#if defined (WINDOWS)
	virtual int resetAudioDevice(const std::string& i_deviceName = "") override;
#endif
	virtual int64_t getCurrentTimestamp() override;
	virtual int64_t getDuration() override;
	int seek(int64_t i_timestamp) override;
	virtual int switchAudioTrack(int i_trackId) override;

private:
	// HJAudioInfo::Ptr audioInfo
	// HJListener playerListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;

	virtual int registerHandlers();
	virtual int registerQueryHandler_hasAudio();
	virtual int registerQueryHandler_canDeliverToOutputs();
	virtual int registerQueryHandler_canPluginEof();
	virtual int registerEventHandler_pluginNotify();
	virtual int registerEventHandler_statusUpdated();
	virtual int registerEventHandler_audioDuration();
	virtual int registerEventHandler_videoFrames();
	virtual int registerEventHandler_videoKeyFrames();
//	virtual int canDemuxerDeliverToOutputs(Information io_info);
//	virtual int canVideoDecoderDeliverToOutputs(Information io_info);

	HJAudioInfo::Ptr m_audioInfo{};
	HJListener m_playerListener{};

	HJLooperThread::Ptr m_audioThread{};
	HJLooperThread::Ptr m_renderThread{};
	HJTimeline::Ptr m_timeline{};

	HJPluginFFDemuxer::Ptr m_demuxer{};
	HJPluginAudioFFDecoder::Ptr m_audioDecoder{};
	HJPluginVideoFFDecoder::Ptr m_videoSFDecoder{};
	HJPluginAudioResampler::Ptr m_audioResampler{};
	HJPluginVideoRender::Ptr m_videoRender{};
#if defined (HarmonyOS)
	HJPluginVideoOHDecoder::Ptr m_videoHWDecoder{};
    HJPluginAudioOHRender::Ptr m_audioRender{};
#elif defined (WINDOWS)
//	HJPluginAudioWORender::Ptr m_audioRender{};
	HJPluginAudioWASRender::Ptr m_audioRender{};
	HJSharedMemoryProducer::Ptr m_sharedMemoryProducer{};
#else
    HJPluginAudioRender::Ptr m_audioRender{};
#endif

	int m_repeats{ 1 };
	int m_currentRepeatNumber{ 0 };
	int m_lastStreamIndex{ -1 };
	int m_videoStreamIndex{ -1 };
	int m_audioStreamIndex{ -1 };
	std::atomic<bool> m_paused{ false };
	std::atomic<int64_t> m_maxTimestamp{ -1 };

	HJLooperThread::Handler::Ptr m_handler{};
	HJLooperThread::Ptr m_thread{};
	int m_seekId{ -1 };

	std::function<bool()> m_canDemuxerDeliverAudioToOutput{};
	std::function<bool()> m_canDemuxerDeliverVideoToOutput{};
	std::function<bool()> m_canVideoDecoderDeliverToOutput{};
};

NS_HJ_END
