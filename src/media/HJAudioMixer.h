#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "HJMediaFrame.h"

typedef struct AVFilterGraph AVFilterGraph;
typedef struct AVFilterContext AVFilterContext;
typedef struct AVFilter AVFilter;

static const int HJ_AUDIO_MIXER_OUTPUT_SAMPLES = 1024;
NS_HJ_BEGIN
//***********************************************************************************//
class HJAudioMixerImpl;

enum HJAudioMixerOutType
{
    HJ_AUDIO_MIXER_OUT_TYPE_AAC = 0,
    HJ_AUDIO_MIXER_OUT_TYPE_PCM = 1,
};

enum HJAudioMixerAacType
{
    HJ_AUDIO_MIXER_AAC_TYPE_RAW = 0,
    HJ_AUDIO_MIXER_AAC_TYPE_ADIF = 1,
    HJ_AUDIO_MIXER_AAC_TYPE_ADTS = 2,
};

struct HJAudioMixerInputDesc {
    std::string m_input_id;
    HJAudioInfo::Ptr m_input_info{};
    float m_volume{ 1.0f };
};

struct HJAudioMixerConfig {
    HJAudioInfo::Ptr m_output_info{};
    int m_out_type{ HJ_AUDIO_MIXER_OUT_TYPE_AAC };
    int m_aac_type{ HJ_AUDIO_MIXER_AAC_TYPE_RAW };
    int m_max_inputs{ 8 };
    int m_frame_samples{ HJ_AUDIO_MIXER_OUTPUT_SAMPLES };
    int m_sync_window_ms{ 42 };
    int m_late_threshold_ms{ 150 };
    bool m_enable_compand{ true };
    bool m_enable_limiter{ true };
    //
    std::vector<HJAudioMixerInputDesc> m_inputs{};
};

class HJAudioMixer : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAudioMixer>;

    HJAudioMixer();
    explicit HJAudioMixer(const HJAudioMixerConfig& i_config);
    virtual ~HJAudioMixer();

    int init(const HJAudioMixerConfig& i_config);
    void done();

    int configureInput(size_t i_slot_index, const HJAudioMixerInputDesc& i_input_desc);
    int clearInput(size_t i_slot_index);
    int setInputVolume(size_t i_slot_index, float i_volume);
    int setMasterMute(bool i_muted);
    bool isMasterMuted() const {
        return m_master_muted.load(std::memory_order_relaxed);
    }

    HJMediaFrame::Ptr mixFrames(const std::vector<HJMediaFrame::Ptr>& i_frames);

    const HJAudioMixerConfig& getConfig() const {
        return m_config;
    }
    const HJAudioInfo::Ptr& getOutputInfo() const {
        return m_config.m_output_info;
    }

    int64_t getInFrameCount() const { return m_inFrameCount; }
    int64_t getOutFrameCount() const { return m_outFrameCount; }
private:
    void printAudioPeaks(AVFrame* avf);
private:
    HJAudioMixerConfig m_config{};
    std::atomic<bool> m_master_muted{ false };
    std::unique_ptr<HJAudioMixerImpl> m_impl{};
    int64_t m_inFrameCount{};
    int64_t m_outFrameCount{};
};

NS_HJ_END
