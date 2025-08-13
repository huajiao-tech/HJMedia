#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

using HJOHSurfaceCb = std::function<int(void* i_window, int i_width, int i_height, bool i_bCreate)>;

class HJPluginVideoOHEncoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoOHEncoder);

	HJPluginVideoOHEncoder(const std::string& i_name = "HJPluginVideoOHEncoder", size_t i_identify = -1)
		: HJPluginCodec(i_name, i_identify) {
		m_mediaType = HJMEDIA_TYPE_VIDEO;
	}

//	virtual void onOutputUpdated() override { }

	int adjustBitrate(int i_newBitrate);

protected:
	// HJHOSurfaceCb surfaceCb
	// HJStreamInfo::Ptr streamInfo
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void afterInit() override { }
	virtual int runTask() override;
	virtual void internalUpdated() override { }
	virtual HJBaseCodec::Ptr createCodec() override;
	virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo) override;
//	virtual void resetCodec(HJMediaFrame::Ptr frame) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

private:
	HJOHSurfaceCb m_surfaceCb{};
	void* m_nativeWindow{};
	int m_bitrate{};
};

NS_HJ_END
