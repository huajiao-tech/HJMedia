//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAVUtils.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
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

std::vector<float> HJAVUtils::calculateAudioPeaks(const AVFrame* i_avf)
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

NS_HJ_END