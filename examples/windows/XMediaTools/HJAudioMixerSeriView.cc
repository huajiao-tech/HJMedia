//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAudioMixerSeriView.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "HJFLog.h"
#include "HJFFUtils.h"
#include "HJAVUtils.h"
#include "HJFileUtil.h"

NS_HJ_BEGIN

namespace {

constexpr int kInputCount = 2;
constexpr int kOutputSampleRate = 48000;
constexpr int kOutputChannels = 2;
constexpr const char* kAacDumpPath = "E:/movies/audio/out/HJAudioMixerSeriView.aac";

void copyStringToBuffer(const std::string& i_value, std::array<char, 1024>& o_buffer)
{
    o_buffer.fill('\0');
    if (i_value.empty()) {
        return;
    }

    const size_t copy_size = (std::min)(i_value.size(), o_buffer.size() - 1);
    std::copy_n(i_value.begin(), copy_size, o_buffer.begin());
}

HJAudioInfo::Ptr makeMixerOutputInfo()
{
    auto output_info = std::make_shared<HJAudioInfo>();
    output_info->setChannels(kOutputChannels);
    output_info->m_sampleFmt = AV_SAMPLE_FMT_S16;
    output_info->m_bytesPerSample = 2;
    output_info->m_samplesRate = kOutputSampleRate;
    output_info->m_samplesPerFrame = HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
    output_info->setSampleCnt(HJ_AUDIO_MIXER_OUTPUT_SAMPLES);
    output_info->m_blockAlign = kOutputChannels * output_info->m_bytesPerSample;
    output_info->m_dataType = HJDATA_TYPE_RS;
    output_info->setTimeBase({ 1, output_info->m_samplesRate });
    return output_info;
}

float clampMonitorLevel(float i_value)
{
    return (std::max)(0.0f, (std::min)(1.0f, i_value));
}

template <typename SampleType, typename PeakFunc>
std::vector<float> calculateChannelPeaks(const AVFrame* i_avf, bool i_is_planar, PeakFunc i_peak_func)
{
    const int channels = i_avf->ch_layout.nb_channels;
    if (channels <= 0) {
        return {};
    }
    std::vector<float> peaks(static_cast<size_t>(channels), 0.0f);
    uint8_t* const* frame_data = i_avf->extended_data != nullptr ? i_avf->extended_data : i_avf->data;

    for (int sample_index = 0; sample_index < i_avf->nb_samples; ++sample_index) {
        for (int channel_index = 0; channel_index < channels; ++channel_index) {
            const int plane_index = i_is_planar ? channel_index : 0;
            if (frame_data[plane_index] == nullptr) {
                continue;
            }

            const SampleType* samples = reinterpret_cast<const SampleType*>(frame_data[plane_index]);
            const int data_index = i_is_planar ? sample_index : (sample_index * channels + channel_index);
            peaks[static_cast<size_t>(channel_index)] =
                (std::max)(peaks[static_cast<size_t>(channel_index)], clampMonitorLevel(i_peak_func(samples[data_index])));
        }
    }

    return peaks;
}

std::vector<float> calculateAudioPeaks(const AVFrame* i_avf)
{
    if (i_avf == nullptr || i_avf->nb_samples <= 0 || i_avf->ch_layout.nb_channels <= 0) {
        return {};
    }

    const AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(i_avf->format);
    const bool is_planar = av_sample_fmt_is_planar(sample_fmt) != 0;

    switch (sample_fmt) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            return calculateChannelPeaks<int16_t>(i_avf, is_planar, [](int16_t i_sample) -> float {
                return static_cast<float>(HJ_ABS(i_sample)) / 32768.0f;
            });
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            return calculateChannelPeaks<float>(i_avf, is_planar, [](float i_sample) -> float {
                return std::fabs(i_sample);
            });
        default:
            return {};
    }
}

} // namespace

HJAudioMixerSeriView::HJAudioMixerSeriView()
{
    m_name = HJMakeGlobalName("Audio Mixer");
    m_inputs.resize(kInputCount);
    for (size_t i = 0; i < m_inputs.size(); ++i) {
        m_inputs[i].m_label = HJFMT("Input {}", i + 1);
        m_inputs[i].m_input_id = HJFMT("mixer_input_{}", i + 1);
    }
    m_status_text = "Ready";
}

HJAudioMixerSeriView::~HJAudioMixerSeriView()
{
    HJFLogi("~HJAudioMixerSeriView name:{} entry", m_name);
    HJMainExecutorSync([=]() {
        stopSession();
    });
    HJFLogi("~HJAudioMixerSeriView name:{} end", m_name);
}

int HJAudioMixerSeriView::init(const std::string info)
{
    const int ret = HJView::init(info);
    if (ret != HJ_OK) {
        return ret;
    }

	std::vector<std::string> default_urls { 
        //"C:/Users/lengzhiyong-hj/Downloads/_LC_ps4_non_h265_SD_15726831216842052020111158_OX.flv", 
        "https://live-pull-1.huajiao.com/main/HJ_0_ali_1_main__h265_274151887_1775814947540_2608_O.flv?auth_key=1775901472-0-0-b69d99dd8598dc6bb934f5b85d4bc9f7",
        "E:/movies/audio/c58733ac51124fe38cdc6540a7b8fa46.mkv" };
    for (size_t i = 0; i < m_inputs.size(); ++i) {
        copyStringToBuffer(default_urls[i], m_inputs[i].m_url_buffer);
    }

    setStatusText("Ready");
    return HJ_OK;
}

int HJAudioMixerSeriView::draw(const HJRectf& rect)
{
    ImGui::SetNextWindowPos({ rect.x, rect.y }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ rect.w, rect.h }, ImGuiCond_Always);

    const ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    bool is_open = m_isOpen;
    if (ImGui::Begin(HJAnsiToUtf8(m_name).c_str(), &is_open, window_flags))
    {
        for (size_t i = 0; i < m_inputs.size(); ++i) {
            std::string label;
            float volume = 1.0f;
            float player_volume = 0.0f;
            bool prepared = false;
            bool input_added = false;
            bool started = false;
            bool playing = false;
            bool play_requested = false;
            bool starved = false;
            bool eof = false;
            int64_t audio_frame_count = 0;
            HJMediaInfo::Ptr media_info{};
            HJAudioInfo::Ptr audio_info{};
            HJMediaFrame::Ptr video_frame{};
            HJFrameView::Ptr frame_view{};
            {
                HJ_AUTO_LOCK(m_mutex);
                label = m_inputs[i].m_label;
                volume = m_inputs[i].m_volume;
                player_volume = m_inputs[i].m_player_volume;
                prepared = m_inputs[i].m_prepared;
                input_added = m_inputs[i].m_input_added;
                started = m_inputs[i].m_started;
                playing = m_inputs[i].m_playing;
                play_requested = m_inputs[i].m_play_requested;
                starved = m_inputs[i].m_starved;
                eof = m_inputs[i].m_eof;
                audio_frame_count = m_inputs[i].m_audio_frame_count;
                media_info = m_inputs[i].m_media_info;
                audio_info = m_inputs[i].m_audio_info;
                video_frame = m_inputs[i].m_video_frame;
                frame_view = m_inputs[i].m_frame_view;
            }
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("%s", label.c_str());
            ImGui::InputText("Url", m_inputs[i].m_url_buffer.data(), m_inputs[i].m_url_buffer.size());
            if (ImGui::SliderFloat("Volume", &volume, 0.0f, 2.0f, "%.2f")) {
                setInputVolume(i, volume);
            }

            ImGui::SameLine();
            if (ImGui::SliderFloat("Player Volume", &player_volume, 0.0f, 2.0f, "%.2f")) {
                setPlayerVolume(i, player_volume);
            }

            if (ImGui::Button("Play")) {
                playInput(i);
            }
            ImGui::SameLine();
            if (ImGui::Button("Pause")) {
                pauseInput(i);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                stopInput(i);
            }

            ImGui::Text(
                "Prepared:%s  Started:%s  Added:%s  PlayReq:%s  Playing:%s  Starved:%s  EOF:%s  Frames:%lld",
                prepared ? "yes" : "no",
                started ? "yes" : "no",
                input_added ? "yes" : "no",
                play_requested ? "yes" : "no",
                playing ? "yes" : "no",
                starved ? "yes" : "no",
                eof ? "yes" : "no",
                static_cast<long long>(audio_frame_count));

            if (audio_info != nullptr) {
                ImGui::Text(
                    "Audio: track=%d codec=%d channels=%d sample_rate=%d sample_fmt=%d",
                    audio_info->m_trackID,
                    audio_info->m_codecID,
                    audio_info->m_channels,
                    audio_info->m_samplesRate,
                    audio_info->m_sampleFmt);
            } else {
                ImGui::Text("Audio: not ready");
            }

            if (ImGui::BeginChild("VideoPreview", ImVec2(0.0f, 150.0f), true)) {
                if (frame_view != nullptr && video_frame != nullptr) {
                    const ImVec2 preview_size = ImGui::GetContentRegionAvail();
                    frame_view->draw(video_frame, HJSizef{ preview_size.x, preview_size.y });
                } else if (media_info != nullptr && media_info->getVideoInfo() != nullptr) {
                    ImGui::TextWrapped("Video track detected, waiting for frames.");
                } else {
                    ImGui::TextUnformatted("No video track.");
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::PopID();
        }

        HJGraphAudioMixerStats mixer_stats{};
        std::string status_text;
        int last_result = HJ_OK;
        int64_t output_frame_count = 0;
        int64_t last_output_pts = HJ_NOTS_VALUE;
        bool monitor_enabled = true;
        float monitor_volume = 1.0f;
        std::array<float, 2> monitor_levels{ 0.0f, 0.0f };
        bool master_muted = false;
        bool mixer_started = false;
        {
            HJ_AUTO_LOCK(m_mutex);
            mixer_stats = m_mixer_stats;
            status_text = m_status_text;
            last_result = m_last_result;
            output_frame_count = m_output_frame_count;
            last_output_pts = m_last_output_pts;
            monitor_enabled = m_monitor_enabled;
            monitor_volume = m_monitor_volume;
            monitor_levels = m_monitor_levels;
            master_muted = m_master_muted;
            mixer_started = m_mixer_started;
        }

        if (ImGui::Checkbox("Monitor", &monitor_enabled)) {
            setMonitorEnabled(monitor_enabled);
        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("Monitor Volume", &monitor_volume, 0.0f, 2.0f, "%.2f")) {
            setMonitorVolume(monitor_volume);
        }

        if (ImGui::Checkbox("Master Mute", &master_muted)) {
            setMasterMute(master_muted);
        }

        ImGui::Text("Monitor Level");
        ImGui::ProgressBar(monitor_levels[0], ImVec2(-1.0f, 0.0f), "L");
        ImGui::ProgressBar(monitor_levels[1], ImVec2(-1.0f, 0.0f), "R");

        if (ImGui::Button("Prepare")) {
            prepareSession();
        }
        ImGui::SameLine();
        if (ImGui::Button("Start Mix")) {
            startMixer();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Mix")) {
            stopMixer();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            stopSession();
        }

        ImGui::Separator();
        ImGui::Text("Mixer started:%s", mixer_started ? "yes" : "no");
        ImGui::Text(
            "Output frames:%lld  Last pts:%lld  Mix pts:%lld  Active inputs:%zu  Starved inputs:%zu  Silence fills:%zu",
            static_cast<long long>(output_frame_count),
            static_cast<long long>(last_output_pts),
            static_cast<long long>(mixer_stats.m_mix_pts),
            mixer_stats.m_active_inputs,
            mixer_stats.m_starved_inputs,
            mixer_stats.m_silence_fill_frames);
        ImGui::Text("Last result:%d", last_result);
        ImGui::TextWrapped("Status: %s", status_text.c_str());
    }
    ImGui::End();

    if (m_isOpen && !is_open) {
        m_isOpen = false;
        stopSession();
    }

    return HJ_OK;
}

int HJAudioMixerSeriView::prepareSession()
{
    stopSession();

    bool has_any_url = false;
    for (size_t i = 0; i < m_inputs.size(); ++i) {
        if (!getInputUrl(i).empty()) {
            has_any_url = true;
            break;
        }
    }

    if (!has_any_url) {
        setStatusText("At least one input url is required", HJErrInvalidParams);
        return HJErrInvalidParams;
    }

    auto mixer = std::make_shared<HJGraphAudioMixer>();
    auto mix_render = HJARenderBase::createAudioRender();
    std::ofstream aac_output_file{};
    HJAudioMixerConfig config{};
    config.m_output_info = makeMixerOutputInfo();
    config.m_max_inputs = kInputCount;
    config.m_out_type = HJ_AUDIO_MIXER_OUT_TYPE_AAC;
    config.m_aac_type = HJ_AUDIO_MIXER_AAC_TYPE_ADTS;
    config.m_sync_window_ms = 42;
    config.m_late_threshold_ms = 200;

    int ret = mixer->initWithConfig(config);
    if (ret != HJ_OK) {
        setStatusText(HJFMT("init mixer failed: {}", ret), ret);
        return ret;
    }
    if (mix_render == nullptr) {
        setStatusText("create mix render failed", HJErrNewObj);
        mixer->done();
        return HJErrNewObj;
    }
    if (config.m_out_type == HJ_AUDIO_MIXER_OUT_TYPE_AAC) {
        aac_output_file = HJFileUtil::create_file(kAacDumpPath, "wb");
        if (!aac_output_file.is_open()) {
            setStatusText(HJFMT("open aac dump file failed: {}", kAacDumpPath), HJErrIOOpen);
            mixer->done();
            return HJErrIOOpen;
        }
    }

    auto render_output_info = std::dynamic_pointer_cast<HJAudioInfo>(config.m_output_info->dup());
    if (render_output_info == nullptr) {
        setStatusText("dup mix output info failed", HJErrNewObj);
        mixer->done();
        return HJErrNewObj;
    }
    bool monitor_enabled = true;
    float monitor_volume = 1.0f;
    {
        HJ_AUTO_LOCK(m_mutex);
        monitor_enabled = m_monitor_enabled;
        monitor_volume = m_monitor_volume;
    }
    ret = mix_render->init(render_output_info);
    if (ret != HJ_OK) {
        setStatusText(HJFMT("init mix render failed: {}", ret), ret);
        mixer->done();
        return ret;
    }
    mix_render->setVolume(monitor_enabled ? monitor_volume : 0.0f);

    HJAudioMixerSeriView::Wtr wself = HJSharedFromThis();
    auto& event_bus = mixer->eventBus();
    auto audio_frame_token = event_bus->subscribe(EVENT_GRAPH_AUDIO_FRAME_ID,
        [wself](const HJMediaFrame::Ptr& i_frame) -> HJRet {
            auto self = wself.lock();
            if (!self) {
                return HJErrAlreadyDone;
            }
            return self->onMixerFrame(i_frame);
        });
    auto input_state_token = event_bus->subscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [wself](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            auto self = wself.lock();
            if (!self) {
                return HJErrAlreadyDone;
            }
            return self->onMixerInputState(i_state);
        });
    auto stats_token = event_bus->subscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [wself](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            auto self = wself.lock();
            if (!self) {
                return HJErrAlreadyDone;
            }
            return self->onMixerStats(i_stats);
        });

    {
        HJ_AUTO_LOCK(m_mutex);
        m_mixer = mixer;
        m_mix_render = mix_render;
        m_audio_frame_token = audio_frame_token;
        m_input_state_token = input_state_token;
        m_stats_token = stats_token;
        m_aac_output_file = std::move(aac_output_file);
        m_aac_output_path = kAacDumpPath;
        m_output_frame_count = 0;
        m_last_output_pts = HJ_NOTS_VALUE;
        m_mixer_stats = HJGraphAudioMixerStats{};
        m_last_result = HJ_OK;
        m_mixer_started = false;
        m_master_muted = false;
        m_monitor_levels = { 0.0f, 0.0f };
        for (auto& input : m_inputs) {
            resetRuntimeState(input);
        }
    }

    for (size_t i = 0; i < m_inputs.size(); ++i) {
        const std::string url = getInputUrl(i);
        if (url.empty()) {
            continue;
        }

        ret = prepareInputPlayer(i);
        if (ret != HJ_OK) {
            setStatusText(HJFMT("prepare input {} player failed: {}", i + 1, ret), ret);
            stopSession();
            return ret;
        }
    }

    setStatusText("Session prepared. Start players and mixer manually.");
    return HJ_OK;
}

int HJAudioMixerSeriView::prepareInputPlayer(size_t i_index)
{
    if (i_index >= m_inputs.size()) {
        return HJErrInvalidParams;
    }

    const std::string url = getInputUrl(i_index);
    if (url.empty()) {
        return HJErrInvalidParams;
    }

    auto player = std::make_shared<HJMediaPlayer>();
    auto frame_view = std::make_shared<HJFrameView>();
    if (frame_view->init() != HJ_OK) {
        HJFLoge("init frame view failed, input:{}", i_index + 1);
        frame_view = nullptr;
    }

    HJAudioMixerSeriView::Wtr wself = HJSharedFromThis();
    player->setName(HJFMT("player_{}", i_index + 1));
    HJFLogi("create player name:{}, url:{}", player->getName(), url);

    player->setMediaFrameListener([wself, i_index](const HJMediaFrame::Ptr i_frame) -> int {
        auto self = wself.lock();
        if (!self) {
            return HJErrAlreadyDone;
        }
        return self->onPlayerFrame(i_index, i_frame);
    });

    {
        HJ_AUTO_LOCK(m_mutex);
        resetRuntimeState(m_inputs[i_index]);
        m_inputs[i_index].m_player = player;
        m_inputs[i_index].m_frame_view = frame_view;
    }

    auto media_url = std::make_shared<HJMediaUrl>(url);
    media_url->setTimeout(5000 * 1000);
    media_url->setUseFast(false);

    const int ret = player->init(media_url, [wself, i_index](const HJNotification::Ptr i_ntf) -> int {
        auto self = wself.lock();
        if (!self) {
            return HJErrAlreadyDone;
        }
        return self->onPlayerNotify(i_index, i_ntf);
    });
    if (ret != HJ_OK) {
        HJ_AUTO_LOCK(m_mutex);
        if (m_inputs[i_index].m_player == player) {
            resetRuntimeState(m_inputs[i_index]);
        }
        return ret;
    }

    return HJ_OK;
}

int HJAudioMixerSeriView::playInput(size_t i_index)
{
    HJMediaPlayer::Ptr player{};
    bool prepared = false;
    bool started = false;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_inputs.size()) {
            return HJErrInvalidParams;
        }
        m_inputs[i_index].m_play_requested = true;
        player = m_inputs[i_index].m_player;
        prepared = m_inputs[i_index].m_prepared;
        started = m_inputs[i_index].m_started;
    }

    if (player == nullptr) {
        setStatusText(HJFMT("input {} is not prepared", i_index + 1), HJErrNotInited);
        return HJErrNotInited;
    }

    if (!prepared) {
        setStatusText(HJFMT("input {} is preparing, will start when ready", i_index + 1));
        return HJ_OK;
    }

    if (!started) {
        const int ret = player->start();
        if (ret != HJ_OK) {
            setStatusText(HJFMT("start input {} failed: {}", i_index + 1, ret), ret);
            return ret;
        }
        setStatusText(HJFMT("starting input {}", i_index + 1));
        return HJ_OK;
    }

    const int ret = player->play();
    if (ret != HJ_OK) {
        setStatusText(HJFMT("play input {} failed: {}", i_index + 1, ret), ret);
        return ret;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index < m_inputs.size() && m_inputs[i_index].m_player == player) {
            m_inputs[i_index].m_playing = true;
        }
    }
    setStatusText(HJFMT("input {} playing", i_index + 1));
    return HJ_OK;
}

int HJAudioMixerSeriView::pauseInput(size_t i_index)
{
    HJMediaPlayer::Ptr player{};
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_inputs.size()) {
            return HJErrInvalidParams;
        }
        m_inputs[i_index].m_play_requested = false;
        player = m_inputs[i_index].m_player;
    }

    if (player == nullptr || !player->isReady()) {
        return HJ_OK;
    }

    const int ret = player->pause();
    if (ret != HJ_OK) {
        setStatusText(HJFMT("pause input {} failed: {}", i_index + 1, ret), ret);
        return ret;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index < m_inputs.size() && m_inputs[i_index].m_player == player) {
            m_inputs[i_index].m_playing = false;
        }
    }
    setStatusText(HJFMT("input {} paused", i_index + 1));
    return HJ_OK;
}

int HJAudioMixerSeriView::stopInput(size_t i_index)
{
    HJGraphAudioMixer::Ptr mixer{};
    HJMediaPlayer::Ptr player{};
    bool input_added = false;
    std::string input_id;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_inputs.size()) {
            return HJErrInvalidParams;
        }
        mixer = m_mixer;
        player = m_inputs[i_index].m_player;
        input_added = m_inputs[i_index].m_input_added;
        input_id = m_inputs[i_index].m_input_id;
        m_inputs[i_index].m_play_requested = false;
    }

    if (mixer != nullptr && input_added) {
        mixer->removeInput(input_id, false);
    }
    if (player != nullptr) {
        player->setMediaFrameListener(nullptr);
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index < m_inputs.size()) {
            resetRuntimeState(m_inputs[i_index]);
        }
    }

    const std::string url = getInputUrl(i_index);
    if (url.empty()) {
        setStatusText(HJFMT("input {} stopped", i_index + 1));
        return HJ_OK;
    }

    const int ret = prepareInputPlayer(i_index);
    if (ret != HJ_OK) {
        setStatusText(HJFMT("recreate input {} failed: {}", i_index + 1, ret), ret);
        return ret;
    }

    setStatusText(HJFMT("input {} stopped and reset", i_index + 1));
    return HJ_OK;
}

int HJAudioMixerSeriView::startMixer()
{
    HJARenderBase::Ptr mix_render{};
    std::vector<std::pair<size_t, HJAudioInfo::Ptr>> inputs_to_register;
    bool monitor_enabled = true;
    float monitor_volume = 1.0f;
    bool master_muted = false;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_mixer == nullptr || m_mix_render == nullptr) {
            return HJErrNotInited;
        }
        if (m_mixer_started) {
            setStatusText("Mixer already started");
            return HJ_OK;
        }

        m_mixer_started = true;
        m_output_frame_count = 0;
        m_last_output_pts = HJ_NOTS_VALUE;
        m_mixer_stats = HJGraphAudioMixerStats{};
        m_monitor_levels = { 0.0f, 0.0f };
        mix_render = m_mix_render;
        monitor_enabled = m_monitor_enabled;
        monitor_volume = m_monitor_volume;
        master_muted = m_master_muted;
        for (size_t i = 0; i < m_inputs.size(); ++i) {
            const auto& input = m_inputs[i];
            if (!input.m_input_added &&
                input.m_play_requested &&
                input.m_audio_info != nullptr) {
                inputs_to_register.push_back({ i, std::dynamic_pointer_cast<HJAudioInfo>(input.m_audio_info->dup()) });
            }
        }
    }

    for (const auto& item : inputs_to_register) {
        if (item.second != nullptr) {
            registerInput(item.first, item.second);
        }
    }

    mix_render->setVolume((monitor_enabled && !master_muted) ? monitor_volume : 0.0f);
    const int ret = mix_render->start();
    if (ret != HJ_OK) {
        HJ_AUTO_LOCK(m_mutex);
        m_mixer_started = false;
        setStatusText(HJFMT("start mixer failed: {}", ret), ret);
        return ret;
    }

    setStatusText("Mixer started");
    return HJ_OK;
}

int HJAudioMixerSeriView::stopMixer()
{
    HJGraphAudioMixer::Ptr mixer{};
    HJARenderBase::Ptr mix_render{};
    std::vector<std::string> input_ids;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_mixer == nullptr) {
            return HJ_OK;
        }
        mixer = m_mixer;
        mix_render = m_mix_render;
        m_mixer_started = false;
        m_output_frame_count = 0;
        m_last_output_pts = HJ_NOTS_VALUE;
        m_mixer_stats = HJGraphAudioMixerStats{};
        m_monitor_levels = { 0.0f, 0.0f };
        for (auto& input : m_inputs) {
            if (input.m_input_added) {
                input_ids.push_back(input.m_input_id);
            }
            input.m_input_added = false;
            input.m_starved = false;
        }
    }

    for (const auto& input_id : input_ids) {
        mixer->removeInput(input_id, false);
    }
    if (mix_render != nullptr) {
        mix_render->stop();
    }

    setStatusText("Mixer stopped");
    return HJ_OK;
}

void HJAudioMixerSeriView::stopSession()
{
    HJGraphAudioMixer::Ptr mixer{};
    HJARenderBase::Ptr mix_render{};
    std::shared_ptr<void> audio_frame_token{};
    std::shared_ptr<void> input_state_token{};
    std::shared_ptr<void> stats_token{};
    std::array<HJMediaPlayer::Ptr, kInputCount> players{};

    {
        HJ_AUTO_LOCK(m_mutex);
        mixer = m_mixer;
        mix_render = m_mix_render;
        audio_frame_token = m_audio_frame_token;
        input_state_token = m_input_state_token;
        stats_token = m_stats_token;
        m_mixer = nullptr;
        m_mix_render = nullptr;
        m_audio_frame_token = nullptr;
        m_input_state_token = nullptr;
        m_stats_token = nullptr;
        if (m_aac_output_file.is_open()) {
            m_aac_output_file.close();
        }
        m_aac_output_path.clear();
        m_output_frame_count = 0;
        m_last_output_pts = HJ_NOTS_VALUE;
        m_mixer_stats = HJGraphAudioMixerStats{};
        m_mixer_started = false;
        m_master_muted = false;
        m_last_result = HJ_OK;
        m_monitor_levels = { 0.0f, 0.0f };
        for (size_t i = 0; i < m_inputs.size(); ++i) {
            players[i] = std::move(m_inputs[i].m_player);
        }
        for (auto& input : m_inputs) {
            resetRuntimeState(input);
        }
    }

    for (auto& player : players) {
        if (player != nullptr) {
            player->setMediaFrameListener(nullptr);
        }
    }
    if (mixer != nullptr && audio_frame_token != nullptr) {
        mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_FRAME_ID, audio_frame_token);
    }
    if (mixer != nullptr && input_state_token != nullptr) {
        mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID, input_state_token);
    }
    if (mixer != nullptr && stats_token != nullptr) {
        mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID, stats_token);
    }
    if (mixer != nullptr) {
        mixer->done();
    }
    if (mix_render != nullptr) {
        mix_render->stop();
    }

    setStatusText("Stopped");
}

int HJAudioMixerSeriView::registerInput(size_t i_index, const HJAudioInfo::Ptr& i_audio_info)
{
    HJGraphAudioMixer::Ptr mixer{};
    HJAudioMixerInputDesc input_desc{};
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_inputs.size() || m_mixer == nullptr || i_audio_info == nullptr) {
            return HJErrInvalidParams;
        }

        auto& input = m_inputs[i_index];
        if (input.m_input_added) {
            return HJ_OK;
        }

        mixer = m_mixer;
        input_desc.m_input_id = input.m_input_id;
        input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(i_audio_info->dup());
        input_desc.m_volume = input.m_volume;
    }

    if (input_desc.m_input_info == nullptr) {
        return HJErrInvalidParams;
    }

    const int ret = mixer->addInput(input_desc);
    if (ret == HJ_OK || ret == HJErrAlreadyExist) {
        HJ_AUTO_LOCK(m_mutex);
        auto& input = m_inputs[i_index];
        input.m_input_added = true;
        input.m_audio_info = std::dynamic_pointer_cast<HJAudioInfo>(input_desc.m_input_info->dup());
        input.m_last_result = HJ_OK;
        return HJ_OK;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index < m_inputs.size()) {
            m_inputs[i_index].m_last_result = ret;
        }
    }
    return ret;
}

int HJAudioMixerSeriView::onPlayerNotify(size_t i_index, const HJNotification::Ptr& i_ntf)
{
    if (i_ntf == nullptr || i_index >= m_inputs.size()) {
        return HJErrAlreadyDone;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_inputs[i_index].m_player == nullptr) {
            return HJErrAlreadyDone;
        }
    }

    switch (i_ntf->getID()) {
        case HJNotify_Prepared: {
            auto media_info = i_ntf->getValue<HJMediaInfo::Ptr>(HJMediaInfo::KEY_WORLDS);
            HJMediaPlayer::Ptr player{};
            float player_volume = 0.0f;
            bool play_requested = false;
            {
                HJ_AUTO_LOCK(m_mutex);
                auto& input = m_inputs[i_index];
                input.m_media_info = media_info;
                input.m_prepared = true;
                input.m_eof = false;
                input.m_last_result = HJ_OK;
                if (media_info != nullptr) {
                    auto audio_info = media_info->getAudioInfo();
                    if (audio_info != nullptr) {
                        input.m_audio_info = std::dynamic_pointer_cast<HJAudioInfo>(audio_info->dup());
                    }
                }
                player = m_inputs[i_index].m_player;
                player_volume = m_inputs[i_index].m_player_volume;
                play_requested = m_inputs[i_index].m_play_requested;
            }

            if (player != nullptr) {
                player->setVolume(player_volume);
                if (play_requested) {
                    HJMainExecutorAsync([player]() {
                        player->start();
                    });
                }
            }
            setStatusText(HJFMT("input {} prepared", i_index + 1));
            break;
        }
        case HJNotify_NeedWindow: {
            HJMediaPlayer::Ptr player{};
            {
                HJ_AUTO_LOCK(m_mutex);
                player = m_inputs[i_index].m_player;
            }
            if (player != nullptr) {
                player->setWindow((HJHND)player.get());
            }
            break;
        }
        case HJNotify_Already: {
            HJMediaPlayer::Ptr player{};
            bool play_requested = false;
            {
                HJ_AUTO_LOCK(m_mutex);
                player = m_inputs[i_index].m_player;
                play_requested = m_inputs[i_index].m_play_requested;
                m_inputs[i_index].m_started = true;
                m_inputs[i_index].m_playing = false;
            }
            if (player != nullptr && play_requested) {
                player->play();
                HJ_AUTO_LOCK(m_mutex);
                if (m_inputs[i_index].m_player == player) {
                    m_inputs[i_index].m_playing = true;
                }
            }
            setStatusText(HJFMT("input {} ready", i_index + 1));
            break;
        }
        case HJNotify_Complete: {
            HJ_AUTO_LOCK(m_mutex);
            m_inputs[i_index].m_playing = false;
            m_inputs[i_index].m_play_requested = false;
            m_inputs[i_index].m_eof = true;
            break;
        }
        case HJNotify_Error: {
            const int result = i_ntf->getVal();
            {
                HJ_AUTO_LOCK(m_mutex);
                m_inputs[i_index].m_last_result = result;
                m_inputs[i_index].m_playing = false;
                m_inputs[i_index].m_play_requested = false;
            }
            setStatusText(HJFMT("input {} error: {}", i_index + 1, result), result);
            break;
        }
        default:
            break;
    }

    return HJ_OK;
}

int HJAudioMixerSeriView::onPlayerFrame(size_t i_index, const HJMediaFrame::Ptr& i_frame)
{
    if (i_frame == nullptr || i_index >= m_inputs.size()) {
        return HJErrAlreadyDone;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_inputs[i_index].m_player == nullptr) {
            return HJErrAlreadyDone;
        }
    }

    if (i_frame->isVideo()) {
        HJ_AUTO_LOCK(m_mutex);
        m_inputs[i_index].m_video_frame = i_frame;
        return HJ_OK;
    }
    if (!i_frame->isAudio() && !i_frame->isEofFrame() && !i_frame->isClearFrame()) {
        return HJ_OK;
    }
    //HJFLogi("input {}, frame: {}", i_index + 1, i_frame->formatInfo());

    HJGraphAudioMixer::Ptr mixer{};
    std::string input_id;
    bool input_added = false;
    bool mixer_started = false;
    HJAudioInfo::Ptr audio_info{};
    {
        HJ_AUTO_LOCK(m_mutex);
        auto& input = m_inputs[i_index];
        mixer = m_mixer;
        input_id = input.m_input_id;
        input_added = input.m_input_added;
        mixer_started = m_mixer_started;
        if (i_frame->isAudio()) {
            ++input.m_audio_frame_count;
            audio_info = i_frame->getAudioInfo();
            if (audio_info != nullptr) {
                input.m_audio_info = std::dynamic_pointer_cast<HJAudioInfo>(audio_info->dup());
            }
        }
    }

    if (mixer == nullptr) {
        return HJErrAlreadyDone;
    }
    if (!mixer_started) {
        return HJ_OK;
    }
    if (i_frame->isAudio()) {
        if (audio_info == nullptr) {
            return HJErrInvalidParams;
        }

        auto channel_layout = audio_info->getChannelLayout();
        if (!channel_layout) {
            audio_info->setChannels(audio_info->m_channels);
        }
        channel_layout = audio_info->getChannelLayout();
        if (!channel_layout) {
            return HJErrInvalidParams;
        }

        if (!input_added) {
            const int ret = registerInput(i_index, audio_info);
            if (ret != HJ_OK) {
                setStatusText(HJFMT("late register input {} failed: {}", i_index + 1, ret), ret);
                return ret;
            }
        }
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        input_added = m_inputs[i_index].m_input_added;
    }
    if (!input_added) {
        return HJ_OK;
    }
    auto cur_time_ms = HJCurrentEpochMS();
    i_frame->setPTSDTS(cur_time_ms, cur_time_ms);
    HJFLogi("input {}, frame: {}", input_id, i_frame->formatInfo());
    
    const int ret = mixer->pushFrame(input_id, i_frame);
    if (ret != HJ_OK && ret != HJErrAlreadyDone) {
        HJ_AUTO_LOCK(m_mutex);
        m_inputs[i_index].m_last_result = ret;
    }
    return HJ_OK;
}

int HJAudioMixerSeriView::onMixerFrame(const HJMediaFrame::Ptr& i_frame)
{
    if (i_frame == nullptr) {
        return HJErrAlreadyDone;
    }

    AVPacket* pkt = i_frame->getAVPacket();
    if (pkt != nullptr) {
        int ret = HJ_OK;
        std::string output_path;
        {
            HJ_AUTO_LOCK(m_mutex);
            if (m_mixer == nullptr) {
                return HJErrAlreadyDone;
            }
            m_monitor_levels = { 0.0f, 0.0f };
            output_path = m_aac_output_path;
            if ((pkt->data != nullptr) && (pkt->size > 0)) {
                if (!m_aac_output_file.is_open()) {
                    ret = HJErrIOOpen;
                } else {
                    m_aac_output_file.write(reinterpret_cast<const char*>(pkt->data), pkt->size);
                    if (!m_aac_output_file.good()) {
                        ret = HJErrIOWrite;
                    }
                }
            }
        }
        HJFLogi("silence:{}, output mixer packet:{}", i_frame->m_silence, i_frame->formatInfo());
        if (ret != HJ_OK) {
            setStatusText(HJFMT("write aac dump failed: {}, path:{}", ret, output_path), ret);
            return ret;
        }
        return HJ_OK;
    }

    HJARenderBase::Ptr mix_render{};
    bool mixer_started = false;
    bool monitor_enabled = true;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_mixer == nullptr) {
            return HJErrAlreadyDone;
        }
        mix_render = m_mix_render;
        mixer_started = m_mixer_started;
        monitor_enabled = m_monitor_enabled;
    }

    {
        HJ_AUTO_LOCK(m_mutex);
        ++m_output_frame_count;
        m_last_output_pts = i_frame->getPTS();
    }
    updateMonitorLevels(i_frame);
    HJFLogi("silence:{}, output mixer frame:{}", i_frame->m_silence, i_frame->formatInfo());

    if (!mixer_started || !monitor_enabled || mix_render == nullptr) {
        return HJ_OK;
    }

    const int ret = mix_render->write(i_frame);
    if (ret != HJ_OK && ret != HJ_WOULD_BLOCK) {
        setStatusText(HJFMT("mix render write failed: {}", ret), ret);
        return ret;
    }

    return HJ_OK;
}

int HJAudioMixerSeriView::onMixerInputState(const HJGraphAudioMixerInputState& i_state)
{
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_mixer == nullptr) {
            return HJErrAlreadyDone;
        }
    }
    if (i_state.m_input_id.empty()) {
        return HJErrAlreadyDone;
    }
    HJFLogi("onMixerInputState id:{}, index:{}, attached:{}, starved:{}, eof:{}",
    i_state.m_input_id, i_state.m_slot_index, i_state.m_attached, i_state.m_starved, i_state.m_eof);

    HJ_AUTO_LOCK(m_mutex);
    for (auto& input : m_inputs) {
        if (input.m_input_id != i_state.m_input_id) {
            continue;
        }
        input.m_input_added = i_state.m_attached;
        input.m_starved = i_state.m_starved;
        input.m_eof = i_state.m_eof;
        break;
    }
    return HJ_OK;
}

int HJAudioMixerSeriView::onMixerStats(const HJGraphAudioMixerStats& i_stats)
{
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_mixer == nullptr) {
            return HJErrAlreadyDone;
        }
    }
    HJFLogi("onMixerStats mix_pts:{}, output_frames:{}, silence_fill:{}, late_drop:{}, active_inputs:{}, starved_inputs:{}",
        i_stats.m_mix_pts, i_stats.m_output_frames, i_stats.m_silence_fill_frames, i_stats.m_late_drop_frames, i_stats.m_active_inputs, i_stats.m_starved_inputs);
    HJ_AUTO_LOCK(m_mutex);
    m_mixer_stats = i_stats;
    return HJ_OK;
}

int HJAudioMixerSeriView::setInputVolume(size_t i_index, float i_volume)
{
    HJGraphAudioMixer::Ptr mixer{};
    std::string input_id;
    bool input_added = false;
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_inputs.size()) {
            return HJErrInvalidParams;
        }
        m_inputs[i_index].m_volume = i_volume;
        mixer = m_mixer;
        input_id = m_inputs[i_index].m_input_id;
        input_added = m_inputs[i_index].m_input_added;
    }

    if (mixer == nullptr || !input_added) {
        return HJ_OK;
    }

    const int ret = mixer->setInputVolume(input_id, i_volume);
    if (ret != HJ_OK) {
        setStatusText(HJFMT("set input {} volume failed: {}", i_index + 1, ret), ret);
    }
    return ret;
}

int HJAudioMixerSeriView::setPlayerVolume(size_t i_index, float i_volume)
{
    HJMediaPlayer::Ptr player{};
    {
        HJ_AUTO_LOCK(m_mutex);
        if (i_index >= m_inputs.size()) {
            return HJErrInvalidParams;
        }
        m_inputs[i_index].m_player_volume = i_volume;
        player = m_inputs[i_index].m_player;
    }

    if (player != nullptr) {
        player->setVolume(i_volume);
    }
    return HJ_OK;
}

int HJAudioMixerSeriView::setMonitorEnabled(bool i_enabled)
{
    HJARenderBase::Ptr mix_render{};
    float monitor_volume = 1.0f;
    bool master_muted = false;
    {
        HJ_AUTO_LOCK(m_mutex);
        m_monitor_enabled = i_enabled;
        mix_render = m_mix_render;
        monitor_volume = m_monitor_volume;
        master_muted = m_master_muted;
    }

    if (mix_render != nullptr) {
        mix_render->setVolume((i_enabled && !master_muted) ? monitor_volume : 0.0f);
    }
    if (!i_enabled) {
        HJ_AUTO_LOCK(m_mutex);
        m_monitor_levels = { 0.0f, 0.0f };
    }
    return HJ_OK;
}

int HJAudioMixerSeriView::setMonitorVolume(float i_volume)
{
    HJARenderBase::Ptr mix_render{};
    bool monitor_enabled = true;
    bool master_muted = false;
    {
        HJ_AUTO_LOCK(m_mutex);
        m_monitor_volume = i_volume;
        mix_render = m_mix_render;
        monitor_enabled = m_monitor_enabled;
        master_muted = m_master_muted;
    }

    if (mix_render != nullptr) {
        mix_render->setVolume((monitor_enabled && !master_muted) ? i_volume : 0.0f);
    }
    return HJ_OK;
}

int HJAudioMixerSeriView::setMasterMute(bool i_muted)
{
    HJGraphAudioMixer::Ptr mixer{};
    HJARenderBase::Ptr mix_render{};
    bool monitor_enabled = true;
    float monitor_volume = 1.0f;
    {
        HJ_AUTO_LOCK(m_mutex);
        m_master_muted = i_muted;
        mixer = m_mixer;
        mix_render = m_mix_render;
        monitor_enabled = m_monitor_enabled;
        monitor_volume = m_monitor_volume;
    }

    int ret = HJ_OK;
    if (mixer == nullptr) {
        if (mix_render != nullptr) {
            mix_render->setVolume((!i_muted && monitor_enabled) ? monitor_volume : 0.0f);
        }
        return HJ_OK;
    }

    ret = mixer->setMasterMute(i_muted);
    if (mix_render != nullptr) {
        mix_render->setVolume((!i_muted && monitor_enabled) ? monitor_volume : 0.0f);
    }
    if (ret != HJ_OK) {
        setStatusText(HJFMT("set master mute failed: {}", ret), ret);
    }
    return ret;
}

void HJAudioMixerSeriView::updateMonitorLevels(const HJMediaFrame::Ptr& i_frame)
{
    if (i_frame == nullptr || !i_frame->isAudio()) {
        return;
    }

    const std::vector<float> peaks = HJAVUtils::calculateAudioPeaks(i_frame->getAVFrame()); //calculateAudioPeaks(i_frame->getAVFrame());
    float peak_left = 0.0f;
    float peak_right = 0.0f;
    if (!peaks.empty()) {
        peak_left = peaks[0];
        peak_right = peaks.size() > 1 ? peaks[1] : peaks[0];
    }

    HJ_AUTO_LOCK(m_mutex);
    m_monitor_levels[0] = peak_left;
    m_monitor_levels[1] = peak_right;
}

void HJAudioMixerSeriView::resetRuntimeState(MixerInputState& io_state)
{
    io_state.m_player = nullptr;
    io_state.m_frame_view = nullptr;
    io_state.m_media_info = nullptr;
    io_state.m_audio_info = nullptr;
    io_state.m_video_frame = nullptr;
    io_state.m_prepared = false;
    io_state.m_input_added = false;
    io_state.m_starved = false;
    io_state.m_eof = false;
    io_state.m_started = false;
    io_state.m_playing = false;
    io_state.m_play_requested = false;
    io_state.m_audio_frame_count = 0;
    io_state.m_last_result = HJ_OK;
}

void HJAudioMixerSeriView::setStatusText(const std::string& i_status, int i_result)
{
    HJ_AUTO_LOCK(m_mutex);
    m_status_text = i_status;
    m_last_result = i_result;
}

std::string HJAudioMixerSeriView::getInputUrl(size_t i_index) const
{
    HJ_AUTO_LOCK(m_mutex);
    if (i_index >= m_inputs.size()) {
        return "";
    }
    return std::string(m_inputs[i_index].m_url_buffer.data());
}

NS_HJ_END
