#pragma once

#include "HJMediaFrameDeque.h"
#include "HJLooperThread.h"
#include "HJNotify.h"
#include "HJPluginNotify.h"

NS_HJ_BEGIN

class HJPlugin : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJPlugin>;
	using WPtr = std::weak_ptr<HJPlugin>;

	HJPlugin(const std::string& i_name = "HJPlugin", size_t i_identify = -1);

	virtual int addInputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId = 0);
	virtual int addOutputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId = 0);
	virtual void onInputDisconnected(size_t i_srcKeyHash) final;
	virtual void onOutputDisconnected(size_t i_dstKeyHash) final;
	virtual int setInputMaxSize(size_t i_srcKeyHash, int i_maxSize);
	virtual int canDeliverToInput(size_t i_srcKeyHash);
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame);
	virtual void onOutputUpdated();

protected:
	struct Output {
		using Ptr = std::shared_ptr<Output>;
		Output() = default;
		virtual const std::string& name();
		HJMediaType type{};
		int trackId{};
		WPtr plugin{};
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

	//	HJLooperThread::Handler::Ptr handler = nullptr
	//	bool needThread = false
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void afterInit() override;
	virtual void beforeDone() override;

	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) { }
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) { }
	virtual Input::Ptr getInput(size_t i_srcKeyHash) final;
	virtual Output::Ptr getOutput(size_t i_dstKeyHash) final;
//	virtual int flush();
	virtual int runTask() = 0;
	virtual void postTask();
	virtual void internalUpdated();
	virtual HJMediaFrame::Ptr receive(size_t i_srcKeyHash, size_t& o_size);
	virtual HJMediaFrame::Ptr preview(size_t i_srcKeyHash, size_t& o_size);
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame);
	static size_t getKeyHash(const std::string& i_name, HJMediaType i_type, int i_trackId = 0);

	InputMap m_inputs;
	HJSync m_inputSync;
	OutputMap m_outputs;
	HJSync m_outputSync;

	int m_runTaskId{ -1 };
	int m_streamIndex{ -1 };

	HJListener m_pluginListener;
	HJLooperThread::Handler::Ptr m_handler{};
	HJLooperThread::Ptr m_thread{};
	std::atomic<bool> m_quitting{};

protected:
	// TODO _每5秒日志_
	int64_t sample_size{};
	int64_t frame_size{};
	int64_t duration_count{ 1 };
	int64_t v_duration_count{ 1 };
	int64_t start_time{};
};

NS_HJ_END
