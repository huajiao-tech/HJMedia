#pragma once

#include "HJPlugin.h"
#include "HJTimeline.h"
#if defined (HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJStatContext;

class HJPluginVideoRender : public HJPlugin
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoRender);

	HJPluginVideoRender(const std::string& i_name = "HJPluginVideoRender", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginVideoRender() {
		HJPluginVideoRender::done();
	}
	virtual int deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr) override;

	virtual void onTimelineUpdated() {
		internalUpdated();
	}
/*
	virtual int start();
	virtual void stop();
	virtual void pause();
*/
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
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;

	virtual void setInfoFrameSize(size_t i_size);

	std::atomic<size_t> m_inputKeyHash{};
	HJTimeline::Ptr m_timeline{};
	bool m_firstFrame{ true };
	HJMediaFrame::Ptr m_currentFrame{};
#if defined (HarmonyOS)
	HJOGRenderWindowBridge::Ptr m_bridge{};
#endif
	bool m_onlyFirstFrame{}; 
	HJDeviceType m_deviceType{ HJDEVICE_TYPE_NONE };

	std::weak_ptr<HJStatContext> m_statCtx;

private:
	void priStatCodecType();
	void priStatDelay(const std::shared_ptr<HJMediaFrame> &i_frame);
	int m_cacheCodecType{ -1 };
	bool m_bFirstFrame = true;
};

NS_HJ_END
