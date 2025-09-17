#pragma once

#include "HJPlugin.h"
#include "HJBaseDemuxer.h"
#include "HJRegularProc.h"
NS_HJ_BEGIN

class HJPluginDemuxer : public HJPlugin
{
public:
	HJPluginDemuxer(const std::string& i_name = "HJPluginDemuxer", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginDemuxer() {
		HJPluginDemuxer::done();
	}
	virtual int done() override;

	virtual int openURL(HJMediaUrl::Ptr i_url);
	virtual int reset(uint64_t i_delay);

protected:
	// HJMediaUrl::Ptr mediaUrl = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void afterInit() override;
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

	virtual HJBaseDemuxer::Ptr createDemuxer() = 0;
	virtual int initDemuxer(const HJMediaUrl::Ptr& i_url, uint64_t i_delay = 0);
	virtual void releaseDemuxer();
	virtual void setInfoMediaType();
	virtual int canDeliverToOutputs(const HJMediaFrame::Ptr& i_currentFrame);

	std::atomic<bool> m_resetting{};
	HJBaseDemuxer::Ptr m_demuxer{};
	HJSync m_demuxerSync;
	HJMediaUrl::Ptr m_mediaUrl{};
	HJMediaInfo::Ptr m_mediaInfo{};
	HJMediaFrame::Ptr m_currentFrame{};
	int64_t m_streamIndexOffset{};

    std::function<void()> m_demuxNotify = nullptr;
};

NS_HJ_END
