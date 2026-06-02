#if defined(HJ_LOG_TAG)
#   undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJAudioMixerJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJFLog.h"
#include "HJComEvent.h"
#include "HJGraphAudioMixer.h"
#include "HJGraphInfo.h"
#include "HJAudioMixer.h"
#include "HJMediaFrame.h"
#include "HJMediaInfo.h"
#include "HJAudioListenerJni.h"
#include "HJBaseListenerJni.h"
#include "HJCacheEnv.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <vector>

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_PLAYER_DECL(HJAudioMixerJni, sig)

extern "C"
{
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeAcquirePCM)(JNIEnv* env, jobject thiz, jlong i_data, jint i_nSize, jobject i_buffer);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInit)(JNIEnv* env, jobject thiz, jobject i_config, jobject i_listener, jobject i_audioListener);
JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeAddInput)(JNIEnv* env, jobject thiz, jobject i_inputDesc);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetInputVolume)(JNIEnv* env, jobject thiz, jstring i_inputId, jfloat i_volume);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetMasterMute)(JNIEnv* env, jobject thiz, jboolean i_muted);
JNIEXPORT jboolean JNICALL HJ_JNIFUN_MEDIA_DECL(nativeIsMasterMuted)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFlushInput)(JNIEnv* env, jobject thiz, jstring i_inputId);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeRemoveInput)(JNIEnv* env, jobject thiz, jstring i_inputId, jboolean i_drain);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativePushPcmPtr)(JNIEnv* env, jobject thiz, jstring i_inputId, jlong i_dataPtr, jint i_size, jint i_samples, jint i_channels, jint i_sampleRate, jint i_sampleFmt, jlong i_ptsMs);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativePushPcmBuffer)(JNIEnv* env, jobject thiz, jstring i_inputId, jobject i_buffer, jint i_size, jint i_samples, jint i_channels, jint i_sampleRate, jint i_sampleFmt, jlong i_ptsMs);
}

namespace
{
constexpr const char* kListClassName = "java/util/List";
constexpr const char* kInputListFieldSig = "Ljava/util/List;";
constexpr const char* kListSizeSig = "()I";
constexpr const char* kListGetSig = "(I)Ljava/lang/Object;";
constexpr int kAudioListenerFlagPcm = HJAudioListenerFlag_PCM;
constexpr int kAudioListenerFlagAacHeader = HJAudioListenerFlag_AACHEADER;
constexpr int kAudioListenerFlagAacData = HJAudioListenerFlag_AACDat;

struct HJAudioMixerJNI
{
    std::mutex mutex;
    std::mutex listenerMutex;
    HJGraphAudioMixer::Ptr mixer = nullptr;
    HJBaseListenerJni::Ptr listener = nullptr;
    HJAudioListenerJni::Ptr audioListener = nullptr;
    std::vector<uint8_t> lastAacCodecConfig{};
};

HJAudioMixerJNI* getHandle(JNIEnv* env, jobject thiz)
{
    return reinterpret_cast<HJAudioMixerJNI*>(
        HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName));
}

HJAudioMixerJNI* ensureHandle(JNIEnv* env, jobject thiz)
{
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (handle)
    {
        return handle;
    }

    handle = new (std::nothrow) HJAudioMixerJNI();
    if (!handle)
    {
        HJFLoge("ensureHandle alloc failed");
        return nullptr;
    }

    HJJNIField::SetLongField(env, thiz, reinterpret_cast<int64_t>(handle), HJJNIField::m_handleName);
    HJFLogi("ensureHandle created handle:{}", static_cast<const void*>(handle));
    return handle;
}

std::string escapeJson(const std::string& i_value)
{
    std::string out;
    out.reserve(i_value.size() + 8);
    for (char ch : i_value)
    {
        switch (ch)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(ch);
            break;
        }
    }
    return out;
}

std::string buildInputStateJson(const HJGraphAudioMixerInputState& i_state)
{
    return HJFMT(
        "{{\"inputId\":\"{}\",\"slotIndex\":{},\"attached\":{},\"starved\":{},\"eof\":{}}}",
        escapeJson(i_state.m_input_id),
        i_state.m_slot_index,
        i_state.m_attached ? "true" : "false",
        i_state.m_starved ? "true" : "false",
        i_state.m_eof ? "true" : "false");
}

std::string buildStatsJson(const HJGraphAudioMixerStats& i_stats)
{
    return HJFMT(
        "{{\"mixPts\":{},\"outputFrames\":{},\"silenceFillFrames\":{},\"lateDropFrames\":{},\"activeInputs\":{},\"starvedInputs\":{}}}",
        i_stats.m_mix_pts,
        i_stats.m_output_frames,
        i_stats.m_silence_fill_frames,
        i_stats.m_late_drop_frames,
        i_stats.m_active_inputs,
        i_stats.m_starved_inputs);
}

std::string buildAudioInfoSummary(const HJAudioInfo::Ptr& i_audioInfo)
{
    return i_audioInfo ? i_audioInfo->formatInfo() : "null";
}

std::string buildInputDescSummary(const HJAudioMixerInputDesc& i_desc)
{
    return HJFMT("inputId:{} volume:{} inputInfo:{}",
                 i_desc.m_input_id,
                 i_desc.m_volume,
                 buildAudioInfoSummary(i_desc.m_input_info));
}

std::string buildMixerConfigSummary(const HJAudioMixerConfig& i_config)
{
    return HJFMT("outputInfo:{} maxInputs:{} frameSamples:{} syncWindowMs:{} lateThresholdMs:{} enableCompand:{} enableLimiter:{} outType:{} aacType:{} inputCount:{}",
                 buildAudioInfoSummary(i_config.m_output_info),
                 i_config.m_max_inputs,
                 i_config.m_frame_samples,
                 i_config.m_sync_window_ms,
                 i_config.m_late_threshold_ms,
                 i_config.m_enable_compand ? "true" : "false",
                 i_config.m_enable_limiter ? "true" : "false",
                 i_config.m_out_type,
                 i_config.m_aac_type,
                 static_cast<int>(i_config.m_inputs.size()));
}

std::string buildPcmArgsSummary(const std::string& i_inputId,
                                int i_size,
                                int i_samples,
                                int i_channels,
                                int i_sampleRate,
                                int i_sampleFmt,
                                int64_t i_ptsMs)
{
    return HJFMT("input:{} size:{} samples:{} channels:{} sampleRate:{} sampleFmt:{} ptsMs:{}",
                 i_inputId,
                 i_size,
                 i_samples,
                 i_channels,
                 i_sampleRate,
                 i_sampleFmt,
                 i_ptsMs);
}

HJBaseListenerJni::Ptr copyBaseListener(HJAudioMixerJNI* handle)
{
    if (!handle)
    {
        return nullptr;
    }
    std::lock_guard<std::mutex> lk(handle->listenerMutex);
    return handle->listener;
}

HJAudioListenerJni::Ptr copyAudioListener(HJAudioMixerJNI* handle)
{
    if (!handle)
    {
        return nullptr;
    }
    std::lock_guard<std::mutex> lk(handle->listenerMutex);
    return handle->audioListener;
}

void notifyAudioJava(HJAudioMixerJNI* handle,
                     const HJAudioInfo::Ptr& i_audioInfo,
                     int i_bytesPerSample,
                     const uint8_t* i_data,
                     int i_size,
                     int i_flag)
{
    if (!handle || !i_audioInfo || !i_data || i_size <= 0)
    {
        HJFLogw("notifyAudioJava skip invalid state handle:{} audioInfo:{} data:{} size:{} flag:{}",
                static_cast<const void*>(handle),
                buildAudioInfoSummary(i_audioInfo),
                static_cast<const void*>(i_data),
                i_size,
                i_flag);
        return;
    }

    const auto audioListener = copyAudioListener(handle);
    if (!audioListener)
    {
        return;
    }

    const int notifyRet = audioListener->notify(i_audioInfo->m_samplesRate,
                                                i_audioInfo->m_channels,
                                                i_audioInfo->m_sampleFmt,
                                                i_bytesPerSample,
                                                reinterpret_cast<int64_t>(const_cast<uint8_t*>(i_data)),
                                                i_size,
                                                i_flag);
    if (notifyRet != HJ_OK)
    {
        HJFLoge("notifyAudioJava failed ret:{} audioInfo:{} bytesPerSample:{} size:{} flag:{}",
                notifyRet,
                buildAudioInfoSummary(i_audioInfo),
                i_bytesPerSample,
                i_size,
                i_flag);
    }
}

void maybeNotifyAacCodecConfigJava(HJAudioMixerJNI* handle,
                                   const HJAudioInfo::Ptr& i_audioInfo)
{
    if (!handle || !i_audioInfo)
    {
        return;
    }

    const auto codecParams = i_audioInfo->getCodecParams();
    const AVCodecParameters* avCodecParams =
        codecParams ? codecParams->getAVCodecParameters() : nullptr;
    if (!avCodecParams || !avCodecParams->extradata ||
        avCodecParams->extradata_size <= 0)
    {
        return;
    }

    bool shouldNotify = false;
    {
        std::lock_guard<std::mutex> guard(handle->mutex);
        const auto codecConfigSize =
            static_cast<size_t>(avCodecParams->extradata_size);
        const bool codecConfigChanged =
            handle->lastAacCodecConfig.size() != codecConfigSize ||
            !std::equal(handle->lastAacCodecConfig.begin(),
                        handle->lastAacCodecConfig.end(),
                        avCodecParams->extradata);
        if (codecConfigChanged)
        {
            handle->lastAacCodecConfig.assign(avCodecParams->extradata,
                                              avCodecParams->extradata +
                                                  codecConfigSize);
            shouldNotify = true;
        }
    }

    if (!shouldNotify)
    {
        return;
    }

    HJFLogi("notify aac codec config size:{} audioInfo:{}",
            avCodecParams->extradata_size,
            buildAudioInfoSummary(i_audioInfo));
    notifyAudioJava(handle,
                    i_audioInfo,
                    0,
                    avCodecParams->extradata,
                    avCodecParams->extradata_size,
                    kAudioListenerFlagAacHeader);
}

void notifyJava(const HJBaseListenerJni::Ptr& i_listener, int i_id, int64_t i_value, const std::string& i_desc)
{
    if (!i_listener)
    {
        return;
    }
    const int notifyRet = i_listener->notify(i_id, i_value, i_desc);
    HJFLogi("notifyJava id:{} value:{} desc:{} ret:{}", i_id, i_value, i_desc, notifyRet);
}

int getPackedPcmSize(int i_samples,
                     int i_channels,
                     int i_sampleFmt,
                     int& o_bytesPerSample,
                     int& o_pcmSize)
{
    if (i_samples <= 0 || i_channels <= 0)
    {
        return HJErrInvalidParams;
    }

    const AVSampleFormat sampleFmt = static_cast<AVSampleFormat>(i_sampleFmt);
    o_bytesPerSample = av_get_bytes_per_sample(sampleFmt);
    if (o_bytesPerSample <= 0 || av_sample_fmt_is_planar(sampleFmt))
    {
        return HJErrNotSupport;
    }

    const uint64_t pcmSize = static_cast<uint64_t>(i_samples) *
                             static_cast<uint64_t>(i_channels) *
                             static_cast<uint64_t>(o_bytesPerSample);
    if (pcmSize == 0 || pcmSize > static_cast<uint64_t>(std::numeric_limits<int>::max()))
    {
        return HJErrInvalidParams;
    }

    o_pcmSize = static_cast<int>(pcmSize);
    return HJ_OK;
}

int64_t ptsMsToSamples(int64_t i_ptsMs, int i_sampleRate)
{
    if (i_ptsMs <= 0 || i_sampleRate <= 0)
    {
        return 0;
    }
    return (i_ptsMs * static_cast<int64_t>(i_sampleRate)) / 1000;
}

jobject getObjectField(JNIEnv* env, jobject i_obj, const char* i_name, const char* i_sig)
{
    if (!env || !i_obj || !i_name || !i_sig)
    {
        return nullptr;
    }

    jclass cls = env->GetObjectClass(i_obj);
    if (!cls)
    {
        return nullptr;
    }

    jfieldID fieldId = env->GetFieldID(cls, i_name, i_sig);
    env->DeleteLocalRef(cls);
    if (!fieldId)
    {
        return nullptr;
    }
    return env->GetObjectField(i_obj, fieldId);
}

int buildAudioInfo(int i_sampleRate,
                   int i_channels,
                   int i_sampleFmt,
                   int i_samples,
                   HJAudioInfo::Ptr& o_audioInfo)
{
    if (i_sampleRate <= 0 || i_channels <= 0 || i_samples <= 0)
    {
        return HJErrInvalidParams;
    }

    int bytesPerSample = 0;
    int pcmSize = 0;
    const int ret = getPackedPcmSize(i_samples, i_channels, i_sampleFmt, bytesPerSample, pcmSize);
    (void)pcmSize;
    if (ret != HJ_OK)
    {
        return ret;
    }

    auto audioInfo = HJAudioInfo::makeAudioInfo(i_channels, i_sampleRate, i_sampleFmt, i_samples);
    if (!audioInfo)
    {
        return HJErrFatal;
    }
    audioInfo->m_bytesPerSample = bytesPerSample;
    audioInfo->m_samplesPerFrame = i_samples;
    o_audioInfo = audioInfo;
    return HJ_OK;
}

int parseInputDesc(JNIEnv* env, jobject i_inputObj, int i_frameSamples, HJAudioMixerInputDesc& o_desc)
{
    if (!env || !i_inputObj)
    {
        HJFLoge("parseInputDesc invalid params env:{} inputObj:{}",
                static_cast<const void*>(env),
                static_cast<const void*>(i_inputObj));
        return HJErrInvalidParams;
    }

    const std::string inputId = HJJNIField::GetStringField(env, i_inputObj, "inputId");
    const int sampleRate = HJJNIField::GetIntField(env, i_inputObj, "sampleRate");
    const int channels = HJJNIField::GetIntField(env, i_inputObj, "channels");
    const int sampleFmt = HJJNIField::GetIntField(env, i_inputObj, "sampleFmt");
    const float volume = HJJNIField::GetFloatField(env, i_inputObj, "volume");
    if (inputId.empty())
    {
        HJFLoge("parseInputDesc empty inputId sampleRate:{} channels:{} sampleFmt:{} volume:{} frameSamples:{}",
                sampleRate,
                channels,
                sampleFmt,
                volume,
                i_frameSamples);
        return HJErrInvalidParams;
    }

    HJAudioInfo::Ptr inputInfo = nullptr;
    const int ret = buildAudioInfo(sampleRate,
                                   channels,
                                   sampleFmt,
                                   i_frameSamples > 0 ? i_frameSamples : HJ_AUDIO_MIXER_OUTPUT_SAMPLES,
                                   inputInfo);
    if (ret != HJ_OK)
    {
        HJFLoge("parseInputDesc buildAudioInfo failed ret:{} inputId:{} sampleRate:{} channels:{} sampleFmt:{} frameSamples:{}",
                ret,
                inputId,
                sampleRate,
                channels,
                sampleFmt,
                i_frameSamples);
        return ret;
    }

    o_desc.m_input_id = inputId;
    o_desc.m_input_info = inputInfo;
    o_desc.m_volume = volume;
    HJFLogi("parseInputDesc {}", buildInputDescSummary(o_desc));
    return HJ_OK;
}

int parseMixerConfig(JNIEnv* env, jobject i_configObj, HJAudioMixerConfig& o_config)
{
    if (!env || !i_configObj)
    {
        HJFLoge("parseMixerConfig invalid params env:{} configObj:{}",
                static_cast<const void*>(env),
                static_cast<const void*>(i_configObj));
        return HJErrInvalidParams;
    }

    const int outputSampleRate = HJJNIField::GetIntField(env, i_configObj, "outputSampleRate");
    const int outputChannels = HJJNIField::GetIntField(env, i_configObj, "outputChannels");
    const int outputSampleFmt = HJJNIField::GetIntField(env, i_configObj, "outputSampleFmt");
    const int maxInputs = HJJNIField::GetIntField(env, i_configObj, "maxInputs");
    const int frameSamples = HJJNIField::GetIntField(env, i_configObj, "frameSamples");
    const int syncWindowMs = HJJNIField::GetIntField(env, i_configObj, "syncWindowMs");
    const int lateThresholdMs = HJJNIField::GetIntField(env, i_configObj, "lateThresholdMs");
    const bool enableCompand = HJJNIField::GetBooleanField(env, i_configObj, "enableCompand") != 0;
    const bool enableLimiter = HJJNIField::GetBooleanField(env, i_configObj, "enableLimiter") != 0;
    const int outType = HJJNIField::GetIntField(env, i_configObj, "outType");
    const int aacType = HJJNIField::GetIntField(env, i_configObj, "aacType");

    HJAudioInfo::Ptr outputInfo = nullptr;
    int ret = buildAudioInfo(outputSampleRate,
                             outputChannels,
                             outputSampleFmt,
                             frameSamples > 0 ? frameSamples : HJ_AUDIO_MIXER_OUTPUT_SAMPLES,
                             outputInfo);
    if (ret != HJ_OK)
    {
        HJFLoge("parseMixerConfig build output audio info failed ret:{} outputSampleRate:{} outputChannels:{} outputSampleFmt:{} frameSamples:{}",
                ret,
                outputSampleRate,
                outputChannels,
                outputSampleFmt,
                frameSamples);
        return ret;
    }

    o_config.m_output_info = outputInfo;
    o_config.m_max_inputs = maxInputs > 0 ? maxInputs : 1;
    o_config.m_frame_samples = frameSamples > 0 ? frameSamples : HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
    o_config.m_sync_window_ms = syncWindowMs > 0 ? syncWindowMs : 42;
    o_config.m_late_threshold_ms = lateThresholdMs > 0 ? lateThresholdMs : 150;
    o_config.m_enable_compand = enableCompand;
    o_config.m_enable_limiter = enableLimiter;
    o_config.m_out_type = outType;
    o_config.m_aac_type = aacType;

    jobject inputsObj = getObjectField(env, i_configObj, "inputs", kInputListFieldSig);
    if (!inputsObj)
    {
        HJFLogi("parseMixerConfig no input list {}", buildMixerConfigSummary(o_config));
        return HJ_OK;
    }

    jclass listClass = env->FindClass(kListClassName);
    if (!listClass)
    {
        env->DeleteLocalRef(inputsObj);
        HJFLoge("parseMixerConfig FindClass failed class:{}", kListClassName);
        return HJErrInvalidParams;
    }

    const jmethodID sizeMethod = env->GetMethodID(listClass, "size", kListSizeSig);
    const jmethodID getMethod = env->GetMethodID(listClass, "get", kListGetSig);
    if (!sizeMethod || !getMethod)
    {
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(inputsObj);
        HJFLoge("parseMixerConfig get list methods failed sizeMethod:{} getMethod:{}",
                static_cast<const void*>(sizeMethod),
                static_cast<const void*>(getMethod));
        return HJErrInvalidParams;
    }

    const jint inputCount = env->CallIntMethod(inputsObj, sizeMethod);
    o_config.m_inputs.reserve(static_cast<size_t>(std::max(inputCount, 0)));
    for (jint i = 0; i < inputCount; ++i)
    {
        jobject inputObj = env->CallObjectMethod(inputsObj, getMethod, i);
        if (!inputObj)
        {
            continue;
        }

        HJAudioMixerInputDesc inputDesc{};
        ret = parseInputDesc(env, inputObj, o_config.m_frame_samples, inputDesc);
        env->DeleteLocalRef(inputObj);
        if (ret != HJ_OK)
        {
            env->DeleteLocalRef(listClass);
            env->DeleteLocalRef(inputsObj);
            HJFLoge("parseMixerConfig parseInputDesc failed ret:{} index:{}",
                    ret,
                    i);
            return ret;
        }
        o_config.m_inputs.push_back(std::move(inputDesc));
    }

    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(inputsObj);
    HJFLogi("parseMixerConfig {}", buildMixerConfigSummary(o_config));
    return HJ_OK;
}

int getMixerFrameSamples(const HJGraphAudioMixer::Ptr& i_mixer)
{
    if (!i_mixer)
    {
        return HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
    }

    const auto outputInfo = i_mixer->getOutputAudioInfo();
    if (!outputInfo)
    {
        return HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
    }

    if (outputInfo->m_samplesPerFrame > 0)
    {
        return outputInfo->m_samplesPerFrame;
    }

    if (outputInfo->m_sampleCnt > 0)
    {
        return outputInfo->m_sampleCnt;
    }

    return HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
}

void notifyMixedAudioJava(HJAudioMixerJNI* handle, const HJMediaFrame::Ptr& i_frame)
{
    if (!handle || !i_frame)
    {
        HJFLogw("notifyMixedAudioJava skip invalid params handle:{} frame:{}",
                static_cast<const void*>(handle),
                static_cast<const void*>(i_frame.get()));
        return;
    }
    HJFPERLogi("mixed audio frame:{}", i_frame->formatInfo());

    const auto audioInfo = i_frame->getAudioInfo();
    AVFrame* avf = i_frame->getAVFrame();
    if (!audioInfo)
    {
        HJFLogw("notifyMixedAudioJava skip frame without audioInfo");
        return;
    }

    AVPacket* avp = i_frame->getAVPacket();
    if (avp != nullptr)
    {
        maybeNotifyAacCodecConfigJava(handle, audioInfo);
        notifyAudioJava(handle,
                        audioInfo,
                        0,
                        avp->data,
                        avp->size,
                        kAudioListenerFlagAacData);
        return;
    }
    if (!avf)
    {
        HJFLogw("notifyMixedAudioJava skip frame without avFrame audioInfo:{}",
                buildAudioInfoSummary(audioInfo));
        return;
    }

    const AVSampleFormat sampleFmt = static_cast<AVSampleFormat>(audioInfo->m_sampleFmt);
    if (av_sample_fmt_is_planar(sampleFmt))
    {
        HJFLogw("skip planar mixed pcm sampleFmt:{}", audioInfo->m_sampleFmt);
        return;
    }

    const int bytesPerSample = audioInfo->m_bytesPerSample > 0
        ? audioInfo->m_bytesPerSample
        : av_get_bytes_per_sample(sampleFmt);
    if (bytesPerSample <= 0 || avf->data[0] == nullptr)
    {
        HJFLogw("notifyMixedAudioJava invalid pcm payload bytesPerSample:{} data:{} audioInfo:{}",
                bytesPerSample,
                static_cast<const void*>(avf->data[0]),
                buildAudioInfoSummary(audioInfo));
        return;
    }

    const int sampleCount = avf->nb_samples > 0 ? avf->nb_samples : audioInfo->m_sampleCnt;
    const int pcmSize = av_samples_get_buffer_size(nullptr,
                                                   audioInfo->m_channels,
                                                   sampleCount,
                                                   sampleFmt,
                                                   1);
    if (pcmSize <= 0)
    {
        HJFLogw("notifyMixedAudioJava invalid pcmSize:{} audioInfo:{} sampleCount:{}",
                pcmSize,
                buildAudioInfoSummary(audioInfo),
                sampleCount);
        return;
    }

    notifyAudioJava(handle,
                    audioInfo,
                    bytesPerSample,
                    avf->data[0],
                    pcmSize,
                    kAudioListenerFlagPcm);
}

void registerHandlers(const std::shared_ptr<HJEventBus>& i_bus, HJAudioMixerJNI* handle)
{
    if (!i_bus || !handle)
    {
        return;
    }

    i_bus->registerHandler(EVENT_GRAPH_AUDIO_FRAME_ID, [handle](const HJMediaFrame::Ptr& frame)
    {
        notifyMixedAudioJava(handle, frame);
    });

    i_bus->registerHandler(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID, [handle](const HJGraphAudioMixerInputState& state)
    {
        notifyJava(copyBaseListener(handle),
                   static_cast<int>(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE::id),
                   state.m_attached ? 1 : 0,
                   buildInputStateJson(state));
    });

    i_bus->registerHandler(EVENT_GRAPH_AUDIO_MIXER_STATS_ID, [handle](const HJGraphAudioMixerStats& stats)
    {
        notifyJava(copyBaseListener(handle),
                   static_cast<int>(EVENT_GRAPH_AUDIO_MIXER_STATS::id),
                   stats.m_mix_pts,
                   buildStatsJson(stats));
    });
}

int initAudioMixerLocked(HJAudioMixerJNI* handle,
                         jobject i_configObj,
                         jobject i_listener,
                         jobject i_audioListener)
{
    if (!handle)
    {
        HJFLoge("initAudioMixerLocked handle null");
        return HJErrInvalidParams;
    }
    HJFLogi("initAudioMixerLocked enter handle:{} listener:{} audioListener:{}",
            static_cast<const void*>(handle),
            static_cast<const void*>(i_listener),
            static_cast<const void*>(i_audioListener));

    if (handle->mixer)
    {
        HJFLogi("initAudioMixerLocked reset existing mixer handle:{}",
                static_cast<const void*>(handle));
        handle->mixer->done();
        handle->mixer.reset();
    }
    {
        std::lock_guard<std::mutex> lk(handle->listenerMutex);
        handle->listener = nullptr;
        handle->audioListener = nullptr;
    }
    handle->lastAacCodecConfig.clear();

    HJAudioMixerConfig config{};
    int ret = parseMixerConfig(HJCacheEnv::justGetEnv(), i_configObj, config);
    if (ret != HJ_OK)
    {
        HJFLoge("parseMixerConfig failed ret:{}", ret);
        return ret;
    }
    HJFLogi("initAudioMixerLocked parsed config:{}", buildMixerConfigSummary(config));

    HJBaseListenerJni::Ptr jListener = nullptr;
    if (i_listener)
    {
        jListener = HJBaseListenerJni::Create();
        ret = jListener ? jListener->init(i_listener) : HJErrNotInited;
        if (ret != HJ_OK)
        {
            HJFLoge("initAudioMixerLocked listener init failed ret:{}", ret);
            return ret;
        }
    }

    HJAudioListenerJni::Ptr audioListener = nullptr;
    if (i_audioListener)
    {
        audioListener = HJAudioListenerJni::Create();
        ret = audioListener ? audioListener->init(i_audioListener) : HJErrNotInited;
        if (ret != HJ_OK)
        {
            HJFLoge("initAudioMixerLocked audioListener init failed ret:{}", ret);
            return ret;
        }
    }

    auto mixer = HJGraphAudioMixer::Create();
    if (!mixer)
    {
        HJFLoge("initAudioMixerLocked create mixer failed");
        return HJErrNotInited;
    }
    registerHandlers(mixer->eventBus(), handle);

    ret = mixer->initWithConfig(config);
    if (ret != HJ_OK)
    {
        HJFLoge("initAudioMixerLocked mixer->initWithConfig failed ret:{} config:{}",
                ret,
                buildMixerConfigSummary(config));
        mixer->done();
        return ret;
    }

    {
        std::lock_guard<std::mutex> lk(handle->listenerMutex);
        handle->listener = jListener;
        handle->audioListener = audioListener;
    }
    handle->mixer = mixer;

    HJFLogi("initAudioMixerLocked done handle:{} mixer:{}",
            static_cast<const void*>(handle),
            static_cast<const void*>(mixer.get()));
    return HJ_OK;
}

int withInputId(JNIEnv* env, jstring i_inputId, const std::function<int(const std::string&)>& i_fn)
{
    if (!env || !i_inputId || !i_fn)
    {
        HJFLoge("withInputId invalid params env:{} inputId:{} fn:{}",
                static_cast<const void*>(env),
                static_cast<const void*>(i_inputId),
                i_fn ? "set" : "null");
        return HJErrInvalidParams;
    }

    const char* inputId = HJJNIField::JStringToString(env, i_inputId);
    if (!inputId || !inputId[0])
    {
        HJJNIField::JStringDestroy(env, inputId, i_inputId);
        HJFLoge("withInputId empty inputId");
        return HJErrInvalidParams;
    }

    const std::string inputIdStr = inputId;
    HJJNIField::JStringDestroy(env, inputId, i_inputId);
    return i_fn(inputIdStr);
}

int pushPcmLocked(HJAudioMixerJNI* handle,
                  const std::string& i_inputId,
                  uint8_t* i_pcm,
                  int i_size,
                  int i_samples,
                  int i_channels,
                  int i_sampleRate,
                  int i_sampleFmt,
                  int64_t i_ptsMs)
{
    if (!handle || !handle->mixer || i_inputId.empty() || !i_pcm || i_size <= 0 || i_samples <= 0 ||
        i_channels <= 0 || i_sampleRate <= 0)
    {
        HJFLoge("pushPcmLocked invalid params handle:{} mixer:{} pcm:{} {}",
                static_cast<const void*>(handle),
                handle ? static_cast<const void*>(handle->mixer.get()) : nullptr,
                static_cast<const void*>(i_pcm),
                buildPcmArgsSummary(i_inputId, i_size, i_samples, i_channels, i_sampleRate, i_sampleFmt, i_ptsMs));
        return HJErrInvalidParams;
    }

    int bytesPerSample = 0;
    int expectedPcmSize = 0;
    const int ret = getPackedPcmSize(i_samples, i_channels, i_sampleFmt, bytesPerSample, expectedPcmSize);
    if (ret != HJ_OK)
    {
        HJFLoge("pushPcmLocked getPackedPcmSize failed ret:{} {}",
                ret,
                buildPcmArgsSummary(i_inputId, i_size, i_samples, i_channels, i_sampleRate, i_sampleFmt, i_ptsMs));
        return ret;
    }
    (void)bytesPerSample;
    if (i_size != expectedPcmSize)
    {
        HJFLogw("pcm size mismatch input:{} size:{} expected:{}", i_inputId, i_size, expectedPcmSize);
        return HJErrInvalidParams;
    }

    const int pushRet = handle->mixer->pushFrame(i_inputId, i_pcm, i_samples, i_channels, i_sampleRate, i_sampleFmt, i_ptsMs);
    if (pushRet != HJ_OK)
    {
        HJFLoge("pushPcmLocked pushFrame failed ret:{} {}",
                pushRet,
                buildPcmArgsSummary(i_inputId, i_size, i_samples, i_channels, i_sampleRate, i_sampleFmt, i_ptsMs));
    }
    return pushRet;
}
} // namespace

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeAcquirePCM)(JNIEnv* env, jobject thiz, jlong i_data, jint i_nSize, jobject i_buffer)
{
    (void)thiz;
    if (!env || !i_buffer || i_data == 0 || i_nSize <= 0)
    {
        HJFLoge("nativeAcquirePCM invalid params env:{} buffer:{} data:{} size:{}",
                static_cast<const void*>(env),
                static_cast<const void*>(i_buffer),
                static_cast<int64_t>(i_data),
                i_nSize);
        return HJErrInvalidParams;
    }

    void* dst = env->GetDirectBufferAddress(i_buffer);
    if (!dst)
    {
        HJFLoge("nativeAcquirePCM GetDirectBufferAddress failed buffer:{}",
                static_cast<const void*>(i_buffer));
        return HJErrInvalidParams;
    }

    const jlong capacity = env->GetDirectBufferCapacity(i_buffer);
    if (capacity < 0 || capacity < i_nSize)
    {
        HJFLoge("nativeAcquirePCM invalid capacity:{} size:{}",
                static_cast<int64_t>(capacity),
                i_nSize);
        return HJErrInvalidParams;
    }

    const void* src = reinterpret_cast<const void*>(static_cast<intptr_t>(i_data));
    if (!src)
    {
        HJFLoge("nativeAcquirePCM src null data:{}", static_cast<int64_t>(i_data));
        return HJErrInvalidParams;
    }

    std::copy_n(reinterpret_cast<const uint8_t*>(src),
                static_cast<size_t>(i_nSize),
                reinterpret_cast<uint8_t*>(dst));
    return HJ_OK;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInit)(JNIEnv* env, jobject thiz, jobject i_config, jobject i_listener, jobject i_audioListener)
{
    HJFLogi("nativeInit enter config:{} listener:{} audioListener:{}",
            static_cast<const void*>(i_config),
            static_cast<const void*>(i_listener),
            static_cast<const void*>(i_audioListener));
    HJAudioMixerJNI* handle = ensureHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeInit ensureHandle failed");
        return HJErrNotInited;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = initAudioMixerLocked(handle, i_config, i_listener, i_audioListener);
    HJFLogi("nativeInit done ret:{} handle:{}", ret, static_cast<const void*>(handle));
    return ret;
}

JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(JNIEnv* env, jobject thiz)
{
    HJFLogi("nativeDone enter");
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLogi("nativeDone skip, handle null");
        return;
    }

    {
        std::lock_guard<std::mutex> guard(handle->mutex);
        if (handle->mixer)
        {
            HJFLogi("nativeDone mixer done enter mixer:{}",
                    static_cast<const void*>(handle->mixer.get()));
            handle->mixer->done();
            handle->mixer.reset();
        }
        {
            HJFLogi("nativeDone clear listeners");
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            handle->listener = nullptr;
            handle->audioListener = nullptr;
        }
    }

    delete handle;
    HJFLogi("nativeDone delete handle");
    HJJNIField::SetLongField(env, thiz, 0, HJJNIField::m_handleName);
    HJFLogi("nativeDone done");
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeAddInput)(JNIEnv* env, jobject thiz, jobject i_inputDesc)
{
    HJFLogi("nativeAddInput enter inputDesc:{}", static_cast<const void*>(i_inputDesc));
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeAddInput handle null");
        return HJErrNotInited;
    }
    if (!i_inputDesc)
    {
        HJFLoge("nativeAddInput inputDesc null");
        return HJErrInvalidParams;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    if (!handle->mixer)
    {
        HJFLoge("nativeAddInput mixer null");
        return HJErrNotInited;
    }

    HJAudioMixerInputDesc inputDesc{};
    const int ret = parseInputDesc(env, i_inputDesc, getMixerFrameSamples(handle->mixer), inputDesc);
    if (ret != HJ_OK)
    {
        HJFLoge("nativeAddInput parseInputDesc failed ret:{}", ret);
        return ret;
    }
    const int addRet = handle->mixer->addInput(inputDesc);
    HJFLogi("nativeAddInput done ret:{} {}", addRet, buildInputDescSummary(inputDesc));
    return addRet;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetInputVolume)(JNIEnv* env, jobject thiz, jstring i_inputId, jfloat i_volume)
{
    HJFLogi("nativeSetInputVolume enter volume:{}", static_cast<float>(i_volume));
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeSetInputVolume handle null");
        return HJErrNotInited;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = withInputId(env, i_inputId, [handle, i_volume](const std::string& inputId)
    {
        return handle->mixer ? handle->mixer->setInputVolume(inputId, i_volume) : HJErrNotInited;
    });
    HJFLogi("nativeSetInputVolume done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetMasterMute)(JNIEnv* env, jobject thiz, jboolean i_muted)
{
    HJFLogi("nativeSetMasterMute enter muted:{}", i_muted == JNI_TRUE);
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle || !handle->mixer)
    {
        HJFLoge("nativeSetMasterMute handle or mixer null handle:{} mixer:{}",
                static_cast<const void*>(handle),
                handle ? static_cast<const void*>(handle->mixer.get()) : nullptr);
        return HJErrNotInited;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->mixer->setMasterMute(i_muted == JNI_TRUE);
    HJFLogi("nativeSetMasterMute done ret:{}", ret);
    return ret;
}

JNIEXPORT jboolean JNICALL HJ_JNIFUN_MEDIA_DECL(nativeIsMasterMuted)(JNIEnv* env, jobject thiz)
{
    HJFLogi("nativeIsMasterMuted enter");
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle || !handle->mixer)
    {
        HJFLoge("nativeIsMasterMuted handle or mixer null handle:{} mixer:{}",
                static_cast<const void*>(handle),
                handle ? static_cast<const void*>(handle->mixer.get()) : nullptr);
        return JNI_FALSE;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const jboolean muted = handle->mixer->isMasterMuted() ? JNI_TRUE : JNI_FALSE;
    HJFLogi("nativeIsMasterMuted done muted:{}", muted == JNI_TRUE);
    return muted;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFlushInput)(JNIEnv* env, jobject thiz, jstring i_inputId)
{
    HJFLogi("nativeFlushInput enter");
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeFlushInput handle null");
        return HJErrNotInited;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = withInputId(env, i_inputId, [handle](const std::string& inputId)
    {
        return handle->mixer ? handle->mixer->flushInput(inputId) : HJErrNotInited;
    });
    HJFLogi("nativeFlushInput done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeRemoveInput)(JNIEnv* env, jobject thiz, jstring i_inputId, jboolean i_drain)
{
    HJFLogi("nativeRemoveInput enter drain:{}", i_drain == JNI_TRUE);
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeRemoveInput handle null");
        return HJErrNotInited;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = withInputId(env, i_inputId, [handle, i_drain](const std::string& inputId)
    {
        return handle->mixer ? handle->mixer->removeInput(inputId, false/*i_drain == JNI_TRUE*/) : HJErrNotInited;
    });
    HJFLogi("nativeRemoveInput done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativePushPcmPtr)(JNIEnv* env,
                                                              jobject thiz,
                                                              jstring i_inputId,
                                                              jlong i_dataPtr,
                                                              jint i_size,
                                                              jint i_samples,
                                                              jint i_channels,
                                                              jint i_sampleRate,
                                                              jint i_sampleFmt,
                                                              jlong i_ptsMs)
{
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativePushPcmPtr handle null");
        return HJErrNotInited;
    }
    if (i_dataPtr == 0)
    {
        HJFLoge("nativePushPcmPtr dataPtr null size:{} samples:{} channels:{} sampleRate:{} sampleFmt:{} ptsMs:{}",
                i_size,
                i_samples,
                i_channels,
                i_sampleRate,
                i_sampleFmt,
                static_cast<int64_t>(i_ptsMs));
        return HJErrInvalidParams;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    return withInputId(env, i_inputId, [handle, i_dataPtr, i_size, i_samples, i_channels, i_sampleRate, i_sampleFmt, i_ptsMs](const std::string& inputId)
    {
        return pushPcmLocked(handle,
                             inputId,
                             reinterpret_cast<uint8_t*>(static_cast<intptr_t>(i_dataPtr)),
                             i_size,
                             i_samples,
                             i_channels,
                             i_sampleRate,
                             i_sampleFmt,
                             i_ptsMs);
    });
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativePushPcmBuffer)(JNIEnv* env,
                                                                 jobject thiz,
                                                                 jstring i_inputId,
                                                                 jobject i_buffer,
                                                                 jint i_size,
                                                                 jint i_samples,
                                                                 jint i_channels,
                                                                 jint i_sampleRate,
                                                                 jint i_sampleFmt,
                                                                 jlong i_ptsMs)
{
    HJAudioMixerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativePushPcmBuffer handle null");
        return HJErrNotInited;
    }
    if (!i_buffer)
    {
        HJFLoge("nativePushPcmBuffer buffer null");
        return HJErrInvalidParams;
    }

    void* data = env->GetDirectBufferAddress(i_buffer);
    const jlong capacity = env->GetDirectBufferCapacity(i_buffer);
    if (!data || capacity < 0 || capacity < i_size)
    {
        HJFLoge("nativePushPcmBuffer invalid direct buffer data:{} capacity:{} size:{}",
                data,
                static_cast<int64_t>(capacity),
                i_size);
        return HJErrInvalidParams;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    return withInputId(env, i_inputId, [handle, data, i_size, i_samples, i_channels, i_sampleRate, i_sampleFmt, i_ptsMs](const std::string& inputId)
    {
        return pushPcmLocked(handle,
                             inputId,
                             reinterpret_cast<uint8_t*>(data),
                             i_size,
                             i_samples,
                             i_channels,
                             i_sampleRate,
                             i_sampleFmt,
                             i_ptsMs);
    });
}
