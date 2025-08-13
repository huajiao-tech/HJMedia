#pragma once

#include "HJGraph.h"
#include "HJPlugins.h"

NS_HJ_BEGIN

class HJGraphPusher : public HJGraph
{
public:
	HJ_DEFINE_CREATE(HJGraphPusher);

	HJGraphPusher(const std::string& i_name = "HJGraphPusher", size_t i_identify = -1);

	void setMute(bool i_mute);
	int adjustBitrate(int i_newBitrate);
	int openRecorder(HJKeyStorage::Ptr i_param);
	void closeRecorder();

private:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJHOSurfaceCb surfaceCb
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;

	HJAudioInfo::Ptr m_audioInfo{};
	HJVideoInfo::Ptr m_videoInfo{};
	HJListener m_pusherListener{};

	HJLooperThread::Ptr m_audioThread{};

	HJPluginFFMuxer::Ptr m_muxer{};
	HJPluginRTMPMuxer::Ptr m_rtmp{};
	HJPluginAVInterleave::Ptr m_avInterleave{};
	HJPluginAudioOHCapturer::Ptr m_audioCapturer{};
	HJPluginAudioResampler::Ptr m_audioResampler{};
	HJPluginFDKAACEncoder::Ptr m_audioEncoder{};
	HJPluginVideoOHEncoder::Ptr m_videoEncoder{};

	bool m_inRecording{};
};

NS_HJ_END
