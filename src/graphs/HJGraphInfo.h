#pragma once

#include "HJMacros.h"
#include "HJMessageBus.h"
#include "HJPluginNotify.h"
#include "HJMediaInfo.h"
#include "HJMediaFrame.h"
#include "HJSEIWrapper.h"
#include "HJAny.h"

NS_HJ_BEGIN

struct HJGraphAudioMixerInputState {
    std::string m_input_id;
    size_t m_slot_index{ 0 };
    bool m_attached{ false };
    bool m_starved{ false };
    bool m_eof{ false };
};

struct HJGraphAudioMixerStats {
    int64_t m_mix_pts{ 0 };
    size_t m_output_frames{ 0 };
    size_t m_silence_fill_frames{ 0 };
    size_t m_late_drop_frames{ 0 };
    size_t m_active_inputs{ 0 };
    size_t m_starved_inputs{ 0 };
};

struct HJGraphRenderedPCM {
    HJAudioInfo::Ptr m_audioInfo{};
    std::shared_ptr<std::vector<uint8_t>> m_pcmData{};
};

// query
HJ_QUERY_DECLARE(bool, QUERY_HAS_AUDIO);
HJ_QUERY_DECLARE(bool, QUERY_CAN_DELIVER_TO_OUTPUTS, size_t, const HJMediaFrame::Ptr&);
HJ_QUERY_DECLARE(bool, QUERY_CAN_PLUGIN_EOF, size_t, int);
HJ_QUERY_DECLARE(std::vector<HJSEIData>, QUERY_SEI_REQUIRE, int64_t, bool);
HJ_QUERY_DECLARE(HJAudioInfo::Ptr, QUERY_GRAPH_AUDIO_MIXER_OUTPUT_INFO);

// report (plugin to graph or user)
	// plugin events
HJ_EVENT_DECLARE(EVENT_PLUGIN_NOTIFY, HJPluginNotifyType, size_t);
HJ_EVENT_DECLARE(EVENT_STATUS_UPDATED, size_t);
HJ_EVENT_DECLARE(EVENT_MEDIA_TYPE, uint32_t);
HJ_EVENT_DECLARE(EVENT_AUDIO_DURATION, size_t, int64_t, bool);
HJ_EVENT_DECLARE(EVENT_VIDEO_FRAMES, size_t, int64_t, bool);
HJ_EVENT_DECLARE(EVENT_VIDEO_KEY_FRAMES, size_t, int64_t, bool);
HJ_EVENT_DECLARE(EVENT_SEEK_SUCCEEDED, size_t);
HJ_EVENT_DECLARE(EVENT_STREAM_OPENED, size_t);

	// graph events
HJ_EVENT_DECLARE(EVENT_GRAPH_STREAM_OPENED);
HJ_EVENT_DECLARE(EVENT_GRAPH_EOF);
HJ_EVENT_DECLARE(EVENT_GRAPH_VIDEO_FRAME, const HJMediaFrame::Ptr&);
HJ_EVENT_DECLARE(EVENT_GRAPH_AUDIO_FRAME, const HJMediaFrame::Ptr&);
HJ_EVENT_DECLARE(EVENT_GRAPH_RENDERED_PCM, const HJGraphRenderedPCM&);
//HJ_EVENT_DECLARE(EVENT_GRAPH_DURATION, int64_t);
HJ_EVENT_DECLARE(EVENT_GRAPH_SEI_INFOS, const std::vector<HJSEIData>&, bool);
HJ_EVENT_DECLARE(EVENT_GRAPH_FIRST_FRAME_RENDERED, const std::string&);
HJ_EVENT_DECLARE(EVENT_GRAPH_AUTODELAY_PARAMS, const std::shared_ptr<HJParams>&);
HJ_EVENT_DECLARE(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE, const HJGraphAudioMixerInputState&);
HJ_EVENT_DECLARE(EVENT_GRAPH_AUDIO_MIXER_STATS, const HJGraphAudioMixerStats&);

// inform (graph to plugin)
HJ_EVENT_DECLARE(EVENT_VIDEO_DECODER_ON_OUTPUT_UPDATED);
HJ_EVENT_DECLARE(EVENT_DEMUXER_ON_OUTPUT_UPDATED);
HJ_EVENT_DECLARE(EVENT_DROPPING_ON_OUTPUT_UPDATED);

NS_HJ_END
