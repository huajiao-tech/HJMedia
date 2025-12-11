#pragma once

#include "HJPluginCodec.h"
#include "HJNotify.h"

NS_HJ_BEGIN

using HJOHSurfaceCb = std::function<int(void* i_window, int i_width, int i_height, bool i_bCreate)>;

class HJPluginVideoOHEncoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoOHEncoder);

	HJPluginVideoOHEncoder(const std::string& i_name = "HJPluginVideoOHEncoder", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_graphInfo) { }
	virtual ~HJPluginVideoOHEncoder() {
		HJPluginVideoOHEncoder::done();
	}

	int adjustBitrate(int i_newBitrate);

protected:
	// HJHOSurfaceCb surfaceCb
	// HJStreamInfo::Ptr streamInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void afterInit() override { }
	virtual int runTask(int64_t* o_delay = nullptr) override;
//	virtual void internalUpdated(int64_t i_delay = 0) override { }
	virtual HJBaseCodec::Ptr createCodec() override;
	virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo) override;
//	virtual void resetCodec(HJMediaFrame::Ptr frame) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

	void requireSEI(HJMediaFrame::Ptr& mvf);
private:
	HJOHSurfaceCb m_surfaceCb{};
	void* m_nativeWindow{};
	int m_bitrate{};
	HJListener m_listener{};
};

NS_HJ_END
