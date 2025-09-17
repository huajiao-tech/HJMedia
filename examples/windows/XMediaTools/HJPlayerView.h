//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJMedias.h"
#include "HJCores.h"
#include "HJGuis.h"
#include "HJImguiUtils.h"
#include "HJMediaPlayer.h"
//#include "HJMediaMuxer.h"
//#include "HJJPusher.h"
//#include "HJHLSParser.h"

NS_HJ_BEGIN
#define HJ_TRY_PLAY_CNT_MAX	10
//***********************************************************************************//
typedef struct HJJPBPlotInfos
{
	HJBuffer::Ptr xsbuffer = nullptr;
	double xsMin = 0.0;
	double xsMax = 1000.0;
	std::unordered_map<std::string, HJBuffer::Ptr> ysbuffers;
	std::unordered_map<std::string, double> ysMaxs;
	int64_t size = 0;
} HJJPBPlotInfos;

class HJPlayerView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJPlayerView>;
	explicit HJPlayerView();
	virtual ~HJPlayerView();

	virtual int init(const std::string info) override;
	virtual int draw(const HJRectf& rect) override;
	virtual void done();
	//
	virtual int drawFrame(const HJRectf& rect);
	virtual int drawStatusBar(const HJRectf& rect);
	virtual int drawPlayerBar(const HJRectf& rect);
	virtual int drawMediaInfo(const HJRectf& rect);
	virtual int drawPusherStat(const HJRectf& rect);
	virtual int drawMuxerStat(const HJRectf& rect);
private:
	void onClickPrepare();
	void onClickPlay(const std::string& tips);
	void onStart();
	void onSetWindow();
	void onSeek(const int64_t pos);
	void onCompleted();
	void onWriteThumb(const HJMediaFrame::Ptr& frame);
	int onProcessError(const int err);
	int onMuxer(const HJMediaFrame::Ptr& frame);
	int onPusher(const HJMediaFrame::Ptr& frame);
	int onPusher2(const HJMediaFrame::Ptr& frame);
	void onClickMute(const std::string& tips);
	void onClickVolume(const float volume);
	void onTest();
	void onTest1();
	void onTestHLS();
	void onTestByteBuffer();
	void onNetBlock(const std::string& tips);
	void onDataBlock(const std::string& tips);
	void onNetBitrate(int bitrate);
	//
	void setMediaInfo(const HJMediaInfo::Ptr minfo) {
		HJ_AUTO_LOCK(m_mutex);
		m_minfo = minfo;
		return;
	}
	HJMediaInfo::Ptr getMediaInfo() {
		HJ_AUTO_LOCK(m_mutex);
		return m_minfo;
	}
	void setProgressInfo(HJProgressInfo::Ptr pinfo) {
		HJ_AUTO_LOCK(m_mutex);
		m_progInfo = pinfo;
		return;
	}
	HJProgressInfo::Ptr getProgressInfo() {
		HJ_AUTO_LOCK(m_mutex);
		return m_progInfo;
	}
protected:
	HJImageButton::Ptr		m_btnFileDialog = nullptr;
	HJFileDialogView::Ptr	m_viewFileDialog = nullptr;
	HJImageButton::Ptr		m_btnPrepare = nullptr;
	HJComplexImageButton::Ptr m_btnRun = nullptr;
	HJCombo::Ptr			m_comboSpeed = nullptr;
	HJComplexImageButton::Ptr m_btnMute = nullptr;
	HJImageButton::Ptr		m_btnClose = nullptr;
	HJImageButton::Ptr		m_btnPush = nullptr;
	bool					m_isAsyncUrl = false;
	bool					m_showMuxer = false;
	bool					m_showPusher = false;
	bool					m_showMInfoWindow = false;
	bool					m_thumbnail = false;
	//HJImageView::Ptr		m_imgView = nullptr;
	HJFrameView::Ptr		m_mvfView = nullptr;
	HJProgressView::Ptr	m_progressView = nullptr;
	HJLoadingView::Ptr		m_loadingView = nullptr;
	HJSlider::Ptr			m_volumeView = nullptr;
	//
	HJComplexImageButton::Ptr m_btnNetBlock = nullptr;
	HJComplexImageButton::Ptr m_btnDataBlock = nullptr;
	HJCombo::Ptr			  m_comboNetBitrate = nullptr;
	std::recursive_mutex    m_statMutex;
	//std::vector<HJJPusherStatParams> m_jpStatParams;
	std::vector<HJRTMPStreamStats> m_muxerStatParams;
	//
	std::string				m_mediaUrl = "";
	std::string				m_muxerUrl = "E:/movies/outs/";
	std::string				m_pushUrl = "rtmp://live-push-0.test.huajiao.com/main/kjl979?auth_key=1755677478-0-0-5427cdf5460ca7a8f9884ebe72ab20c7";
	//std::string				m_pushUrl = "rtmp://localhost/live/livestream";
	std::string				m_thumbUrl = "";
	HJMediaPlayer::Ptr		m_player = nullptr;
	HJMediaInfo::Ptr		m_minfo = nullptr;
	HJMediaFrame::Ptr		m_mvf = nullptr;
	//HJImageWriter::Ptr     m_imageWriter = nullptr;
	int						m_imageIdx = 0;
	HJProgressInfo::Ptr		m_progInfo = nullptr;
	HJStreamInfoList		m_minfos;
	std::recursive_mutex    m_mutex;
	int64_t					m_curPos = 0;
	//HJImageWriter::Ptr     m_imageWriter = nullptr;
	int64_t					m_timeout = 3000 * 1000;	//us
	int						m_tryCnt = 0;				//max

	//HJMediaMuxer::Ptr		m_muxer = nullptr;
	HJFFMuxer::Ptr			m_muxer = nullptr;
	//HJJPusher::Ptr			m_pusher = nullptr;
	HJRTMPMuxer::Ptr 		m_rtmpMuxer = nullptr;
	//HJHLSParser::Ptr		m_hlsParser = nullptr;

	HJSemaphore            m_sem = HJSemaphore(0, false);
};


NS_HJ_END
