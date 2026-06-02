#include "HJAudioMixer.h"
#include "HJAudioFifo.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <unordered_map>

#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJAVUtils.h"

//***********************************************************************************//
NS_HJ_BEGIN

namespace { 
    struct HJAudioMixerSlotRuntime {
        HJAudioMixerInputDesc m_input_desc{};
        bool m_configured{ false };
    };

    HJAudioInfo::Ptr normalizeOutputInfo(const HJAudioMixerConfig& i_config)
    {
        if (i_config.m_output_info == nullptr) {
            return nullptr;
        }

        auto output_info = std::dynamic_pointer_cast<HJAudioInfo>(i_config.m_output_info->dup());
        if (output_info == nullptr) {
            return nullptr;
        }

        if (output_info->getChannelLayout() == nullptr) {
            output_info->setChannels(output_info->m_channels);
        }
        output_info->m_type = HJMEDIA_TYPE_AUDIO;
        output_info->m_dataType = HJDATA_TYPE_RS;
        output_info->m_sampleCnt = i_config.m_frame_samples;
        output_info->m_samplesPerFrame = i_config.m_frame_samples;
        output_info->m_bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(output_info->m_sampleFmt));
        return output_info;
    }

    float getPeakValue(const std::vector<float>& i_peaks, size_t i_index)
    {
        if (i_index < i_peaks.size()) {
            return i_peaks[i_index];
        }
        if (!i_peaks.empty()) {
            return i_peaks.front();
        }
        return 0.0f;
    }

    bool isCompatibleAudioFrame(const AVFrame* i_avf, const HJAudioInfo::Ptr& i_output_info)
    {
        if (i_avf == nullptr || i_output_info == nullptr || i_avf->nb_samples <= 0) {
            return false;
        }
        if (i_avf->format != i_output_info->m_sampleFmt || i_avf->sample_rate != i_output_info->m_samplesRate) {
            return false;
        }

        HJChannelLayout::Ptr output_layout = i_output_info->getChannelLayout();
        if (output_layout == nullptr || output_layout->getAVCHLayout() == nullptr) {
            return i_avf->ch_layout.nb_channels == i_output_info->m_channels;
        }

        const AVChannelLayout* expected_layout =
            static_cast<const AVChannelLayout*>(output_layout->getAVCHLayout());
        return av_channel_layout_compare(&i_avf->ch_layout, expected_layout) == 0;
    }

    template <typename SampleType>
    void applyPackedGain(SampleType* i_samples, int i_channels, int i_nb_samples, float i_gain)
    {
        if (i_samples == nullptr || i_channels <= 0 || i_nb_samples <= 0) {
            return;
        }

        const int sample_count = i_channels * i_nb_samples;
        for (int sample_index = 0; sample_index < sample_count; ++sample_index) {
            i_samples[sample_index] = static_cast<SampleType>(i_samples[sample_index] * i_gain);
        }
    }

    template <>
    void applyPackedGain<int16_t>(int16_t* i_samples, int i_channels, int i_nb_samples, float i_gain)
    {
        if (i_samples == nullptr || i_channels <= 0 || i_nb_samples <= 0) {
            return;
        }

        const int sample_count = i_channels * i_nb_samples;
        for (int sample_index = 0; sample_index < sample_count; ++sample_index) {
            const float scaled = static_cast<float>(i_samples[sample_index]) * i_gain;
            const long rounded = std::lround(scaled);
            i_samples[sample_index] = static_cast<int16_t>(HJ_CLIP(rounded, -32768L, 32767L));
        }
    }

    template <typename SampleType>
    void applyPlanarGain(uint8_t* const* i_data, int i_channels, int i_nb_samples, float i_gain)
    {
        if (i_data == nullptr || i_channels <= 0 || i_nb_samples <= 0) {
            return;
        }

        for (int channel_index = 0; channel_index < i_channels; ++channel_index) {
            SampleType* samples = reinterpret_cast<SampleType*>(i_data[channel_index]);
            if (samples == nullptr) {
                continue;
            }
            for (int sample_index = 0; sample_index < i_nb_samples; ++sample_index) {
                samples[sample_index] = static_cast<SampleType>(samples[sample_index] * i_gain);
            }
        }
    }

    template <>
    void applyPlanarGain<int16_t>(uint8_t* const* i_data, int i_channels, int i_nb_samples, float i_gain)
    {
        if (i_data == nullptr || i_channels <= 0 || i_nb_samples <= 0) {
            return;
        }

        for (int channel_index = 0; channel_index < i_channels; ++channel_index) {
            int16_t* samples = reinterpret_cast<int16_t*>(i_data[channel_index]);
            if (samples == nullptr) {
                continue;
            }
            for (int sample_index = 0; sample_index < i_nb_samples; ++sample_index) {
                const float scaled = static_cast<float>(samples[sample_index]) * i_gain;
                const long rounded = std::lround(scaled);
                samples[sample_index] = static_cast<int16_t>(HJ_CLIP(rounded, -32768L, 32767L));
            }
        }
    }

    int applyGainToAudioFrame(AVFrame* io_avf, float i_gain)
    {
        if (io_avf == nullptr) {
            return HJErrInvalidParams;
        }
        if (i_gain <= 0.0f) {
            av_samples_set_silence(
                io_avf->data,
                0,
                io_avf->nb_samples,
                io_avf->ch_layout.nb_channels,
                static_cast<AVSampleFormat>(io_avf->format));
            return HJ_OK;
        }
        if (std::fabs(i_gain - 1.0f) <= 0.0001f) {
            return HJ_OK;
        }

        const int ret = av_frame_make_writable(io_avf);
        if (ret < HJ_OK) {
            return ret;
        }

        uint8_t* const* frame_data = io_avf->extended_data != nullptr ? io_avf->extended_data : io_avf->data;
        const int channels = io_avf->ch_layout.nb_channels;
        const int nb_samples = io_avf->nb_samples;
        const AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(io_avf->format);
        switch (sample_fmt) {
            case AV_SAMPLE_FMT_S16:
                applyPackedGain<int16_t>(reinterpret_cast<int16_t*>(frame_data[0]), channels, nb_samples, i_gain);
                return HJ_OK;
            case AV_SAMPLE_FMT_S16P:
                applyPlanarGain<int16_t>(frame_data, channels, nb_samples, i_gain);
                return HJ_OK;
            case AV_SAMPLE_FMT_FLT:
                applyPackedGain<float>(reinterpret_cast<float*>(frame_data[0]), channels, nb_samples, i_gain);
                return HJ_OK;
            case AV_SAMPLE_FMT_FLTP:
                applyPlanarGain<float>(frame_data, channels, nb_samples, i_gain);
                return HJ_OK;
            default:
                return HJErrNotSupport;
        }
    }
}

class HJAudioMixerImpl : public HJObject {
public:
    void releaseFilterGraph(HJAudioMixerImpl& io_state);

    HJMediaFrame::Ptr makeSilenceFrame(const HJAudioInfo::Ptr& i_output_info, int64_t i_pts, const HJTimeBase& i_tb);
    void muteAudioFrame(const HJMediaFrame::Ptr& i_frame);
    size_t parseInputIndex(const char* i_name);
    std::string buildFilterDescription(const HJAudioMixerConfig& i_config);
    int rebuildFilterGraph(HJAudioMixerImpl& io_runtime, const HJAudioMixerConfig& i_config);
    HJMediaFrame::Ptr pullMixedFrame(const HJAudioMixerConfig& i_config, HJAudioMixerImpl& io_runtime);

public:
    std::mutex m_mutex;
    AVFilterGraph* m_graph{ nullptr };
    std::unordered_map<size_t, AVFilterContext*> m_input_filters;
    AVFilterContext* m_output_filter{ nullptr };
    std::vector<HJAudioMixerSlotRuntime> m_slots;
    HJAudioFifo::Ptr m_mix_fifo{};
    HJTimeBase m_time_base{ 1, HJ_SAMPLE_RATE_DEFAULT };
    int64_t m_next_pts{ 0 };
};

void HJAudioMixerImpl::releaseFilterGraph(HJAudioMixerImpl& io_state)
{
    io_state.m_input_filters.clear();
    io_state.m_output_filter = nullptr;
    io_state.m_mix_fifo = nullptr;
    if (io_state.m_graph != nullptr) {
        AVFilterGraph* graph = io_state.m_graph;
        avfilter_graph_free(&graph);
        io_state.m_graph = nullptr;
    }
}

HJMediaFrame::Ptr HJAudioMixerImpl::makeSilenceFrame(const HJAudioInfo::Ptr& i_output_info, int64_t i_pts, const HJTimeBase& i_tb)
{
    if (i_output_info == nullptr) {
        return nullptr;
    }

    auto output_info = std::dynamic_pointer_cast<HJAudioInfo>(i_output_info->dup());
    if (output_info == nullptr) {
        return nullptr;
    }
    output_info->m_sampleCnt = output_info->m_samplesPerFrame;

    auto frame = HJMediaFrame::makeSilenceAudioFrame(output_info);
    if (frame != nullptr && i_pts != HJ_NOTS_VALUE) {
        frame->setPTSDTS(i_pts, i_pts, i_tb);
    }
    frame->m_silence = true;

    return frame;
}

void HJAudioMixerImpl::muteAudioFrame(const HJMediaFrame::Ptr& i_frame)
{
    if (i_frame == nullptr) {
        return;
    }

    AVFrame* avf = i_frame->getAVFrame();
    if (avf == nullptr) {
        return;
    }

    const int channels = avf->ch_layout.nb_channels;
    if (channels <= 0) {
        return;
    }

    av_samples_set_silence(avf->data, 0, avf->nb_samples, channels, static_cast<AVSampleFormat>(avf->format));
}

size_t HJAudioMixerImpl::parseInputIndex(const char* i_name)
{
    if (i_name == nullptr) {
        return (std::numeric_limits<size_t>::max)();
    }
    std::string name(i_name);
    if (name.size() <= 2 || name[0] != 'i' || name[1] != 'n') {
        return (std::numeric_limits<size_t>::max)();
    }

    return static_cast<size_t>(std::strtoull(name.substr(2).c_str(), nullptr, 10));
}

std::string HJAudioMixerImpl::buildFilterDescription(const HJAudioMixerConfig& i_config)
{
    std::string desc;
    std::string weights;
    for (int i = 0; i < i_config.m_max_inputs; ++i) {
        desc += HJFMT("[in{}]", i);
        weights += HJFMT("{}{}", 1.0f, (i + 1 < i_config.m_max_inputs) ? " " : "");
    }

    desc += HJFMT(
        "amix=inputs={}:normalize=0:duration=longest:dropout_transition=0:weights='{}'[amix_out]",
        i_config.m_max_inputs, weights);

    std::string last_pad = "amix_out";
    if (i_config.m_enable_compand) {
        //desc += ";[amix_out]compand=0.02:0.15:-80/-80|-18/-18|-8/-10|0/-14:6:0:-90:0.02[compand_out]";
        desc += ";[amix_out]compand=0.005:0.06:-80/-80|-24/-24|-12/-10|-6/-7|0/-6:4:0:-90:0.005[compand_out]";
        //desc += ";[amix_out]compand=0.003:0.05:-80/-80|-24/-24|-12/-10|-6/-7|0/-6:4:0:-90:0[compand_out]";
        last_pad = "compand_out";
    }

    if (i_config.m_enable_limiter) {
        desc += HJFMT(";[{}]alimiter=limit=0.95:attack=5:release=50:level=0:latency=1[out]", last_pad);
    } else {
        desc += HJFMT(";[{}]anull[out]", last_pad);
    }

    return desc;
}

int HJAudioMixerImpl::rebuildFilterGraph(HJAudioMixerImpl& io_runtime, const HJAudioMixerConfig& i_config)
{
    if (i_config.m_output_info == nullptr || i_config.m_max_inputs <= 0) {
        return HJErrInvalidParams;
    }

    auto output_info = normalizeOutputInfo(i_config);
    if (output_info == nullptr || output_info->getChannelLayout() == nullptr) {
        return HJErrInvalidParams;
    }
	//HJFLogi("rebuild filter graph entry, output_info({}), slots size:{}", output_info->formatInfo(), io_runtime.m_slots.size());

    std::lock_guard<std::mutex> lock(io_runtime.m_mutex);
    releaseFilterGraph(io_runtime);

    io_runtime.m_slots.resize(i_config.m_max_inputs);
    io_runtime.m_time_base = { 1, output_info->m_samplesRate };
    io_runtime.m_mix_fifo = std::make_shared<HJAudioFifo>(
        output_info->m_channels,
        output_info->m_sampleFmt,
        output_info->m_samplesRate);
    if (io_runtime.m_mix_fifo == nullptr) {
        HJFLoge("create mix fifo failed");
        return HJErrNewObj;
    }
    int fifo_ret = io_runtime.m_mix_fifo->init(i_config.m_frame_samples);
    if (fifo_ret != HJ_OK) {
        HJFLoge("init mix fifo failed, ret({})", fifo_ret);
        io_runtime.m_mix_fifo = nullptr;
        return fifo_ret;
    }

    AVFilterGraph* graph = avfilter_graph_alloc();
    if (graph == nullptr) {
        HJFLoge("alloc filter graph failed");
        return HJErrNewObj;
    }
    graph->nb_threads = 1;
    io_runtime.m_graph = graph;

    AVFilterInOut* inputs = nullptr;
    AVFilterInOut* outputs = nullptr;
    int ret = HJ_OK;
    do {
        const std::string desc = buildFilterDescription(i_config);
        HJFLogi("build filter graph, desc({})", desc);
        ret = avfilter_graph_parse2(graph, desc.c_str(), &inputs, &outputs);
        if (ret < HJ_OK) {
            HJFLoge("parse filter graph failed, ret({}), desc({})", ret, desc);
            ret = HJErrFatal;
            break;
        }

        HJChannelLayout::Ptr channel_layout = output_info->getChannelLayout();
        char channel_layout_desc[64] = { 0 };
        av_channel_layout_describe(
            static_cast<const AVChannelLayout*>(channel_layout->getAVCHLayout()),
            channel_layout_desc, sizeof(channel_layout_desc));

        for (AVFilterInOut* input = inputs; input != nullptr; input = input->next) {
            size_t index = parseInputIndex(input->name);
            if (index == (std::numeric_limits<size_t>::max)() || index >= static_cast<size_t>(i_config.m_max_inputs)) {
                ret = HJErrInvalidParams;
                break;
            }

            AVBPrint args;
            av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
            av_bprintf(
                &args,
                "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                io_runtime.m_time_base.num,
                io_runtime.m_time_base.den,
                output_info->m_samplesRate,
                av_get_sample_fmt_name(static_cast<AVSampleFormat>(output_info->m_sampleFmt)),
                channel_layout_desc);

            AVFilterContext* filter = nullptr;
            ret = avfilter_graph_create_filter(
                &filter,
                avfilter_get_by_name("abuffer"),
                HJMakeGlobalName("abuffer").c_str(),
                args.str,
                nullptr,
                graph);
            if (ret < HJ_OK || filter == nullptr) {
                HJFLoge("create abuffer filter failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            ret = avfilter_link(filter, 0, input->filter_ctx, input->pad_idx);
            if (ret < HJ_OK) {
                HJFLoge("link filter failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            io_runtime.m_input_filters[index] = filter;
        }
        if (ret != HJ_OK) {
            break;
        }

        for (AVFilterInOut* output = outputs; output != nullptr; output = output->next) {
            AVFilterContext* filter = avfilter_graph_alloc_filter(
                graph,
                avfilter_get_by_name("abuffersink"),
                HJMakeGlobalName("abuffersink").c_str());
            if (filter == nullptr) {
                HJFLoge("alloc abuffersink filter failed");
                ret = HJErrFatal;
                break;
            }

            AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(output_info->m_sampleFmt);
            ret = av_opt_set_array(
                filter,
                "sample_formats",
                AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                0,
                1,
                AV_OPT_TYPE_SAMPLE_FMT,
                &sample_fmt);
            if (ret < HJ_OK) {
                HJFLoge("set sample_formats failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            const AVChannelLayout* av_channel_layout =
                static_cast<const AVChannelLayout*>(channel_layout->getAVCHLayout());
            ret = av_opt_set_array(
                filter,
                "channel_layouts",
                AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                0,
                1,
                AV_OPT_TYPE_CHLAYOUT,
                av_channel_layout);
            if (ret < HJ_OK) {
                HJFLoge("set channel_layouts failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            const int sample_rate = output_info->m_samplesRate;
            ret = av_opt_set_array(
                filter,
                "samplerates",
                AV_OPT_SEARCH_CHILDREN | AV_OPT_ARRAY_REPLACE,
                0,
                1,
                AV_OPT_TYPE_INT,
                &sample_rate);
            if (ret < HJ_OK) {
                HJFLoge("set samplerates failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            ret = avfilter_init_str(filter, nullptr);
            if (ret < HJ_OK) {
                HJFLoge("init abuffersink failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            ret = avfilter_link(output->filter_ctx, output->pad_idx, filter, 0);
            if (ret < HJ_OK) {
                HJFLoge("link abuffersink failed, ret({})", ret);
                ret = HJErrFatal;
                break;
            }

            io_runtime.m_output_filter = filter;
        }
        if (ret != HJ_OK) {
            break;
        }

        ret = avfilter_graph_config(graph, nullptr);
        if (ret < HJ_OK) {
            HJFLoge("config filter graph failed, ret({})", ret);
            ret = HJErrFatal;
            break;
        }
    } while (false);

    if (inputs != nullptr) {
        avfilter_inout_free(&inputs);
    }
    if (outputs != nullptr) {
        avfilter_inout_free(&outputs);
    }

    if (ret != HJ_OK) {
        releaseFilterGraph(io_runtime);
    }
	//HJFLogi("rebuild filter graph end, ret:{}", ret);

    return ret;
}

HJMediaFrame::Ptr HJAudioMixerImpl::pullMixedFrame(const HJAudioMixerConfig& i_config, HJAudioMixerImpl& io_runtime)
{
    if (io_runtime.m_output_filter == nullptr || i_config.m_output_info == nullptr || io_runtime.m_mix_fifo == nullptr) {
        return nullptr;
    }

    HJMediaFrame::Ptr out_frame{};
    while (true) {
        AVFrame* avf = av_frame_alloc();
        if (avf == nullptr) {
            break;
        }

        int ret = av_buffersink_get_frame(io_runtime.m_output_filter, avf);
        if (ret < HJ_OK) {
            av_frame_free(&avf);
            break;
        }

        auto output_info = normalizeOutputInfo(i_config);
        if (output_info != nullptr) {
            output_info->m_sampleCnt = avf->nb_samples;
            output_info->m_samplesPerFrame = avf->nb_samples;

            if (AV_NOPTS_VALUE == avf->pts) {
                avf->pts = io_runtime.m_next_pts;
            }
            if (AV_NOPTS_VALUE == avf->best_effort_timestamp) {
                avf->best_effort_timestamp = avf->pts;
            }
            avf->time_base = av_rational_from_hj_rational(io_runtime.m_time_base);
            const int64_t fragment_pts = avf->pts;

            auto mixed_fragment = HJMediaFrame::makeAudioFrame(output_info);
            if (mixed_fragment != nullptr) {
                int64_t dts = avf->best_effort_timestamp;
                if (dts == AV_NOPTS_VALUE) {
                    dts = avf->pts;
                }
                mixed_fragment->setMFrame(avf);
                mixed_fragment->setPTSDTS(fragment_pts, dts, io_runtime.m_time_base);
                mixed_fragment->setDuration(avf->nb_samples, io_runtime.m_time_base);
                avf = nullptr;
                io_runtime.m_next_pts = fragment_pts + output_info->m_sampleCnt;

                ret = io_runtime.m_mix_fifo->addFrame(std::move(mixed_fragment));
                if (ret != HJ_OK) {
                    HJFLoge("mix fifo add frame failed, ret({})", ret);
                    break;
                }
            }
            //printAudioPeaks(avf);
            //HJFLogi(
            //    "pull mixed fragment: pts:{}, nb_samples:{}]",
            //    fragment_pts,
            //    output_info->m_sampleCnt);
        }

        if (avf != nullptr) {
            av_frame_free(&avf);
        }
    }

    out_frame = io_runtime.m_mix_fifo->getFrame(false);
    return out_frame;
}

HJAudioMixer::HJAudioMixer()
    : m_impl(new HJAudioMixerImpl())
{}

HJAudioMixer::HJAudioMixer(const HJAudioMixerConfig& i_config)
    : HJAudioMixer()
{
    init(i_config);
}

HJAudioMixer::~HJAudioMixer()
{
    done();
}

int HJAudioMixer::init(const HJAudioMixerConfig& i_config)
{
    auto output_info = normalizeOutputInfo(i_config);
    if (output_info == nullptr || i_config.m_max_inputs <= 0) {
		HJLoge("invalid mixer config, output_info({}), max_inputs({})", i_config.m_output_info ? i_config.m_output_info->formatInfo() : "null", i_config.m_max_inputs);
        return HJErrInvalidParams;
    }

    m_config = i_config;
    m_config.m_output_info = output_info;
    m_master_muted.store(false, std::memory_order_relaxed);
    HJFLogi("init audio mixer, output_info({}), max_inputs({}), frame_samples({}), sync_window_ms({}), late_threshold_ms({}), enable_compand({}), enable_limiter({})",
		m_config.m_output_info->formatInfo(), m_config.m_max_inputs, m_config.m_frame_samples, m_config.m_sync_window_ms, m_config.m_late_threshold_ms, m_config.m_enable_compand, m_config.m_enable_limiter);

    if (m_impl == nullptr) {
        HJFLoge("m_impl not ready");
        return HJErrFatal;
    }

    {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        m_impl->m_slots.clear();
        m_impl->m_slots.resize(m_config.m_max_inputs);
        m_impl->m_next_pts = 0;
        m_inFrameCount = 0;
        m_outFrameCount = 0;
    }

    int ret = m_impl->rebuildFilterGraph(*m_impl, m_config);
	HJFLogi("end, ret:{}", ret);

	return ret;
}

void HJAudioMixer::done()
{
    if (m_impl == nullptr) {
        return;
    }
    HJFLogi("entry");

    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    m_impl->releaseFilterGraph(*m_impl);
    m_impl->m_slots.clear();
    m_impl->m_next_pts = 0;

	HJFLogi("end");
}

int HJAudioMixer::configureInput(size_t i_slot_index, const HJAudioMixerInputDesc& i_input_desc)
{
    if (m_impl == nullptr || i_slot_index >= static_cast<size_t>(m_config.m_max_inputs) || i_input_desc.m_input_info == nullptr) {
        return HJErrInvalidParams;
    }
	HJFLogi("slot_index({}), input_info({}), volume:{}", i_slot_index, i_input_desc.m_input_info->formatInfo(), i_input_desc.m_volume);
    {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        m_impl->m_slots[i_slot_index].m_configured = true;
        m_impl->m_slots[i_slot_index].m_input_desc = i_input_desc;
    }

    return HJ_OK;
}

int HJAudioMixer::clearInput(size_t i_slot_index)
{
    if (m_impl == nullptr || i_slot_index >= static_cast<size_t>(m_config.m_max_inputs)) {
        return HJErrInvalidParams;
    }
	HJFLogi("slot_index({})", i_slot_index);
    {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        m_impl->m_slots[i_slot_index] = HJAudioMixerSlotRuntime{};
    }

    return HJ_OK;
}

int HJAudioMixer::setInputVolume(size_t i_slot_index, float i_volume)
{
    if (m_impl == nullptr || i_slot_index >= static_cast<size_t>(m_config.m_max_inputs)) {
        return HJErrInvalidParams;
    }
	HJFLogi("slot_index({}), volume:{}", i_slot_index, i_volume);
    {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        if (!m_impl->m_slots[i_slot_index].m_configured) {
			HJFLoge("slot not configured, slot_index({})", i_slot_index);
            return HJErrNotAlready;
        }
        m_impl->m_slots[i_slot_index].m_input_desc.m_volume = i_volume;
    }

    return HJ_OK;
}

int HJAudioMixer::setMasterMute(bool i_muted)
{
    m_master_muted.store(i_muted, std::memory_order_relaxed);
    return HJ_OK;
}

HJMediaFrame::Ptr HJAudioMixer::mixFrames(const std::vector<HJMediaFrame::Ptr>& i_frames)
{
    if (m_impl == nullptr || m_config.m_output_info == nullptr) {
		HJFLoge("m_impl not ready or output_info not ready");
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    if (m_impl->m_graph == nullptr || m_impl->m_output_filter == nullptr) {
		HJFLoge("filter graph not ready");
        return nullptr;
    }
    
    int64_t ref_pts = HJ_NOTS_VALUE;
    HJTimeBase ref_tb = m_impl->m_time_base;
    for (const auto& f : i_frames) {
        if (f && f->getPTS() != HJ_NOTS_VALUE) {
            ref_pts = f->getPTS();
            ref_tb = f->getTimeBase();
            break;
        }
    }

    if (ref_pts == HJ_NOTS_VALUE) {
        ref_pts = m_impl->m_next_pts;
        ref_tb = m_impl->m_time_base;
    } else {
        m_impl->m_next_pts = av_rescale_q(ref_pts, av_rational_from_hj_rational(ref_tb), av_rational_from_hj_rational(m_impl->m_time_base));
    }
    m_inFrameCount++;
    for (int i = 0; i < m_config.m_max_inputs; ++i) {
        auto filter_it = m_impl->m_input_filters.find(static_cast<size_t>(i));
        if (filter_it == m_impl->m_input_filters.end() || filter_it->second == nullptr) {
			HJFLogw("input filter not found for slot {}, skip writing frame", i);
            return nullptr;
        }

        HJMediaFrame::Ptr input_frame{};
        if (i < static_cast<int>(i_frames.size()) && i < static_cast<int>(m_impl->m_slots.size()) &&
            m_impl->m_slots[i].m_configured) {
            input_frame = i_frames[i];
        }
        if (input_frame == nullptr) {
            input_frame = m_impl->makeSilenceFrame(m_config.m_output_info, ref_pts, ref_tb);
            if (input_frame == nullptr) {
                HJFLoge("make silence frame failed");
                return nullptr;
            }
            HJFLogi("make silence frame:{}", input_frame->formatInfo());
        }

        if (input_frame->getAudioInfo() == nullptr) {
            input_frame->setInfo(std::dynamic_pointer_cast<HJAudioInfo>(m_config.m_output_info->dup()));
        }
        input_frame->setAVTimeBase(m_impl->m_time_base);

        AVFrame* avf = input_frame->getAVFrame();
        if (avf == nullptr) {
            avf = static_cast<AVFrame*>(input_frame->makeAVFrame());
        }
        if (avf == nullptr) {
			HJFLoge("make AVFrame failed for input slot {}, skip writing frame", i);
            return nullptr;
        }
        if (!isCompatibleAudioFrame(avf, m_config.m_output_info)) {
            HJFLogw(
                "input frame incompatible with mixer output, slot:{}, frame:{}, output:{}",
                i,
                input_frame->formatInfo(),
                m_config.m_output_info->formatInfo());
            input_frame = m_impl->makeSilenceFrame(m_config.m_output_info, ref_pts, ref_tb);
            if (input_frame == nullptr) {
                HJFLoge("make silence frame failed for incompatible input, slot:{}", i);
                return nullptr;
            }
            input_frame->setAVTimeBase(m_impl->m_time_base);
            avf = input_frame->getAVFrame();
            if (avf == nullptr) {
                HJFLoge("make AVFrame failed for incompatible input silence replacement, slot:{}", i);
                return nullptr;
            }
        }

        const HJAudioMixerSlotRuntime& slot = m_impl->m_slots[static_cast<size_t>(i)];
        const float input_gain = HJ_CLIP(slot.m_input_desc.m_volume, 0.0f, 10.0f);
        if (input_gain <= 0.0f) {
            input_frame = m_impl->makeSilenceFrame(m_config.m_output_info, ref_pts, ref_tb);
            if (input_frame == nullptr) {
                HJFLoge("make silence frame failed for zero-volume input, slot:{}", i);
                return nullptr;
            }
            input_frame->setAVTimeBase(m_impl->m_time_base);
            avf = input_frame->getAVFrame();
            if (avf == nullptr) {
                HJFLoge("make AVFrame failed for zero-volume input silence replacement, slot:{}", i);
                return nullptr;
            }
        } else if (std::fabs(input_gain - 1.0f) > 0.0001f) {
            input_frame = input_frame->deepDup();
            if (input_frame == nullptr) {
                HJFLoge("deepDup input frame failed for slot {}", i);
                return nullptr;
            }
            input_frame->setAVTimeBase(m_impl->m_time_base);
            avf = input_frame->getAVFrame();
            if (avf == nullptr) {
                HJFLoge("deepDup AVFrame failed for slot {}", i);
                return nullptr;
            }

            const int gain_ret = applyGainToAudioFrame(avf, input_gain);
            if (gain_ret != HJ_OK) {
                HJFLogw("apply gain failed, slot:{}, volume:{}, ret:{}, use silence fallback", i, input_gain, gain_ret);
                input_frame = m_impl->makeSilenceFrame(m_config.m_output_info, ref_pts, ref_tb);
                if (input_frame == nullptr) {
                    HJFLoge("make silence frame failed after gain failure, slot:{}", i);
                    return nullptr;
                }
                input_frame->setAVTimeBase(m_impl->m_time_base);
                avf = input_frame->getAVFrame();
                if (avf == nullptr) {
                    HJFLoge("make AVFrame failed after gain failure, slot:{}", i);
                    return nullptr;
                }
            }
        }

        //printAudioPeaks(avf);
        HJFPERLogi(
            "filter:{} write frame for input slot: [nb:{}, silence:{}], {}]",
            i,
            m_inFrameCount,
            input_frame->m_silence,
            input_frame->formatInfo());
        int ret = av_buffersrc_write_frame(filter_it->second, avf);
        if (ret < HJ_OK) {
            HJFLoge("HJAudioMixer av_buffersrc_write_frame failed, slot({}), ret({})", i, ret);
            return nullptr;
        }
    }

    auto out_frame = m_impl->pullMixedFrame(m_config, *m_impl);
    if (m_master_muted.load(std::memory_order_relaxed) && out_frame != nullptr) {
        m_impl->muteAudioFrame(out_frame);
    }
    if (out_frame != nullptr) {
        m_outFrameCount++;
        HJFPERLogi("nb:{}, output frame:{}", m_outFrameCount, out_frame->formatInfo());
    }

    return out_frame;
}

void HJAudioMixer::printAudioPeaks(AVFrame* avf)
{
    auto peak_vals = HJAVUtils::calculateAudioPeaks(avf);
    const float peak0 = getPeakValue(peak_vals, 0);
    const float peak1 = getPeakValue(peak_vals, 1);
    HJFLogi("peaks:[{}, {}]", peak0, peak1);
}

NS_HJ_END
