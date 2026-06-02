#include "HJPluginAudioOHRender.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

#define FRAME_SIZE      1024
/*
int HJPluginAudioOHRender::flush(size_t i_srcKeyHash)
{
    auto input = getInput(i_srcKeyHash);
    if (input == nullptr) {
        return HJErrNotFind;
    }

    input->mediaFrames.flush(true);
    HJMediaFrameDeque::FrameDequeInfo info;
    setInfos(input->eventFlags, info);

    return HJ_OK;
}
*/
OH_AudioData_Callback_Result onWriteDataCallback(OH_AudioRenderer* renderer, void* userData, void* audioData, int32_t audioDataSize)
{
    return ((HJPluginAudioOHRender*)userData)->onDataCallback(audioData, audioDataSize);
}

OH_AudioData_Callback_Result HJPluginAudioOHRender::onDataCallback(void* o_audioData, int32_t i_audioDataSize)
{
//    addInIdx();
    auto log = logRunTask();
    if (log) {
        RUNTASKLog("{}, enter", getName());
    }
    
    std::string route{};
    int64_t size{ -1 };
    OH_AudioData_Callback_Result ret{ AUDIO_DATA_CALLBACK_RESULT_VALID };
    do {
        auto[err, validSize, kernalFrame] = fillAudioBuffer(route, o_audioData, i_audioDataSize, size);
        if (err != HJ_OK) {
            route += "_0";
            ret = AUDIO_DATA_CALLBACK_RESULT_INVALID;
            break;
        }

        if (validSize < i_audioDataSize) {
            route += "_1";
            memset(static_cast<int8_t*>(o_audioData) + validSize, 0, i_audioDataSize - validSize);
        }
        else if (kernalFrame != nullptr) {
            route += "_2";
            store(m_inputKeyHash.load(), kernalFrame);
        }
        applyOutputVolume(o_audioData, i_audioDataSize);
        if (m_pcmCallback) {
            route += "_3";
            appendPCMCallbackData(static_cast<const uint8_t*>(o_audioData), i_audioDataSize);
        }

        route += "_4";
    } while (false);
//    addOutIdx();
    if (log) {
        RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
    }
    return ret;
}

int64_t HJPluginAudioOHRender::getRenderTimestamp()
{
    if (!m_renderer || !m_audioInfo || m_audioInfo->m_samplesRate <= 0) {
        return 0;
    }

    int64_t framePosition = 0;
    int64_t timestamp = 0;
    OH_AudioStream_Result result = OH_AudioRenderer_GetTimestamp(
        m_renderer,
        CLOCK_MONOTONIC,
        &framePosition,
        &timestamp);
    if (result != AUDIOSTREAM_SUCCESS || framePosition <= 0) {
        return 0;
    }

    return (framePosition * 1000LL) / m_audioInfo->m_samplesRate;
}

int HJPluginAudioOHRender::initRender(const HJAudioInfo::Ptr& i_audioInfo)
{
    HJFLogi("{} initRender enter this:{}", getName(), size_t(this));
    // configure audio source
    // create builder
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStreamBuilder_Create(&m_streamBuilder, type);
    if (!m_streamBuilder) {
        releaseRender();
        return HJErrFatal;
    }

    // set params and callbacks
    OH_AudioStreamBuilder_SetSamplingRate(m_streamBuilder, i_audioInfo->m_samplesRate);
    OH_AudioStreamBuilder_SetChannelCount(m_streamBuilder, i_audioInfo->m_channels);
//    OH_AudioStreamBuilder_SetSampleFormat(m_streamBuilder, AUDIOSTREAM_SAMPLE_S16LE);
//    OH_AudioStreamBuilder_SetEncodingType(m_streamBuilder, AUDIOSTREAM_ENCODING_TYPE_RAW);
//    OH_AudioStreamBuilder_SetRendererInfo(m_streamBuilder, AUDIOSTREAM_USAGE_MUSIC);
    OH_AudioStreamBuilder_SetLatencyMode(m_streamBuilder, AUDIOSTREAM_LATENCY_MODE_NORMAL);
    OH_AudioStreamBuilder_SetFrameSizeInCallback(m_streamBuilder, FRAME_SIZE);

    OH_AudioRenderer_Callbacks rendererCallbacks;
    rendererCallbacks.OH_AudioRenderer_OnWriteData = nullptr;
    rendererCallbacks.OH_AudioRenderer_OnStreamEvent = nullptr;
    rendererCallbacks.OH_AudioRenderer_OnInterruptEvent = nullptr;
    rendererCallbacks.OH_AudioRenderer_OnError = nullptr;
    OH_AudioStreamBuilder_SetRendererCallback(m_streamBuilder, rendererCallbacks, this);

    OH_AudioStreamBuilder_SetRendererWriteDataCallback(m_streamBuilder, onWriteDataCallback, this);

    // create OH_AudioRenderer
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(m_streamBuilder, &m_renderer);
    if (result != AUDIOSTREAM_SUCCESS) {
        releaseRender();
        return HJErrFatal;
    }
 
    m_blockAlign = i_audioInfo->m_channels * i_audioInfo->m_bytesPerSample;
    m_running = false;
    
    HJFLogi("{} initRender end this:{}", getName(), size_t(this));
	return HJ_OK;
}

void HJPluginAudioOHRender::releaseRender()
{
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("{} releaseRender enter this:{}", getName(), size_t(this));
    
	if (m_renderer) {
        HJFLogi("{} releaseRender before OH_AudioRenderer_Stop", getName());
        OH_AudioRenderer_Stop(m_renderer);
        HJFLogi("{} releaseRender before OH_AudioRenderer_Flush", getName());
        OH_AudioRenderer_Flush(m_renderer);
        HJFLogi("{} release Render before OH_AudioRenderer_Release", getName());
        OH_AudioRenderer_Release(m_renderer);
		m_renderer = nullptr;
	}
    HJFLogi("{} releaseRender before OH_AudioStreamBuilder_Destroy", getName());
    if (m_streamBuilder) {
        OH_AudioStreamBuilder_Destroy(m_streamBuilder);
        m_streamBuilder = nullptr;
    }

    int64_t t1 = HJCurrentSteadyMS();
    m_running = false;

    HJFLogi("{} releaseRender end this:{} diff:{}", getName(), size_t(this), (t1 - t0));
}

int HJPluginAudioOHRender::setStreamRunning(bool i_running, bool i_eofStop)
{
    if (!m_renderer) {
        return HJ_OK;
    }

    if (i_running) {
        const auto result = OH_AudioRenderer_Start(m_renderer);
        if (result != AUDIOSTREAM_SUCCESS) {
            return HJErrFatal;
        }
        return HJ_OK;
    }

    const auto result = i_eofStop ? OH_AudioRenderer_Stop(m_renderer) : OH_AudioRenderer_Pause(m_renderer);
    if (result != AUDIOSTREAM_SUCCESS) {
        return HJErrFatal;
    }
    if (i_eofStop) {
        OH_AudioRenderer_Flush(m_renderer);
    }
    return HJ_OK;
}

NS_HJ_END
