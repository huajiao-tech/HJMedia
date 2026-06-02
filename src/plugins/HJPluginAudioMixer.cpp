#include "HJPluginAudioMixer.h"

#include <algorithm>
#include <limits>

#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN

namespace {

enum AudioMixerWaitReason {
  kAudioMixerWaitNone = 0,
  kAudioMixerWaitStatusDone = 1,
  kAudioMixerWaitNotReady = 2,
  kAudioMixerWaitNoAttached = 3,
  kAudioMixerWaitNoPending = 4,
  kAudioMixerWaitStartup = 5,
  kAudioMixerWaitSteady = 6,
  kAudioMixerWaitAllEof = 7,
  kAudioMixerWaitMixNull = 8,
  kAudioMixerWaitJoin = 9,
};

constexpr char kThreadParamKey[] = "thread";
constexpr char kAudioMixerConfigParamKey[] = "audioMixerConfig";
constexpr char kCreateThreadParamKey[] = "createThread";
constexpr int64_t kInvalidPreviewPts = -1;
const int64_t kInvalidFirstPts = (std::numeric_limits<int64_t>::max)();
constexpr int kMixerDeclickFadeMs = 5;
constexpr size_t kMixerGapConcealmentFrames = 2;

std::string audioInfoToString(const HJAudioInfo::Ptr &i_audio_info) {
  return i_audio_info ? i_audio_info->formatInfo() : "null";
}

std::string mixerConfigToString(const HJAudioMixerConfig &i_config) {
  return HJFMT(
      "output_info:{}, max_inputs:{}, frame_samples:{}, sync_window_ms:{}, "
      "late_threshold_ms:{}, enable_compand:{}, enable_limiter:{}",
      audioInfoToString(i_config.m_output_info), i_config.m_max_inputs,
      i_config.m_frame_samples, i_config.m_sync_window_ms,
      i_config.m_late_threshold_ms, HJB2String(i_config.m_enable_compand),
      HJB2String(i_config.m_enable_limiter));
}

std::string inputDescToString(const HJAudioMixerInputDesc &i_input_desc) {
  return HJFMT("input_id:{}, input_info:{}, volume:{}",
               i_input_desc.m_input_id,
               audioInfoToString(i_input_desc.m_input_info),
               i_input_desc.m_volume);
}

std::string slotStateToString(size_t i_slot_index,
                              const HJAudioMixerSlotState &i_slot_state) {
  return HJFMT(
      "slot:{}, input_id:{}, input_key_hash:{}, attached:{}, volume:{}, "
      "seen_first_frame:{}, starved:{}, eof:{}, gap_concealment_frames:{}, "
      "silence_fill_frames:{}, "
      "late_drop_frames:{}",
      i_slot_index, i_slot_state.m_input_id, i_slot_state.m_input_key_hash,
      HJB2String(i_slot_state.m_attached), i_slot_state.m_volume,
      HJB2String(i_slot_state.m_seen_first_frame),
      HJB2String(i_slot_state.m_starved), HJB2String(i_slot_state.m_eof),
      i_slot_state.m_gap_concealment_frames, i_slot_state.m_silence_fill_frames,
      i_slot_state.m_late_drop_frames);
}

float normalizeSlotVolume(float i_volume) {
  return HJ_CLIP(i_volume, 0.0f, 10.0f);
}

bool isSlotSilenced(const HJAudioMixerSlotState &i_slot_state) {
  return normalizeSlotVolume(i_slot_state.m_volume) <= 0.0f;
}

std::string
slotStatesToString(const std::vector<HJAudioMixerSlotState> &i_slot_states) {
  std::vector<std::string> states;
  states.reserve(i_slot_states.size());
  for (size_t i = 0; i < i_slot_states.size(); ++i) {
    states.emplace_back(slotStateToString(i, i_slot_states[i]));
  }
  return HJUtilitys::vectorToString(states);
}

const char *waitReasonToString(int i_reason) {
  switch (i_reason) {
  case kAudioMixerWaitStatusDone:
    return "status_done";
  case kAudioMixerWaitNotReady:
    return "not_ready";
  case kAudioMixerWaitNoAttached:
    return "no_attached_inputs";
  case kAudioMixerWaitNoPending:
    return "no_pending_frames";
  case kAudioMixerWaitStartup:
    return "startup_wait";
  case kAudioMixerWaitSteady:
    return "steady_wait";
  case kAudioMixerWaitJoin:
    return "join_wait";
  case kAudioMixerWaitAllEof:
    return "all_eof";
  case kAudioMixerWaitMixNull:
    return "mix_return_null";
  default:
    return "none";
  }
}

int64_t buildWaitMetric(size_t i_ready_inputs, size_t i_wait_inputs,
                        int64_t i_queued_duration_max) {
  const int64_t ready_part = static_cast<int64_t>(i_ready_inputs & 0xffff);
  const int64_t wait_part = static_cast<int64_t>(i_wait_inputs & 0xffff);
  const int64_t queue_part = i_queued_duration_max & 0xffffffff;
  return (ready_part << 48) | (wait_part << 32) | queue_part;
}

int64_t ptsMsToSamples(const HJAudioMixerConfig &i_config, int64_t i_pts_ms) {
  if (i_config.m_output_info == nullptr ||
      i_config.m_output_info->m_samplesRate <= 0) {
    return 0;
  }

  return av_rescale_q(i_pts_ms, {1, 1000},
                      {1, i_config.m_output_info->m_samplesRate});
}

int64_t samplesToPtsMs(const HJAudioMixerConfig &i_config, int64_t i_samples) {
  if (i_config.m_output_info == nullptr ||
      i_config.m_output_info->m_samplesRate <= 0) {
    return 0;
  }

  return av_rescale_q(i_samples, {1, i_config.m_output_info->m_samplesRate},
                      {1, 1000});
}

int64_t frameStepMs(const HJAudioMixerConfig &i_config, int64_t i_pts_samples) {
  const int64_t next_pts_samples = i_pts_samples + i_config.m_frame_samples;
  return (std::max<int64_t>)(1, samplesToPtsMs(i_config, next_pts_samples) -
                                    samplesToPtsMs(i_config, i_pts_samples));
}

HJMediaFrame::Ptr makeMixerSilenceFrame(const HJAudioMixerConfig &i_config,
                                        int64_t i_pts_samples) {
  if (i_config.m_output_info == nullptr) {
    return nullptr;
  }

  auto output_info =
      std::dynamic_pointer_cast<HJAudioInfo>(i_config.m_output_info->dup());
  if (output_info == nullptr) {
    return nullptr;
  }
  output_info->m_sampleCnt = i_config.m_frame_samples;
  output_info->m_samplesPerFrame = i_config.m_frame_samples;

  auto frame = HJMediaFrame::makeSilenceAudioFrame(output_info);
  if (frame == nullptr) {
    return nullptr;
  }

  const int64_t pts_ms = samplesToPtsMs(i_config, i_pts_samples);
  frame->setPTSDTS(pts_ms, pts_ms);
  frame->setDuration(frameStepMs(i_config, i_pts_samples), HJ_TIMEBASE_MS);
  frame->m_silence = true;

  return frame;
}

int buildMixerDeclickFadeSamples(const HJAudioMixerConfig &i_config) {
  if (i_config.m_output_info == nullptr ||
      i_config.m_output_info->m_samplesRate <= 0) {
    return 0;
  }

  const int fade_samples = static_cast<int>(
      av_rescale_q(kMixerDeclickFadeMs, {1, 1000},
                   {1, i_config.m_output_info->m_samplesRate}));
  return (std::min)(i_config.m_frame_samples, (std::max)(1, fade_samples));
}

void updateSlotWithSilenceFrame(const HJAudioMixerConfig &i_config,
                                int64_t i_pts_samples,
                                HJAudioMixerSlotState &io_slot_state,
                                HJMediaFrame::Ptr &io_frame,
                                bool i_reset_gap_concealment = true) {
  io_frame = makeMixerSilenceFrame(i_config, i_pts_samples);
  if (io_frame == nullptr) {
    return;
  }

  if (!io_slot_state.m_outputting_silence &&
      io_slot_state.m_declicker.hasTailSamples()) {
    const int fade_ret = io_slot_state.m_declicker.applyFadeOutToSilenceFrame(
        io_frame, buildMixerDeclickFadeSamples(i_config));
    if (fade_ret != HJ_OK && fade_ret != HJErrNotAlready &&
        fade_ret != HJErrNotSupport) {
      HJFLogw("audiomixer.declick fade_out failed ret:{}, input_id:{}",
              fade_ret, io_slot_state.m_input_id);
    }
  }

  const int capture_ret = io_slot_state.m_declicker.captureTailSamples(io_frame);
  if (capture_ret != HJ_OK && capture_ret != HJErrNotSupport) {
    HJFLogw("audiomixer.declick capture_silence_tail failed ret:{}, input_id:{}",
            capture_ret, io_slot_state.m_input_id);
  }

  io_slot_state.m_outputting_silence = true;
  io_slot_state.m_pending_fade_in = true;
  if (i_reset_gap_concealment) {
    io_slot_state.m_gap_concealment_frames = 0;
  }
}

float buildGapConcealmentGain(size_t i_frame_index) {
  const float step = static_cast<float>(i_frame_index) /
                     static_cast<float>(kMixerGapConcealmentFrames);
  return (std::max)(0.0f, 1.0f - step);
}

void updateSlotWithConcealmentFrame(const HJAudioMixerConfig &i_config,
                                    int64_t i_pts_samples,
                                    HJAudioMixerSlotState &io_slot_state,
                                    HJMediaFrame::Ptr &io_frame) {
  if (!io_slot_state.m_declicker.hasReferenceFrame() ||
      io_slot_state.m_gap_concealment_frames >= kMixerGapConcealmentFrames) {
    updateSlotWithSilenceFrame(i_config, i_pts_samples, io_slot_state, io_frame,
                               false);
    return;
  }

  const size_t concealment_index = io_slot_state.m_gap_concealment_frames;
  const float start_gain = buildGapConcealmentGain(concealment_index);
  const float end_gain = buildGapConcealmentGain(concealment_index + 1);
  io_frame = io_slot_state.m_declicker.makeConcealmentFrame(
      samplesToPtsMs(i_config, i_pts_samples), frameStepMs(i_config, i_pts_samples),
      start_gain, end_gain, buildMixerDeclickFadeSamples(i_config));
  if (io_frame == nullptr) {
    updateSlotWithSilenceFrame(i_config, i_pts_samples, io_slot_state, io_frame,
                               false);
    return;
  }

  const int capture_ret = io_slot_state.m_declicker.captureTailSamples(io_frame);
  if (capture_ret != HJ_OK && capture_ret != HJErrNotSupport) {
    HJFLogw(
        "audiomixer.declick capture_concealment_tail failed ret:{}, input_id:{}",
        capture_ret, io_slot_state.m_input_id);
  }

  io_slot_state.m_outputting_silence = false;
  io_slot_state.m_pending_fade_in = true;
  ++io_slot_state.m_gap_concealment_frames;
}

void updateSlotWithRealFrame(const HJAudioMixerConfig &i_config,
                             HJAudioMixerSlotState &io_slot_state,
                             HJMediaFrame::Ptr &io_frame) {
  if (io_frame == nullptr) {
    return;
  }

  if (io_slot_state.m_pending_fade_in) {
    const int fade_ret = io_slot_state.m_declicker.applyCrossfadeFromTail(
        io_frame, buildMixerDeclickFadeSamples(i_config));
    if (fade_ret != HJ_OK && fade_ret != HJErrNotSupport) {
      HJFLogw("audiomixer.declick fade_in failed ret:{}, input_id:{}",
              fade_ret, io_slot_state.m_input_id);
    }
  }

  const int reference_ret =
      io_slot_state.m_declicker.captureReferenceFrame(io_frame);
  if (reference_ret != HJ_OK && reference_ret != HJErrNotSupport) {
    HJFLogw("audiomixer.declick capture_reference failed ret:{}, input_id:{}",
            reference_ret, io_slot_state.m_input_id);
  }

  const int capture_ret = io_slot_state.m_declicker.captureTailSamples(io_frame);
  if (capture_ret != HJ_OK && capture_ret != HJErrNotSupport) {
    HJFLogw("audiomixer.declick capture_tail failed ret:{}, input_id:{}",
            capture_ret, io_slot_state.m_input_id);
  }

  io_slot_state.m_outputting_silence = false;
  io_slot_state.m_pending_fade_in = false;
  io_slot_state.m_gap_concealment_frames = 0;
}

void resetSlotRuntimeState(HJAudioMixerSlotState &io_slot_state) {
  io_slot_state.m_seen_first_frame = false;
  io_slot_state.m_starved = false;
  io_slot_state.m_eof = false;
  io_slot_state.m_outputting_silence = true;
  io_slot_state.m_pending_fade_in = false;
  io_slot_state.m_gap_concealment_frames = 0;
  io_slot_state.m_declicker.reset();
}

} // namespace

int HJPluginAudioMixer::configureLocked(const HJAudioMixerConfig &i_config) {
  CHECK_DONE_STATUS(HJErrAlreadyDone);

  HJFLogi("{}, audiomixer.configure request {}", getName(),
          mixerConfigToString(i_config));
  m_config = i_config;
  m_config.m_output_info =
      std::dynamic_pointer_cast<HJAudioInfo>(i_config.m_output_info->dup());
  if (m_config.m_output_info == nullptr) {
    HJFLoge("{}, audiomixer.configure invalid output_info, request {}",
            getName(), mixerConfigToString(i_config));
    return HJErrInvalidParams;
  }
  m_config.m_output_info->m_sampleCnt = m_config.m_frame_samples;
  m_config.m_output_info->m_samplesPerFrame = m_config.m_frame_samples;

  m_slot_states.assign(m_config.m_max_inputs, HJAudioMixerSlotState{});
  m_input_slot_map.clear();
  m_mix_samples.store(0);
  m_output_frames.store(0);

  if (m_audio_mixer == nullptr) {
    m_audio_mixer = std::make_shared<HJAudioMixer>();
  }
  const int ret = m_audio_mixer->init(m_config);
  if (ret == HJ_OK) {
    resetWaitLogState();
    HJFLogi("{}, audiomixer.configure result:{}, {}", getName(), ret,
            mixerConfigToString(m_config));
  } else {
    HJFLoge("{}, audiomixer.configure failed ret:{}, {}", getName(), ret,
            mixerConfigToString(m_config));
  }
  return ret;
}

int HJPluginAudioMixer::configure(const HJAudioMixerConfig &i_config) {
  if (i_config.m_output_info == nullptr || i_config.m_max_inputs <= 0) {
    HJFLogw("{}, audiomixer.configure invalid params {}", getName(),
            mixerConfigToString(i_config));
    return HJErrInvalidParams;
  }

  return SYNC_PROD_LOCK(
      [this, &i_config] { return configureLocked(i_config); });
}

int HJPluginAudioMixer::bindInput(size_t i_slot_index, size_t i_input_key_hash,
                                  const HJAudioMixerInputDesc &i_input_desc) {
  if (i_slot_index >= m_slot_states.size() || i_input_key_hash == 0 ||
      i_input_desc.m_input_info == nullptr) {
    HJFLogw("{}, audiomixer.bind invalid params slot:{}, input_key_hash:{}, "
            "desc:{}",
            getName(), i_slot_index, i_input_key_hash,
            inputDescToString(i_input_desc));
    return HJErrInvalidParams;
  }

  return SYNC_PROD_LOCK([this, i_slot_index, i_input_key_hash, &i_input_desc] {
    CHECK_DONE_STATUS(HJErrAlreadyDone);

    if (getInput(i_input_key_hash) == nullptr) {
      HJFLogw("{}, audiomixer.bind input not found slot:{}, input_key_hash:{}, "
              "desc:{}",
              getName(), i_slot_index, i_input_key_hash,
              inputDescToString(i_input_desc));
      return HJErrNotFind;
    }

    auto &slot_state = m_slot_states[i_slot_index];
    slot_state = HJAudioMixerSlotState{};
    slot_state.m_input_id = i_input_desc.m_input_id;
    slot_state.m_input_key_hash = i_input_key_hash;
    slot_state.m_attached = true;
    slot_state.m_volume = i_input_desc.m_volume;

    m_input_slot_map[i_input_key_hash] = i_slot_index;
    int ret = m_audio_mixer
                  ? m_audio_mixer->configureInput(i_slot_index, i_input_desc)
                  : HJErrNotAlready;
    if (ret == HJ_OK) {
      resetWaitLogState();
      HJFLogi("{}, audiomixer.bind result:{}, {}, {}", getName(), ret,
              inputDescToString(i_input_desc),
              slotStateToString(i_slot_index, slot_state));
      publishInputState(i_slot_index);
    } else {
      HJFLogw("{}, audiomixer.bind failed ret:{}, slot:{}, input_key_hash:{}, "
              "desc:{}",
              getName(), ret, i_slot_index, i_input_key_hash,
              inputDescToString(i_input_desc));
    }
    return ret;
  });
}

int HJPluginAudioMixer::unbindInput(size_t i_slot_index) {
  const int slot_ret = validateSlotIndex(i_slot_index, "unbind");
  if (slot_ret != HJ_OK) {
    return slot_ret;
  }

  return SYNC_PROD_LOCK([this, i_slot_index] {
    CHECK_DONE_STATUS(HJErrAlreadyDone);

    const int attached_ret = ensureAttachedSlot(i_slot_index, "unbind");
    if (attached_ret != HJ_OK) {
      return attached_ret;
    }

    auto &slot_state = m_slot_states[i_slot_index];
    const std::string slot_summary =
        slotStateToString(i_slot_index, slot_state);
    m_input_slot_map.erase(slot_state.m_input_key_hash);
    slot_state = HJAudioMixerSlotState{};

    int ret = m_audio_mixer ? m_audio_mixer->clearInput(i_slot_index)
                            : HJErrNotAlready;
    if (ret == HJ_OK) {
      resetWaitLogState();
      HJFLogi("{}, audiomixer.unbind result:{}, previous:{}", getName(), ret,
              slot_summary);
      publishInputState(i_slot_index);
    } else {
      HJFLogw("{}, audiomixer.unbind failed ret:{}, previous:{}", getName(),
              ret, slot_summary);
    }
    return ret;
  });
}

int HJPluginAudioMixer::flushInput(size_t i_slot_index) {
  const int slot_ret = validateSlotIndex(i_slot_index, "flush");
  if (slot_ret != HJ_OK) {
    return slot_ret;
  }

  const int attached_ret = ensureAttachedSlot(i_slot_index, "flush");
  if (attached_ret != HJ_OK) {
    return attached_ret;
  }

  auto &slot_state = m_slot_states[i_slot_index];
  auto input = getInput(slot_state.m_input_key_hash);
  if (input == nullptr) {
    HJFLogw("{}, audiomixer.flush input not found {}", getName(),
            slotStateToString(i_slot_index, slot_state));
    return HJErrNotFind;
  }

  const std::string slot_summary = slotStateToString(i_slot_index, slot_state);
  input->mediaFrames.flush(false);
  const bool publish_state = shouldPublishInputState(slot_state);
  resetSlotRuntimeState(slot_state);
  if (publish_state) {
    publishInputState(i_slot_index);
  }
  resetWaitLogState();
  HJFLogi("{}, audiomixer.flush {}", getName(), slot_summary);
  onInputUpdated();
  return HJ_OK;
}

int HJPluginAudioMixer::setInputVolume(size_t i_slot_index, float i_volume) {
  if (i_slot_index >= m_slot_states.size() || m_audio_mixer == nullptr) {
    HJFLogw("{}, audiomixer.volume invalid params slot:{}, volume:{}, "
            "mixer_ready:{}",
            getName(), i_slot_index, i_volume,
            HJB2String(m_audio_mixer != nullptr));
    return HJErrInvalidParams;
  }

  return SYNC_PROD_LOCK([this, i_slot_index, i_volume] {
    CHECK_DONE_STATUS(HJErrAlreadyDone);
    const int attached_ret = ensureAttachedSlot(i_slot_index, "volume");
    if (attached_ret != HJ_OK) {
      return attached_ret;
    }

    const int ret = m_audio_mixer->setInputVolume(i_slot_index, i_volume);
    if (ret == HJ_OK) {
      m_slot_states[i_slot_index].m_volume = i_volume;
      resetWaitLogState();
      HJFLogd("{}, audiomixer.volume slot:{}, input_id:{}, volume:{}",
              getName(), i_slot_index, m_slot_states[i_slot_index].m_input_id,
              i_volume);
    } else {
      HJFLogw("{}, audiomixer.volume failed ret:{}, slot:{}, input_id:{}, "
              "volume:{}",
              getName(), ret, i_slot_index,
              m_slot_states[i_slot_index].m_input_id, i_volume);
    }
    return ret;
  });
}

int HJPluginAudioMixer::setMasterMute(bool i_muted) {
  if (m_audio_mixer == nullptr) {
    HJFLogw("{}, audiomixer.master_mute mixer not ready muted:{}", getName(),
            HJB2String(i_muted));
    return HJErrNotAlready;
  }
  const int ret = m_audio_mixer->setMasterMute(i_muted);
  if (ret == HJ_OK) {
    resetWaitLogState();
    HJFLogi("{}, audiomixer.master_mute result:{}, muted:{}", getName(), ret,
            HJB2String(i_muted));
  } else {
    HJFLogw("{}, audiomixer.master_mute failed ret:{}, muted:{}", getName(),
            ret, HJB2String(i_muted));
  }
  return ret;
}

bool HJPluginAudioMixer::isMasterMuted() const {
  return m_audio_mixer ? m_audio_mixer->isMasterMuted() : false;
}

const HJAudioInfo::Ptr &HJPluginAudioMixer::getOutputInfo() const {
  if (m_audio_mixer != nullptr) {
    return m_audio_mixer->getOutputInfo();
  }
  return m_config.m_output_info;
}

int HJPluginAudioMixer::internalInit(HJKeyStorage::Ptr i_param) {
  auto param = HJKeyStorage::dupFrom(
      i_param ? i_param : std::make_shared<HJKeyStorage>());
  HJLooperThread::Ptr thread{};
  if (i_param != nullptr && i_param->haveValue(kThreadParamKey)) {
    thread = i_param->getValue<HJLooperThread::Ptr>(kThreadParamKey);
  }
  (*param)[kCreateThreadParamKey] = (thread == nullptr);
  HJFLogi("{}, audiomixer.init begin has_param:{}, external_thread:{}, "
          "has_config:{}",
          getName(), HJB2String(i_param != nullptr),
          HJB2String(thread != nullptr),
          HJB2String(i_param != nullptr &&
                        i_param->haveValue(kAudioMixerConfigParamKey)));

  int ret = HJPlugin::internalInit(param);
  if (ret != HJ_OK) {
    HJFLoge("{}, audiomixer.init base init failed ret:{}", getName(), ret);
    return ret;
  }

  if (i_param != nullptr && i_param->haveValue(kAudioMixerConfigParamKey)) {
    auto config =
        i_param->getValue<HJAudioMixerConfig>(kAudioMixerConfigParamKey);
    if (config.m_output_info == nullptr || config.m_max_inputs <= 0) {
      HJFLoge("{}, audiomixer.init invalid config {}", getName(),
              mixerConfigToString(config));
      return HJErrInvalidParams;
    }
    ret = configureLocked(config);
    if (ret != HJ_OK) {
      HJFLoge("{}, audiomixer.init configure failed ret:{}, {}", getName(), ret,
              mixerConfigToString(config));
      return ret;
    }
  }

  ret = m_config.m_output_info ? HJ_OK : HJErrInvalidParams;
  if (ret == HJ_OK) {
    HJFLogi("{}, audiomixer.init ready {}", getName(),
            mixerConfigToString(m_config));
  } else {
    HJFLoge("{}, audiomixer.init missing output config after init", getName());
  }
  return ret;
}

void HJPluginAudioMixer::internalRelease() {
  const HJGraphAudioMixerStats stats = buildStats();
  HJFLogi("{}, audiomixer.release begin mix_pts:{}, output_frames:{}, "
          "active_inputs:{}, starved_inputs:{}, silence_fill_frames:{}, "
          "late_drop_frames:{}, slot_states:{}",
          getName(), stats.m_mix_pts, stats.m_output_frames,
          stats.m_active_inputs, stats.m_starved_inputs,
          stats.m_silence_fill_frames, stats.m_late_drop_frames,
          slotStatesToString(m_slot_states));
  if (m_audio_mixer != nullptr) {
    m_audio_mixer->done();
    m_audio_mixer = nullptr;
  }

  m_slot_states.clear();
  m_input_slot_map.clear();
  m_mix_samples.store(0);
  m_output_frames.store(0);
  resetWaitLogState();

  HJPlugin::internalRelease();
  HJFLogi("{}, audiomixer.release end", getName());
}

void HJPluginAudioMixer::onInputAdded(size_t i_src_key_hash,
                                      HJMediaType i_type) {
  HJPlugin::onInputAdded(i_src_key_hash, i_type);
  auto input = m_inputs[i_src_key_hash];
  if (input != nullptr && i_type == HJMEDIA_TYPE_AUDIO) {
    input->eventFlags |= EVENT_FLAG_AUDIO_DURATION;
    HJFLogd(
        "{}, audiomixer.input_added input_key_hash:{}, type:{}, event_flags:{}",
        getName(), i_src_key_hash, HJMediaType2String(i_type), input->eventFlags);
  }
}

HJGraphAudioMixerInputState
HJPluginAudioMixer::buildInputState(size_t i_slot_index) const {
  HJGraphAudioMixerInputState state{};
  if (i_slot_index >= m_slot_states.size()) {
    return state;
  }

  const auto &slot_state = m_slot_states[i_slot_index];
  state.m_input_id = slot_state.m_input_id;
  state.m_slot_index = i_slot_index;
  state.m_attached = slot_state.m_attached;
  state.m_starved = slot_state.m_starved;
  state.m_eof = slot_state.m_eof;
  return state;
}

HJGraphAudioMixerStats HJPluginAudioMixer::buildStats() const {
  HJGraphAudioMixerStats stats{};
  stats.m_mix_pts = samplesToPtsMs(m_config, m_mix_samples.load());
  stats.m_output_frames = m_output_frames.load();
  for (const auto &slot_state : m_slot_states) {
    stats.m_silence_fill_frames += slot_state.m_silence_fill_frames;
    stats.m_late_drop_frames += slot_state.m_late_drop_frames;
    if (slot_state.m_attached && !slot_state.m_eof) {
      ++stats.m_active_inputs;
    }
    if (slot_state.m_starved) {
      ++stats.m_starved_inputs;
    }
  }
  return stats;
}

int HJPluginAudioMixer::findSlotIndexByInputKeyHash(
    size_t i_input_key_hash, size_t &o_slot_index) const {
  auto it = m_input_slot_map.find(i_input_key_hash);
  if (it == m_input_slot_map.end()) {
    return HJErrNotFind;
  }
  o_slot_index = it->second;
  return HJ_OK;
}

int HJPluginAudioMixer::validateSlotIndex(size_t i_slot_index,
                                          const char *i_action) const {
  if (i_slot_index < m_slot_states.size()) {
    return HJ_OK;
  }

  HJFLogw("{}, audiomixer.{} invalid slot:{}", getName(), i_action,
          i_slot_index);
  return HJErrInvalidParams;
}

int HJPluginAudioMixer::ensureAttachedSlot(size_t i_slot_index,
                                           const char *i_action) const {
  const auto &slot_state = m_slot_states[i_slot_index];
  if (slot_state.m_attached) {
    return HJ_OK;
  }

  HJFLogw("{}, audiomixer.{} slot not attached {}", getName(), i_action,
          slotStateToString(i_slot_index, slot_state));
  return HJErrNotAlready;
}

void HJPluginAudioMixer::publishInputState(size_t i_slot_index) {
  publish(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
          buildInputState(i_slot_index));
}

bool HJPluginAudioMixer::shouldPublishInputState(
    const HJAudioMixerSlotState &i_slot_state) const {
  return i_slot_state.m_attached &&
         (i_slot_state.m_seen_first_frame || i_slot_state.m_starved ||
          i_slot_state.m_eof);
}

void HJPluginAudioMixer::resetAllSlotRuntimeStates() {
  for (size_t i = 0; i < m_slot_states.size(); ++i) {
    auto &slot_state = m_slot_states[i];
    const bool should_publish = shouldPublishInputState(slot_state);
    resetSlotRuntimeState(slot_state);
    if (should_publish) {
      publishInputState(i);
    }
  }
}

int HJPluginAudioMixer::handleClearPreviewFrame(
    size_t i_slot_index, const HJAudioMixerSlotState &i_slot_state,
    int64_t i_mix_pts) {
  HJFLogi("{}, audiomixer.state event:clear_frame slot:{}, input_id:{}, "
          "mix_pts:{}, action:flush_reset",
          getName(), i_slot_index, i_slot_state.m_input_id, i_mix_pts);
  receive(i_slot_state.m_input_key_hash);
  for (size_t i = 0; i < m_slot_states.size(); ++i) {
    const auto &slot_state = m_slot_states[i];
    if (!slot_state.m_attached || slot_state.m_input_key_hash == 0) {
      continue;
    }

    auto input = getInput(slot_state.m_input_key_hash);
    if (input == nullptr) {
      continue;
    }

    input->mediaFrames.flush(false);
  }
  m_mix_samples.store(0);
  m_output_frames.store(0);
  resetAllSlotRuntimeStates();
  resetWaitLogState();

  if (m_audio_mixer != nullptr) {
    int ret = m_audio_mixer->init(m_config);
    if (ret != HJ_OK) {
      HJFLoge("{}, audiomixer.clear reset mixer failed ret:{}, slot:{}",
              getName(), ret, i_slot_index);
      return ret;
    }

    for (size_t i = 0; i < m_slot_states.size(); ++i) {
      const auto &slot_state = m_slot_states[i];
      if (!slot_state.m_attached) {
        continue;
      }

      HJAudioMixerInputDesc input_desc{};
      input_desc.m_input_id = slot_state.m_input_id;
      input_desc.m_input_info =
          std::dynamic_pointer_cast<HJAudioInfo>(m_config.m_output_info->dup());
      input_desc.m_volume = slot_state.m_volume;
      if (input_desc.m_input_info == nullptr) {
        HJFLoge("{}, audiomixer.clear dup input info failed slot:{}, input_id:{}",
                getName(), i, slot_state.m_input_id);
        return HJErrInvalidData;
      }

      ret = m_audio_mixer->configureInput(i, input_desc);
      if (ret != HJ_OK) {
        HJFLoge("{}, audiomixer.clear restore input failed ret:{}, slot:{}, "
                "input_id:{}",
                getName(), ret, i, slot_state.m_input_id);
        return ret;
      }
    }
  }

  return runFlush();
}

bool HJPluginAudioMixer::isFrameLateForMix(const HJMediaFrame::Ptr &i_frame,
                                           int64_t i_mix_pts) const {
  if (i_frame == nullptr || i_frame->isClearFrame() || i_frame->isEofFrame()) {
    return false;
  }

  const int64_t late_window_ms = m_config.m_sync_window_ms / 2;
  return i_frame->getPTS() < i_mix_pts - late_window_ms;
}

bool HJPluginAudioMixer::canFrameParticipateInMix(
    const HJMediaFrame::Ptr &i_frame, int64_t i_mix_pts) const {
  if (i_frame == nullptr || i_frame->isClearFrame() || i_frame->isEofFrame()) {
    return false;
  }

  return i_frame->getPTS() >= i_mix_pts - m_config.m_sync_window_ms &&
         i_frame->getPTS() <= i_mix_pts + m_config.m_sync_window_ms;
}

HJMediaFrame::Ptr
HJPluginAudioMixer::dropLatePreviewFrames(size_t i_slot_index,
                                          HJAudioMixerSlotState &io_slot_state,
                                          int64_t i_mix_pts) {
  HJMediaFrame::Ptr preview_frame = preview(io_slot_state.m_input_key_hash);
  size_t dropped_frames = 0;
  while (preview_frame != nullptr && !preview_frame->isClearFrame() &&
         !preview_frame->isEofFrame() &&
         isFrameLateForMix(preview_frame, i_mix_pts)) {
    HJFLogw("{}, audiomixer.state event:late_drop slot:{}, input_id:{}, "
            "input_key_hash:{}, pts:{}, mix_pts:{}",
            getName(), i_slot_index, io_slot_state.m_input_id,
            io_slot_state.m_input_key_hash, preview_frame->getPTS(), i_mix_pts);
    receive(io_slot_state.m_input_key_hash);
    ++io_slot_state.m_late_drop_frames;
    ++dropped_frames;
    preview_frame = preview(io_slot_state.m_input_key_hash);
  }

  if (dropped_frames > 0) {
    HJFLogi(
        "{}, audiomixer.state event:late_drop_summary slot:{}, input_id:{}, "
        "input_key_hash:{}, drop_count:{}, mix_pts:{}, next_preview_pts:{}",
        getName(), i_slot_index, io_slot_state.m_input_id,
        io_slot_state.m_input_key_hash, dropped_frames, i_mix_pts,
        preview_frame ? preview_frame->getPTS() : kInvalidPreviewPts);
    report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIOMIXER_MIXFRAME,
           getID());
  }

  return preview_frame;
}

int HJPluginAudioMixer::scanInputsForMix(bool i_in_startup,
                                         int64_t i_gate_mix_pts,
                                         HJAudioMixerScanState &o_scan_state) {
  o_scan_state = HJAudioMixerScanState{};
  o_scan_state.m_first_pts = kInvalidFirstPts;
  o_scan_state.m_queued_duration.assign(m_slot_states.size(), 0);

  for (size_t i = 0; i < m_slot_states.size(); ++i) {
    auto &slot_state = m_slot_states[i];
    if (!slot_state.m_attached || slot_state.m_input_key_hash == 0) {
      continue;
    }
    o_scan_state.m_has_attached = true;

    auto input = getInput(slot_state.m_input_key_hash);
    if (input == nullptr) {
      continue;
    }

    auto preview_frame =
        i_in_startup
            ? preview(slot_state.m_input_key_hash)
            : dropLatePreviewFrames(i, slot_state, i_gate_mix_pts);
    if (preview_frame != nullptr && preview_frame->isClearFrame()) {
      o_scan_state.m_handled_clear = true;
      return handleClearPreviewFrame(i, slot_state, i_gate_mix_pts);
    }

    const int64_t queued_duration = input->mediaFrames.audioDuration();
    o_scan_state.m_queued_duration[i] = static_cast<size_t>(queued_duration);
    o_scan_state.m_queued_duration_max =
        (std::max)(o_scan_state.m_queued_duration_max, queued_duration);
    if (preview_frame != nullptr) {
      o_scan_state.m_has_pending = true;
    }

    const bool slot_active =
        !slot_state.m_eof &&
        !(preview_frame != nullptr && preview_frame->isEofFrame());
    if (!slot_active) {
      continue;
    }

    ++o_scan_state.m_active_inputs;
    if (!slot_state.m_seen_first_frame && !isSlotSilenced(slot_state)) {
      o_scan_state.m_has_unseen_inputs = true;
    }
    if (isSlotSilenced(slot_state)) {
      continue;
    }

    ++o_scan_state.m_startup_wait_inputs;
    if (preview_frame != nullptr && !preview_frame->isClearFrame() &&
        !preview_frame->isEofFrame()) {
      ++o_scan_state.m_startup_ready_inputs;
      o_scan_state.m_first_pts =
          (std::min)(o_scan_state.m_first_pts, preview_frame->getPTS());
      if (canFrameParticipateInMix(preview_frame, i_gate_mix_pts)) {
        ++o_scan_state.m_join_ready_inputs;
      }
    }

    if (i_in_startup || !slot_state.m_seen_first_frame) {
      continue;
    }

    ++o_scan_state.m_steady_wait_inputs;
    if (canFrameParticipateInMix(preview_frame, i_gate_mix_pts)) {
      ++o_scan_state.m_steady_ready_inputs;
    }
  }

  return HJ_OK;
}

int HJPluginAudioMixer::waitForMixReadiness(
    bool i_in_startup, int64_t i_gate_mix_pts,
    const HJAudioMixerScanState &i_scan_state) {
  if (!i_scan_state.m_has_attached) {
    logWaitReason(kAudioMixerWaitNoAttached, i_gate_mix_pts,
                  static_cast<int64_t>(m_slot_states.size()),
                  HJFMT("slot_count:{}, output_frames:{}", m_slot_states.size(),
                        m_output_frames.load()));
    return HJ_WOULD_BLOCK;
  }

  if (!i_scan_state.m_has_pending) {
    logWaitReason(
        kAudioMixerWaitNoPending, i_gate_mix_pts,
        i_scan_state.m_queued_duration_max,
        HJFMT("active_inputs:{}, queued_duration_max:{}, queued_duration:{}",
              i_scan_state.m_active_inputs, i_scan_state.m_queued_duration_max,
              HJUtilitys::vectorToString(i_scan_state.m_queued_duration)));
    return HJ_WOULD_BLOCK;
  }

  const bool wait_deadline_reached =
      m_config.m_late_threshold_ms <= 0 ||
      i_scan_state.m_queued_duration_max >=
          static_cast<int64_t>(m_config.m_late_threshold_ms);
  if (i_in_startup && i_scan_state.m_startup_wait_inputs > 0) {
    bool startup_ready = false;
    if (i_scan_state.m_startup_wait_inputs == 1) {
      startup_ready = (i_scan_state.m_startup_ready_inputs == 1);
    } else {
      startup_ready = i_scan_state.m_startup_ready_inputs > 0 &&
                      ((i_scan_state.m_startup_ready_inputs ==
                        i_scan_state.m_startup_wait_inputs) ||
                       wait_deadline_reached);
    }

    if (startup_ready) {
      return HJ_OK;
    }

    logWaitReason(
        kAudioMixerWaitStartup, i_gate_mix_pts,
        buildWaitMetric(i_scan_state.m_startup_ready_inputs,
                        i_scan_state.m_startup_wait_inputs,
                        i_scan_state.m_queued_duration_max),
        HJFMT("startup_ready:{}/{}, wait_deadline_reached:{}, first_pts:{}, "
              "queued_duration_max:{}, queued_duration:{}",
              i_scan_state.m_startup_ready_inputs,
              i_scan_state.m_startup_wait_inputs,
              HJB2String(wait_deadline_reached), i_scan_state.m_first_pts,
              i_scan_state.m_queued_duration_max,
              HJUtilitys::vectorToString(i_scan_state.m_queued_duration)));
    return HJ_WOULD_BLOCK;
  }

  const bool should_wait_join =
      !i_in_startup && i_scan_state.m_has_unseen_inputs &&
      i_scan_state.m_startup_wait_inputs > 1 &&
      i_scan_state.m_join_ready_inputs < i_scan_state.m_startup_wait_inputs &&
      !wait_deadline_reached;
  if (should_wait_join) {
    logWaitReason(
        kAudioMixerWaitJoin, i_gate_mix_pts,
        buildWaitMetric(i_scan_state.m_join_ready_inputs,
                        i_scan_state.m_startup_wait_inputs,
                        i_scan_state.m_queued_duration_max),
        HJFMT("join_ready:{}/{}, startup_ready:{}/{}, "
              "wait_deadline_reached:{}, first_pts:{}, "
              "queued_duration_max:{}, queued_duration:{}",
              i_scan_state.m_join_ready_inputs,
              i_scan_state.m_startup_wait_inputs,
              i_scan_state.m_startup_ready_inputs,
              i_scan_state.m_startup_wait_inputs,
              HJB2String(wait_deadline_reached), i_scan_state.m_first_pts,
              i_scan_state.m_queued_duration_max,
              HJUtilitys::vectorToString(i_scan_state.m_queued_duration)));
    return HJ_WOULD_BLOCK;
  }

  const bool should_wait_steady =
      !i_in_startup && m_config.m_late_threshold_ms > 0 &&
      i_scan_state.m_steady_wait_inputs > 1 &&
      i_scan_state.m_steady_ready_inputs < i_scan_state.m_steady_wait_inputs &&
      !wait_deadline_reached;
  if (!should_wait_steady) {
    return HJ_OK;
  }

  logWaitReason(
      kAudioMixerWaitSteady, i_gate_mix_pts,
      buildWaitMetric(i_scan_state.m_steady_ready_inputs,
                      i_scan_state.m_steady_wait_inputs,
                      i_scan_state.m_queued_duration_max),
      HJFMT("steady_ready:{}/{}, wait_deadline_reached:{}, "
            "queued_duration_max:{}, queued_duration:{}",
            i_scan_state.m_steady_ready_inputs,
            i_scan_state.m_steady_wait_inputs,
            HJB2String(wait_deadline_reached),
            i_scan_state.m_queued_duration_max,
            HJUtilitys::vectorToString(i_scan_state.m_queued_duration)));
  return HJ_WOULD_BLOCK;
}

HJPluginAudioMixer::HJAudioMixerBuildState
HJPluginAudioMixer::prepareMixBuildState(
    bool i_in_startup, const HJAudioMixerScanState &i_scan_state) {
  HJAudioMixerBuildState build_state{};
  build_state.m_in_startup = i_in_startup;
  build_state.m_active_inputs = i_scan_state.m_active_inputs;
  build_state.m_mix_samples = m_mix_samples.load();
  if (i_in_startup) {
    build_state.m_mix_samples =
        i_scan_state.m_first_pts == kInvalidFirstPts
            ? 0
            : ptsMsToSamples(m_config, i_scan_state.m_first_pts);
    m_mix_samples.store(build_state.m_mix_samples);
  }

  build_state.m_mix_pts = samplesToPtsMs(m_config, build_state.m_mix_samples);
  build_state.m_mix_frames.assign(static_cast<size_t>(m_config.m_max_inputs),
                                  nullptr);
  return build_state;
}

int HJPluginAudioMixer::fillMixFrames(HJAudioMixerBuildState &io_build_state) {
  for (size_t i = 0; i < m_slot_states.size(); ++i) {
    const int ret = fillMixFrameForSlot(i, io_build_state);
    if (io_build_state.m_handled_clear || ret != HJ_OK) {
      return ret;
    }
  }

  return HJ_OK;
}

int HJPluginAudioMixer::fillMixFrameForSlot(
    size_t i_slot_index, HJAudioMixerBuildState &io_build_state) {
  auto &slot_state = m_slot_states[i_slot_index];
  if (!slot_state.m_attached || slot_state.m_input_key_hash == 0) {
    updateSlotWithSilenceFrame(m_config, io_build_state.m_mix_samples,
                               slot_state,
                               io_build_state.m_mix_frames[i_slot_index]);
    return HJ_OK;
  }
  auto input = getInput(slot_state.m_input_key_hash);
  if (input == nullptr) {
    if (!slot_state.m_eof) {
      io_build_state.m_all_eof = false;
    }
    updateSlotWithSilenceFrame(m_config, io_build_state.m_mix_samples,
                               slot_state,
                               io_build_state.m_mix_frames[i_slot_index]);
    HJFLogw("{}, audiomixer.mix input_missing {}", getName(),
            slotStateToString(i_slot_index, slot_state));
    return HJ_OK;
  }

  HJMediaFrame::Ptr preview_frame =
      io_build_state.m_in_startup
          ? preview(slot_state.m_input_key_hash)
          : dropLatePreviewFrames(i_slot_index, slot_state,
                                  io_build_state.m_mix_pts);
  if (preview_frame != nullptr && preview_frame->isClearFrame()) {
    io_build_state.m_handled_clear = true;
    return handleClearPreviewFrame(i_slot_index, slot_state,
                                   io_build_state.m_mix_pts);
  }

  HJMediaFrame::Ptr current_frame{};
  const bool slot_silenced = isSlotSilenced(slot_state);
  bool slot_starved = !slot_silenced;
  bool publish_state = false;
  if (preview_frame != nullptr && preview_frame->isEofFrame()) {
    receive(slot_state.m_input_key_hash);
    slot_state.m_eof = true;
    slot_starved = false;
    publish_state = true;
    HJFLogi("{}, audiomixer.state event:eof {}, mix_pts:{}, preview_pts:{}",
            getName(), slotStateToString(i_slot_index, slot_state),
            io_build_state.m_mix_pts, preview_frame->getPTS());
    report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_AUDIOMIXER_INPUT_EOF,
           getID());
  } else if (canFrameParticipateInMix(preview_frame,
                                      io_build_state.m_mix_pts)) {
    current_frame = receive(slot_state.m_input_key_hash);
    slot_starved = false;
    slot_state.m_seen_first_frame = true;
    slot_state.m_eof = false;
  } else if (!slot_state.m_eof && !slot_silenced) {
    ++slot_state.m_silence_fill_frames;
  }

  if (slot_state.m_starved != slot_starved) {
    slot_state.m_starved = slot_starved;
    publish_state = true;
    HJFLogi("{}, audiomixer.state event:{} {}, mix_pts:{}, preview_pts:{}",
            getName(), slot_starved ? "starved" : "recovered",
            slotStateToString(i_slot_index, slot_state),
            io_build_state.m_mix_pts,
            preview_frame ? preview_frame->getPTS() : kInvalidPreviewPts);
    report(EVENT_PLUGIN_NOTIFY_ID,
           slot_starved ? HJ_PLUGIN_NOTIFY_AUDIOMIXER_INPUT_STARVED
                        : HJ_PLUGIN_NOTIFY_AUDIOMIXER_INPUT_RECOVERED,
           getID());
  }
  if (publish_state) {
    publishInputState(i_slot_index);
  }

  if (current_frame == nullptr || slot_silenced) {
    const bool use_gap_concealment =
        (current_frame == nullptr) && !slot_silenced && !slot_state.m_eof &&
        slot_state.m_declicker.hasReferenceFrame() &&
        slot_state.m_gap_concealment_frames < kMixerGapConcealmentFrames;
    HJFLogw(
        "{}, audiomixer.mix use_{} slot:{}, input_id:{}, reason:{}, "
        "mix_pts:{}, preview_pts:{}, concealment_index:{}",
        getName(), use_gap_concealment ? "concealment" : "silence",
        i_slot_index, slot_state.m_input_id,
        slot_silenced ? "zero_volume" : "no_eligible_frame",
        io_build_state.m_mix_pts,
        preview_frame ? preview_frame->getPTS() : kInvalidPreviewPts,
        slot_state.m_gap_concealment_frames);
    if (use_gap_concealment) {
      updateSlotWithConcealmentFrame(m_config, io_build_state.m_mix_samples,
                                     slot_state,
                                     io_build_state.m_mix_frames[i_slot_index]);
    } else {
      const bool reset_gap_concealment =
          slot_silenced || slot_state.m_eof ||
          !slot_state.m_declicker.hasReferenceFrame();
      updateSlotWithSilenceFrame(m_config, io_build_state.m_mix_samples,
                                 slot_state,
                                 io_build_state.m_mix_frames[i_slot_index],
                                 reset_gap_concealment);
    }
    if (!slot_state.m_eof) {
      io_build_state.m_all_eof = false;
    }
    return HJ_OK;
  }

  io_build_state.m_all_eof = false;
  updateSlotWithRealFrame(m_config, slot_state, current_frame);
  io_build_state.m_mix_frames[i_slot_index] = current_frame;
  return HJ_OK;
}

int HJPluginAudioMixer::outputMixedFrame(
    const HJAudioMixerBuildState &i_build_state, int64_t i_frame_ms) {
  if (i_build_state.m_all_eof) {
    logWaitReason(kAudioMixerWaitAllEof, i_build_state.m_mix_pts,
                  static_cast<int64_t>(i_build_state.m_active_inputs),
                  HJFMT("active_inputs:{}, slot_states:{}",
                        i_build_state.m_active_inputs,
                        slotStatesToString(m_slot_states)));
    return HJ_WOULD_BLOCK;
  }

  auto mixed_frame = m_audio_mixer->mixFrames(i_build_state.m_mix_frames);
  if (mixed_frame == nullptr) {
    logWaitReason(kAudioMixerWaitMixNull, i_build_state.m_mix_pts,
                  static_cast<int64_t>(i_build_state.m_active_inputs),
                  HJFMT("active_inputs:{}, frame_count:{}, slot_states:{}",
                        i_build_state.m_active_inputs,
                        i_build_state.m_mix_frames.size(),
                        slotStatesToString(m_slot_states)));
    return HJ_WOULD_BLOCK;
  }

  resetWaitLogState();
  HJFLogd("{}, audiomixer.mix output mix_samples:{}, mix_pts:{}, frame_ms:{}, "
          "delta:{}, info:{}",
          getName(), i_build_state.m_mix_samples, i_build_state.m_mix_pts,
          i_frame_ms, i_build_state.m_mix_pts - mixed_frame->getPTS(),
          mixed_frame->formatInfo());

  if (m_config.m_out_type == HJ_AUDIO_MIXER_OUT_TYPE_PCM) {
    publish(EVENT_GRAPH_AUDIO_FRAME_ID, mixed_frame);
  }
  deliverToOutputs(mixed_frame);
  m_output_frames.fetch_add(1);
  m_mix_samples.store(i_build_state.m_mix_samples + m_config.m_frame_samples);
  publish(EVENT_GRAPH_AUDIO_MIXER_STATS_ID, buildStats());
  return HJ_OK;
}

void HJPluginAudioMixer::resetWaitLogState() {
  m_last_wait_reason.store(kAudioMixerWaitNone, std::memory_order_relaxed);
  m_last_wait_mix_pts.store(0, std::memory_order_relaxed);
  m_last_wait_metric.store(0, std::memory_order_relaxed);
}

void HJPluginAudioMixer::logWaitReason(int i_reason, int64_t i_mix_pts,
                                       int64_t i_metric,
                                       const std::string &i_detail) {
  if (i_reason == kAudioMixerWaitNone) {
    return;
  }

  if (m_last_wait_reason.load(std::memory_order_relaxed) == i_reason &&
      m_last_wait_mix_pts.load(std::memory_order_relaxed) == i_mix_pts &&
      m_last_wait_metric.load(std::memory_order_relaxed) == i_metric) {
    return;
  }

  m_last_wait_reason.store(i_reason, std::memory_order_relaxed);
  m_last_wait_mix_pts.store(i_mix_pts, std::memory_order_relaxed);
  m_last_wait_metric.store(i_metric, std::memory_order_relaxed);

  HJFPERLogi("{}, audiomixer.wait reason:{}, mix_pts:{}, metric:{}, {}", getName(),
          waitReasonToString(i_reason), i_mix_pts, i_metric, i_detail);
}

int HJPluginAudioMixer::runTask(int64_t *o_delay) {
  const int64_t current_mix_samples = m_mix_samples.load();
  const int64_t frame_ms = frameStepMs(m_config, current_mix_samples);
  const int64_t current_mix_pts = samplesToPtsMs(m_config, current_mix_samples);
  if (o_delay != nullptr) {
    *o_delay = frame_ms;
  }

  const auto status = SYNC_CONS_LOCK([this] { return m_status; });
  if (status == HJSTATUS_Done) {
    logWaitReason(kAudioMixerWaitStatusDone, current_mix_pts, status,
                  HJFMT("status:{}, frame_ms:{}", static_cast<int>(status), frame_ms));
    return HJErrAlreadyDone;
  }
  if (status < HJSTATUS_Inited || m_audio_mixer == nullptr ||
      m_config.m_output_info == nullptr) {
    logWaitReason(
        kAudioMixerWaitNotReady, current_mix_pts, status,
        HJFMT("status:{}, mixer_ready:{}, output_ready:{}, frame_ms:{}", static_cast<int>(status),
              HJB2String(m_audio_mixer != nullptr),
              HJB2String(m_config.m_output_info != nullptr), frame_ms));
    return HJ_WOULD_BLOCK;
  }

  const bool in_startup = (m_output_frames.load() == 0);
  const int64_t gate_mix_pts = samplesToPtsMs(m_config, m_mix_samples.load());
  HJAudioMixerScanState scan_state{};
  int ret = scanInputsForMix(in_startup, gate_mix_pts, scan_state);
  if (scan_state.m_handled_clear || ret != HJ_OK) {
    return ret;
  }

  ret = waitForMixReadiness(in_startup, gate_mix_pts, scan_state);
  if (ret != HJ_OK) {
    return ret;
  }

  HJAudioMixerBuildState build_state =
      prepareMixBuildState(in_startup, scan_state);
  HJFLogd("{}, audiomixer.mix entry mix_samples:{}, mix_pts:{}, first_pts:{}, "
          "active_inputs:{}, startup_wait:{}/{}, steady_wait:{}/{}, "
          "queued_duration_max:{} - {}",
          getName(), build_state.m_mix_samples, build_state.m_mix_pts,
          scan_state.m_first_pts, scan_state.m_active_inputs,
          scan_state.m_startup_ready_inputs, scan_state.m_startup_wait_inputs,
          scan_state.m_steady_ready_inputs, scan_state.m_steady_wait_inputs,
          scan_state.m_queued_duration_max,
          HJUtilitys::vectorToString(scan_state.m_queued_duration));

  ret = fillMixFrames(build_state);
  if (build_state.m_handled_clear || ret != HJ_OK) {
    return ret;
  }

  return outputMixedFrame(build_state, frame_ms);
}

NS_HJ_END
