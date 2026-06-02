#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "HJGraph.h"
#include "HJPluginAudioInput.h"
#include "HJPluginAudioResampler.h"
#include "HJPluginAudioMixer.h"
#include "HJPluginFDKAACEncoder.h"

NS_HJ_BEGIN

struct HJGraphAudioMixerSlot {
    std::string m_input_id;
    HJAudioMixerInputDesc m_input_desc{};
    HJPluginAudioInput::Ptr m_input_plugin{};
    HJPluginAudioResampler::Ptr m_resampler_plugin{};
    bool m_attached{ false };
};

class HJGraphAudioMixer : public HJGraph
{
public:
    HJ_DEFINE_CREATE(HJGraphAudioMixer);

    HJGraphAudioMixer(const std::string& i_name = "HJGraphAudioMixer", size_t i_identify = 0)
        : HJGraph(i_name, i_identify) {}
    virtual ~HJGraphAudioMixer() { done(); }

    int initWithConfig(const HJAudioMixerConfig& i_config);

    int addInput(const HJAudioMixerInputDesc& i_input_desc);
    int removeInput(const std::string& i_input_id, bool i_drain = false);
    int flushInput(const std::string& i_input_id);

    int pushFrame(const std::string& i_input_id, HJMediaFrame::Ptr i_frame);
    int pushFrame(const std::string& i_input_id, uint8_t* pcm, int samples, int channel, int samplerate, int fmt, int64_t pts);
   
    int setInputVolume(const std::string& i_input_id, float i_volume);
    int setMasterMute(bool i_muted);
    bool isMasterMuted() const;

    HJAudioInfo::Ptr getOutputAudioInfo() const;

protected:
    int internalInit(HJKeyStorage::Ptr i_param) override;
    void internalRelease() override;

private:
    int registerHandlers();
    int registerQueryHandler_outputInfo();
    int validateConfiguredInputs() const;
    int attachConfiguredInputs();
    int attachSlot(size_t i_slot_index, const HJAudioMixerInputDesc& i_input_desc);
    int detachSlot(size_t i_slot_index, bool i_drain);
    int findSlotIndexByInputId(const std::string& i_input_id, size_t& o_slot_index) const;
    int findIdleSlotIndex(size_t& o_slot_index) const;
    HJGraphAudioMixerInputState buildInputState(size_t i_slot_index, bool i_starved = false, bool i_eof = false) const;

    HJAudioMixerConfig m_config{};
    std::vector<HJGraphAudioMixerSlot> m_slots;
    std::unordered_map<std::string, size_t> m_input_slots;
    std::unordered_set<std::string> m_draining_inputs;
    HJPluginAudioMixer::Ptr m_mixer_plugin{};
    HJPluginFDKAACEncoder::Ptr m_aac_encoder_plugin{};
    HJLooperThread::Ptr m_audio_thread{};
};

NS_HJ_END
