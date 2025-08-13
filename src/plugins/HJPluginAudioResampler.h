#pragma once

#include "HJPlugin.h"
#include "HJAudioConverter.h"
#include "HJAudioFifo.h"

NS_HJ_BEGIN

class HJPluginAudioResampler : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioResampler);

	HJPluginAudioResampler(const std::string& name = "HJPluginAudioResampler", size_t identify = -1)
		: HJPlugin(name, identify) { }

	virtual void onOutputUpdated() override {}

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask() override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

	std::atomic<size_t> m_inputKeyHash{};
	HJAudioInfo::Ptr m_audioInfo{};
	HJAudioConverter::Ptr m_converter{};
	HJAudioFifo::Ptr m_fifo{};
};

NS_HJ_END
