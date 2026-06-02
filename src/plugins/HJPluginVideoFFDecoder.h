#pragma once

#include "HJPluginCodec.h"

NS_HJ_BEGIN

// Video decoder plugin based on FFmpeg. See doc/HJPluginVideoFFDecoder.md for details.

class HJPluginVideoFFDecoder : public HJPluginCodec
{
public:
	HJ_DEFINE_CREATE(HJPluginVideoFFDecoder);

	HJPluginVideoFFDecoder(const std::string& i_name = "HJPluginVideoFFDecoder", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginCodec(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginVideoFFDecoder() {	done();	}

protected:
	// HJVideoInfo::Ptr videoInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	// bool onlyFirstFrame
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual HJMediaType getType() override { return HJMEDIA_TYPE_VIDEO; }
	virtual HJBaseCodec::Ptr createCodec() override;
	virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo) override;

//	virtual int canDeliverToOutputs();

	int m_firstFrameCount{ 4 };
	bool m_onlyFirstFrame{};
};

NS_HJ_END
