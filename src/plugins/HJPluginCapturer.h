#pragma once

#include "HJPlugin.h"
#include "HJBaseCapture.h"

NS_HJ_BEGIN

class HJPluginCapturer : public HJPlugin
{
public:
	HJPluginCapturer(const std::string& i_name = "HJPluginCapturer", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginCapturer() {
		HJPluginCapturer::done();
	}

protected:
	// HJStreamInfo::Ptr streamInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void afterInit() override { }
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual void onOutputUpdated() override {}

	virtual HJBaseCapture::Ptr createCapturer() = 0;
	virtual int initCapturer(const HJStreamInfo::Ptr& i_streamInfo);

	HJBaseCapture::Ptr m_capturer{};
};

NS_HJ_END
