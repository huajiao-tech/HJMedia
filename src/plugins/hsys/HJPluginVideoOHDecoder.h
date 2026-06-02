#pragma once

#include "HJPluginCodec.h"
#include "HJOGRenderWindowBridge.h"

NS_HJ_BEGIN

// Video decoder plugin based on OH codec. See doc/HJPluginVideoOHDecoder.md for details.

class HJPluginVideoOHDecoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoOHDecoder);

	HJPluginVideoOHDecoder(const std::string& i_name = "HJPluginVideoOHDecoder", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginVideoOHDecoder() { done(); }

protected:
	// HJVideoInfo::Ptr videoInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// HJOGRenderWindowBridge::Ptr bridge
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJMediaType getType() override { return HJMEDIA_TYPE_VIDEO; }
	virtual HJBaseCodec::Ptr createCodec() override;
    virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo) override;

//    virtual int canDeliverToOutputs();

	int m_firstFrameCount{ 14 };
	HJOGRenderWindowBridge::Ptr m_bridge{};
};

NS_HJ_END
