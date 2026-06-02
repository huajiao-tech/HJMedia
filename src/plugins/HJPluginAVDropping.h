#pragma once

#include "HJPlugin.h"

NS_HJ_BEGIN

// HJPluginAVDropping: see doc/HJPluginAVDropping.md for details.

class HJPluginAVDropping : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginAVDropping);

	HJPluginAVDropping(const std::string& i_name = "HJPluginAVDropping", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginAVDropping() { done(); }

	virtual int setDropPerameter(int64_t i_maxAudioDuration, int64_t i_minAudioDuration);

protected:
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// HJGraph graph
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

//	virtual int canDeliverToOutputs(HJMediaFrame::Ptr i_mediaFrame);

	std::atomic<size_t> m_inputKeyHash{};
	std::atomic<size_t> m_outputAudioKeyHash{};
	std::atomic<size_t> m_outputVideoKeyHash{};
	int64_t m_maxAudioDuration{ 10000 };
	int64_t m_minAudioDuration{ 5000 };

	HJMediaFrame::Ptr m_currentFrame{};

private:
	bool tryDropFrames(std::string& route, size_t inputKeyHash, int64_t& size);
};

NS_HJ_END
