#pragma once

#include "HJPlugin.h"
#include "HJBaseCapture.h"

NS_HJ_BEGIN

class HJPluginCapturer : public HJPlugin
{
public:
	HJPluginCapturer(const std::string& i_name = "HJPluginCapturer", size_t i_identify = -1)
		: HJPlugin(i_name, i_identify) { }

protected:
	// HJStreamInfo::Ptr streamInfo
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void afterInit() override { }
	virtual int runTask() override;
	virtual void internalUpdated() override { }

	virtual HJBaseCapture::Ptr createCapturer() = 0;
	virtual int initCapturer(const HJStreamInfo::Ptr& i_streamInfo);

	HJBaseCapture::Ptr m_capturer{};
};

NS_HJ_END
