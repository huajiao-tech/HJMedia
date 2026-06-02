#include "HJPluginAudioAARender.h"
#include "HJGraph.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"
#include "asys/HJAAudioLoader.h"

#if defined(HJ_OS_ANDROID)
#include <android/api-level.h>
#endif

NS_HJ_BEGIN

namespace {

#if defined(HJ_OS_ANDROID)
aaudio_format_t toAAudioFormat(int sampleFmt) {
    switch (sampleFmt) {
#if defined(AV_SAMPLE_FMT_S16)
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        return AAUDIO_FORMAT_PCM_I16;
#endif
#if defined(AV_SAMPLE_FMT_S32)
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        return AAUDIO_FORMAT_PCM_I32;
#endif
#if defined(AV_SAMPLE_FMT_FLT)
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        return AAUDIO_FORMAT_PCM_FLOAT;
#endif
    default:
        return AAUDIO_FORMAT_PCM_I16;
    }
}
#endif

int resolveAndroidApiLevel() {
#if defined(HJ_OS_ANDROID)
//#if defined(__ANDROID_API__)
//    return __ANDROID_API__;
//#else
    return android_get_device_api_level();
//#endif
#else
    return 0;
#endif
}

#if defined(HJ_OS_ANDROID)

SLuint32 toOpenSLSampleRate(int sampleRate) {
    switch (sampleRate) {
    case 8000: return SL_SAMPLINGRATE_8;
    case 11025: return SL_SAMPLINGRATE_11_025;
    case 12000: return SL_SAMPLINGRATE_12;
    case 16000: return SL_SAMPLINGRATE_16;
    case 22050: return SL_SAMPLINGRATE_22_05;
    case 24000: return SL_SAMPLINGRATE_24;
    case 32000: return SL_SAMPLINGRATE_32;
    case 44100: return SL_SAMPLINGRATE_44_1;
    case 48000: return SL_SAMPLINGRATE_48;
    case 64000: return SL_SAMPLINGRATE_64;
    case 88200: return SL_SAMPLINGRATE_88_2;
    case 96000: return SL_SAMPLINGRATE_96;
    case 192000: return SL_SAMPLINGRATE_192;
    default:
        return static_cast<SLuint32>(sampleRate * 1000);
    }
}

SLuint32 toOpenSLChannelMask(int channels) {
    if (channels == 1) {
        return SL_SPEAKER_FRONT_CENTER;
    }
    if (channels == 2) {
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    }
    return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
}

struct OpenSLFormat {
    bool usePcmEx{ false };
    SLDataFormat_PCM pcm{};
    SLAndroidDataFormat_PCM_EX pcmEx{};
};

OpenSLFormat makeOpenSLFormat(const HJAudioInfo::Ptr& info) {
    OpenSLFormat format{};
    const SLuint32 sampleRate = toOpenSLSampleRate(info->m_samplesRate);
    const SLuint32 channelMask = toOpenSLChannelMask(info->m_channels);
    int bytesPerSample = info->m_bytesPerSample;
    if (bytesPerSample <= 0) {
        bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(info->m_sampleFmt));
        if (bytesPerSample <= 0) {
            bytesPerSample = 2;
        }
    }
    const SLuint32 bitsPerSample = static_cast<SLuint32>(bytesPerSample * 8);

    bool wantFloat = false;
    bool wantInt32 = false;
    switch (info->m_sampleFmt) {
#if defined(AV_SAMPLE_FMT_FLT)
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        wantFloat = true;
        break;
#endif
#if defined(AV_SAMPLE_FMT_S32)
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        wantInt32 = true;
        break;
#endif
    default:
        break;
    }

#if defined(SL_ANDROID_DATAFORMAT_PCM_EX)
    if (wantFloat || wantInt32) {
        format.usePcmEx = true;
        format.pcmEx.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
        format.pcmEx.numChannels = static_cast<SLuint32>(info->m_channels);
        format.pcmEx.sampleRate = sampleRate;
        format.pcmEx.bitsPerSample = bitsPerSample;
        format.pcmEx.containerSize = bitsPerSample;
        format.pcmEx.channelMask = channelMask;
        format.pcmEx.endianness = SL_BYTEORDER_LITTLEENDIAN;
#if defined(SL_ANDROID_PCM_REPRESENTATION_FLOAT)
        if (wantFloat) {
            format.pcmEx.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;
        } else {
            format.pcmEx.representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
        }
#else
        format.pcmEx.representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
#endif
        return format;
    }
#endif

    format.usePcmEx = false;
    format.pcm.formatType = SL_DATAFORMAT_PCM;
    format.pcm.numChannels = static_cast<SLuint32>(info->m_channels);
    format.pcm.samplesPerSec = sampleRate;
    format.pcm.bitsPerSample = bitsPerSample;
    format.pcm.containerSize = bitsPerSample;
    format.pcm.channelMask = channelMask;
    format.pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    return format;
}

#endif

} // namespace

void HJPluginAudioAARender::handleData(void* output, int32_t frameCount)
{
    if (!output || frameCount <= 0) {
        return;
    }

    auto log = logRunTask();
    if (log) {
        HJFLogi("{}, enter", getName());
    }

    std::string route{};
    int64_t size{ -1 };
    int32_t dataSize = static_cast<int32_t>(frameCount * m_blockAlign);

    auto [err, validSize, kernalFrame] = fillAudioBuffer(route, output, dataSize, size);
    if (err != HJ_OK || validSize <= 0) {
        memset(output, 0, static_cast<size_t>(dataSize));
        if (m_pcmCallback) {
            appendPCMCallbackData(static_cast<const uint8_t*>(output), dataSize);
        }
        if (log) {
            HJFLogi("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size,
                    (HJCurrentSteadyMS() - m_enterTimestamp), err);
        }
        return;
    }

    if (validSize < dataSize) {
        memset(static_cast<uint8_t*>(output) + validSize, 0, dataSize - validSize);
    } else if (kernalFrame != nullptr) {
        store(m_inputKeyHash.load(), kernalFrame);
    }
    applyOutputVolume(output, dataSize);
    if (m_pcmCallback) {
        appendPCMCallbackData(static_cast<const uint8_t*>(output), dataSize);
    }

    if (log) {
        HJFLogi("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size,
                (HJCurrentSteadyMS() - m_enterTimestamp), err);
    }
}

int64_t HJPluginAudioAARender::getRenderTimestamp()
{
#if !defined(HJ_OS_ANDROID)
    return 0;
#else
    if (!m_audioInfo || m_audioInfo->m_samplesRate <= 0) {
        return 0;
    }

    if (m_backend == Backend::AAudio && m_aaStream) {
        auto& loader = HJAAudioLoader::instance();
        if (loader.isAvailable() && loader.streamGetFramesRead) {
            const int64_t framesRead = loader.streamGetFramesRead(m_aaStream);
            if (framesRead > 0) {
                return (framesRead * 1000LL) / m_audioInfo->m_samplesRate;
            }
        }
    }

    if (m_backend == Backend::OpenSL) {
        const int64_t playedFrames = m_slPlayedFrames.load();
        if (playedFrames > 0) {
            return (playedFrames * 1000LL) / m_audioInfo->m_samplesRate;
        }
    }

    return 0;
#endif
}

int HJPluginAudioAARender::initRender(const HJAudioInfo::Ptr& i_audioInfo)
{
#if !defined(HJ_OS_ANDROID)
    (void)i_audioInfo;
    return HJErrNotSupport;
#else
    if (!i_audioInfo) {
        return HJErrInvalidParams;
    }

    HJFLogi("{} initRender enter this:{}", getName(), size_t(this));

    int bytesPerSample = i_audioInfo->m_bytesPerSample;
    if (bytesPerSample <= 0) {
        bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(i_audioInfo->m_sampleFmt));
        if (bytesPerSample <= 0) {
            bytesPerSample = 2;
        }
    }
    m_blockAlign = i_audioInfo->m_channels * bytesPerSample;

    m_backend = Backend::None;
    m_running = false;
    m_slPlayedFrames.store(0);

#if defined(HJ_OS_ANDROID)
    const int api = resolveAndroidApiLevel();
    auto& loader = HJAAudioLoader::instance();
    if (api >= 26 && loader.isAvailable()) {
        aaudio_result_t result = loader.createStreamBuilder(&m_aaBuilder);
        if (result == AAUDIO_OK && m_aaBuilder) {
            loader.streamBuilderSetSampleRate(m_aaBuilder, i_audioInfo->m_samplesRate);
            loader.streamBuilderSetChannelCount(m_aaBuilder, i_audioInfo->m_channels);
            loader.streamBuilderSetFormat(m_aaBuilder, toAAudioFormat(i_audioInfo->m_sampleFmt));
            loader.streamBuilderSetSharingMode(m_aaBuilder, AAUDIO_SHARING_MODE_SHARED);
            loader.streamBuilderSetPerformanceMode(m_aaBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
            if (i_audioInfo->m_samplesPerFrame > 0) {
                loader.streamBuilderSetFramesPerDataCallback(m_aaBuilder, i_audioInfo->m_samplesPerFrame);
            }
            loader.streamBuilderSetDataCallback(m_aaBuilder, HJPluginAudioAARender::onAAudioDataCallback, this);
            loader.streamBuilderSetErrorCallback(m_aaBuilder, HJPluginAudioAARender::onAAudioErrorCallback, this);

            result = loader.streamBuilderOpenStream(m_aaBuilder, &m_aaStream);
            if (result == AAUDIO_OK && m_aaStream) {
                m_backend = Backend::AAudio;
            } else {
                HJFLoge("{}, AAudioStreamBuilder_openStream error({})", getName(), result);
            }
        } else {
            HJFLoge("{}, AAudio_createStreamBuilder error({})", getName(), result);
        }
    }
#endif

    if (m_backend == Backend::None) {
#if defined(HJ_OS_ANDROID)
        if (m_aaStream) {
            auto& loader = HJAAudioLoader::instance();
            if (loader.isAvailable() && loader.streamClose) {
                loader.streamClose(m_aaStream);
            }
            m_aaStream = nullptr;
        }
        if (m_aaBuilder) {
            auto& loader = HJAAudioLoader::instance();
            if (loader.isAvailable() && loader.streamBuilderDelete) {
                loader.streamBuilderDelete(m_aaBuilder);
            }
            m_aaBuilder = nullptr;
        }
#endif

        // OpenSL ES fallback
        const int32_t framesPerBuffer = (i_audioInfo->m_samplesPerFrame > 0)
            ? i_audioInfo->m_samplesPerFrame
            : 1024;
        m_slFramesPerBuffer = framesPerBuffer;

        SLresult result = slCreateEngine(&m_slEngineObj, 0, nullptr, 0, nullptr, nullptr);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, slCreateEngine error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }
        result = (*m_slEngineObj)->Realize(m_slEngineObj, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, slEngine Realize error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }
        result = (*m_slEngineObj)->GetInterface(m_slEngineObj, SL_IID_ENGINE, &m_slEngine);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, slEngine GetInterface error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        result = (*m_slEngine)->CreateOutputMix(m_slEngine, &m_slOutputMixObj, 0, nullptr, nullptr);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, CreateOutputMix error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }
        result = (*m_slOutputMixObj)->Realize(m_slOutputMixObj, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, OutputMix Realize error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        SLDataLocator_AndroidSimpleBufferQueue locBufQ = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
        OpenSLFormat osFormat = makeOpenSLFormat(i_audioInfo);
        SLDataSource audioSrc{};
        if (osFormat.usePcmEx) {
            audioSrc = { &locBufQ, &osFormat.pcmEx };
        } else {
            audioSrc = { &locBufQ, &osFormat.pcm };
        }

        SLDataLocator_OutputMix locOutMix = { SL_DATALOCATOR_OUTPUTMIX, m_slOutputMixObj };
        SLDataSink audioSnk = { &locOutMix, nullptr };

        const SLInterfaceID ids[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME };
        const SLboolean req[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
        result = (*m_slEngine)->CreateAudioPlayer(m_slEngine, &m_slPlayerObj, &audioSrc, &audioSnk,
                                                  sizeof(ids) / sizeof(ids[0]), ids, req);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, CreateAudioPlayer error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        result = (*m_slPlayerObj)->Realize(m_slPlayerObj, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, AudioPlayer Realize error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        result = (*m_slPlayerObj)->GetInterface(m_slPlayerObj, SL_IID_PLAY, &m_slPlay);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, AudioPlayer GetInterface(PLAY) error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        result = (*m_slPlayerObj)->GetInterface(m_slPlayerObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &m_slBufferQueue);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, AudioPlayer GetInterface(BUFFERQUEUE) error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        (*m_slPlayerObj)->GetInterface(m_slPlayerObj, SL_IID_VOLUME, &m_slVolume);

        result = (*m_slBufferQueue)->RegisterCallback(m_slBufferQueue, HJPluginAudioAARender::onOpenSLBufferQueueCallback, this);
        if (result != SL_RESULT_SUCCESS) {
            HJFLoge("{}, BufferQueue RegisterCallback error({})", getName(), result);
            releaseRender();
            return HJErrFatal;
        }

        const int32_t bufferSize = m_slFramesPerBuffer * m_blockAlign;
        m_slBuffers.clear();
        m_slBuffers.resize(2);
        for (auto& buffer : m_slBuffers) {
            buffer.resize(static_cast<size_t>(bufferSize));
        }
        m_slBufferIndex = 0;

        m_backend = Backend::OpenSL;
    }

    HJFLogi("{} initRender end this:{}", getName(), size_t(this));
    return HJ_OK;
#endif
}

void HJPluginAudioAARender::releaseRender()
{
#if defined(HJ_OS_ANDROID)
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("{} releaseRender enter this:{}", getName(), size_t(this));
    discardPendingPCMCallbackData();

#if defined(HJ_OS_ANDROID)
    if (m_aaStream) {
        auto& loader = HJAAudioLoader::instance();
        if (loader.isAvailable()) {
            if (loader.streamRequestStop) {
                loader.streamRequestStop(m_aaStream);
            }
            if (loader.streamClose) {
                loader.streamClose(m_aaStream);
            }
        }
        m_aaStream = nullptr;
    }
    if (m_aaBuilder) {
        auto& loader = HJAAudioLoader::instance();
        if (loader.isAvailable() && loader.streamBuilderDelete) {
            loader.streamBuilderDelete(m_aaBuilder);
        }
        m_aaBuilder = nullptr;
    }
#endif

    if (m_slPlay) {
        (*m_slPlay)->SetPlayState(m_slPlay, SL_PLAYSTATE_STOPPED);
    }
    {
        std::lock_guard<std::mutex> lock(m_openSLMutex);
        if (m_slBufferQueue) {
            (*m_slBufferQueue)->Clear(m_slBufferQueue);
            m_slBufferQueue = nullptr;
        }
        if (m_slPlayerObj) {
            (*m_slPlayerObj)->Destroy(m_slPlayerObj);
            m_slPlayerObj = nullptr;
            m_slPlay = nullptr;
            m_slVolume = nullptr;
        }
        if (m_slOutputMixObj) {
            (*m_slOutputMixObj)->Destroy(m_slOutputMixObj);
            m_slOutputMixObj = nullptr;
        }
        if (m_slEngineObj) {
            (*m_slEngineObj)->Destroy(m_slEngineObj);
            m_slEngineObj = nullptr;
            m_slEngine = nullptr;
        }
        m_slBuffers.clear();
        m_slBufferIndex = 0;
        m_slFramesPerBuffer = 0;
        m_slPlayedFrames.store(0);
    }
    m_backend = Backend::None;
    m_running = false;

    int64_t t1 = HJCurrentSteadyMS();
    HJFLogi("{} releaseRender end this:{} diff:{}", getName(), size_t(this), (t1 - t0));
#endif
}

int HJPluginAudioAARender::setStreamRunning(bool i_running, bool i_eofStop)
{
#if !defined(HJ_OS_ANDROID)
    (void)i_running;
    (void)i_eofStop;
    return HJ_OK;
#else
    if (i_running) {
        if (m_backend == Backend::AAudio) {
            if (!m_aaStream) {
                return HJErrFatal;
            }
            auto& loader = HJAAudioLoader::instance();
            if (!loader.isAvailable() || !loader.streamRequestStart) {
                return HJErrFatal;
            }
            const auto result = loader.streamRequestStart(m_aaStream);
            if (result != AAUDIO_OK) {
                return HJErrFatal;
            }
            return HJ_OK;
        }

        if (m_backend == Backend::OpenSL) {
            if (!m_slPlay) {
                return HJErrFatal;
            }

            size_t preloadCount = 0;
            {
                std::lock_guard<std::mutex> lock(m_openSLMutex);
                if (!m_slBufferQueue || m_slBuffers.empty()) {
                    return HJErrFatal;
                }

                (*m_slBufferQueue)->Clear(m_slBufferQueue);
                m_slBufferIndex = 0;
                preloadCount = m_slBuffers.size();
            }
            for (size_t i = 0; i < preloadCount; ++i) {
                enqueueOpenSLBuffer(false);
            }
            const SLresult result = (*m_slPlay)->SetPlayState(m_slPlay, SL_PLAYSTATE_PLAYING);
            if (result != SL_RESULT_SUCCESS) {
                return HJErrFatal;
            }
            return HJ_OK;
        }
        return HJErrFatal;
    }

    if (m_backend == Backend::AAudio) {
        if (!m_aaStream) {
            return HJ_OK;
        }
        auto& loader = HJAAudioLoader::instance();
        if (loader.isAvailable()) {
            if (i_eofStop) {
                if (loader.streamRequestStop) {
                    loader.streamRequestStop(m_aaStream);
                }
                if (loader.streamRequestFlush) {
                    loader.streamRequestFlush(m_aaStream);
                }
            } else if (loader.streamRequestPause) {
                loader.streamRequestPause(m_aaStream);
            } else if (loader.streamRequestStop) {
                loader.streamRequestStop(m_aaStream);
            }
        }
        return HJ_OK;
    }

    if (m_backend == Backend::OpenSL) {
        if (m_slPlay) {
            const SLuint32 playState = i_eofStop ? SL_PLAYSTATE_STOPPED : SL_PLAYSTATE_PAUSED;
            const SLresult result = (*m_slPlay)->SetPlayState(m_slPlay, playState);
            if (result != SL_RESULT_SUCCESS) {
                return HJErrFatal;
            }
        }
        if (i_eofStop) {
            std::lock_guard<std::mutex> lock(m_openSLMutex);
            if (m_slBufferQueue) {
                (*m_slBufferQueue)->Clear(m_slBufferQueue);
            }
        }
        return HJ_OK;
    }

    return HJ_OK;
#endif
}

#if defined(HJ_OS_ANDROID)
aaudio_data_callback_result_t HJPluginAudioAARender::onAAudioDataCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames)
{
    (void)stream;
    auto* render = static_cast<HJPluginAudioAARender*>(userData);
    if (!render || !audioData) {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    render->handleData(audioData, numFrames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void HJPluginAudioAARender::onAAudioErrorCallback(AAudioStream* stream, void* userData, aaudio_result_t error)
{
    (void)stream;
    auto* render = static_cast<HJPluginAudioAARender*>(userData);
    if (render) {
        HJFLoge("{}, AAudio error callback({})", render->getName(), error);
        if (error == AAUDIO_ERROR_DISCONNECTED) {
            render->postReinit();
        }
    }
}
#endif

#if defined(HJ_OS_ANDROID)
void HJPluginAudioAARender::onOpenSLBufferQueueCallback(SLAndroidSimpleBufferQueueItf queue, void* context)
{
    (void)queue;
    auto* render = static_cast<HJPluginAudioAARender*>(context);
    if (render) {
        render->handleOpenSLBufferQueue();
    }
}

void HJPluginAudioAARender::handleOpenSLBufferQueue()
{
    enqueueOpenSLBuffer(true);
}

void HJPluginAudioAARender::enqueueOpenSLBuffer(bool countPlayedFrames)
{
    int32_t dataSize = 0;
    int32_t framesPerBuffer = 0;

    {
        std::lock_guard<std::mutex> lock(m_openSLMutex);
        if (!m_slBufferQueue || m_slBuffers.empty() || m_slFramesPerBuffer <= 0 || m_blockAlign <= 0) {
            return;
        }

        dataSize = static_cast<int32_t>(m_slBuffers[m_slBufferIndex].size());
        framesPerBuffer = m_slFramesPerBuffer;
    }

    std::vector<uint8_t> workBuffer(static_cast<size_t>(dataSize));
    std::string route{};
    int64_t size{ -1 };
    auto [err, validSize, kernalFrame] = fillAudioBuffer(route, workBuffer.data(), dataSize, size);
    if (err != HJ_OK || validSize <= 0) {
        memset(workBuffer.data(), 0, workBuffer.size());
    } else if (validSize < dataSize) {
        memset(workBuffer.data() + validSize, 0, static_cast<size_t>(dataSize - validSize));
    } else if (kernalFrame != nullptr) {
        store(m_inputKeyHash.load(), kernalFrame);
    }
    applyOutputVolume(workBuffer.data(), dataSize);

    std::lock_guard<std::mutex> lock(m_openSLMutex);
    if (!m_slBufferQueue || m_slBuffers.empty() || m_slFramesPerBuffer != framesPerBuffer) {
        return;
    }

    std::vector<uint8_t>& submitBuffer = m_slBuffers[m_slBufferIndex];
    if (submitBuffer.size() != workBuffer.size()) {
        return;
    }
    memcpy(submitBuffer.data(), workBuffer.data(), workBuffer.size());

    SLresult result = (*m_slBufferQueue)->Enqueue(m_slBufferQueue, submitBuffer.data(), submitBuffer.size());
    if (result != SL_RESULT_SUCCESS) {
        HJFLoge("{}, OpenSL Enqueue error({})", getName(), result);
        return;
    }

    if (countPlayedFrames) {
        m_slPlayedFrames.fetch_add(framesPerBuffer);
    }
    if (m_pcmCallback) {
        appendPCMCallbackData(submitBuffer.data(), dataSize);
    }
    m_slBufferIndex = (m_slBufferIndex + 1) % m_slBuffers.size();
}
#endif

NS_HJ_END
