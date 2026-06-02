#pragma once

#include "HJPluginAudioRender.h"

#if defined(HJ_OS_IOS)
#include <AudioToolbox/AudioToolbox.h>
#endif

#include <mutex>

NS_HJ_BEGIN

// iOS audio render using AudioUnit RemoteIO.
class HJPluginAudioIOSRender : public HJPluginAudioRender
{
public:
    HJ_DEFINE_CREATE(HJPluginAudioIOSRender);

    HJPluginAudioIOSRender(const std::string& i_name = "HJPluginAudioIOSRender", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
        : HJPluginAudioRender(i_name, i_identify, i_graphInfo) {}
    virtual ~HJPluginAudioIOSRender() { done(); }

protected:
    virtual int64_t getRenderTimestamp() override;
    virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
    virtual void releaseRender() override;

private:
    static OSStatus onRenderCallback(void* inRefCon,
                                    AudioUnitRenderActionFlags* ioActionFlags,
                                    const AudioTimeStamp* inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList* ioData);
    OSStatus handleRender(const AudioTimeStamp* inTimeStamp, AudioBufferList* ioData, UInt32 inNumberFrames);

private:
#if defined(HJ_OS_IOS)
    AudioUnit m_audioUnit{ nullptr };
    AudioStreamBasicDescription m_streamDesc{};
    std::mutex m_renderTimestampMutex{};
    int64_t m_renderSampleTime{ 0 };
    uint64_t m_renderHostTime{ 0 };
#endif

private:
    int setStreamRunning(bool i_running, bool i_eofStop = false);
};

NS_HJ_END
