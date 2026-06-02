#pragma once

#include "HJPlugin.h"
#include "HJTimeline.h"
#include <mutex>
#if defined (HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#elif defined (WINDOWS)
#include "HJSharedMemory.h"
#endif

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJStatContext;

// See doc/HJPluginVideoRender.md for usage and lifecycle details.

class HJPluginVideoRender : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoRender);

	HJPluginVideoRender(const std::string& i_name = "HJPluginVideoRender", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginVideoRender() { done(); }

	void setEnableSEIUUids(const std::string& i_uuid, bool bKeyMustCb);
protected:
	// HJTimeline::Ptr timeline
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// HJOGRenderWindowBridge::Ptr bridge
	// bool onlyFirstFrame
	// HJDeviceType deviceType
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual int runFlush() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;

	std::atomic<size_t> m_inputKeyHash{};
	HJTimeline::Ptr m_timeline{};
	bool m_firstFrame{ true };
//	HJMediaFrame::Ptr m_currentFrame{};
#if defined (HarmonyOS)
	HJOGRenderWindowBridge::Ptr m_bridge{};
#elif defined (WINDOWS)
	HJSharedMemoryProducer::Ptr m_sharedMemoryProducer{};
#endif
	bool m_onlyFirstFrame{}; 
	HJDeviceType m_deviceType{ HJDEVICE_TYPE_NONE };

	std::weak_ptr<HJStatContext> m_statCtx;

	std::mutex m_seiMutex;
	std::map<std::string, bool> m_enableSEIUUids;
private:
	void priStatCodecType();
	void priStatDelay(const std::shared_ptr<HJMediaFrame> &i_frame);
	int m_cacheCodecType{ -1 };
	void procSEI(const HJMediaFrame::Ptr& mvf);

private:
	virtual std::tuple<int, int64_t> syncVideoFrame(std::string& route, HJMediaFrame::Ptr& currentFrame);
	virtual int postprocessVideoFrame(std::string& route, HJMediaFrame::Ptr& currentFrame);
};

NS_HJ_END
