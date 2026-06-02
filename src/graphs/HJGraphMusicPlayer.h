#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include "HJGraph.h"
#include "HJPlugins.h"

NS_HJ_BEGIN

class HJGraphMusicPlayer : public HJGraphPlayer
{
public:
    HJ_DEFINE_CREATE(HJGraphMusicPlayer);

    HJGraphMusicPlayer(const std::string& i_name = "HJGraphMusicPlayer", size_t i_identify = 0);
    virtual ~HJGraphMusicPlayer() { done(); }

    virtual int openURL(HJMediaUrl::Ptr i_url) override;
    virtual int close();
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
	virtual std::vector<int> getAudioTrackIds() override;
    virtual HJAudioTrackDisplayInfoVector getAudioTrackDisplayInfos() override;
	virtual int switchAudioTrack(int i_trackId) override;
    virtual int setRepeats(int i_repeats) override;

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
    virtual int registerEventHandler_mediaType() override;
    virtual int registerQueryHandler_seekSucceeded();
    virtual int registerQueryHandler_streamOpened();
    virtual int registerEventHandler_statusUpdated();
    virtual int registerEventHandler_audioDuration();

private:
    HJAudioInfo::Ptr m_audioInfo{};
    HJListener m_playerListener{};

    HJLooperThread::Ptr m_audioThread{};
    HJLooperThread::Ptr m_renderThread{};
    HJTimeline::Ptr m_timeline{};

    HJPluginFFDemuxer::Ptr m_demuxer{};
    HJPluginAudioFFDecoder::Ptr m_audioDecoder{};
    HJPluginAudioResampler::Ptr m_audioResampler{};
#if defined (HarmonyOS)
    HJPluginAudioOHRender::Ptr m_audioRender{};
#elif defined (WINDOWS)
	HJPluginAudioWASRender::Ptr m_audioRender{};
#elif defined (HJ_OS_IOS)
    HJPluginAudioIOSRender::Ptr m_audioRender{};
#elif defined (HJ_OS_ANDROID)
    HJPluginAudioAARender::Ptr m_audioRender{};
#else
    HJPluginAudioRender::Ptr m_audioRender{};
#endif

    int m_repeats{ 1 };
    std::atomic<bool> m_paused{ false };

    std::mutex m_playbackStateMutex{};
    bool m_pendingDemuxerFinalEof{ false };
    int m_currentRepeatNumber{ 0 };
    int m_lastStreamIndex{ -1 };
    bool m_audioPlaybackCompleted{ false };
    int64_t m_maxTimestamp{ -1 };

    HJLooperThread::Handler::Ptr m_handler{};
    HJLooperThread::Ptr m_thread{};
    int m_seekId{ -1 };

    std::function<bool()> m_canDemuxerDeliverAudioToOutput{};

    // temp
    virtual int openRecorder(HJKeyStorage::Ptr i_param) override;
    virtual void closeRecorder() override;
    HJPluginFFMuxer::Ptr m_muxer{};
    bool m_inRecording{};
};

NS_HJ_END
