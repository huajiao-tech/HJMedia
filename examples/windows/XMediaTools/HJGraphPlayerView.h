//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJMedias.h"
#include "HJCores.h"
#include "HJGraph.h"
#include "HJGuis.h"
#include "HJImguiUtils.h"
#include <algorithm>
#include <cctype>
//#include "HJMediaPlayer.h"
//#include "HJMediaMuxer.h"
//#include "HJJPusher.h"
//#include "HJHLSParser.h"

NS_HJ_BEGIN
#define HJ_TRY_PLAY_CNT_MAX	10

class HJEventBus;
class HJGraphLivePlayer;
class HJGraphVodPlayer;
//***********************************************************************************//
/**
*         (*m_params)["stat"] = getStateString();
		(*m_params)["calc"] = m_calcName;
		(*m_params)["dura"] = curDuration;
		(*m_params)["max"] = durationMax;
		(*m_params)["min"] = durationMin;
		(*m_params)["avg"] = durationAvg;
		(*m_params)["stut"] = stutterRatio;
		(*m_params)["slop"] = m_slope;
		(*m_params)["speed"] = m_lastPlaybackSpeed;
		//(*m_params)["edura"] = expectTargetDuration;
		//(*m_params)["elimit"] = expectLimitedRange;
		//(*m_params)["rdura"] = m_currentTargetDuration;
		//(*m_params)["rlimit"] = m_currentLimitedRange;
*/
typedef struct HJPlayerDelayParams
{
	std::string stat = "";
	std::string calc = "";
	int64_t  dura = 0;
	int64_t  max = 0;
	int64_t  min = 0;
	int64_t  avg = 0;
	float    stut = 0.0;
	float    slop = 0.0;
	float    speed = 0.0;
	int64_t  edura = 0;
	int64_t  elimit = 0;
	int64_t  rdura = 0;
	int64_t  rlimit = 0;
	int64_t  watch = 0;
} HJPlayerDelayParams;

typedef struct HJPlayerPluginInfos
{
	int64_t audio_dropping = 0;
	int64_t video_dropping = 0;
	int64_t audio_decoder = 0;
	int64_t audio_render = 0;
	int64_t video_main_decoder = 0;
	int64_t video_main_render = 0;
	int64_t video_soft_decoder = 0;
	int64_t video_soft_render = 0;
	int64_t time = 0;
} HJPlayerPluginInfos;

typedef struct HJGraphBPlotInfos
{
	HJBuffer::Ptr xsbuffer = nullptr;
	double xsMin = 0.0;
	double xsMax = 1000.0;
	std::unordered_map<std::string, HJBuffer::Ptr> ysbuffers;
	std::unordered_map<std::string, double> ysMaxs;
	int64_t size = 0;
} HJGraphBPlotInfos;

class HJGraphPlayerView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJGraphPlayerView>;
	enum PlayerTypeHint
	{
		kPlayerTypeAuto = 0,
		kPlayerTypeVod = 1,
		kPlayerTypeLive = 2,
	};
	explicit HJGraphPlayerView();
	virtual ~HJGraphPlayerView();

	static HJGraphType resolveGraphTypeForUrl(const std::string& media_url, int player_type_hint = kPlayerTypeAuto)
	{
		if (player_type_hint == kPlayerTypeVod) {
			return HJGraphType_VOD;
		}
		if (player_type_hint == kPlayerTypeLive) {
			return HJGraphType_LIVESTREAM;
		}

		auto to_lower_copy = [](const std::string& value) {
			std::string lowered = value;
			std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
				return static_cast<char>(std::tolower(ch));
			});
			return lowered;
		};
		auto starts_with = [](const std::string& value, const std::string& prefix) {
			return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
		};
		auto contains = [](const std::string& value, const std::string& token) {
			return value.find(token) != std::string::npos;
		};
		auto unwrap_media_url = [&](const std::string& value) {
			std::string unwrapped = value;
			while (true) {
				const std::string prefix = to_lower_copy(HJUtilitys::getPrefix(unwrapped));
				if (prefix == "exasync" || prefix == "hjds") {
					unwrapped = unwrapped.substr(prefix.size() + 1);
					continue;
				}
				return unwrapped;
			}
		};
		auto is_likely_local_path = [&](const std::string& value) {
			if (value.size() > 2 && std::isalpha(static_cast<unsigned char>(value[0])) && value[1] == ':' &&
				(value[2] == '/' || value[2] == '\\')) {
				return true;
			}
			return starts_with(value, "/") || starts_with(value, "./") || starts_with(value, "../") || starts_with(value, "file://");
		};

		const std::string unwrapped_url = unwrap_media_url(media_url);
		const std::string core_url = to_lower_copy(HJUtilitys::getCoreUrl(unwrapped_url));
		const std::string prefix = to_lower_copy(HJUtilitys::getPrefix(core_url));
		const std::string suffix = to_lower_copy(HJUtilitys::getSuffix(core_url));
		const bool is_local_path = is_likely_local_path(core_url);
		const bool is_replay_like =
			contains(core_url, "replay") ||
			contains(core_url, "/record/") ||
			contains(core_url, "record/main") ||
			contains(core_url, "playback");
		const bool is_live_like =
			contains(core_url, "live-pull") ||
			contains(core_url, "live_push") ||
			contains(core_url, "live_huajiao") ||
			contains(core_url, "/live/") ||
			contains(core_url, "httpflv") ||
			contains(core_url, "livestream");

		if (prefix == "rtmp" || prefix == "rtsp") {
			return HJGraphType_LIVESTREAM;
		}

		if (suffix == ".mp4" || suffix == ".mov" || suffix == ".m4a" || suffix == ".m4v" ||
			suffix == ".mkv" || suffix == ".webm" || suffix == ".mp3" || suffix == ".aac" ||
			suffix == ".wav" || suffix == ".ogg" || suffix == ".ts" || suffix == ".m4s") {
			return HJGraphType_VOD;
		}

		if (suffix == ".m3u8") {
			if (is_replay_like || is_local_path) {
				return HJGraphType_VOD;
			}
			if (is_live_like) {
				return HJGraphType_LIVESTREAM;
			}
		}

		if (suffix == ".flv") {
			if (is_local_path || is_replay_like) {
				return HJGraphType_VOD;
			}
			return HJGraphType_LIVESTREAM;
		}

		if (is_replay_like || is_local_path) {
			return HJGraphType_VOD;
		}
		return HJGraphType_LIVESTREAM;
	}

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
	virtual int drawPlayerStat(const HJRectf& rect);
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
	int onSwitchAudioTrack(const int trackID);
	void onTest();
	void onTest1();
	void onTestHLS();
	void onTestByteBuffer();
	void onNetBlock(const std::string& tips);
	void onDataBlock(const std::string& tips);
	void onNetBitrate(int bitrate);
	//
	void onCreatePlayer();
	void onCreateVodPlayer();
	void onClosePlayer();

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

	//
	void addDelayParams(std::shared_ptr<HJPlayerDelayParams>& params) {
		HJ_AUTOU_LOCK(m_mutex);
		if (m_delayParams.size() > 5000) {
			m_delayParams.erase(m_delayParams.begin());
		}
		m_delayParams.push_back(params);
	}
	HJGraphBPlotInfos getDelayParams();

	void addPluginInfos(std::shared_ptr<HJPlayerPluginInfos>& infos) {
		HJ_AUTOU_LOCK(m_mutex);
		if (m_pluginInfos.size() > 5000) {
			m_pluginInfos.erase(m_pluginInfos.begin());
		}
		m_pluginInfos.push_back(infos);
	}

	void registerHandlers(const std::shared_ptr<HJEventBus> i_bus);
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
	int						m_playerTypeHint = kPlayerTypeAuto;
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
	int64_t					m_mediaRepeat{ 0 };
	std::string				m_muxerUrl = "E:/movies/outs/";
	std::string				m_pushUrl = "rtmp://live-push-0.test.huajiao.com/main/kjl979?auth_key=1755677478-0-0-5427cdf5460ca7a8f9884ebe72ab20c7";
	//std::string				m_pushUrl = "rtmp://localhost/live/livestream";
	std::string				m_thumbUrl = "";
	std::shared_ptr<HJGraphLivePlayer>	m_player = nullptr;
	std::shared_ptr<HJGraphVodPlayer>	m_vodPlayer = nullptr;
	HJMediaInfo::Ptr		m_minfo = nullptr;
	HJMediaFrame::Ptr		m_mvf = nullptr;
	//HJImageWriter::Ptr     m_imageWriter = nullptr;
	int						m_imageIdx = 0;
	HJProgressInfo::Ptr		m_progInfo = nullptr;
	HJStreamInfoList		m_minfos;
	std::recursive_mutex    m_mutex;
	int						m_selectedAudioTrackID = -1;
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

	//
	std::vector<std::shared_ptr<HJPlayerDelayParams>> m_delayParams;
	std::vector<std::shared_ptr<HJPlayerPluginInfos>> m_pluginInfos;
	std::string				m_statName = "";
	std::string				m_calcName = "";
	float					m_stutterRatio = 0.0;
	float					m_fillRefValue = 0.0f;
};

NS_HJ_END
