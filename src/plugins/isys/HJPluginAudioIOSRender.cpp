#include "HJPluginAudioIOSRender.h"
#include "HJGraph.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

namespace {

AudioStreamBasicDescription makeStreamDesc(const HJAudioInfo::Ptr& info, int& bytesPerSampleOut) {
    AudioStreamBasicDescription desc{};
    desc.mSampleRate = static_cast<Float64>(info->m_samplesRate);
    desc.mChannelsPerFrame = static_cast<UInt32>(info->m_channels);
    desc.mFramesPerPacket = 1;
    desc.mFormatID = kAudioFormatLinearPCM;
    desc.mFormatFlags = kAudioFormatFlagIsPacked;

    int bytesPerSample = 2;
    switch (info->m_sampleFmt) {
//#if defined(AV_SAMPLE_FMT_S16)
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        bytesPerSample = 2;
        desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
        break;
//#endif
//#if defined(AV_SAMPLE_FMT_S32)
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        bytesPerSample = 4;
        desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
        break;
//#endif
//#if defined(AV_SAMPLE_FMT_FLT)
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        bytesPerSample = 4;
        desc.mFormatFlags |= kAudioFormatFlagIsFloat;
        break;
//#endif
    default:
        bytesPerSample = 2;
        desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
        break;
    }

    bytesPerSampleOut = bytesPerSample;
    desc.mBitsPerChannel = static_cast<UInt32>(bytesPerSample * 8);
    desc.mBytesPerFrame = static_cast<UInt32>(bytesPerSample * info->m_channels);
    desc.mBytesPerPacket = desc.mBytesPerFrame * desc.mFramesPerPacket;
    return desc;
}

} // namespace

OSStatus HJPluginAudioIOSRender::onRenderCallback(void* inRefCon,
                                                  AudioUnitRenderActionFlags* ioActionFlags,
                                                  const AudioTimeStamp* inTimeStamp,
                                                  UInt32 inBusNumber,
                                                  UInt32 inNumberFrames,
                                                  AudioBufferList* ioData)
{
    (void)ioActionFlags;
    (void)inTimeStamp;
    (void)inBusNumber;

    if (!inRefCon || !ioData) {
        return noErr;
    }

    auto* render = static_cast<HJPluginAudioIOSRender*>(inRefCon);
    return render->handleRender(inTimeStamp, ioData, inNumberFrames);
}

OSStatus HJPluginAudioIOSRender::handleRender(const AudioTimeStamp* inTimeStamp, AudioBufferList* ioData, UInt32 inNumberFrames)
{
    if (inTimeStamp) {
        std::lock_guard<std::mutex> lock(m_renderTimestampMutex);
        if (inTimeStamp->mFlags & kAudioTimeStampSampleTimeValid) {
            const int64_t sampleTime = static_cast<int64_t>(inTimeStamp->mSampleTime);
            if (sampleTime >= 0) {
                m_renderSampleTime = sampleTime;
            }
        }
        if (inTimeStamp->mFlags & kAudioTimeStampHostTimeValid) {
            m_renderHostTime = HJCurrentSteadyMS(); //inTimeStamp->mHostTime;
        }
    }

    if (!ioData) {
        return noErr;
    }

    if (ioData->mNumberBuffers == 0) {
        return noErr;
    }

    auto log = logRunTask();
    if (log) {
        HJFLogi("{}, enter", getName());
    }

    std::string route{};
    int64_t size{ -1 };
    AudioBuffer& buffer = ioData->mBuffers[0];
    int32_t dataSize = static_cast<int32_t>(inNumberFrames * m_blockAlign);
    if (buffer.mDataByteSize < static_cast<UInt32>(dataSize)) {
        dataSize = static_cast<int32_t>(buffer.mDataByteSize);
    }

    auto [err, validSize, kernalFrame] = fillAudioBuffer(route, buffer.mData, dataSize, size);
    if (err != HJ_OK || validSize <= 0) {
        memset(buffer.mData, 0, buffer.mDataByteSize);
        if (m_pcmCallback) {
            appendPCMCallbackData(static_cast<const uint8_t*>(buffer.mData), dataSize);
        }
        if (log) {
            HJFLogi("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size,
                    (HJCurrentSteadyMS() - m_enterTimestamp), err);
        }
        return noErr;
    }

    if (validSize < dataSize) {
        memset(static_cast<uint8_t*>(buffer.mData) + validSize, 0, dataSize - validSize);
    } else if (kernalFrame != nullptr) {
        store(m_inputKeyHash.load(), kernalFrame);
    }
    applyOutputVolume(buffer.mData, dataSize);
    if (m_pcmCallback) {
        appendPCMCallbackData(static_cast<const uint8_t*>(buffer.mData), dataSize);
    }

    if (log) {
        HJFLogi("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size,
                (HJCurrentSteadyMS() - m_enterTimestamp), err);
    }
    return noErr;
}

int64_t HJPluginAudioIOSRender::getRenderTimestamp()
{
#if !defined(HJ_OS_IOS)
    return 0;
#else
    if (!m_audioInfo || m_audioInfo->m_samplesRate <= 0) {
        return 0;
    }

    int64_t sampleTime = 0;
    uint64_t renderHostTime = 0;
    {
        std::lock_guard<std::mutex> lock(m_renderTimestampMutex);
        sampleTime = m_renderSampleTime;
        renderHostTime = m_renderHostTime;
    }

    if (sampleTime <= 0) {
        return 0;
    }

    int64_t renderTimestampMs = (sampleTime * 1000LL) / m_audioInfo->m_samplesRate;
    if (renderHostTime > 0) {
        const uint64_t nowHostTime = HJCurrentSteadyMS(); //AudioGetCurrentHostTime();
        if (nowHostTime > renderHostTime) {
            const uint64_t deltaNanos = nowHostTime - renderHostTime; //AudioConvertHostTimeToNanos(nowHostTime - renderHostTime);
            renderTimestampMs += deltaNanos; //static_cast<int64_t>(deltaNanos / 1000000ULL);
        }
    }

    return renderTimestampMs;
#endif
}

int HJPluginAudioIOSRender::initRender(const HJAudioInfo::Ptr& i_audioInfo)
{
#if !defined(HJ_OS_IOS)
    (void)i_audioInfo;
    return HJErrNotSupport;
#else
    if (!i_audioInfo) {
        return HJErrInvalidParams;
    }

    HJFLogi("{} initRender enter this:{}", getName(), size_t(this));

    int bytesPerSample = 0;
    m_streamDesc = makeStreamDesc(i_audioInfo, bytesPerSample);
    m_blockAlign = i_audioInfo->m_channels * bytesPerSample;
    {
        std::lock_guard<std::mutex> lock(m_renderTimestampMutex);
        m_renderSampleTime = 0;
        m_renderHostTime = 0;
    }
    m_running = false;

    AudioComponentDescription desc{};
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) {
        HJFLoge("{}, AudioComponentFindNext failed", getName());
        return HJErrFatal;
    }

    OSStatus status = AudioComponentInstanceNew(comp, &m_audioUnit);
    if (status != noErr || !m_audioUnit) {
        HJFLoge("{}, AudioComponentInstanceNew error({})", getName(), (int)status);
        return HJErrFatal;
    }

    AURenderCallbackStruct callback{};
    callback.inputProc = HJPluginAudioIOSRender::onRenderCallback;
    callback.inputProcRefCon = this;
    status = AudioUnitSetProperty(m_audioUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input,
                                  0,
                                  &callback,
                                  sizeof(callback));
    if (status != noErr) {
        HJFLoge("{}, AudioUnitSetProperty(RenderCallback) error({})", getName(), (int)status);
        releaseRender();
        return HJErrFatal;
    }

    status = AudioUnitSetProperty(m_audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  0,
                                  &m_streamDesc,
                                  sizeof(m_streamDesc));
    if (status != noErr) {
        HJFLoge("{}, AudioUnitSetProperty(StreamFormat) error({})", getName(), (int)status);
        releaseRender();
        return HJErrFatal;
    }

    status = AudioUnitInitialize(m_audioUnit);
    if (status != noErr) {
        HJFLoge("{}, AudioUnitInitialize error({})", getName(), (int)status);
        releaseRender();
        return HJErrFatal;
    }

    HJFLogi("{} initRender end this:{}", getName(), size_t(this));
    return HJ_OK;
#endif
}

void HJPluginAudioIOSRender::releaseRender()
{
#if defined(HJ_OS_IOS)
    int64_t t0 = HJCurrentSteadyMS();
    HJFLogi("{} releaseRender enter this:{}", getName(), size_t(this));
    discardPendingPCMCallbackData();
    {
        std::lock_guard<std::mutex> lock(m_renderTimestampMutex);
        m_renderSampleTime = 0;
        m_renderHostTime = 0;
    }

    if (m_audioUnit) {
        AudioOutputUnitStop(m_audioUnit);
        AudioUnitUninitialize(m_audioUnit);
        AudioComponentInstanceDispose(m_audioUnit);
        m_audioUnit = nullptr;
    }
    m_running = false;

    int64_t t1 = HJCurrentSteadyMS();
    HJFLogi("{} releaseRender end this:{} diff:{}", getName(), size_t(this), (t1 - t0));
#endif
}

int HJPluginAudioIOSRender::setStreamRunning(bool i_running, bool i_eofStop)
{
#if !defined(HJ_OS_IOS)
    (void)i_running;
    (void)i_eofStop;
    return HJ_OK;
#else
    if (!m_audioUnit) {
        return HJ_OK;
    }

    if (i_running) {
        const OSStatus status = AudioOutputUnitStart(m_audioUnit);
        if (status != noErr) {
            return HJErrFatal;
        }
        return HJ_OK;
    }

    const OSStatus status = AudioOutputUnitStop(m_audioUnit);
    if (status != noErr) {
        return HJErrFatal;
    }
    if (i_eofStop) {
        {
            std::lock_guard<std::mutex> lock(m_renderTimestampMutex);
            m_renderSampleTime = 0;
            m_renderHostTime = 0;
        }
        const OSStatus resetStatus = AudioUnitReset(m_audioUnit, kAudioUnitScope_Global, 0);
        if (resetStatus != noErr) {
            return HJErrFatal;
        }
    }
    return HJ_OK;
#endif
}

NS_HJ_END
