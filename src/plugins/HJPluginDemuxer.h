#pragma once

#include <atomic>

#include "HJPlugin.h"
#include "HJBaseDemuxer.h"
#include "HJRegularProc.h"

NS_HJ_BEGIN

// Demuxer plugin bridging HJBaseDemuxer outputs to graph; see doc/HJPluginDemuxer.md for usage and lifecycle details.

class HJPluginDemuxer : public HJPlugin
{
public:
	using Wtr = std::weak_ptr<HJPluginDemuxer>;
	enum class InitReason {
		InternalInit,
		OpenURL,
		Reset,
	};

	HJPluginDemuxer(const std::string& i_name = "HJPluginDemuxer", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginDemuxer() { done(); }
	virtual int flush(size_t i_srcKeyHash) override { return HJErrNotSupport; }

	virtual int openURL(HJMediaUrl::Ptr i_url);
	virtual int reset(uint64_t i_delay);
	virtual int seek(int64_t i_timestamp);
	virtual int switchAudioTrack(int i_trackId);
	HJMediaInfo::Ptr getMediaInfo();
	int64_t getDuration() const {
		return m_duration.load();
	}
	uint32_t getMediaType();

protected:
	// HJMediaUrl::Ptr mediaUrl = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual int beforeDone() override;
	virtual void afterInit() override {}
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual int runEof(int i_streamIndex) override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

	virtual int postInit(const HJMediaUrl::Ptr& i_url, int i_initIndex, InitReason i_reason, int64_t i_delay = 0);
	virtual int runInit(const HJMediaUrl::Ptr& i_url, int i_initIndex, InitReason i_reason);
	virtual int runSeek(int64_t i_timestamp);
	virtual int runSwitchAudioTrack(int i_trackId);
	virtual HJBaseDemuxer::Ptr createDemuxer() = 0;
	virtual int initDemuxer(const HJMediaUrl::Ptr& i_url, int i_initIndex);
	virtual int quitDemuxer();
	virtual void releaseDemuxer();

protected:
	virtual void procSEI(const HJMediaFrame::Ptr& mvf);

protected:
	HJBaseDemuxer::Ptr m_demuxer{};
	std::atomic<bool> m_quittingDemuxer{};
	int m_quitIndex{ -1 };
	int m_initIndex{};
	HJSync m_demuxerSync;

	HJMediaUrl::Ptr m_mediaUrl{};
	HJMediaFrame::Ptr m_currentFrame{};
	HJSync m_currentFrameSync;
	int64_t m_streamIndexOffset{};
	uint32_t m_mediaType{};
	std::atomic<int64_t> m_duration{};
	int m_audioTrackId{ -1 };
	//	std::function<void()> m_demuxNotify{};
	int m_runInitId{ -1 };
	int m_runSeekId{ -1 };
	int m_runSwitchAudioTrackId{ -1 };
	bool m_hasReportedStreamOpened{ false };

private:
	virtual HJMediaFrame::Ptr loadCurrentFrame();
	virtual std::tuple<int, HJMediaFrame::Ptr> getOutputFrame(std::string& route);
	virtual void storeCurrentFrame(HJMediaFrame::Ptr& frame);
};

NS_HJ_END
