#if defined(HJ_LOG_TAG)
#undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJVideoEnhanceJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJFLog.h"
#include "HJReflCpp.h"
#include "HJReflCppJNIField.h"
#include "utils/HJVideoEnhance.h"

#include <cstdint>
#include <memory>
#include <new>

REFL_AUTO_SIMPLE(HJVideoEnhanceOption,
                 denoiseStrength,
                 sharpness,
                 enableExtraSharpen,
                 extraSharpenBoost,
                 match,
                 enableDenoise,
                 enableSR)

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_PLAYER_DECL(HJVideoEnhanceJni, sig)

extern "C"
{
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInit)(
        JNIEnv* env,
        jobject thiz,
        jobject i_option);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetOption)(
        JNIEnv* env,
        jobject thiz,
        jobject i_option);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcess)(
        JNIEnv* env,
        jobject thiz,
        jint i_textureId,
        jint i_inputWidth,
        jint i_inputHeight,
        jint i_outputWidth,
        jint i_outputHeight);

JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(
        JNIEnv* env,
        jobject thiz);
}

namespace
{
    struct HJVideoEnhanceJNI
    {
        std::unique_ptr<HJVideoEnhance> videoEnhance;
        HJVideoEnhanceOption option{};
        bool hasOption = false;
    };

    HJVideoEnhanceJNI* getHandle(JNIEnv* env, jobject thiz)
    {
        return reinterpret_cast<HJVideoEnhanceJNI*>(
                HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName));
    }

    HJVideoEnhanceJNI* ensureHandle(JNIEnv* env, jobject thiz)
    {
        HJVideoEnhanceJNI* handle = getHandle(env, thiz);
        if (handle)
        {
            return handle;
        }

        handle = new (std::nothrow) HJVideoEnhanceJNI();
        if (!handle)
        {
            HJFLoge("ensureHandle alloc failed");
            return nullptr;
        }

        HJJNIField::SetLongField(env, thiz, reinterpret_cast<int64_t>(handle), HJJNIField::m_handleName);
        HJFLogi("ensureHandle created handle:{}", static_cast<const void*>(handle));
        return handle;
    }

    void setJavaProcessResult(JNIEnv* env, jobject thiz, int outputTextureId, int64_t elapsedMs)
    {
        if (!env || !thiz)
        {
            return;
        }

        HJJNIField::SetIntField(env, thiz, static_cast<int32_t>(outputTextureId), "m_lastOutputTextureId");
        HJJNIField::SetLongField(env, thiz, static_cast<int64_t>(elapsedMs), "m_lastElapsedMs");
    }

    int deserialOption(JNIEnv* env, jobject i_option, HJVideoEnhanceOption& o_option)
    {
        o_option = HJVideoEnhanceOption();
        if (!i_option)
        {
            HJFLogi("deserialOption use default option");
            return HJ_OK;
        }

        const int i_err = HJReflCppJNIField::deserial(env, i_option, o_option);
        if (i_err < 0)
        {
            HJFLoge("deserialOption failed ret:{}", i_err);
            return i_err;
        }

        HJFLogi("deserialOption ok enableDenoise:{} enableSR:{} denoiseStrength:{} sharpness:{} enableExtraSharpen:{} extraSharpenBoost:{} match:{}",
                o_option.enableDenoise,
                o_option.enableSR,
                o_option.denoiseStrength,
                o_option.sharpness,
                o_option.enableExtraSharpen,
                o_option.extraSharpenBoost,
                o_option.match);
        return HJ_OK;
    }

    int initVideoEnhanceLocked(HJVideoEnhanceJNI* handle, const HJVideoEnhanceOption& option)
    {
        if (!handle)
        {
            HJFLoge("initVideoEnhanceLocked handle null");
            return HJErrInvalidParams;
        }
        if (!handle->videoEnhance)
        {
            handle->videoEnhance = std::make_unique<HJVideoEnhance>();
            if (!handle->videoEnhance)
            {
                HJFLoge("initVideoEnhanceLocked alloc videoEnhance failed");
                return HJErrNewObj;
            }
        }

        const int i_err = handle->videoEnhance->init(option);
        if (i_err < 0)
        {
            HJFLoge("initVideoEnhanceLocked init failed ret:{}", i_err);
            return i_err;
        }

        handle->option = option;
        handle->hasOption = true;
        HJFLogi("initVideoEnhanceLocked done handle:{} videoEnhance:{}",
                static_cast<const void*>(handle),
                static_cast<const void*>(handle->videoEnhance.get()));
        return HJ_OK;
    }

    int adjustVideoEnhanceLocked(HJVideoEnhanceJNI* handle, const HJVideoEnhanceOption& option)
    {
        if (!handle)
        {
            HJFLoge("adjustVideoEnhanceLocked handle null");
            return HJErrInvalidParams;
        }

        if (!handle->videoEnhance)
        {
            handle->videoEnhance = std::make_unique<HJVideoEnhance>();
            if (!handle->videoEnhance)
            {
                HJFLoge("adjustVideoEnhanceLocked alloc videoEnhance failed");
                return HJErrNewObj;
            }
        }

        const int i_err = handle->videoEnhance->adjustParam(option);
        if (i_err < 0)
        {
            HJFLoge("adjustVideoEnhanceLocked adjustParam failed ret:{}", i_err);
            return i_err;
        }

        handle->option = option;
        handle->hasOption = true;
        HJFLogi("adjustVideoEnhanceLocked done handle:{} videoEnhance:{}",
                static_cast<const void*>(handle),
                static_cast<const void*>(handle->videoEnhance.get()));
        return HJ_OK;
    }
} // namespace

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInit)(
        JNIEnv* env,
        jobject thiz,
        jobject i_option)
{
    HJFLogi("nativeInit enter option:{}", static_cast<const void*>(i_option));
    HJVideoEnhanceJNI* handle = ensureHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeInit ensureHandle failed");
        return HJErrNotInited;
    }

    HJVideoEnhanceOption option{};
    int i_err = deserialOption(env, i_option, option);
    if (i_err < 0)
    {
        return i_err;
    }

    i_err = initVideoEnhanceLocked(handle, option);
    HJFLogi("nativeInit done ret:{} handle:{}", i_err, static_cast<const void*>(handle));
    return i_err;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeSetOption)(
        JNIEnv* env,
        jobject thiz,
        jobject i_option)
{
    HJFLogi("nativeSetOption enter option:{}", static_cast<const void*>(i_option));
    HJVideoEnhanceJNI* handle = ensureHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeSetOption ensureHandle failed");
        return HJErrNotInited;
    }

    HJVideoEnhanceOption option{};
    int i_err = deserialOption(env, i_option, option);
    if (i_err < 0)
    {
        return i_err;
    }

    i_err = adjustVideoEnhanceLocked(handle, option);
    HJFLogi("nativeSetOption done ret:{} handle:{}", i_err, static_cast<const void*>(handle));
    return i_err;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcess)(
        JNIEnv* env,
        jobject thiz,
        jint i_textureId,
        jint i_inputWidth,
        jint i_inputHeight,
        jint i_outputWidth,
        jint i_outputHeight)
{
    HJFLogi("nativeProcess enter tex:{} input:{}x{} output:{}x{}",
            static_cast<int>(i_textureId),
            static_cast<int>(i_inputWidth),
            static_cast<int>(i_inputHeight),
            static_cast<int>(i_outputWidth),
            static_cast<int>(i_outputHeight));
    if (i_textureId <= 0 || i_inputWidth <= 0 || i_inputHeight <= 0 || i_outputWidth <= 0 || i_outputHeight <= 0)
    {
        HJFLoge("nativeProcess invalid params tex:{} input:{}x{} output:{}x{}",
                static_cast<int>(i_textureId),
                static_cast<int>(i_inputWidth),
                static_cast<int>(i_inputHeight),
                static_cast<int>(i_outputWidth),
                static_cast<int>(i_outputHeight));
        return HJErrInvalidParams;
    }

    HJVideoEnhanceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeProcess handle null");
        return HJErrNotInited;
    }

    if (!handle->videoEnhance || !handle->hasOption)
    {
        HJFLoge("nativeProcess videoEnhance not initialized handle:{} videoEnhance:{} hasOption:{}",
                static_cast<const void*>(handle),
                handle->videoEnhance ? static_cast<const void*>(handle->videoEnhance.get()) : nullptr,
                handle->hasOption);
        return HJErrNotInited;
    }

    uint32_t outputTextureId = 0;
    int64_t elapsedMs = 0;
    const int i_err = handle->videoEnhance->process(
            static_cast<uint32_t>(i_textureId),
            static_cast<int>(i_inputWidth),
            static_cast<int>(i_inputHeight),
            static_cast<int>(i_outputWidth),
            static_cast<int>(i_outputHeight),
            outputTextureId,
            elapsedMs);
    if (i_err < 0)
    {
        HJFLoge("nativeProcess process failed ret:{}", i_err);
        return i_err;
    }

    setJavaProcessResult(env, thiz, static_cast<int>(outputTextureId), elapsedMs);
    HJFLogi("nativeProcess done ret:{} outTex:{} elapsedMs:{}",
            i_err,
            outputTextureId,
            elapsedMs);
    return i_err;
}

JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(
        JNIEnv* env,
        jobject thiz)
{
    HJFLogi("nativeDone enter");
    HJVideoEnhanceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLogi("nativeDone skip, handle null");
        return;
    }

    if (handle->videoEnhance)
    {
        handle->videoEnhance->done();
        handle->videoEnhance.reset();
    }
    handle->hasOption = false;

    delete handle;
    HJJNIField::SetLongField(env, thiz, 0, HJJNIField::m_handleName);
    HJFLogi("nativeDone done");
}

