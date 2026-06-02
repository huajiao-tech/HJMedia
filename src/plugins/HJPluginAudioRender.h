#pragma once

#include "HJPlugin.h"
#include "HJTimeline.h"

#include <atomic>
#include <mutex>
#include <vector>

NS_HJ_BEGIN

// See doc/HJPluginAudioRender.md for details.

class HJPluginAudioRender : public HJPlugin
{
public:
	using Ptr = std::shared_ptr<HJPluginAudioRender>;
	using Wtr = std::weak_ptr<HJPluginAudioRender>;

	HJPluginAudioRender(const std::string& i_name = "HJPluginAudioRender", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginAudioRender() { done(); }
	virtual void onOutputUpdated() override {}

	virtual int setMute(bool i_mute);
	virtual bool isMuted();
	virtual int setVolume(float i_volume);
	virtual float getVolume();

protected:
    enum class RunOp {
        Start,
        StopPause,
        StopEof
    };

	// HJAudioInfo::Ptr audioInfo
	// HJTimeline::Ptr timeline
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputUpdated() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override { return HJ_WOULD_BLOCK; }
	virtual int runFlush() override;
	virtual void onPauseStateChanged(bool i_pause) override;

	virtual int runInit(const HJAudioInfo::Ptr& i_audioInfo);
	virtual void postReinit();
	virtual int runReinit();
	virtual void runPCMCallback(uint64_t now, uint64_t next);
	virtual int64_t getRenderTimestamp() { return 0; }
	virtual int onPlaybackCompleted();
	virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) = 0;
	virtual void releaseRender() = 0;
    virtual int setStreamRunning(bool i_running, bool i_eofStop = false) = 0;
    virtual void postRunOp(RunOp op);
    virtual void runRunOp(RunOp op);

	std::atomic<size_t> m_inputKeyHash{};
	HJTimeline::Ptr m_timeline{};
	HJAudioInfo::Ptr m_audioInfo{};
	std::atomic<bool> m_inited{};
	bool m_buffering{};
	std::atomic<bool> m_muted{};
	std::atomic<float> m_volume{ 1.0f };
	bool m_prerolling{ true };
	int64_t m_prerollDurationMs{ 120 };
	bool m_pcmCallback{ true };
    int64_t m_pcmCallbackIntervalMs{ 20 };
    bool m_running{ false };
    int m_runOpMsgId{ 0 };
    int m_reinitMsgId{ 0 };
#if defined (WINDOWS)
	std::string m_audioDeviceName{};
#endif
	int m_blockAlign{ 32 };
	std::mutex m_pcmCallbackMutex{};
	std::vector<uint8_t> m_pcmCallbackCache{};
	size_t m_pcmCallbackCapacityBytes{ 0 };
	size_t m_pcmCallbackReadPos{ 0 };
	size_t m_pcmCallbackWritePos{ 0 };
	size_t m_pcmCallbackBufferedBytes{ 0 };
	uint64_t m_pcmWrittenFrames{ 0 };
	uint64_t m_pcmDrainedFrames{ 0 };
	uint64_t m_pcmLastRenderedFrames{ 0 };

protected:
	virtual std::tuple<int, int32_t, HJMediaFrame::Ptr> fillAudioBuffer(std::string& route, void* data, int32_t dataSize, int64_t& size);
	virtual void appendPCMCallbackData(const uint8_t* data, int32_t dataSize);
	virtual void clearPCMCallbackData(uint64_t renderedFrames = 0);
	virtual void discardPendingPCMCallbackData();
	virtual void applyOutputVolume(void* data, int32_t dataSize);
	virtual int64_t getQueuedAudioDurationMs() const;
	virtual bool hasQueuedControlFrame() const;
	virtual size_t getQueuedSize() const;
	virtual int stopOnPlaybackCompleted() { return HJ_OK; }
   };

NS_HJ_END
