//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseMuxer.h"
#include "HJBaseCodec.h"
#include "HJRTMPAsyncWrapper.h"
#include "HJXIOFile.h"
#include "HJRTMPPacketManager.h"
#include "HJStatContext.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJRTMPMuxer : public HJBaseMuxer, public HJRTMPWrapperDelegate
{
public:
    using Ptr = std::shared_ptr<HJRTMPMuxer>;
	HJRTMPMuxer();
	HJRTMPMuxer(HJListener listener);
	virtual ~HJRTMPMuxer();

	virtual int init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts = nullptr) override;
	virtual int init(const std::string url, int mediaTypes = HJMEDIA_TYPE_AV, HJOptions::Ptr opts = nullptr) override;
	virtual int addFrame(const HJMediaFrame::Ptr& frame) override;
	virtual int writeFrame(const HJMediaFrame::Ptr& frame) override;
	/**
	* must call done before destruct, Delegate run will hold Ptr, after run done, Ptr will be destructed
	*/
	virtual void done() override;
	//
	virtual void setQuit(const bool isQuit = true) override;
	//
	HJRTMPStreamStats getStats() {
		if (m_packetManager) {
			return m_packetManager->getStats();
		}
		return HJRTMPStreamStats();
	}
	void setNetBlock(const bool block);
	void setNetSimulater(int bitrate);
private:
	/**
	* HJRTMPWrapperDelegate
	*/
	virtual int onRTMPWrapperNotify(HJNotification::Ptr ntf) override;
	virtual HJBuffer::Ptr onAcquireMediaTag(int64_t timeout, bool isHeader) override;
	//
	int addRTMPPacket(HJMediaFrame::Ptr frame, int64_t tsOffset = 0);
	int gatherMediaInfo(const HJMediaFrame::Ptr& frame);
	int64_t waitStartDTSOffset(HJMediaFrame::Ptr frame);
	// stats
	void notifyStatInfo();
	void notifyStatLiveInfo();
	void notifyStatDoneInfo();
private:
	HJListener						m_listener = nullptr;
	std::string						m_url{ "rtmp://localhost/live" };
	int 							m_mediaTypes{ HJMEDIA_TYPE_NONE };
    HJOptions::Ptr					m_opts = nullptr;
	HJRTMPAsyncWrapper::Ptr			m_rtmpWrapper = nullptr;
	//
	//std::mutex						m_packetMutex;
	HJRTMPPacketManager::Utr		m_packetManager = {nullptr};
	bool							m_isEnhancedRtmp{ false };
	//
	int64_t							m_startDTSOffset = HJ_NOTS_VALUE;
	std::deque<HJMediaFrame::Ptr>	m_framesQueue;

	//optionals
	std::string						m_localUrl{ "" };
	HJXIOFile::Ptr					m_localFile = nullptr;

	//std::shared_ptr<HJRTMPSink>		m_sink = nullptr;
	
	HJTrackHolderMap				m_tracks;
	HJNipMuster::Ptr				m_nipMuster = nullptr;
	//
	std::weak_ptr<HJStatContext>	m_statCtx;
	HJPeriodicRunner				m_periodicRunner;
	//int64_t							m_lastStatTime = HJ_NOTS_VALUE;
};
NS_HJ_END
