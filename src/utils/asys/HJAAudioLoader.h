#pragma once

#if defined(HJ_OS_ANDROID)

#include <aaudio/AAudio.h>

class HJAAudioLoader
{
public:
    static HJAAudioLoader& instance();

    bool isAvailable() const { return m_ok; }

    aaudio_result_t (*createStreamBuilder)(AAudioStreamBuilder** builder) = nullptr;
    void (*streamBuilderDelete)(AAudioStreamBuilder* builder) = nullptr;
    aaudio_result_t (*streamBuilderOpenStream)(AAudioStreamBuilder* builder, AAudioStream** stream) = nullptr;
    void (*streamBuilderSetSampleRate)(AAudioStreamBuilder* builder, int32_t sampleRate) = nullptr;
    void (*streamBuilderSetChannelCount)(AAudioStreamBuilder* builder, int32_t channelCount) = nullptr;
    void (*streamBuilderSetFormat)(AAudioStreamBuilder* builder, aaudio_format_t format) = nullptr;
    void (*streamBuilderSetSharingMode)(AAudioStreamBuilder* builder, aaudio_sharing_mode_t mode) = nullptr;
    void (*streamBuilderSetPerformanceMode)(AAudioStreamBuilder* builder, aaudio_performance_mode_t mode) = nullptr;
    void (*streamBuilderSetFramesPerDataCallback)(AAudioStreamBuilder* builder, int32_t frames) = nullptr;
    void (*streamBuilderSetDataCallback)(AAudioStreamBuilder* builder, AAudioStream_dataCallback callback, void* userData) = nullptr;
    void (*streamBuilderSetErrorCallback)(AAudioStreamBuilder* builder, AAudioStream_errorCallback callback, void* userData) = nullptr;

    aaudio_result_t (*streamRequestStart)(AAudioStream* stream) = nullptr;
    aaudio_result_t (*streamRequestPause)(AAudioStream* stream) = nullptr;
    aaudio_result_t (*streamRequestStop)(AAudioStream* stream) = nullptr;
    aaudio_result_t (*streamRequestFlush)(AAudioStream* stream) = nullptr;
    aaudio_result_t (*streamClose)(AAudioStream* stream) = nullptr;
    int64_t (*streamGetFramesRead)(AAudioStream* stream) = nullptr;

private:
    HJAAudioLoader();

    void* m_handle = nullptr;
    bool m_ok = false;
};

#endif
