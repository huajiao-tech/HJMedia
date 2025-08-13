#pragma once

#include "HJPlugin.h"
#include "HJBaseMuxer.h"

NS_HJ_BEGIN

class HJPluginMuxer : public HJPlugin
{
public:
	HJPluginMuxer(const std::string& i_name = "HJPluginMuxer", size_t i_identify = -1)
		: HJPlugin(i_name, i_identify) { }

protected:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJLooperThread::Ptr thread = nullptr
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask() override;

	virtual int initMuxer(std::string i_url, int i_mediaTypes);
	virtual bool dropping(HJMediaFrame::Ptr i_mediaFrame);
	virtual void onWriteFrame(HJMediaFrame::Ptr& io_outFrame) { }

	std::atomic<size_t> m_inputKeyHash{};
	HJBaseMuxer::Ptr m_muxer{};
	int m_mediaTypes{};
	bool m_dropping{};
};

NS_HJ_END
