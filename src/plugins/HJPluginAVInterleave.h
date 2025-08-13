#pragma once

#include "HJPlugin.h"

NS_HJ_BEGIN

class HJPluginAVInterleave : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginAVInterleave);

	HJPluginAVInterleave(const std::string& i_name = "HJPluginAVInterleave", size_t i_identify = -1)
		: HJPlugin(i_name, i_identify) {
	}

	virtual void onOutputUpdated() override {}

protected:
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask() override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

	std::atomic<size_t> m_inputAudioKeyHash{};
	std::atomic<size_t> m_inputVideoKeyHash{};
	std::atomic<int> m_flag{};
};

NS_HJ_END
