#if defined(HJ_LOG_TAG)
#	undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJMusicPlayerJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJFLog.h"
#include "HJGraph.h"
#include "HJMediaInfo.h"
#include "HJComEvent.h"
#include "HJAudioListenerJni.h"
#include "HJBaseListenerJni.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <vector>

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_PLAYER_DECL(HJMusicPlayerJni, sig)

extern "C"
{
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInit)(JNIEnv* env, jobject thiz, jint i_repeats, jlong i_prerollDurationMs, jlong i_pcmCallbackIntervalMs, jint i_sampleRate, jint i_channels, jobject i_listener, jobject i_audioListener);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeAcquirePCM)(JNIEnv* env, jobject thiz, jlong i_data, jint i_nSize, jobject i_buffer);
JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeOpen)(JNIEnv* env, jobject thiz, jstring i_url, jlong i_startTimestamp);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativePause)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeResume)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSeek)(JNIEnv* env, jobject thiz, jlong i_timestamp);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetMute)(JNIEnv* env, jobject thiz, jboolean i_mute);
JNIEXPORT jboolean JNICALL HJ_JNIFUN_MEDIA_DECL(nativeIsMuted)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetVolume)(JNIEnv* env, jobject thiz, jfloat i_volume);
JNIEXPORT jfloat JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetVolume)(JNIEnv* env, jobject thiz);
JNIEXPORT jlong JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetCurrentTimestamp)(JNIEnv* env, jobject thiz);
JNIEXPORT jlong JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetDuration)(JNIEnv* env, jobject thiz);
JNIEXPORT jintArray JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetAudioTrackIds)(JNIEnv* env, jobject thiz);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSwitchAudioTrack)(JNIEnv* env, jobject thiz, jint i_trackId);
}

namespace
{
struct HJMusicPlayerJNI
{
    std::mutex mutex;
    std::mutex listenerMutex;
    HJGraphPlayer::Ptr player = nullptr;
    HJBaseListenerJni::Ptr listener = nullptr;
    HJAudioListenerJni::Ptr audioListener = nullptr;
};

HJMusicPlayerJNI* getHandle(JNIEnv* env, jobject thiz)
{
    return reinterpret_cast<HJMusicPlayerJNI*>(
        HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName));
}

HJMusicPlayerJNI* ensureHandle(JNIEnv* env, jobject thiz)
{
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (handle)
    {
        return handle;
    }

    handle = new (std::nothrow) HJMusicPlayerJNI();
    if (!handle)
    {
        return nullptr;
    }

    HJJNIField::SetLongField(env, thiz, reinterpret_cast<int64_t>(handle), HJJNIField::m_handleName);
    return handle;
}


static void notifyJava(const HJBaseListenerJni::Ptr& listener, int id, int64_t value, const std::string& desc)
{
    if (!listener)
    {
        return;
    }
    int notifyRet = listener->notify(id, value, desc);
    HJFLogi("HJMusicPlayerJni notifyJava id:{} value:{} desc:{} ret:{}", id, value, desc, notifyRet);
}

static void notifyAudioJava(const HJAudioListenerJni::Ptr& listener, const HJGraphRenderedPCM& pcm)
{
    if (!listener || !pcm.m_audioInfo || !pcm.m_pcmData || pcm.m_pcmData->empty())
    {
        const auto& audioInfo = pcm.m_audioInfo;
        const auto& pcmData = pcm.m_pcmData;
        HJFLogi("notifyAudioJava return listener:{} channels:{} bytesPerSample:{} sampleRate:{} PCM data size:{}",
                listener ? "valid" : "null",
                audioInfo ? audioInfo->m_channels : 0,
                audioInfo ? audioInfo->m_bytesPerSample : 0,
                audioInfo ? audioInfo->m_samplesRate : 0,
                pcmData ? pcmData->size() : 0);
        return;
    }

    const auto& audioInfo = pcm.m_audioInfo;
    const auto& pcmData = pcm.m_pcmData;
    const int64_t data = static_cast<int64_t>(reinterpret_cast<intptr_t>(pcmData->data()));
    const int size = static_cast<int>(pcmData->size());
    listener->notify(audioInfo->m_samplesRate, audioInfo->m_channels,
                     audioInfo->m_sampleFmt, audioInfo->m_bytesPerSample,
                     data, size, HJAudioListenerFlag_PCM);
    HJFLogi("notifyAudioJava notify channels:{} bytesPerSample:{} sampleRate:{} PCM data size:{}",
            audioInfo->m_channels,
            audioInfo->m_bytesPerSample,
            audioInfo->m_samplesRate,
            pcmData->size());
}

static void registerHandlers(const std::shared_ptr<HJEventBus> i_bus, HJMusicPlayerJNI* handle)
{
    if (!i_bus || !handle)
    {
        return;
    }

    i_bus->registerHandler(EVENT_GRAPH_STREAM_OPENED_ID, [handle]() {
        HJBaseListenerJni::Ptr listener;
        {
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            listener = handle->listener;
        }
        notifyJava(listener, static_cast<int>(EVENT_GRAPH_STREAM_OPENED::id), 0, "EVENT_GRAPH_STREAM_OPENED_ID");
    });

    i_bus->registerHandler(EVENT_GRAPH_EOF_ID, [handle]() {
        HJBaseListenerJni::Ptr listener;
        {
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            listener = handle->listener;
        }
        notifyJava(listener, static_cast<int>(EVENT_GRAPH_EOF::id), 0, "EVENT_GRAPH_EOF_ID");
    });

    i_bus->registerHandler(EVENT_GRAPH_RENDERED_PCM_ID, [handle](const HJGraphRenderedPCM& pcm)
    {
        HJAudioListenerJni::Ptr audioListener;
        {
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            audioListener = handle->audioListener;
        }
        notifyAudioJava(audioListener, pcm);
    });
}

int initMusicPlayerLocked(HJMusicPlayerJNI* handle, int repeats, int64_t prerollDurationMs, int64_t pcmCallbackIntervalMs,
                          int sampleRate, int channels,
                          jobject i_listener, jobject i_audioNotifyObj = nullptr)
{
    if (!handle)
    {
        return HJErrInvalidParams;
    }

    if (handle->player)
    {
        handle->player->done();
        handle->player.reset();
    }
    {
        std::lock_guard<std::mutex> lk(handle->listenerMutex);
        handle->listener = nullptr;
        handle->audioListener = nullptr;
    }

    if (i_listener)
    {
        HJBaseListenerJni::Ptr jListener = HJBaseListenerJni::Create();
        int i_err = jListener ? jListener->init(i_listener) : HJErrNotInited;
        if (i_err < 0)
        {
            HJFLoge("nativeInit jListener init failed i_err:{}", i_err);
            return i_err;
        }
        {
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            handle->listener = jListener;
        }
    }

    jobject audioNotifyObj = i_audioNotifyObj ? i_audioNotifyObj : i_listener;
    if (audioNotifyObj)
    {
        HJAudioListenerJni::Ptr jAudioListener = HJAudioListenerJni::Create();
        int i_err = jAudioListener ? jAudioListener->init(audioNotifyObj) : HJErrNotInited;
        if (i_err < 0)
        {
            HJFLoge("nativeInit audioListener init failed i_err:{}", i_err);
            return i_err;
        }
        {
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            handle->audioListener = jAudioListener;
        }
    }

    auto player = HJGraphPlayer::createGraph(HJGraphType_MUSIC, 0);
    if (!player)
    {
        HJFLoge("nativeInit create music graph failed");
        return HJErrNotInited;
    }
    registerHandlers(player->eventBus(), handle);

    auto param = std::make_shared<HJKeyStorage>();
    (*param)["repeats"] = repeats;
    (*param)["prerollDurationMs"] = prerollDurationMs;
    (*param)["pcmCallbackIntervalMs"] = pcmCallbackIntervalMs;

    auto audioInfo = std::make_shared<HJAudioInfo>();
    audioInfo->m_samplesRate = sampleRate;
    audioInfo->setChannels(channels);
    audioInfo->m_sampleFmt = 1;
    audioInfo->m_bytesPerSample = 2;
    (*param)["audioInfo"] = audioInfo;

    int ret = player->init(param);
    if (ret < 0)
    {
        HJFLoge("nativeInit player->init failed ret:{}", ret);
        player->done();
        return ret;
    }

    handle->player = player;
    return HJ_OK;
}
} // namespace

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInit)(JNIEnv* env, jobject thiz, jint i_repeats, jlong i_prerollDurationMs, jlong i_pcmCallbackIntervalMs, jint i_sampleRate, jint i_channels, jobject i_listener, jobject i_audioListener)
{
    HJFLogi("HJMusicPlayerJni nativeInit enter repeats:{} preroll:{} pcmCallbackInterval:{} sampleRate:{} channels:{} listener:{}",
            (int)i_repeats,
            (int64_t)i_prerollDurationMs,
            (int64_t)i_pcmCallbackIntervalMs,
            (int)i_sampleRate,
            (int)i_channels,
            i_listener ? 1 : 0);
    HJMusicPlayerJNI* handle = ensureHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeInit ensureHandle failed");
        return HJErrNotInited;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    const int repeats = i_repeats < 0 ? 0 : static_cast<int>(i_repeats);
    const int64_t prerollDurationMs = i_prerollDurationMs < 0 ? 0 : static_cast<int64_t>(i_prerollDurationMs);
    const int64_t pcmCallbackIntervalMs = i_pcmCallbackIntervalMs <= 0 ? 1 : static_cast<int64_t>(i_pcmCallbackIntervalMs);
    const int sampleRate = i_sampleRate > 0 ? static_cast<int>(i_sampleRate) : 48000;
    const int channels = i_channels >= 2 ? 2 : 1;
    const int ret = initMusicPlayerLocked(handle,
                                          repeats,
                                          prerollDurationMs,
                                          pcmCallbackIntervalMs,
                                          sampleRate,
                                          channels,
                                          i_listener,
                                          i_audioListener);
    HJFLogi("HJMusicPlayerJni nativeInit done ret:{} repeats:{} preroll:{} pcmCallbackInterval:{} sampleRate:{} channels:{}",
            ret,
            repeats,
            prerollDurationMs,
            pcmCallbackIntervalMs,
            sampleRate,
            channels);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeAcquirePCM)(JNIEnv* env, jobject thiz, jlong i_data, jint i_nSize, jobject i_buffer)
{
    (void)thiz;
    if (!env || !i_buffer || i_data == 0 || i_nSize <= 0)
    {
        return HJErrInvalidParams;
    }

    void* dst = env->GetDirectBufferAddress(i_buffer);
    if (!dst)
    {
        return HJErrInvalidParams;
    }

    const jlong capacity = env->GetDirectBufferCapacity(i_buffer);
    if (capacity < 0 || capacity < i_nSize)
    {
        return HJErrInvalidParams;
    }

    const void* src = reinterpret_cast<const void*>(static_cast<intptr_t>(i_data));
    if (!src)
    {
        return HJErrInvalidParams;
    }

    std::copy_n(reinterpret_cast<const uint8_t*>(src),
                static_cast<size_t>(i_nSize),
                reinterpret_cast<uint8_t*>(dst));
    return HJ_OK;
}

JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeDone enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLogi("HJMusicPlayerJni nativeDone skip, handle null");
        return;
    }

    {
        std::lock_guard<std::mutex> guard(handle->mutex);
        if (handle->player)
        {
            handle->player->done();
            handle->player.reset();
        }
        {
            std::lock_guard<std::mutex> lk(handle->listenerMutex);
            handle->listener = nullptr;
            handle->audioListener = nullptr;
        }
    }

    delete handle;
    HJJNIField::SetLongField(env, thiz, 0, HJJNIField::m_handleName);
    HJFLogi("HJMusicPlayerJni nativeDone done");
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeOpen)(JNIEnv* env, jobject thiz, jstring i_url, jlong i_startTimestamp)
{
    const char* url = HJJNIField::JStringToString(env, i_url);
    HJFLogi("HJMusicPlayerJni nativeOpen enter url:{} startTimestamp:{}", url ? url : "", static_cast<int64_t>(i_startTimestamp));
    int ret = HJ_OK;
    do
    {
        if (!url || !url[0])
        {
            ret = HJErrInvalidParams;
            HJFLoge("HJMusicPlayerJni nativeOpen invalid url");
            break;
        }

        HJMusicPlayerJNI* handle = getHandle(env, thiz);
        if (!handle)
        {
            ret = HJErrNotInited;
            HJFLoge("HJMusicPlayerJni nativeOpen handle null");
            break;
        }

        std::lock_guard<std::mutex> guard(handle->mutex);
        if (!handle->player)
        {
            ret = HJErrNotInited;
            HJFLoge("HJMusicPlayerJni nativeOpen player null");
            break;
        }

        HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(std::string(url));
        (*mediaUrl)["startTimestamp"] = static_cast<int64_t>(i_startTimestamp);
        ret = handle->player->openURL(mediaUrl);
    } while (false);

    HJJNIField::JStringDestroy(env, url, i_url);
    HJFLogi("HJMusicPlayerJni nativeOpen done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativePause)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativePause enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativePause handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->pause() : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativePause done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeResume)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeResume enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeResume handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->resume() : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativeResume done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSeek)(JNIEnv* env, jobject thiz, jlong i_timestamp)
{
    HJFLogi("HJMusicPlayerJni nativeSeek enter ts:{}", (int64_t)i_timestamp);
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeSeek handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->seek(static_cast<int64_t>(i_timestamp)) : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativeSeek done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetMute)(JNIEnv* env, jobject thiz, jboolean i_mute)
{
    HJFLogi("HJMusicPlayerJni nativeSetMute enter mute:{}", i_mute == JNI_TRUE);
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeSetMute handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->setMute(i_mute == JNI_TRUE) : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativeSetMute done ret:{}", ret);
    return ret;
}

JNIEXPORT jboolean JNICALL HJ_JNIFUN_MEDIA_DECL(nativeIsMuted)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeIsMuted enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeIsMuted handle null");
        return JNI_FALSE;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const jboolean ret = (handle->player && handle->player->isMuted()) ? JNI_TRUE : JNI_FALSE;
    HJFLogi("HJMusicPlayerJni nativeIsMuted done muted:{}", ret == JNI_TRUE);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetVolume)(JNIEnv* env, jobject thiz, jfloat i_volume)
{
    HJFLogi("HJMusicPlayerJni nativeSetVolume enter volume:{}", static_cast<float>(i_volume));
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeSetVolume handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->setVolume(static_cast<float>(i_volume)) : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativeSetVolume done ret:{}", ret);
    return ret;
}

JNIEXPORT jfloat JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetVolume)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeGetVolume enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeGetVolume handle null");
        return 1.0f;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const jfloat ret = handle->player ? static_cast<jfloat>(handle->player->getVolume()) : 1.0f;
    HJFLogi("HJMusicPlayerJni nativeGetVolume done volume:{}", static_cast<float>(ret));
    return ret;
}

JNIEXPORT jlong JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetCurrentTimestamp)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeGetCurrentTimestamp enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeGetCurrentTimestamp handle null");
        return 0;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    if (!handle->player)
    {
        HJFLoge("HJMusicPlayerJni nativeGetCurrentTimestamp player null");
        return 0;
    }
    const jlong ts = static_cast<jlong>(handle->player->getCurrentTimestamp());
    HJFLogi("HJMusicPlayerJni nativeGetCurrentTimestamp done ts:{}", (int64_t)ts);
    return ts;
}

JNIEXPORT jlong JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetDuration)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeGetDuration enter");
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeGetDuration handle null");
        return 0;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    if (!handle->player)
    {
        HJFLoge("HJMusicPlayerJni nativeGetDuration player null");
        return 0;
    }
    const jlong duration = static_cast<jlong>(handle->player->getDuration());
    HJFLogi("HJMusicPlayerJni nativeGetDuration done duration:{}", (int64_t)duration);
    return duration;
}

JNIEXPORT jintArray JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetAudioTrackIds)(JNIEnv* env, jobject thiz)
{
    HJFLogi("HJMusicPlayerJni nativeGetAudioTrackIds enter");
    jintArray out = env->NewIntArray(0);
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeGetAudioTrackIds handle null");
        return out;
    }

    std::lock_guard<std::mutex> guard(handle->mutex);
    if (!handle->player)
    {
        HJFLoge("HJMusicPlayerJni nativeGetAudioTrackIds player null");
        return out;
    }

    const std::vector<int> ids = handle->player->getAudioTrackIds();
    out = env->NewIntArray(static_cast<jsize>(ids.size()));
    if (!out || ids.empty())
    {
        HJFLogi("HJMusicPlayerJni nativeGetAudioTrackIds empty");
        return out;
    }

    env->SetIntArrayRegion(out, 0, static_cast<jsize>(ids.size()), reinterpret_cast<const jint*>(ids.data()));
    HJFLogi("HJMusicPlayerJni nativeGetAudioTrackIds done count:{}", (int)ids.size());
    return out;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSwitchAudioTrack)(JNIEnv* env, jobject thiz, jint i_trackId)
{
    HJFLogi("HJMusicPlayerJni nativeSwitchAudioTrack enter trackId:{}", (int)i_trackId);
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeSwitchAudioTrack handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->switchAudioTrack(static_cast<int>(i_trackId)) : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativeSwitchAudioTrack done ret:{}", ret);
    return ret;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetRepeats)(JNIEnv* env, jobject thiz, jint i_repeats)
{
    HJFLogi("HJMusicPlayerJni nativeSetRepeats enter repeats:{}", (int)i_repeats);
    HJMusicPlayerJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("HJMusicPlayerJni nativeSetRepeats handle null");
        return HJErrNotInited;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    const int ret = handle->player ? handle->player->setRepeats(static_cast<int>(i_repeats)) : HJErrNotInited;
    HJFLogi("HJMusicPlayerJni nativeSetRepeats done ret:{}", ret);
    return ret;
}
