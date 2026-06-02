#include "HJGraphAudioMixer.h"

#include <algorithm>
#include <climits>
#include <unordered_set>
#include <vector>

#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN

namespace {

constexpr int kAudioTrackId = 0;
constexpr const char* kAudioMixerConfigKey = "audioMixerConfig";
constexpr const char* kAudioInfoKey = "audioInfo";
constexpr const char* kFifoKey = "fifo";
constexpr const char* kThreadKey = "thread";
constexpr const char* kMixerAacTypeKey = "aac_type";

int normalizeOutputType(int i_out_type)
{
    return i_out_type == HJ_AUDIO_MIXER_OUT_TYPE_PCM
        ? HJ_AUDIO_MIXER_OUT_TYPE_PCM
        : HJ_AUDIO_MIXER_OUT_TYPE_AAC;
}

int normalizeAacType(int i_aac_type)
{
    switch (i_aac_type) {
    case HJ_AUDIO_MIXER_AAC_TYPE_RAW:
    case HJ_AUDIO_MIXER_AAC_TYPE_ADIF:
    case HJ_AUDIO_MIXER_AAC_TYPE_ADTS:
        return i_aac_type;
    default:
        return HJ_AUDIO_MIXER_AAC_TYPE_RAW;
    }
}

void normalizeAacMixerOutputInfo(const HJAudioInfo::Ptr& io_output_info)
{
    if (io_output_info == nullptr) {
        return;
    }

    io_output_info->m_sampleFmt = AV_SAMPLE_FMT_S16;
    io_output_info->m_bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    io_output_info->m_blockAlign = io_output_info->m_bytesPerSample * io_output_info->m_channels;
    io_output_info->m_dataType = HJDATA_TYPE_RS;
}

HJAudioInfo::Ptr buildAacEncoderAudioInfo(const HJAudioMixerConfig& i_config)
{
    if (i_config.m_output_info == nullptr) {
        return nullptr;
    }

    auto audio_info =
        std::dynamic_pointer_cast<HJAudioInfo>(i_config.m_output_info->dup());
    if (audio_info == nullptr) {
        return nullptr;
    }

    HJMediaFrame::setCodecIdAAC(audio_info->m_codecID);
    if (audio_info->getBitrate() <= 0) {
        audio_info->setBitrate(96 * 1000);
    }
    (*audio_info)[kMixerAacTypeKey] = i_config.m_aac_type;
    return audio_info;
}

int normalizePcmInput(const uint8_t* i_pcm, int i_samples, int i_channel,
                      AVSampleFormat i_sample_fmt, const uint8_t*& o_pcm,
                      int& o_sample_fmt, size_t& o_pcm_size,
                      std::vector<uint8_t>& o_storage)
{
    const int bytes_per_sample = av_get_bytes_per_sample(i_sample_fmt);
    if (bytes_per_sample <= 0) {
        return HJErrNotSupport;
    }

    const uint64_t plane_size = static_cast<uint64_t>(i_samples) *
        static_cast<uint64_t>(bytes_per_sample);
    const uint64_t total_size = plane_size * static_cast<uint64_t>(i_channel);
    if (plane_size == 0 || total_size == 0 ||
        total_size > static_cast<uint64_t>(INT_MAX)) {
        return HJErrInvalidParams;
    }

    if (av_sample_fmt_is_planar(i_sample_fmt) == 0) {
        o_pcm = i_pcm;
        o_sample_fmt = static_cast<int>(i_sample_fmt);
        o_pcm_size = static_cast<size_t>(total_size);
        return HJ_OK;
    }

    const AVSampleFormat packed_fmt = av_get_packed_sample_fmt(i_sample_fmt);
    if (packed_fmt == AV_SAMPLE_FMT_NONE) {
        return HJErrNotSupport;
    }

    o_storage.resize(static_cast<size_t>(total_size));
    for (int sample_index = 0; sample_index < i_samples; ++sample_index) {
        for (int channel_index = 0; channel_index < i_channel; ++channel_index) {
            const uint8_t* src = i_pcm +
                static_cast<size_t>(channel_index) * static_cast<size_t>(plane_size) +
                static_cast<size_t>(sample_index) * static_cast<size_t>(bytes_per_sample);
            uint8_t* dst = o_storage.data() +
                (static_cast<size_t>(sample_index) * static_cast<size_t>(i_channel) +
                 static_cast<size_t>(channel_index)) *
                static_cast<size_t>(bytes_per_sample);
            std::copy_n(src, bytes_per_sample, dst);
        }
    }

    o_pcm = o_storage.data();
    o_sample_fmt = static_cast<int>(packed_fmt);
    o_pcm_size = o_storage.size();
    return HJ_OK;
}

size_t makePluginLinkKey(const std::string& i_name, HJMediaType i_type, int i_track_id)
{
    size_t seed = std::hash<std::string>{}(i_name);
    const size_t type_hash = std::hash<int>{}(static_cast<int>(i_type));
    const size_t track_hash = std::hash<int>{}(i_track_id);
    seed ^= type_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= track_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

HJAudioMixerConfig normalizeMixerConfig(const HJAudioMixerConfig& i_config)
{
    HJAudioMixerConfig config = i_config;
    config.m_out_type = normalizeOutputType(i_config.m_out_type);
    config.m_aac_type = normalizeAacType(i_config.m_aac_type);
    if (config.m_output_info != nullptr) {
        config.m_output_info = std::dynamic_pointer_cast<HJAudioInfo>(config.m_output_info->dup());
        if (config.m_output_info != nullptr) {
            if (config.m_out_type == HJ_AUDIO_MIXER_OUT_TYPE_AAC) {
                normalizeAacMixerOutputInfo(config.m_output_info);
            }
            config.m_frame_samples = HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
            config.m_output_info->m_sampleCnt = config.m_frame_samples;
            config.m_output_info->m_samplesPerFrame = config.m_frame_samples;
        }
    }

    config.m_inputs.clear();
    config.m_inputs.reserve(i_config.m_inputs.size());
    for (const auto& input_desc : i_config.m_inputs) {
        HJAudioMixerInputDesc normalized_input = input_desc;
        if (input_desc.m_input_info != nullptr) {
            normalized_input.m_input_info =
                std::dynamic_pointer_cast<HJAudioInfo>(input_desc.m_input_info->dup());
        }
        config.m_inputs.push_back(std::move(normalized_input));
    }
    return config;
}

} // namespace

int HJGraphAudioMixer::initWithConfig(const HJAudioMixerConfig& i_config)
{
    if (i_config.m_output_info == nullptr || i_config.m_max_inputs <= 0) {
        return HJErrInvalidParams;
    }
    HJFLogi("init with config: max inputs: {}, frame samples: {}, sync window: {}, late threshold: {}, enable compand: {}, enable limiter: {}, output info: {}, input cnt: {}", i_config.m_max_inputs, i_config.m_frame_samples, i_config.m_sync_window_ms, i_config.m_late_threshold_ms, i_config.m_enable_compand, i_config.m_enable_limiter, 
        i_config.m_output_info ? i_config.m_output_info->formatInfo() : "null", i_config.m_inputs.size());

    auto param = std::make_shared<HJKeyStorage>();
    (*param)[kAudioMixerConfigKey] = normalizeMixerConfig(i_config);
    return init(param);
}

int HJGraphAudioMixer::addInput(const HJAudioMixerInputDesc& i_input_desc)
{
    if (i_input_desc.m_input_id.empty() || i_input_desc.m_input_info == nullptr) {
        return HJErrInvalidParams;
    }
    HJFLogi("add input: {}, info:{}", i_input_desc.m_input_id, i_input_desc.m_input_info->formatInfo());

    return SYNC_PROD_LOCK([this, &i_input_desc] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_status < HJSTATUS_Inited || m_mixer_plugin == nullptr) {
            return HJErrNotInited;
        }
        if (m_input_slots.find(i_input_desc.m_input_id) != m_input_slots.end() ||
            m_draining_inputs.find(i_input_desc.m_input_id) != m_draining_inputs.end()) {
            return HJErrAlreadyExist;
        }

        size_t slot_index = 0;
        int ret = findIdleSlotIndex(slot_index);
        if (ret != HJ_OK) {
            return HJErrFull;
        }
        return attachSlot(slot_index, i_input_desc);
    });
}

int HJGraphAudioMixer::removeInput(const std::string& i_input_id, bool i_drain)
{
    if (i_input_id.empty()) {
        return HJErrInvalidParams;
    }
    HJFLogi("remove input: {}, drain:{}", i_input_id, i_drain);

    return SYNC_PROD_LOCK([this, &i_input_id, i_drain] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_status < HJSTATUS_Inited) {
            return HJErrNotInited;
        }
        if (i_drain && m_draining_inputs.find(i_input_id) != m_draining_inputs.end()) {
            return HJErrAlreadyExist;
        }

        size_t slot_index = 0;
        int ret = findSlotIndexByInputId(i_input_id, slot_index);
        if (ret != HJ_OK) {
            return ret;
        }
        return detachSlot(slot_index, i_drain);
    });
}

int HJGraphAudioMixer::pushFrame(const std::string& i_input_id, HJMediaFrame::Ptr i_frame)
{
    if (i_input_id.empty() || i_frame == nullptr) {
        return HJErrInvalidParams;
    }
    HJFPERLogi("{}, input: {}, frame info:{}", getName(), i_input_id, i_frame->formatInfo());

    return SYNC_CONS_LOCK([this, &i_input_id, &i_frame] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_status < HJSTATUS_Inited) {
            return HJErrNotInited;
        }
        if (m_draining_inputs.find(i_input_id) != m_draining_inputs.end()) {
            return HJErrAlreadyDone;
        }

        size_t slot_index = 0;
        int ret = findSlotIndexByInputId(i_input_id, slot_index);
        if (ret != HJ_OK) {
            return ret;
        }

        const auto& slot = m_slots[slot_index];
        if (!slot.m_attached || slot.m_input_plugin == nullptr) {
            return HJErrNotAlready;
        }
        return slot.m_input_plugin->pushFrame(i_frame);
    });
}

int HJGraphAudioMixer::pushFrame(const std::string& i_input_id, uint8_t* pcm, int samples, int channel, int samplerate, int fmt, int64_t pts)
{
    if (i_input_id.empty() || pcm == nullptr || samples <= 0 || channel <= 0 || samplerate <= 0) {
        return HJErrInvalidParams;
    }

    const AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(fmt);
    const uint8_t* normalized_pcm = pcm;
    int normalized_fmt = fmt;
    size_t pcm_size = 0;
    std::vector<uint8_t> packed_pcm_storage;
    const int normalize_ret = normalizePcmInput(
        pcm, samples, channel, sample_fmt, normalized_pcm,
        normalized_fmt, pcm_size, packed_pcm_storage);
    if (normalize_ret != HJ_OK) {
        return normalize_ret;
    }

    auto audio_info = HJAudioInfo::makeAudioInfo(
        channel, samplerate, normalized_fmt, samples);
    //audio_info->m_dataType = HJDATA_TYPE_RS;
    //audio_info->setChannels(channel);
    //audio_info->setSampleRate(samplerate);
    //audio_info->m_sampleFmt = fmt;
    //audio_info->m_bytesPerSample = bytes_per_sample;
    //audio_info->m_blockAlign = channel * bytes_per_sample;
    //audio_info->m_sampleLayout = 0;
    //audio_info->m_sampleCnt = samples;
    //audio_info->m_samplesPerFrame = samples;

    const int64_t pts_samples = av_rescale_q(pts, { 1, 1000 }, { 1, samplerate });
    auto frame = HJMediaFrame::makeAudioFrameWithSample(
        audio_info, normalized_pcm, pcm_size, pts_samples, pts_samples, { 1, samplerate });
    if (frame == nullptr) {
        return HJErrFatal;
    }

    return pushFrame(i_input_id, frame);
}

int HJGraphAudioMixer::flushInput(const std::string& i_input_id)
{
    if (i_input_id.empty()) {
        return HJErrInvalidParams;
    }
    HJFLogi("flush input: {}", i_input_id);

    return SYNC_PROD_LOCK([this, &i_input_id] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_status < HJSTATUS_Inited) {
            return HJErrNotInited;
        }
        if (m_draining_inputs.find(i_input_id) != m_draining_inputs.end()) {
            return HJErrAlreadyDone;
        }

        size_t slot_index = 0;
        int ret = findSlotIndexByInputId(i_input_id, slot_index);
        if (ret != HJ_OK) {
            return ret;
        }

        const HJAudioMixerInputDesc input_desc = m_slots[slot_index].m_input_desc;
        ret = detachSlot(slot_index, false);
        if (ret != HJ_OK) {
            return ret;
        }
        return attachSlot(slot_index, input_desc);
    });
}

int HJGraphAudioMixer::setInputVolume(const std::string& i_input_id, float i_volume)
{
    if (i_input_id.empty()) {
        return HJErrInvalidParams;
    }
    HJFLogi("set input volume: {}, input: {}", i_volume, i_input_id);

    return SYNC_PROD_LOCK([this, &i_input_id, i_volume] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_status < HJSTATUS_Inited || m_mixer_plugin == nullptr) {
            return HJErrNotInited;
        }
        if (m_draining_inputs.find(i_input_id) != m_draining_inputs.end()) {
            return HJErrAlreadyDone;
        }

        size_t slot_index = 0;
        int ret = findSlotIndexByInputId(i_input_id, slot_index);
        if (ret != HJ_OK) {
            return ret;
        }

        ret = m_mixer_plugin->setInputVolume(slot_index, i_volume);
        if (ret == HJ_OK) {
            m_slots[slot_index].m_input_desc.m_volume = i_volume;
        }
        return ret;
    });
}

int HJGraphAudioMixer::setMasterMute(bool i_muted)
{
    return SYNC_CONS_LOCK([this, i_muted] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_status < HJSTATUS_Inited || m_mixer_plugin == nullptr) {
            return HJErrNotInited;
        }
        return m_mixer_plugin->setMasterMute(i_muted);
    });
}

bool HJGraphAudioMixer::isMasterMuted() const
{
    if (m_status < HJSTATUS_Inited || m_mixer_plugin == nullptr) {
        return false;
    }
    return m_mixer_plugin->isMasterMuted();
}

HJAudioInfo::Ptr HJGraphAudioMixer::getOutputAudioInfo() const
{
    if (m_config.m_output_info == nullptr) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<HJAudioInfo>(m_config.m_output_info->dup());
}

int HJGraphAudioMixer::internalInit(HJKeyStorage::Ptr i_param)
{
    MUST_HAVE_PARAMETERS;
    if (!i_param->haveValue(kAudioMixerConfigKey)) {
        return HJErrInvalidParams;
    }

    int ret = HJGraph::internalInit(i_param);
    if (ret != HJ_OK) {
        return ret;
    }

    m_config = normalizeMixerConfig(i_param->getValue<HJAudioMixerConfig>(kAudioMixerConfigKey));
    if (m_config.m_output_info == nullptr || m_config.m_max_inputs <= 0) {
        return HJErrInvalidParams;
    }
    ret = validateConfiguredInputs();
    if (ret != HJ_OK) {
        return ret;
    }
    HJFLogi("init audio mixer, output_info({}), max_inputs({}), frame_samples({}), sync_window_ms({}), late_threshold_ms({}), enable_compand({}), enable_limiter({})",
        m_config.m_output_info->formatInfo(), m_config.m_max_inputs, m_config.m_frame_samples,
		m_config.m_sync_window_ms, m_config.m_late_threshold_ms, m_config.m_enable_compand, m_config.m_enable_limiter);

    ret = registerHandlers();
    if (ret != HJ_OK) {
        return ret;
    }

    m_slots.assign(static_cast<size_t>(m_config.m_max_inputs), HJGraphAudioMixerSlot{});
    m_input_slots.clear();
    m_draining_inputs.clear();

    m_audio_thread = HJLooperThread::quickStart(getName() + "_audio");
    if (m_audio_thread == nullptr) {
        return HJErrCreateThread;
    }
    addThread(m_audio_thread);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = std::static_pointer_cast<HJGraph>(SHARED_FROM_THIS);

    m_mixer_plugin = HJPluginAudioMixer::Create<HJPluginAudioMixer>(HJFMT("audioMixer_{}", getID()), 0, graph_info);
    if (m_mixer_plugin == nullptr) {
        return HJErrFatal;
    }
    addPlugin(m_mixer_plugin);

    if (m_config.m_out_type == HJ_AUDIO_MIXER_OUT_TYPE_AAC) {
        m_aac_encoder_plugin = HJPluginFDKAACEncoder::Create<HJPluginFDKAACEncoder>(
            HJFMT("audioAacEncoder_{}", getID()), 0, graph_info);
        if (m_aac_encoder_plugin == nullptr) {
            return HJErrFatal;
        }
        addPlugin(m_aac_encoder_plugin);
    }

    for (size_t i = 0; i < m_slots.size(); ++i) {
        auto& slot = m_slots[i];
        slot.m_input_plugin = HJPluginAudioInput::Create<HJPluginAudioInput>(HJFMT("audioInput_{}_{}", getID(), i), 0, graph_info);
        slot.m_resampler_plugin = HJPluginAudioResampler::Create<HJPluginAudioResampler>(HJFMT("audioResampler_{}_{}", getID(), i), 0, graph_info);
        if (slot.m_input_plugin == nullptr || slot.m_resampler_plugin == nullptr) {
            return HJErrFatal;
        }

        addPlugin(slot.m_input_plugin);
        addPlugin(slot.m_resampler_plugin);

        ret = connectPlugins(slot.m_input_plugin, slot.m_resampler_plugin, HJMEDIA_TYPE_AUDIO, kAudioTrackId);
        if (ret != HJ_OK) {
            return ret;
        }
        ret = connectPlugins(slot.m_resampler_plugin, m_mixer_plugin, HJMEDIA_TYPE_AUDIO, kAudioTrackId);
        if (ret != HJ_OK) {
            return ret;
        }
    }

    if (m_aac_encoder_plugin != nullptr) {
        ret = connectPlugins(m_mixer_plugin, m_aac_encoder_plugin,
            HJMEDIA_TYPE_AUDIO, kAudioTrackId);
        if (ret != HJ_OK) {
            return ret;
        }
    }

    auto plugin_param = std::make_shared<HJKeyStorage>();
    (*plugin_param)[kThreadKey] = m_audio_thread;
    (*plugin_param)[kAudioMixerConfigKey] = m_config;
    ret = m_mixer_plugin->init(plugin_param);
    if (ret != HJ_OK) {
        return ret;
    }

    if (m_aac_encoder_plugin != nullptr) {
        auto encoder_audio_info = buildAacEncoderAudioInfo(m_config);
        if (encoder_audio_info == nullptr) {
            return HJErrInvalidParams;
        }

        plugin_param = std::make_shared<HJKeyStorage>();
        (*plugin_param)[kThreadKey] = m_audio_thread;
        (*plugin_param)[kAudioInfoKey] = encoder_audio_info;
        ret = m_aac_encoder_plugin->init(plugin_param);
        if (ret != HJ_OK) {
            return ret;
        }
    }

    for (auto& slot : m_slots) {
        plugin_param = std::make_shared<HJKeyStorage>();
        (*plugin_param)[kThreadKey] = m_audio_thread;
        ret = slot.m_input_plugin->init(plugin_param);
        if (ret != HJ_OK) {
            return ret;
        }

        plugin_param = std::make_shared<HJKeyStorage>();
        (*plugin_param)[kThreadKey] = m_audio_thread;
        (*plugin_param)[kAudioInfoKey] = m_config.m_output_info;
        (*plugin_param)[kFifoKey] = true;
        ret = slot.m_resampler_plugin->init(plugin_param);
        if (ret != HJ_OK) {
            return ret;
        }
    }

    ret = attachConfiguredInputs();
    if (ret != HJ_OK) {
        return ret;
    }
    HJFLogi("init audio mixer end");

    return HJ_OK;
}

void HJGraphAudioMixer::internalRelease()
{
    m_input_slots.clear();
    m_draining_inputs.clear();
    m_slots.clear();
    m_mixer_plugin = nullptr;
    m_aac_encoder_plugin = nullptr;
    m_audio_thread = nullptr;
    m_config = HJAudioMixerConfig{};

    HJGraph::internalRelease();
}

int HJGraphAudioMixer::registerHandlers()
{
    int ret = registerQueryHandler_outputInfo();
    if (ret != HJ_OK) {
        return ret;
    }

    std::weak_ptr<HJGraphAudioMixer> w_graph = std::dynamic_pointer_cast<HJGraphAudioMixer>(SHARED_FROM_THIS);
    m_eventBus->subscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [w_graph](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            auto graph = w_graph.lock();
            if (graph == nullptr) {
                return HJErrAlreadyDone;
            }
            if (!i_state.m_eof || i_state.m_input_id.empty()) {
                return HJ_OK;
            }

            return graph->m_sync.prodLock([graph, i_state]() -> HJRet {
                if (graph->m_status == HJSTATUS_Done) {
                    return HJErrAlreadyDone;
                }
                if (graph->m_draining_inputs.find(i_state.m_input_id) == graph->m_draining_inputs.end()) {
                    return HJ_OK;
                }

                size_t slot_index = 0;
                int ret = graph->findSlotIndexByInputId(i_state.m_input_id, slot_index);
                if (ret != HJ_OK) {
                    graph->m_draining_inputs.erase(i_state.m_input_id);
                    return HJ_OK;
                }
                return graph->detachSlot(slot_index, false);
            });
        });
    return HJ_OK;
}

int HJGraphAudioMixer::registerQueryHandler_outputInfo()
{
    return m_queryBus->registerHandler(QUERY_GRAPH_AUDIO_MIXER_OUTPUT_INFO_ID, [this]() -> HJAudioInfo::Ptr {
        if (m_config.m_output_info == nullptr) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<HJAudioInfo>(m_config.m_output_info->dup());
    });
}

int HJGraphAudioMixer::validateConfiguredInputs() const
{
    if (m_config.m_inputs.size() > static_cast<size_t>(m_config.m_max_inputs)) {
        HJFLogw("configured inputs exceed max inputs, inputs:{}, max_inputs:{}",
            m_config.m_inputs.size(), m_config.m_max_inputs);
        return HJErrInvalidParams;
    }

    std::unordered_set<std::string> input_ids;
    input_ids.reserve(m_config.m_inputs.size());
    for (const auto& input_desc : m_config.m_inputs) {
        if (input_desc.m_input_id.empty() || input_desc.m_input_info == nullptr) {
            HJFLogw("configured input is invalid, input_id:{}, has_info:{}",
                input_desc.m_input_id, input_desc.m_input_info != nullptr);
            return HJErrInvalidParams;
        }
        if (!input_ids.insert(input_desc.m_input_id).second) {
            HJFLogw("configured input id duplicated, input_id:{}", input_desc.m_input_id);
            return HJErrInvalidParams;
        }
    }
    return HJ_OK;
}

int HJGraphAudioMixer::attachConfiguredInputs()
{
    size_t attached_count = 0;
    for (; attached_count < m_config.m_inputs.size(); ++attached_count) {
        const int ret = attachSlot(attached_count, m_config.m_inputs[attached_count]);
        if (ret == HJ_OK) {
            continue;
        }

        for (size_t rollback_index = attached_count; rollback_index > 0; --rollback_index) {
            const int detach_ret = detachSlot(rollback_index - 1, false);
            if (detach_ret != HJ_OK) {
                HJFLogw("rollback configured mixer input failed, slot:{}, ret:{}",
                    rollback_index - 1, detach_ret);
            }
        }
        return ret;
    }
    return HJ_OK;
}

int HJGraphAudioMixer::attachSlot(size_t i_slot_index, const HJAudioMixerInputDesc& i_input_desc)
{
    if (i_slot_index >= m_slots.size() || i_input_desc.m_input_id.empty() || i_input_desc.m_input_info == nullptr ||
        m_mixer_plugin == nullptr) {
        return HJErrInvalidParams;
    }
    HJFLogi("attach slot: {}, input: {}", i_slot_index, i_input_desc.m_input_id);

    auto& slot = m_slots[i_slot_index];
    if (slot.m_attached || slot.m_input_plugin == nullptr || slot.m_resampler_plugin == nullptr) {
        return HJErrAlreadyExist;
    }

    HJAudioMixerInputDesc input_desc = i_input_desc;
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(i_input_desc.m_input_info->dup());
    if (input_desc.m_input_info == nullptr) {
        return HJErrInvalidParams;
    }

    int ret = slot.m_input_plugin->bindInput(input_desc.m_input_id, input_desc.m_input_info);
    if (ret != HJ_OK) {
        return ret;
    }

    HJAudioMixerInputDesc mixer_input_desc = i_input_desc;
    mixer_input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(m_config.m_output_info->dup());
    const size_t mixer_input_key_hash = makePluginLinkKey(slot.m_resampler_plugin->getName(), HJMEDIA_TYPE_AUDIO, kAudioTrackId);
    ret = m_mixer_plugin->bindInput(i_slot_index, mixer_input_key_hash, mixer_input_desc);
    if (ret != HJ_OK) {
        slot.m_input_plugin->unbindInput();
        return ret;
    }

    slot.m_input_id = input_desc.m_input_id;
    slot.m_input_desc = input_desc;
    slot.m_attached = true;
    m_input_slots[input_desc.m_input_id] = i_slot_index;
    m_draining_inputs.erase(input_desc.m_input_id);
    HJFLogi("attach slot end");

    return HJ_OK;
}

int HJGraphAudioMixer::detachSlot(size_t i_slot_index, bool i_drain)
{
    if (i_slot_index >= m_slots.size()) {
        return HJErrInvalidParams;
    }
    HJFLogi("detach slot: {}, drain:{}", i_slot_index, i_drain);

    auto& slot = m_slots[i_slot_index];
    if (!slot.m_attached || slot.m_input_plugin == nullptr || slot.m_resampler_plugin == nullptr || m_mixer_plugin == nullptr) {
        return HJErrNotAlready;
    }

    if (i_drain) {
        m_draining_inputs.insert(slot.m_input_id);
        int ret = slot.m_input_plugin->signalEof();
        if (ret != HJ_OK) {
            m_draining_inputs.erase(slot.m_input_id);
        }
        return ret;
    }

    const std::string input_id = slot.m_input_id;
    const size_t resampler_input_key_hash = makePluginLinkKey(slot.m_input_plugin->getName(), HJMEDIA_TYPE_AUDIO, kAudioTrackId);

    int ret = HJ_OK;
    auto store_first_error = [&ret](int i_err) {
        if (ret == HJ_OK && i_err != HJ_OK) {
            ret = i_err;
        }
    };

    store_first_error(slot.m_input_plugin->unbindInput());
    store_first_error(slot.m_resampler_plugin->flush(resampler_input_key_hash));
    store_first_error(m_mixer_plugin->flushInput(i_slot_index));
    store_first_error(m_mixer_plugin->unbindInput(i_slot_index));

    m_input_slots.erase(input_id);
    m_draining_inputs.erase(input_id);

    slot.m_input_id.clear();
    slot.m_input_desc = HJAudioMixerInputDesc{};
    slot.m_attached = false;
    HJFLogi("detach slot end");
    return ret;
}

int HJGraphAudioMixer::findSlotIndexByInputId(const std::string& i_input_id, size_t& o_slot_index) const
{
    auto it = m_input_slots.find(i_input_id);
    if (it == m_input_slots.end()) {
        return HJErrNotFind;
    }
    o_slot_index = it->second;
    return HJ_OK;
}

int HJGraphAudioMixer::findIdleSlotIndex(size_t& o_slot_index) const
{
    for (size_t i = 0; i < m_slots.size(); ++i) {
        if (!m_slots[i].m_attached) {
            o_slot_index = i;
            return HJ_OK;
        }
    }
    return HJErrFull;
}

HJGraphAudioMixerInputState HJGraphAudioMixer::buildInputState(size_t i_slot_index, bool i_starved, bool i_eof) const
{
    HJGraphAudioMixerInputState state{};
    if (i_slot_index >= m_slots.size()) {
        return state;
    }

    const auto& slot = m_slots[i_slot_index];
    state.m_input_id = slot.m_input_id;
    state.m_slot_index = i_slot_index;
    state.m_attached = slot.m_attached;
    state.m_starved = i_starved;
    state.m_eof = i_eof;
    return state;
}

NS_HJ_END
