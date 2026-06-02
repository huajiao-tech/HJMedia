#pragma once

#include "HJPlugin.h"
#include "HJBaseMuxer.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

class HJPluginMuxer : public HJPlugin
{
public:
	using Wtr = std::weak_ptr<HJPluginMuxer>;

	HJPluginMuxer(const std::string& i_name = "HJPluginMuxer", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginMuxer() { done(); }

protected:
	// HJMediaUrl::Ptr mediaUrl
	// HJAudioInfo::Ptr audioInfo
	// HJVideoInfo::Ptr videoInfo
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int beforeDone() override;
	virtual void afterInit() override {}
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
	virtual int runTask(int64_t* o_delay = nullptr) override;

	virtual int runInit(const std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx);
	virtual HJBaseMuxer::Ptr createMuxer() = 0;
	virtual int initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx);
	virtual void releaseMuxer();
	virtual bool dropping(HJMediaFrame::Ptr i_mediaFrame);
	virtual void onWriteFrame(HJMediaFrame::Ptr& io_outFrame) {}
	virtual void setQuit();

	std::atomic<size_t> m_inputKeyHash{};
	HJBaseMuxer::Ptr m_muxer{};
	HJSync m_muxerSync;
	int m_mediaTypes{};
	bool m_dropping{ true };
};

NS_HJ_END
