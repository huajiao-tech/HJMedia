#pragma once

#include "HJPlugin.h"
#include "HJAudioProcessor.h"
#include "HJAutoCacheController.h"
#include "HJCacheObserver.h"

NS_HJ_BEGIN
class HJGraph;
class HJPluginSpeedControl : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginSpeedControl);

	HJPluginSpeedControl(const std::string& name = "HJPluginSpeedControl", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(name, i_graphInfo) { }
	virtual ~HJPluginSpeedControl() {
		HJPluginSpeedControl::done();
	}

	virtual void onOutputUpdated() override;

	void setSpeed(float speed) {
		m_speed.store(speed, std::memory_order_relaxed);
	}
    float getSpeed() const {
		return m_speed.load(std::memory_order_relaxed);
	}

protected:
	// HJAudioInfo::Ptr audioInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual void onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;

	std::atomic<size_t> m_inputKeyHash{};
	std::atomic<size_t> m_outputKeyHash{};

	HJAudioProcessor::Ptr m_audioProcessor{};
	std::atomic<float> m_speed = { 1.0f };
	HJCacheObserver::Ptr m_cacheObserver{};
	HJAutoCacheController::Ptr m_autoCacheController{};
};

NS_HJ_END
