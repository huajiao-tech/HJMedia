#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "HJFFUtils.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN

class HJAudioDeclicker {
public:
  void reset() {
    m_reference_frame.reset();
    m_tail_samples.clear();
  }

  bool hasTailSamples() const { return !m_tail_samples.empty(); }
  bool hasReferenceFrame() const { return m_reference_frame != nullptr; }

  int captureReferenceFrame(const HJMediaFrame::Ptr &i_frame) {
    AVFrame *avf = nullptr;
    const int ret = validateAudioFrame(i_frame, avf);
    if (ret != HJ_OK) {
      return ret;
    }

    m_reference_frame = i_frame->deepDup();
    if (m_reference_frame == nullptr ||
        m_reference_frame->getAVFrame() == nullptr) {
      m_reference_frame.reset();
      return HJErrInvalidParams;
    }

    return HJ_OK;
  }

  int captureTailSamples(const HJMediaFrame::Ptr &i_frame) {
    AVFrame *avf = nullptr;
    const int ret = validateAudioFrame(i_frame, avf);
    if (ret != HJ_OK) {
      return ret;
    }

    uint8_t *const *frame_data =
        avf->extended_data != nullptr ? avf->extended_data : avf->data;
    const int channels = avf->ch_layout.nb_channels;
    const int tail_index = avf->nb_samples - 1;
    m_tail_samples.assign(static_cast<size_t>(channels), 0.0f);

    switch (static_cast<AVSampleFormat>(avf->format)) {
    case AV_SAMPLE_FMT_S16: {
      const int16_t *samples = reinterpret_cast<const int16_t *>(frame_data[0]);
      for (int channel = 0; channel < channels; ++channel) {
        m_tail_samples[static_cast<size_t>(channel)] =
            s16ToFloat(samples[tail_index * channels + channel]);
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_S16P: {
      for (int channel = 0; channel < channels; ++channel) {
        const int16_t *samples =
            reinterpret_cast<const int16_t *>(frame_data[channel]);
        m_tail_samples[static_cast<size_t>(channel)] =
            s16ToFloat(samples[tail_index]);
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLT: {
      const float *samples = reinterpret_cast<const float *>(frame_data[0]);
      for (int channel = 0; channel < channels; ++channel) {
        m_tail_samples[static_cast<size_t>(channel)] =
            samples[tail_index * channels + channel];
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLTP: {
      for (int channel = 0; channel < channels; ++channel) {
        const float *samples = reinterpret_cast<const float *>(frame_data[channel]);
        m_tail_samples[static_cast<size_t>(channel)] = samples[tail_index];
      }
      return HJ_OK;
    }
    default:
      m_tail_samples.clear();
      return HJErrNotSupport;
    }
  }

  int applyFadeOutToSilenceFrame(const HJMediaFrame::Ptr &io_frame,
                                 int i_fade_samples) const {
    if (!hasTailSamples()) {
      return HJErrNotAlready;
    }

    AVFrame *avf = nullptr;
    const int ret = validateAudioFrame(io_frame, avf);
    if (ret != HJ_OK) {
      return ret;
    }

    const int fade_samples = clampFadeSamples(avf, i_fade_samples);
    if (fade_samples <= 0 ||
        static_cast<int>(m_tail_samples.size()) != avf->ch_layout.nb_channels) {
      return HJErrInvalidParams;
    }

    const int writable_ret = av_frame_make_writable(avf);
    if (writable_ret < HJ_OK) {
      return writable_ret;
    }

    uint8_t *const *frame_data =
        avf->extended_data != nullptr ? avf->extended_data : avf->data;
    const int channels = avf->ch_layout.nb_channels;

    switch (static_cast<AVSampleFormat>(avf->format)) {
    case AV_SAMPLE_FMT_S16: {
      int16_t *samples = reinterpret_cast<int16_t *>(frame_data[0]);
      for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
        const float gain = fadeGain(sample_index, fade_samples, true);
        for (int channel = 0; channel < channels; ++channel) {
          samples[sample_index * channels + channel] =
              floatToS16(m_tail_samples[static_cast<size_t>(channel)] * gain);
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_S16P: {
      for (int channel = 0; channel < channels; ++channel) {
        int16_t *samples = reinterpret_cast<int16_t *>(frame_data[channel]);
        for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
          samples[sample_index] = floatToS16(
              m_tail_samples[static_cast<size_t>(channel)] *
              fadeGain(sample_index, fade_samples, true));
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLT: {
      float *samples = reinterpret_cast<float *>(frame_data[0]);
      for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
        const float gain = fadeGain(sample_index, fade_samples, true);
        for (int channel = 0; channel < channels; ++channel) {
          samples[sample_index * channels + channel] =
              m_tail_samples[static_cast<size_t>(channel)] * gain;
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLTP: {
      for (int channel = 0; channel < channels; ++channel) {
        float *samples = reinterpret_cast<float *>(frame_data[channel]);
        for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
          samples[sample_index] =
              m_tail_samples[static_cast<size_t>(channel)] *
              fadeGain(sample_index, fade_samples, true);
        }
      }
      return HJ_OK;
    }
    default:
      return HJErrNotSupport;
    }
  }

  int applyFadeIn(const HJMediaFrame::Ptr &io_frame, int i_fade_samples) const {
    return applyGainRamp(io_frame, 0.0f, 1.0f, i_fade_samples);
  }

  int applyCrossfadeFromTail(const HJMediaFrame::Ptr &io_frame,
                             int i_fade_samples) const {
    if (!hasTailSamples()) {
      return applyFadeIn(io_frame, i_fade_samples);
    }

    AVFrame *avf = nullptr;
    const int ret = validateAudioFrame(io_frame, avf);
    if (ret != HJ_OK) {
      return ret;
    }

    const int fade_samples = clampFadeSamples(avf, i_fade_samples);
    if (fade_samples <= 0 ||
        static_cast<int>(m_tail_samples.size()) != avf->ch_layout.nb_channels) {
      return HJErrInvalidParams;
    }

    const int writable_ret = av_frame_make_writable(avf);
    if (writable_ret < HJ_OK) {
      return writable_ret;
    }

    uint8_t *const *frame_data =
        avf->extended_data != nullptr ? avf->extended_data : avf->data;
    const int channels = avf->ch_layout.nb_channels;

    switch (static_cast<AVSampleFormat>(avf->format)) {
    case AV_SAMPLE_FMT_S16: {
      int16_t *samples = reinterpret_cast<int16_t *>(frame_data[0]);
      for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
        const float gain = fadeGain(sample_index, fade_samples, false);
        const float tail_gain = 1.0f - gain;
        for (int channel = 0; channel < channels; ++channel) {
          const size_t index =
              static_cast<size_t>(sample_index * channels + channel);
          samples[index] = floatToS16(
              m_tail_samples[static_cast<size_t>(channel)] * tail_gain +
              s16ToFloat(samples[index]) * gain);
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_S16P: {
      for (int channel = 0; channel < channels; ++channel) {
        int16_t *samples = reinterpret_cast<int16_t *>(frame_data[channel]);
        for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
          const float gain = fadeGain(sample_index, fade_samples, false);
          const float tail_gain = 1.0f - gain;
          samples[sample_index] =
              floatToS16(m_tail_samples[static_cast<size_t>(channel)] *
                             tail_gain +
                         s16ToFloat(samples[sample_index]) * gain);
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLT: {
      float *samples = reinterpret_cast<float *>(frame_data[0]);
      for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
        const float gain = fadeGain(sample_index, fade_samples, false);
        const float tail_gain = 1.0f - gain;
        for (int channel = 0; channel < channels; ++channel) {
          const size_t index =
              static_cast<size_t>(sample_index * channels + channel);
          samples[index] = m_tail_samples[static_cast<size_t>(channel)] *
                               tail_gain +
                           samples[index] * gain;
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLTP: {
      for (int channel = 0; channel < channels; ++channel) {
        float *samples = reinterpret_cast<float *>(frame_data[channel]);
        for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
          const float gain = fadeGain(sample_index, fade_samples, false);
          samples[sample_index] =
              m_tail_samples[static_cast<size_t>(channel)] * (1.0f - gain) +
              samples[sample_index] * gain;
        }
      }
      return HJ_OK;
    }
    default:
      return HJErrNotSupport;
    }
  }

  HJMediaFrame::Ptr makeConcealmentFrame(int64_t i_pts_ms, int64_t i_duration_ms,
                                         float i_start_gain,
                                         float i_end_gain,
                                         int i_crossfade_samples) const {
    if (!hasReferenceFrame()) {
      return nullptr;
    }

    auto frame = m_reference_frame->deepDup();
    if (frame == nullptr || frame->getAVFrame() == nullptr) {
      return nullptr;
    }

    frame->setPTSDTS(i_pts_ms, i_pts_ms);
    frame->setDuration(i_duration_ms, HJ_TIMEBASE_MS);
    frame->m_silence = false;
    if (applyGainRamp(frame, i_start_gain, i_end_gain) != HJ_OK) {
      return nullptr;
    }
    if (hasTailSamples() &&
        applyCrossfadeFromTail(frame, i_crossfade_samples) != HJ_OK) {
      return nullptr;
    }
    return frame;
  }

private:
  static int validateAudioFrame(const HJMediaFrame::Ptr &i_frame,
                                AVFrame *&o_avf) {
    if (i_frame == nullptr || !i_frame->isAudio()) {
      return HJErrInvalidParams;
    }

    o_avf = i_frame->getAVFrame();
    if (o_avf == nullptr || o_avf->nb_samples <= 0 ||
        o_avf->ch_layout.nb_channels <= 0) {
      return HJErrInvalidParams;
    }

    return HJ_OK;
  }

  static int clampFadeSamples(const AVFrame *i_avf, int i_fade_samples) {
    if (i_avf == nullptr || i_avf->nb_samples <= 0 || i_fade_samples <= 0) {
      return 0;
    }
    return (std::min)(i_avf->nb_samples, i_fade_samples);
  }

  static float fadeGain(int i_sample_index, int i_fade_samples,
                        bool i_fade_out) {
    if (i_fade_samples <= 1) {
      return i_fade_out ? 0.0f : 1.0f;
    }

    const float ratio = static_cast<float>(i_sample_index) /
                        static_cast<float>(i_fade_samples - 1);
    return i_fade_out ? (1.0f - ratio) : ratio;
  }

  static float s16ToFloat(int16_t i_sample) {
    return static_cast<float>(i_sample) / 32768.0f;
  }

  static int16_t floatToS16(float i_sample) {
    const float clamped =
        (std::max)(-1.0f, (std::min)(1.0f, i_sample));
    const long rounded = std::lround(clamped * 32767.0f);
    return static_cast<int16_t>(rounded);
  }

  static float interpolateGain(float i_start_gain, float i_end_gain,
                               int i_sample_index, int i_fade_samples) {
    if (i_fade_samples <= 1) {
      return i_end_gain;
    }

    const float ratio = static_cast<float>(i_sample_index) /
                        static_cast<float>(i_fade_samples - 1);
    return i_start_gain + (i_end_gain - i_start_gain) * ratio;
  }

  static int applyGainRamp(const HJMediaFrame::Ptr &io_frame,
                           float i_start_gain, float i_end_gain,
                           int i_fade_samples = -1) {
    AVFrame *avf = nullptr;
    const int ret = validateAudioFrame(io_frame, avf);
    if (ret != HJ_OK) {
      return ret;
    }

    const int fade_samples =
        i_fade_samples > 0 ? clampFadeSamples(avf, i_fade_samples)
                           : avf->nb_samples;
    if (fade_samples <= 0) {
      return HJErrInvalidParams;
    }

    const int writable_ret = av_frame_make_writable(avf);
    if (writable_ret < HJ_OK) {
      return writable_ret;
    }

    uint8_t *const *frame_data =
        avf->extended_data != nullptr ? avf->extended_data : avf->data;
    const int channels = avf->ch_layout.nb_channels;

    switch (static_cast<AVSampleFormat>(avf->format)) {
    case AV_SAMPLE_FMT_S16: {
      int16_t *samples = reinterpret_cast<int16_t *>(frame_data[0]);
      for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
        const float gain = interpolateGain(i_start_gain, i_end_gain,
                                           sample_index, fade_samples);
        for (int channel = 0; channel < channels; ++channel) {
          const size_t index =
              static_cast<size_t>(sample_index * channels + channel);
          samples[index] = floatToS16(s16ToFloat(samples[index]) * gain);
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_S16P: {
      for (int channel = 0; channel < channels; ++channel) {
        int16_t *samples = reinterpret_cast<int16_t *>(frame_data[channel]);
        for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
          samples[sample_index] = floatToS16(
              s16ToFloat(samples[sample_index]) *
              interpolateGain(i_start_gain, i_end_gain, sample_index,
                              fade_samples));
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLT: {
      float *samples = reinterpret_cast<float *>(frame_data[0]);
      for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
        const float gain = interpolateGain(i_start_gain, i_end_gain,
                                           sample_index, fade_samples);
        for (int channel = 0; channel < channels; ++channel) {
          samples[sample_index * channels + channel] *= gain;
        }
      }
      return HJ_OK;
    }
    case AV_SAMPLE_FMT_FLTP: {
      for (int channel = 0; channel < channels; ++channel) {
        float *samples = reinterpret_cast<float *>(frame_data[channel]);
        for (int sample_index = 0; sample_index < fade_samples; ++sample_index) {
          samples[sample_index] *= interpolateGain(i_start_gain, i_end_gain,
                                                   sample_index, fade_samples);
        }
      }
      return HJ_OK;
    }
    default:
      return HJErrNotSupport;
    }
  }

  HJMediaFrame::Ptr m_reference_frame;
  std::vector<float> m_tail_samples;
};

NS_HJ_END
