#pragma once

#include "HJPlugin.h"
#include "HJAudioConverter.h"
#include "HJAudioFifo.h"

NS_HJ_BEGIN

// HJPluginAudioResampler: see doc/HJPluginAudioResampler.md

class HJPluginAudioResampler : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioResampler);

	HJPluginAudioResampler(const std::string& name = "HJPluginAudioResampler", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(name, i_identify, i_graphInfo) {}
	virtual ~HJPluginAudioResampler() { done(); }

//	virtual void onOutputUpdated() override;

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// bool fifo
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
//	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) override;
	virtual int runFlush() override;
	virtual int runTask(int64_t* o_delay = nullptr) override;

	std::atomic<size_t> m_inputKeyHash{};
//	std::atomic<size_t> m_outputKeyHash{};
	HJAudioInfo::Ptr m_audioInfo{};
	HJAudioConverter::Ptr m_converter{};
	HJAudioFifo::Ptr m_fifo{};
	int64_t m_last_time{HJ_NOTS_VALUE};

private:
//	virtual int processInputFrame(std::string& route, HJMediaFrame::Ptr& inFrame);
//	virtual int drainFifoFrames(std::string& route, int streamIndex);
	virtual std::tuple<int, HJMediaFrame::Ptr, bool> processMediaFrame(std::string& route, HJMediaFrame::Ptr& inFrame);
	virtual std::tuple<int, HJMediaFrame::Ptr> getAndDeliverOutputFrame(std::string& route);
};

NS_HJ_END
