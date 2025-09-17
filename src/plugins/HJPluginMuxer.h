#pragma once

#include "HJPlugin.h"
#include "HJBaseMuxer.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

class HJPluginMuxer : public HJPlugin
{
public:
	HJPluginMuxer(const std::string& i_name = "HJPluginMuxer", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginMuxer() {
		HJPluginMuxer::done();
	}

protected:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;

	virtual int initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx);
	virtual bool dropping(HJMediaFrame::Ptr i_mediaFrame);
	virtual void onWriteFrame(HJMediaFrame::Ptr& io_outFrame) { }

	std::atomic<size_t> m_inputKeyHash{};
	HJBaseMuxer::Ptr m_muxer{};
	int m_mediaTypes{};
	bool m_dropping{};
};

NS_HJ_END
