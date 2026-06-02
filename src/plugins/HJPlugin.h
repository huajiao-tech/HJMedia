#pragma once

#include <utility>

#include "HJMediaFrameDeque.h"
#include "HJLooperThread.h"
#include "HJNotify.h"
#include "HJPluginNotify.h"
#include "HJMessageBus.h"
#include "HJGraphInfo.h"

NS_HJ_BEGIN

#define IF_FALSE_BREAK(CONDITION, RET)		if (!(CONDITION)) { \
												ret = (RET); \
												break; \
											}

#define IF_FALSE_RETURN(CONDITION, ...)		if (!(CONDITION)) { \
												return __VA_ARGS__; \
											}

#define IF_FAIL_BREAK(CONDITION, RET)		if ((CONDITION) != HJ_OK) { \
												ret = (RET); \
												break; \
											}

#define LOG_INTERNAL						5000

#define EVENT_FLAG_NONE						0x0000
#define EVENT_FLAG_DEQUE_SIZE				0x0001
#define EVENT_FLAG_AUDIO_DURATION			0x0002
#define EVENT_FLAG_MASK_AUDIO				(EVENT_FLAG_AUDIO_DURATION)
#define EVENT_FLAG_VIDEO_KEY_FRAMES			0x0004
#define EVENT_FLAG_VIDEO_FRAMES				0x0008
#define EVENT_FLAG_MASK_VIDEO				(EVENT_FLAG_VIDEO_KEY_FRAMES | EVENT_FLAG_VIDEO_FRAMES)
#define EVENT_FLAG_MASK_ALL					(EVENT_FLAG_MASK_AUDIO | EVENT_FLAG_MASK_VIDEO | EVENT_FLAG_DEQUE_SIZE)

// Base plugin managing IO links, task dispatch, and status; see doc/HJPlugin.md for lifecycle and usage.
//class HJGraph;
class HJPlugin : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJPlugin>;
	using Wtr = std::weak_ptr<HJPlugin>;

	HJPlugin(const std::string& i_name = "HJPlugin", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr);
	virtual ~HJPlugin();
	virtual const std::string& getName() const override;
	virtual int init(HJKeyStorage::Ptr i_param = nullptr) override;
	virtual int done() override;

	virtual int addInputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId = 0);
	virtual int addOutputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId = 0);
	virtual void onInputDisconnected(size_t i_srcKeyHash) final;
	virtual void onOutputDisconnected(size_t i_dstKeyHash) final;
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame);
	virtual int flush(size_t i_srcKeyHash);
	virtual void onOutputUpdated();
	virtual void setPause(bool i_pause);
	    
protected:
	struct Output {
		using Ptr = std::shared_ptr<Output>;
		Output() = default;
		virtual const std::string& name();
		HJMediaType type{};
		int trackId{};
		Wtr plugin{};
		size_t myKeyHash{};
	};

	struct Input : public Output {
		using Ptr = std::shared_ptr<Input>;
		Input() = default;
		virtual ~Input();
		HJMediaFrameDeque mediaFrames;
		uint16_t eventFlags{};
	};

	using InputMap = std::unordered_map<size_t, Input::Ptr>;
	using InputMapIt = InputMap::iterator;
	using OutputMap = std::unordered_map<size_t, Output::Ptr>;
	using OutputMapIt = OutputMap::iterator;

	// HJLooperThread::Ptr thread = nullptr
	// bool createThread = false
	// HJListener pluginListener = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int beforeDone() override;
	virtual void afterInit() override;

	virtual void onInputUpdated();
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) {}
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) {}
	virtual Input::Ptr getInput(size_t i_srcKeyHash) final;
	virtual Output::Ptr getOutput(size_t i_dstKeyHash) final;
	virtual int runTask(int64_t* o_delay = nullptr) = 0;
	virtual void postTask(int64_t i_delay = 0);
	virtual int runFlush();
	virtual int runEof(int i_streamIndex) { return HJ_OK; }
	virtual HJMediaFrame::Ptr receive(size_t i_srcKeyHash, int64_t* o_size = nullptr);
	virtual HJMediaFrame::Ptr preview(size_t i_srcKeyHash);
	virtual bool store(size_t i_srcKeyHash, HJMediaFrame::Ptr i_mediaFrame);
	virtual void onPauseStateChanged(bool i_pause) {}
//	virtual void setInfoStatus(HJStatus i_status);
	virtual void clearInputsAndOutputs();

protected:
	virtual void reportFrameDequeInfo(const uint16_t i_flags, const HJMediaFrameDeque::FrameDequeInfo& i_info, bool reduce = true);
	virtual bool setStatus(HJStatus i_status);
	virtual std::tuple<int, HJMediaFrame::Ptr> receiveInputFrame(std::string& route, size_t inputKeyHash, int64_t& size);
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame);

//	std::weak_ptr<HJGraph> m_graph;
	HJQueryBus::Wtr m_graphQueryBus;
	HJEventBus::Wtr m_graphEventBus;
//	HJListener m_pluginListener;	// TODO: consider using graph->setInfo() instead.

	InputMap m_inputs;
	HJSync m_inputSync;
	OutputMap m_outputs;
	HJSync m_outputSync;

	HJLooperThread::Handler::Ptr m_handler{};
	HJSync m_handlerSync;

	int m_runTaskId{ -1 };
	std::atomic<bool> m_quitting{};
	bool m_paused{ false };
	int m_streamIndex{};

private:
	std::string m_insName{};
	HJLooperThread::Ptr m_thread{};

	// TODO: for log
protected:
	bool logRunTask();
	int64_t m_enterTimestamp{ -1 };
	int64_t m_lastLogTimestamp{ -1 };
	// TODO: per-second log.
//	int64_t sample_size{};
//	int64_t frame_size{};
//	int64_t duration_count{ 1 };
//	int64_t v_duration_count{ 1 };
//	int64_t start_time{};
/*
public:   
	int64_t getInIdx() {
		return m_inIdx;
	}
	int64_t getOutIdx()	{
		return m_outIdx;
	}
protected:
	void addInIdx() {
		m_inIdx++;
	}
	void addOutIdx() {
		m_outIdx++;
	}
	std::atomic<int64_t> m_inIdx{0};
    std::atomic<int64_t> m_outIdx{0};
*/
protected:
	template <typename Msg, typename... CallArgs>
	HJResult<typename Msg::Ret> query(Msg, CallArgs&&... args) {
		auto bus = m_graphQueryBus.lock();
		if (bus == nullptr) {
			return HJResult<typename Msg::Ret>::error(HJErrFatal);
		}

		return bus->query<Msg>(std::forward<CallArgs>(args)...);
	}

	template <typename Msg, typename... CallArgs>
	HJRet publish(Msg, CallArgs&&... args) const {
		auto bus = m_graphEventBus.lock();
		if (bus == nullptr) {
			return HJErrFatal;
		}

		return bus->publish<Msg>(std::forward<CallArgs>(args)...);
	}

	template <typename Msg, typename... CallArgs>
	HJRet report(Msg, CallArgs&&... args) const {
		auto bus = m_graphEventBus.lock();
		if (bus == nullptr) {
			return HJErrFatal;
		}

		return bus->report<Msg>(std::forward<CallArgs>(args)...);
	}
};

NS_HJ_END
