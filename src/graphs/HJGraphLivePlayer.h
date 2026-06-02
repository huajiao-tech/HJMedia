#pragma once

#include "HJGraph.h"
#include "HJPlugins.h"

NS_HJ_BEGIN

class HJGraphLivePlayer : public HJGraphPlayer
{
public:
	HJ_DEFINE_CREATE(HJGraphLivePlayer);

	HJGraphLivePlayer(const std::string& i_name = "HJGraphLivePlayer", size_t i_identify = 0);
	virtual ~HJGraphLivePlayer() { done(); }
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
#if defined (WINDOWS)
	virtual int resetAudioDevice(const std::string& i_deviceName = "") override;
#endif
	virtual int setEnableSEIUUids(const std::string& i_uuid, bool bKeyMustCb) override;
	virtual int64_t getDuration() override;
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
//	virtual int canDroppingDeliverToOutputs(Information io_info);
//	virtual int canVideoDecoderDeliverToOutputs(Information io_info);

	HJAudioInfo::Ptr m_audioInfo{};
	HJListener m_playerListener{};

	HJLooperThread::Ptr m_audioThread{};
	HJLooperThread::Ptr m_renderThread{};
	HJTimeline::Ptr m_timeline{};

	HJPluginFFDemuxer::Ptr m_demuxer{};
	HJPluginAVDropping::Ptr m_dropping{};
	HJPluginAudioFFDecoder::Ptr m_audioDecoder{};
	HJPluginAudioResampler::Ptr m_audioResampler{};
	HJPluginSpeedControl::Ptr m_speedControl{};
	HJPluginVideoRender::Ptr m_videoMainRender{};
	HJPluginVideoRender::Ptr m_videoSoftRender{};
	HJPluginVideoFFDecoder::Ptr m_videoFFDecoder{};
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

	std::function<bool()> m_canDroppingDeliverAudioToOutput{};
	std::function<bool()> m_canDroppingDeliverVideoToOutput{};
	std::function<bool()> m_canVideoDecoderDeliverToOutput{};

private:
	void priHeartbeatRegist(const std::string& i_uniqueName, int i_interval);
	void priHeartbeatUnRegist(const std::string& i_uniqueName);
        
	std::weak_ptr<HJStatContext> m_statCtx;
    std::atomic<int> m_firstRenderIdx{0};
    
	int64_t m_entryTime = 0;
	static std::string s_statPlaying;
	static std::string s_statLoading;
};

NS_HJ_END
