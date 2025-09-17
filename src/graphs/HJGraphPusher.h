#pragma once

#include "HJGraph.h"
#include "HJPlugins.h"

NS_HJ_BEGIN

class HJGraphPusher : public HJGraph
{
public:
	HJ_DEFINE_CREATE(HJGraphPusher);

	HJGraphPusher(const std::string& i_name = "HJGraphPusher", size_t i_identify = -1);
	virtual ~HJGraphPusher();
	virtual int setInfo(const Information info) override {
		return HJ_OK;
	}
	virtual int getInfo(Information info) override {
		return HJ_OK;
	}

	void setMute(bool i_mute);
	int adjustBitrate(int i_newBitrate);
	int openRecorder(HJKeyStorage::Ptr i_param);
	void closeRecorder();
	int openSpeechRecognizer(HJKeyStorage::Ptr i_param);
	void closeSpeechRecognizer();

private:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJHOSurfaceCb surfaceCb
	// HJListener pusherListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;

	HJAudioInfo::Ptr m_audioInfo{};
	HJVideoInfo::Ptr m_videoInfo{};
	HJListener m_pusherListener{};

	HJLooperThread::Ptr m_audioThread{};

	HJPluginFFMuxer::Ptr m_muxer{};
	HJPluginRTMPMuxer::Ptr m_rtmp{};
	HJPluginAVInterleave::Ptr m_avInterleave{};
	HJPluginAudioResampler::Ptr m_audioResampler{};
	HJPluginAudioResampler::Ptr m_speechResampler{};
	HJPluginSpeechRecognizer::Ptr m_speechRecognizer{};
	HJPluginFDKAACEncoder::Ptr m_audioEncoder{};
#if defined(HarmonyOS)
	HJPluginAudioOHCapturer::Ptr m_audioCapturer{};
	HJPluginVideoOHEncoder::Ptr m_videoEncoder{};
#endif

	bool m_inRecording{};
	bool m_speechRecognizing{};
};

NS_HJ_END
