#pragma once

#include "HJPlugin.h"
#include "HJAudioConverter.h"
#include "HJAudioFifo.h"

NS_HJ_BEGIN

class HJPluginAudioResampler : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioResampler);

	HJPluginAudioResampler(const std::string& name = "HJPluginAudioResampler", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(name, i_graphInfo) { }
	virtual ~HJPluginAudioResampler() {
		HJPluginAudioResampler::done();
	}

	virtual void onOutputUpdated() override;

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// bool fifo
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;

	std::atomic<size_t> m_inputKeyHash{};
	std::atomic<size_t> m_outputKeyHash{};
	HJAudioInfo::Ptr m_audioInfo{};
	HJAudioConverter::Ptr m_converter{};
	HJAudioFifo::Ptr m_fifo{};
};

NS_HJ_END
