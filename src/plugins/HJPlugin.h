#pragma once

#include "HJMediaFrameDeque.h"
#include "HJLooperThread.h"
#include "HJNotify.h"
#include "HJPluginNotify.h"

NS_HJ_BEGIN

#define LOG_INTERNAL		5000

class HJGraph;
class HJPlugin : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJPlugin>;
	using Wtr = std::weak_ptr<HJPlugin>;
	using HJGraphPtr = std::shared_ptr<HJGraph>;

	HJPlugin(const std::string& i_name = "HJPlugin", HJKeyStorage::Ptr i_graphInfo = nullptr);
	virtual ~HJPlugin();
	virtual const std::string& getName() const override;
	virtual int done() override;

	virtual int addInputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId = 0);
	virtual int addOutputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId = 0);
	virtual void onInputDisconnected(size_t i_srcKeyHash) final;
	virtual void onOutputDisconnected(size_t i_dstKeyHash) final;
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr);
	virtual void onOutputUpdated();
	    
    void addInIdx()
    {
        m_inIdx++;
    }
    void addOutIdx()
    {
        m_outIdx++;
    }
    int64_t getInIdx()
    {
        return m_inIdx;
    }
    int64_t getOutIdx()
    {
        return m_outIdx;
    }

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
	virtual void afterInit() override;

	virtual void onInputUpdated();
	virtual bool setStatus(HJStatus i_status, bool i_underLock = true);
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) { }
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) { }
	virtual Input::Ptr getInput(size_t i_srcKeyHash) final;
	virtual Output::Ptr getOutput(size_t i_dstKeyHash) final;
//	virtual int flush();
	virtual int runTask(int64_t* o_delay = nullptr) = 0;
	virtual void postTask(int64_t i_delay = 0);
	virtual HJMediaFrame::Ptr receive(size_t i_srcKeyHash, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr);
	virtual HJMediaFrame::Ptr preview(size_t i_srcKeyHash, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr);
    virtual int64_t audioSamplesOfInput(size_t i_srcKeyHash) final;
	virtual int64_t audioDurationOfInput(size_t i_srcKeyHash) final;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame);
	virtual void setInfoStatus(HJStatus i_status);
	static size_t getKeyHash(const std::string& i_name, HJMediaType i_type, int i_trackId = 0);

	InputMap m_inputs;
	HJSync m_inputSync;
	OutputMap m_outputs;
	HJSync m_outputSync;

	std::string m_insName{};
	int m_insIdx{ -1 };
	std::weak_ptr<HJGraph> m_graph{};

	HJLooperThread::Handler::Ptr m_handler{};
	HJSync m_handlerSync;
	HJLooperThread::Ptr m_thread{};
	HJListener m_pluginListener;
	int m_runTaskId{ -1 };
	std::atomic<bool> m_quitting{};
	int m_streamIndex{};

protected:
	// TODO _每5秒日志_
	int64_t sample_size{};
	int64_t frame_size{};
	int64_t duration_count{ 1 };
	int64_t v_duration_count{ 1 };
	int64_t start_time{};
    
    std::atomic<int64_t> m_inIdx{0};
    std::atomic<int64_t> m_outIdx{0};

	int64_t m_lastEnterTimestamp{ -1 };
};

NS_HJ_END
