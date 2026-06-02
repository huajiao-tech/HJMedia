#pragma once

#include <array>
#include <fstream>
#include <vector>

#include "HJMediaPlayer.h"
#include "HJGraphAudioMixer.h"
#include "HJARenderBase.h"
#include "HJGuis.h"
#include "HJImguiUtils.h"

NS_HJ_BEGIN

class HJAudioMixerView : public HJView
{
public:
    HJ_DECLARE_PUWTR(HJAudioMixerView);

    explicit HJAudioMixerView();
    virtual ~HJAudioMixerView();

    virtual int init(const std::string info) override;
    virtual int draw(const HJRectf& rect) override;

private:
    struct MixerInputState {
        std::string m_label;
        std::string m_input_id;
        std::array<char, 1024> m_url_buffer{};
        HJMediaPlayer::Ptr m_player{};
        HJFrameView::Ptr m_frame_view{};
        HJMediaInfo::Ptr m_media_info{};
        HJAudioInfo::Ptr m_audio_info{};
        HJMediaFrame::Ptr m_video_frame{};
        bool m_prepared{ false };
        bool m_input_added{ false };
        bool m_starved{ false };
        bool m_eof{ false };
        bool m_playing{ false };
        float m_volume{ 1.0f };
        float m_player_volume{ 0.0f };
        int64_t m_audio_frame_count{ 0 };
        int m_last_result{ HJ_OK };
    };

    int prepareSession();
    void stopSession();
    void togglePlayState();
    int registerInput(size_t i_index, const HJAudioInfo::Ptr& i_audio_info);
    int onPlayerNotify(size_t i_index, const HJNotification::Ptr& i_ntf);
    int onPlayerFrame(size_t i_index, const HJMediaFrame::Ptr& i_frame);
    int onMixerFrame(const HJMediaFrame::Ptr& i_frame);
    int onMixerInputState(const HJGraphAudioMixerInputState& i_state);
    int onMixerStats(const HJGraphAudioMixerStats& i_stats);
    int setInputVolume(size_t i_index, float i_volume);
    int setPlayerVolume(size_t i_index, float i_volume);
    int setMonitorEnabled(bool i_enabled);
    int setMonitorVolume(float i_volume);
    int setMasterMute(bool i_muted);
    void updateMonitorLevels(const HJMediaFrame::Ptr& i_frame);
    void resetRuntimeState(MixerInputState& io_state);
    void setStatusText(const std::string& i_status, int i_result = HJ_OK);
    std::string getInputUrl(size_t i_index) const;

    std::string m_name;
    mutable std::recursive_mutex m_mutex;
    std::vector<MixerInputState> m_inputs;
    HJGraphAudioMixer::Ptr m_mixer{};
    HJARenderBase::Ptr m_mix_render{};
    std::shared_ptr<void> m_audio_frame_token{};
    std::shared_ptr<void> m_input_state_token{};
    std::shared_ptr<void> m_stats_token{};
    HJGraphAudioMixerStats m_mixer_stats{};
    std::string m_status_text;
    int m_last_result{ HJ_OK };
    int64_t m_output_frame_count{ 0 };
    int64_t m_last_output_pts{ HJ_NOTS_VALUE };
    bool m_should_play{ false };
    bool m_monitor_enabled{ true };
    float m_monitor_volume{ 1.0f };
    std::array<float, 2> m_monitor_levels{ 0.0f, 0.0f };
    std::ofstream m_aac_output_file{};
    std::string m_aac_output_path;
    bool m_master_muted{ false };
};

NS_HJ_END
