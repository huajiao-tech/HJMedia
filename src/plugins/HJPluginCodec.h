#pragma once

#include "HJPlugin.h"
#include "HJBaseCodec.h"

NS_HJ_BEGIN

class HJPluginCodec : public HJPlugin
{
public:
	HJPluginCodec(const std::string& i_name = "HJPluginCodec", size_t i_identify = -1)
		: HJPlugin(i_name, i_identify) { }

protected:
	// HJStreamInfo::Ptr streamInfo
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
//	virtual int flush() override;

	virtual HJBaseCodec::Ptr createCodec() = 0;
	virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo);
//	virtual void resetCodec(HJMediaFrame::Ptr frame) { }

	HJMediaType m_mediaType{};
	std::atomic<size_t> m_inputKeyHash{};
	HJBaseCodec::Ptr m_codec{};
	int m_frameFlag{};
};

NS_HJ_END
