#include "HJPluginAudioOHRender.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

#define FRAME_SIZE      1024

OH_AudioData_Callback_Result onWriteDataCallback(OH_AudioRenderer* renderer, void* userData, void* audioData, int32_t audioDataSize)
{
    return ((HJPluginAudioOHRender*)userData)->onDataCallback(audioData, audioDataSize);
}

OH_AudioData_Callback_Result HJPluginAudioOHRender::onDataCallback(void* o_audioData, int32_t i_audioDataSize)
{
    addInIdx();
    int64_t enter = HJCurrentSteadyMS();
    bool log = false;
    if (m_lastEnterTimestamp < 0 || enter >= m_lastEnterTimestamp + LOG_INTERNAL) {
        m_lastEnterTimestamp = enter;
        log = true;
    }
    if (log) {
        RUNTASKLog("{}, enter", getName());
    }
    
    std::string route{};
    size_t size = -1;
    int64_t audioDuration = 0;
    int64_t audioSamples = 0;
    auto ret = AUDIO_DATA_CALLBACK_RESULT_VALID;
    do {
        HJMediaFrame::Ptr kernalBuffer{};
        HJAudioInfo::Ptr audioInfo{};
        auto err = SYNC_CONS_LOCK([&route, &kernalBuffer, &audioInfo, this] {
            if (m_status == HJSTATUS_Done) {
                route += "_0";
                return HJErrAlreadyDone;
            }
            if (m_status >= HJSTATUS_EOF) {
                route += "_1";
                return HJ_WOULD_BLOCK;
            }
    
            kernalBuffer = m_kernalFrame;
            audioInfo = m_audioInfo;
            return HJ_OK;
        });
        if (err != HJ_OK) {
            ret = AUDIO_DATA_CALLBACK_RESULT_INVALID;
            break;
        }

        route += "_2";
        size_t inputKeyHash = m_inputKeyHash.load();
        int64_t audioSize = audioSamplesOfInput(inputKeyHash) * audioInfo->m_channels * audioInfo->m_bytesPerSample;
        if (kernalBuffer) {
            route += "_3";
            HJAVFrame::Ptr avFrame = kernalBuffer->getMFrame();
            AVFrame* frame = avFrame->getAVFrame();
            int64_t bufferSize = frame->nb_samples * m_audioInfo->m_channels * m_audioInfo->m_bytesPerSample;
            audioSize += (bufferSize - kernalBuffer->m_bufferPos);
        }
        if (audioSize < i_audioDataSize) {
            route += "_4";
            if (m_eof.load()) {
                route += "_5";
                if (m_pluginListener) {
                    route += "_6";
                    m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_EOF)));
                }
                setStatus(HJSTATUS_EOF);
            }
            else if (!m_buffering) {
                route += "_10";
                m_buffering = true;
                if (m_pluginListener) {
                    route += "_11";
                    m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING)));
                }
            }

            ret = AUDIO_DATA_CALLBACK_RESULT_INVALID;
            break;
        }
 
        void* audioData = o_audioData;
        int32_t audioDataSize = i_audioDataSize;
        while (audioDataSize > 0) {
            route += "_7";
            if (!kernalBuffer) {
                route += "_8";
                kernalBuffer = receive(inputKeyHash, &size, &audioDuration, nullptr, &audioSamples);
                if (!kernalBuffer) {
                    route += "_9";
                    // why?

                    ret = AUDIO_DATA_CALLBACK_RESULT_INVALID;
                    break;
                }

                setInfoAudioDuration(audioSamples);

                if (m_pluginListener) {
                    route += "_27";
                    auto notify = HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_FRAME);
                    (*notify)["frame"] = kernalBuffer;
                    m_pluginListener(std::move(notify));
                }
                
                if (m_buffering) {
                    route += "_12";
                    m_buffering = false;
                    if (m_pluginListener) {
                        route += "_13";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING)));
                    }
                }
            }

            err = SYNC_CONS_LOCK([&route, &kernalBuffer, &audioData, &audioDataSize, this] {
                if (m_status == HJSTATUS_Done) {
                    route += "_14";
                    return HJErrAlreadyDone;
                }
    
                HJAVFrame::Ptr avFrame = kernalBuffer->getMFrame();
                AVFrame* frame = avFrame->getAVFrame();
                int32_t bufferSize = frame->nb_samples * m_audioInfo->m_channels * m_audioInfo->m_bytesPerSample;
                int32_t copySize = std::min<int32_t>(audioDataSize, bufferSize - kernalBuffer->m_bufferPos);
                memcpy(audioData, frame->data[0] + kernalBuffer->m_bufferPos, copySize);
                audioData = (int8_t*)audioData + copySize;
                audioDataSize -= copySize;
                kernalBuffer->m_bufferPos += copySize;
    
                if (kernalBuffer->m_bufferPos >= bufferSize) {
                    route += "_15";
                    m_timeline->setTimestamp(kernalBuffer->m_streamIndex, kernalBuffer->getPTS(), kernalBuffer->getSpeed());
                    kernalBuffer = nullptr;
                }
    
                m_kernalFrame = kernalBuffer;
                return HJ_OK;
            });
            if (err != HJ_OK) {
                ret = AUDIO_DATA_CALLBACK_RESULT_INVALID;
                break;
            }
        }
    } while (false);
    addOutIdx();
    if (log) {
        RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
    }
    return ret;
}

void HJPluginAudioOHRender::internalSetMute()
{
    if (m_renderer) {
        float volume = m_muted ? 0.0f : 1.0f;
        OH_AudioStream_Result result = OH_AudioRenderer_SetVolume(m_renderer, volume);
        if (result != AUDIOSTREAM_SUCCESS) {
            HJFLoge("{}, OH_AudioRenderer_SetVolume() error({})", getName(), result);
        }
    }
}

int HJPluginAudioOHRender::initRender(const HJAudioInfo::Ptr& i_audioInfo)
{
    HJFLogi("{} initRender enter this:{}", getName(), size_t(this));
    // configure audio source
    // create builder
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStreamBuilder_Create(&m_streamBuilder, type);

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
    
//    OH_AudioRenderer_Pause(m_renderer);
    OH_AudioRenderer_Start(m_renderer);
    internalSetMute();
/*
    if (m_pluginListener) {
        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING)));
    }
*/    
    HJFLogi("{} initRender end this:{}", getName(), size_t(this));
	return HJ_OK;
}

bool HJPluginAudioOHRender::releaseRender()
{
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("{} releaseRender enter this:{}", getName(), size_t(this));
    
    bool ret = false;
	if (m_renderer) {
//        if (m_pluginListener) {
//            m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING)));
//        }

        HJFLogi("{} releaseRender before OH_AudioRenderer_Stop", getName());
        OH_AudioRenderer_Stop(m_renderer);
        HJFLogi("{} releaseRender before OH_AudioRenderer_Flush", getName());
        OH_AudioRenderer_Flush(m_renderer);
        HJFLogi("{} release Render before OH_AudioRenderer_Release", getName());
        OH_AudioRenderer_Release(m_renderer);
		m_renderer = nullptr;
        
        ret = true;
	}
    HJFLogi("{} releaseRender before OH_AudioStreamBuilder_Destroy", getName());
    if (m_streamBuilder) {
        OH_AudioStreamBuilder_Destroy(m_streamBuilder);
        m_streamBuilder = nullptr;
        
        ret = true;
    }

    int64_t t1 = HJCurrentSteadyMS();
    m_kernalFrame = nullptr;
    HJFLogi("{} releaseRender end this:{} diff:{}", getName(), size_t(this), (t1 - t0));
    return ret;
}

void HJPluginAudioOHRender::setInfoAudioDuration(int64_t i_audioSamples)
{
    i_audioSamples = i_audioSamples > FRAME_SIZE ? i_audioSamples - FRAME_SIZE : 0;
    HJPluginAudioRender::setInfoAudioDuration(i_audioSamples);
}

NS_HJ_END
