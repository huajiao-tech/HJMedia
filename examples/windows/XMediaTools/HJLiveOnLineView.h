#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "HJGuis.h"
#include "HJImageWriter.h"
#include "HJImguiUtils.h"
#include "HJLiveOnLineThumbCapture.h"
#include "HJMediaPlayer.h"

#ifndef HJ_XMEDIATOOLS_HAS_LIBCURL
#define HJ_XMEDIATOOLS_HAS_LIBCURL 0
#endif

#if HJ_XMEDIATOOLS_HAS_LIBCURL
#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#include "HJNets.h"
#else
NS_HJ_BEGIN
struct HJLiveResourceItem {
    std::string m_uid;
    std::string m_liveid;
    std::string m_sn;
    std::string m_m3u8;
    std::string m_flvurl;
    std::string m_link;
    std::string m_nickname;
    std::string m_image_url;
    std::string m_stream_status;
    std::string m_stream_error;
};
NS_HJ_END
#endif

NS_HJ_BEGIN

class HJLiveOnLineView : public HJView
{
public:
    HJ_DECLARE_PUWTR(HJLiveOnLineView);

    explicit HJLiveOnLineView();
    virtual ~HJLiveOnLineView();

    virtual int init(const std::string info) override;
    virtual int draw(const HJRectf& rect) override;

private:
    struct LiveCardState {
        HJLiveResourceItem m_item{};
        std::string m_stream_url;
        std::string m_thumb_cache_path;
        std::string m_thumb_error;
        HJImage::Ptr m_thumbnail{};
        bool m_thumb_loading{ false };
        bool m_thumb_ready{ false };
        bool m_thumb_failed{ false };
    };

    struct ThumbCaptureTaskState {
        bool m_active{ false };
        int m_level_index{ 0 };
        std::string m_level;
        std::string m_url;
        std::string m_output_dir;
        HJThumbCaptureSchedule m_schedule{};
        HJImageWriter::Ptr m_image_writer{};
        std::string m_hint_text;
        bool m_hint_is_error{ false };
    };

    int startFetch();
    void stopPlayback();
    int startPlayback(size_t i_index);
    void seekPlayback(int64_t i_pos);
    void applyPlaybackVolume();
    void setStatusText(const std::string& i_status, int i_result = HJ_OK);
    std::string currentLoginType() const;
    std::string currentFilter() const;
    bool isFetchSessionCurrent(uint64_t i_session_id) const;
    bool isPlaybackSessionCurrent(uint64_t i_session_id) const;
    void applyFetchResult(uint64_t i_session_id,
                          const std::vector<HJLiveResourceItem>& i_items,
                          bool i_prefer_flv);
    void applyFetchError(uint64_t i_session_id, const std::string& i_error);
    void updateThumbnailState(uint64_t i_session_id,
                              size_t i_index,
                              bool i_ready,
                              bool i_failed,
                              const std::string& i_error);
    int onPlayerNotify(uint64_t i_session_id, const HJNotification::Ptr& i_ntf);
    int onPlayerFrame(uint64_t i_session_id, const HJMediaFrame::Ptr& i_frame, std::string url);
    int drawToolbar();
    int drawContent();
    int drawCardGrid(const ImVec2& i_size);
    int drawPlayerPanel(const ImVec2& i_size);
    bool matchesFilter(const LiveCardState& i_card, const std::string& i_filter) const;
    void resetThumbCaptureTask();

    std::string m_name;
    mutable std::recursive_mutex m_mutex;
    std::array<char, 256> m_user_buffer{};
    std::array<char, 256> m_password_buffer{};
    std::array<char, 512> m_page_url_buffer{};
    std::array<char, 256> m_filter_buffer{};
    int m_login_type_index{ 0 };
    int m_page_url_index{ -1 };
    bool m_prefer_flv{ true };
    bool m_fetching{ false };
    uint64_t m_fetch_session_id{ 0 };
    uint64_t m_playback_session_id{ 0 };
    std::vector<std::string> m_page_urls;
    std::vector<LiveCardState> m_cards;
    int m_selected_index{ -1 };
    std::string m_status_text;
    int m_last_result{ HJ_OK };
    HJMediaPlayer::Ptr m_player{};
    HJFrameView::Ptr m_frame_view{};
    HJProgressView::Ptr m_progress_view{};
    HJMediaFrame::Ptr m_video_frame{};
    HJMediaInfo::Ptr m_player_media_info{};
    HJProgressInfo::Ptr m_player_progress_info{};
    std::string m_player_stream_url;
    bool m_player_prepared{ false };
    bool m_player_playing{ false };
    bool m_player_muted{ false };
    float m_player_volume{ 1.0f };
    HJCombo::Ptr m_thumb_level_combo{};
    ThumbCaptureTaskState m_thumb_capture_task{};
};

NS_HJ_END
