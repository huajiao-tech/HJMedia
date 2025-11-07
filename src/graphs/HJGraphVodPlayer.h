#pragma once

#include "HJGraph.h"
#include "HJPlugins.h"

NS_HJ_BEGIN

class HJGraphVodPlayer : public HJGraphPlayer
{
public:
	HJ_DEFINE_CREATE(HJGraphVodPlayer);

	HJGraphVodPlayer(const std::string& i_name = "HJGraphVodPlayer", size_t i_identify = -1);
	virtual ~HJGraphVodPlayer();
	virtual int setInfo(const Information i_info) override;
	virtual int getInfo(Information io_info) override;

	virtual int listener(const HJNotification::Ptr);
	virtual int openURL(HJMediaUrl::Ptr i_url) override;
	virtual int close();
	virtual bool hasAudio() override;
	virtual int setMute(bool i_mute) override;
	virtual bool isMuted() override;

private:
	// HJAudioInfo::Ptr audioInfo
	// HJListener playerListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;

	virtual int canDemuxerDeliverToOutputs(Information io_info);
	virtual int canVideoDecoderDeliverToOutputs(Information io_info);

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
#if defined (WINDOWS)
	HJPluginAudioWASRender::Ptr m_audioRender{};
#elif defined (HarmonyOS)
	HJPluginVideoOHDecoder::Ptr m_videoHWDecoder{};
    HJPluginAudioOHRender::Ptr m_audioRender{};
#else
    HJPluginAudioRender::Ptr m_audioRender{};
#endif
};

NS_HJ_END
