#pragma once

#include <atomic>
#include <unordered_map>
#include <vector>

#include "HJAudioDeclicker.h"
#include "HJAudioMixer.h"
#include "HJPlugin.h"

NS_HJ_BEGIN

struct HJAudioMixerSlotState {
  std::string m_input_id;
  size_t m_input_key_hash{0};
  bool m_attached{false};
  float m_volume{1.0f};
  bool m_seen_first_frame{false};
  bool m_starved{false};
  bool m_eof{false};
  bool m_outputting_silence{true};
  bool m_pending_fade_in{false};
  size_t m_gap_concealment_frames{0};
  size_t m_silence_fill_frames{0};
  size_t m_late_drop_frames{0};
  HJAudioDeclicker m_declicker{};
};

class HJPluginAudioMixer : public HJPlugin {
public:
  HJ_DEFINE_CREATE(HJPluginAudioMixer);

  HJPluginAudioMixer(const std::string &i_name = "HJPluginAudioMixer",
                     size_t i_identify = 0,
                     HJKeyStorage::Ptr i_graphInfo = nullptr)
      : HJPlugin(i_name, i_identify, i_graphInfo) {}
  virtual ~HJPluginAudioMixer() { done(); }

  int configure(const HJAudioMixerConfig &i_config);
  int bindInput(size_t i_slot_index, size_t i_input_key_hash,
                const HJAudioMixerInputDesc &i_input_desc);
  int unbindInput(size_t i_slot_index);
  int flushInput(size_t i_slot_index);
  int setInputVolume(size_t i_slot_index, float i_volume);
  int setMasterMute(bool i_muted);
  bool isMasterMuted() const;
  const HJAudioInfo::Ptr &getOutputInfo() const;

protected:
  int internalInit(HJKeyStorage::Ptr i_param) override;
  void internalRelease() override;
  void onInputAdded(size_t i_src_key_hash, HJMediaType i_type) override;
  int runTask(int64_t *o_delay = nullptr) override;

private:
  struct HJAudioMixerScanState {
    bool m_handled_clear{false};
    bool m_has_attached{false};
    bool m_has_pending{false};
    bool m_has_unseen_inputs{false};
    size_t m_active_inputs{0};
    size_t m_startup_wait_inputs{0};
    size_t m_startup_ready_inputs{0};
    size_t m_join_ready_inputs{0};
    size_t m_steady_wait_inputs{0};
    size_t m_steady_ready_inputs{0};
    int64_t m_queued_duration_max{0};
    int64_t m_first_pts{0};
    std::vector<size_t> m_queued_duration;
  };

  struct HJAudioMixerBuildState {
    bool m_in_startup{false};
    bool m_handled_clear{false};
    bool m_all_eof{true};
    size_t m_active_inputs{0};
    int64_t m_mix_samples{0};
    int64_t m_mix_pts{0};
    std::vector<HJMediaFrame::Ptr> m_mix_frames;
  };

  int configureLocked(const HJAudioMixerConfig &i_config);
  HJGraphAudioMixerInputState buildInputState(size_t i_slot_index) const;
  HJGraphAudioMixerStats buildStats() const;
  int findSlotIndexByInputKeyHash(size_t i_input_key_hash,
                                  size_t &o_slot_index) const;
  int validateSlotIndex(size_t i_slot_index, const char *i_action) const;
  int ensureAttachedSlot(size_t i_slot_index, const char *i_action) const;
  void publishInputState(size_t i_slot_index);
  bool shouldPublishInputState(const HJAudioMixerSlotState &i_slot_state) const;
  void resetAllSlotRuntimeStates();
  int handleClearPreviewFrame(size_t i_slot_index,
                              const HJAudioMixerSlotState &i_slot_state,
                              int64_t i_mix_pts);
  bool isFrameLateForMix(const HJMediaFrame::Ptr &i_frame,
                         int64_t i_mix_pts) const;
  bool canFrameParticipateInMix(const HJMediaFrame::Ptr &i_frame,
                                int64_t i_mix_pts) const;
  HJMediaFrame::Ptr dropLatePreviewFrames(size_t i_slot_index,
                                          HJAudioMixerSlotState &io_slot_state,
                                          int64_t i_mix_pts);
  int scanInputsForMix(bool i_in_startup, int64_t i_gate_mix_pts,
                       HJAudioMixerScanState &o_scan_state);
  int waitForMixReadiness(bool i_in_startup, int64_t i_gate_mix_pts,
                          const HJAudioMixerScanState &i_scan_state);
  HJAudioMixerBuildState
  prepareMixBuildState(bool i_in_startup,
                       const HJAudioMixerScanState &i_scan_state);
  int fillMixFrames(HJAudioMixerBuildState &io_build_state);
  int fillMixFrameForSlot(size_t i_slot_index,
                          HJAudioMixerBuildState &io_build_state);
  int outputMixedFrame(const HJAudioMixerBuildState &i_build_state,
                       int64_t i_frame_ms);
  void resetWaitLogState();
  void logWaitReason(int i_reason, int64_t i_mix_pts, int64_t i_metric,
                     const std::string &i_detail);

  HJAudioMixerConfig m_config{};
  HJAudioMixer::Ptr m_audio_mixer{};
  std::vector<HJAudioMixerSlotState> m_slot_states;
  std::unordered_map<size_t, size_t> m_input_slot_map;
  std::atomic<int64_t> m_mix_samples{0};
  std::atomic<size_t> m_output_frames{0};
  std::atomic<int> m_last_wait_reason{0};
  std::atomic<int64_t> m_last_wait_mix_pts{0};
  std::atomic<int64_t> m_last_wait_metric{0};
};

NS_HJ_END
