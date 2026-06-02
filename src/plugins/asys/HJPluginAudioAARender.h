#pragma once

#include "HJPluginAudioRender.h"

#if defined(HJ_OS_ANDROID)
#include <aaudio/AAudio.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#endif

#include <atomic>
#include <mutex>
#include <vector>

NS_HJ_BEGIN

// Android audio render using AAudio (API 26+) with OpenSL ES fallback.
class HJPluginAudioAARender : public HJPluginAudioRender
{
public:
    HJ_DEFINE_CREATE(HJPluginAudioAARender);

    HJPluginAudioAARender(const std::string& i_name = "HJPluginAudioAARender", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
        : HJPluginAudioRender(i_name, i_identify, i_graphInfo) {}
    virtual ~HJPluginAudioAARender() { done(); }

protected:
    virtual int64_t getRenderTimestamp() override;
    virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
    virtual void releaseRender() override;

private:
    enum class Backend {
        None,
        AAudio,
        OpenSL
    };

    void handleData(void* output, int32_t frameCount);

#if defined(HJ_OS_ANDROID)
    static aaudio_data_callback_result_t onAAudioDataCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames);
    static void onAAudioErrorCallback(AAudioStream* stream, void* userData, aaudio_result_t error);

    static void onOpenSLBufferQueueCallback(SLAndroidSimpleBufferQueueItf queue, void* context);
    void handleOpenSLBufferQueue();
    void enqueueOpenSLBuffer(bool countPlayedFrames);
#endif

    Backend m_backend{ Backend::None };
#if defined(HJ_OS_ANDROID)
    // AAudio backend
    AAudioStreamBuilder* m_aaBuilder{ nullptr };
    AAudioStream* m_aaStream{ nullptr };

    // OpenSL ES backend
    SLObjectItf m_slEngineObj{ nullptr };
    SLEngineItf m_slEngine{ nullptr };
    SLObjectItf m_slOutputMixObj{ nullptr };
    SLObjectItf m_slPlayerObj{ nullptr };
    SLPlayItf m_slPlay{ nullptr };
    SLAndroidSimpleBufferQueueItf m_slBufferQueue{ nullptr };
    SLVolumeItf m_slVolume{ nullptr };
    std::vector<std::vector<uint8_t>> m_slBuffers{};
    size_t m_slBufferIndex{ 0 };
    int32_t m_slFramesPerBuffer{ 0 };
    std::atomic<int64_t> m_slPlayedFrames{ 0 };
    std::mutex m_openSLMutex{};
#endif

private:
    int setStreamRunning(bool i_running, bool i_eofStop = false);
};

NS_HJ_END
