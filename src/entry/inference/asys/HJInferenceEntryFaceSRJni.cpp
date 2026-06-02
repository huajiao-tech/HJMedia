#if defined(HJ_LOG_TAG)
#undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJInferenceEntryFaceSRJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJFLog.h"
#include "utils/HJFaceSRWrapper.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"
#include "HJSRUtils.h"
#include "HJReflCpp.h"
#include "HJReflCppJNIField.h"
#include "HJTime.h"
#include "libyuv.h"

#include <android/bitmap.h>

#include <string>
#include <memory>
#include <mutex>
#include <new>
#include <cstring>

REFL_AUTO_SIMPLE(HJFaceDetectWrapperOption,
                 tnnFaceAlignThreshold, tnnFaceAlignMinFaceSize,
                 ncnnRetinaFaceProbThreshold, ncnnRetinaFaceNmsThreshold, ncnnRetinaFaceThreadNums, retinaFaceTargetSize, ncnnRetinaFaceUseGPU,
                 ncnnScrfdEqualScale, ncnnScrfdTargetSize, ncnnScrfdProbThreshold, ncnnScrfdNmsThreshold, ncnnScrfdThreadNums, ncnnScrfdUseGPU,
                 coreMLRetinaFaceComputeMode,
                 visionRectTargetSize, visionRectComputeMode)

REFL_AUTO_SIMPLE(HJVideoSRWrapperOption,
                 ncnnRealESRGANType, ncnnRealESRGANDenose, ncnnRealCUGANType, ncnnUseGPU, ncnnThreadNums, ncnnScale,
                 textureFsrDenoiseStrength, textureFsrSharpness, textureFsrEnableExtraSharpen, textureFsrExtraSharpenBoost, textureFsrScale)

REFL_AUTO_SIMPLE(HJFaceSRProcessOption,
                 mode, preScaleWidth, preScaleHeight, composeCanvasWidth, composeCanvasHeight,
                 bFeather, bEnablePostSRDisplayResize, bMixedEnable, mixAlphaRatio, policy)

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
// Java side method lives in com.HJMediasdk.entry.inference.HJInferenceEntryJni
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_INFERENCE_DECL(HJInferenceEntryFaceSRJni, sig)

extern "C"
{
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInitFaceSR)(
        JNIEnv* env,
        jobject thiz,
        jstring i_modelPath,
        jint i_faceDetectType,
        jobject i_faceDetectOption,
        jint i_srType,
        jobject i_srOption);

JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(
        JNIEnv* env,
        jobject thiz);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcess)(
        JNIEnv* env,
        jobject thiz,
        jobject i_bitmap,
        jobject i_processOption);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcessTexture)(
        JNIEnv* env,
        jobject thiz,
        jint i_textureId,
        jint i_inputWidth,
        jint i_inputHeight);

JNIEXPORT jobject JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRBitmap)(
        JNIEnv* env,
        jobject thiz);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRTextureId)(
        JNIEnv* env,
        jobject thiz);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRTextureWidth)(
        JNIEnv* env,
        jobject thiz);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRTextureHeight)(
        JNIEnv* env,
        jobject thiz);
}

namespace
{
    struct HJInferenceJNI
    {
        std::mutex mutex;
        std::unique_ptr<HJFaceSRWrapper> faceSRWrapper;
        uint32_t lastSRTextureId = 0;
        int lastSRTextureWidth = 0;
        int lastSRTextureHeight = 0;
    };

    HJInferenceJNI* getHandle(JNIEnv* env, jobject thiz)
    {
        return reinterpret_cast<HJInferenceJNI*>(
                HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName));
    }

    HJInferenceJNI* ensureHandle(JNIEnv* env, jobject thiz)
    {
        HJInferenceJNI* handle = getHandle(env, thiz);
        if (handle)
        {
            return handle;
        }

        handle = new (std::nothrow) HJInferenceJNI();
        if (!handle)
        {
            return nullptr;
        }

        HJJNIField::SetLongField(env, thiz, reinterpret_cast<int64_t>(handle), HJJNIField::m_handleName);
        return handle;
    }

    int initFaceSRInternal(
            HJInferenceJNI* handle,
            const std::string& modelRoot,
            int faceDetectType,
            const HJFaceDetectWrapperOption& faceDetectOption,
            int srType,
            const HJVideoSRWrapperOption& srOption)
    {
        if (!handle)
        {
            return HJErrInvalidParams;
        }

        std::lock_guard<std::mutex> guard(handle->mutex);

        handle->lastSRTextureId = 0;
        handle->lastSRTextureWidth = 0;
        handle->lastSRTextureHeight = 0;
        handle->faceSRWrapper = std::make_unique<HJFaceSRWrapper>();

        if (!handle->faceSRWrapper)
        {
            handle->faceSRWrapper.reset();
            return HJErrNotInited;
        }

        int i_err = handle->faceSRWrapper->init(
                modelRoot,
                static_cast<HJFaceDetectWrapperType>(faceDetectType),
                static_cast<HJVideoSRWrapperType>(srType),
                faceDetectOption,
                srOption);
        if (i_err < 0)
        {
            handle->faceSRWrapper.reset();
            return i_err;
        }

        return HJ_OK;
    }

    int bitmapToRGBFrame(JNIEnv* env, jobject i_bitmap, HJTransferMediaData::Ptr& o_rgbFrame)
    {
        if (!i_bitmap)
        {
            return HJErrInvalidParams;
        }

        AndroidBitmapInfo info;
        std::memset(&info, 0, sizeof(info));
        if (AndroidBitmap_getInfo(env, i_bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS)
        {
            HJFLoge("bitmapToRGBFrame AndroidBitmap_getInfo failed");
            return HJErrInvalidParams;
        }
        if (info.width == 0 || info.height == 0 || info.stride == 0)
        {
            HJFLoge("bitmapToRGBFrame invalid bitmap info w:{} h:{} stride:{}", info.width, info.height, info.stride);
            return HJErrInvalidParams;
        }
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        {
            HJFLoge("bitmapToRGBFrame unsupported bitmap format:{}, expect RGBA_8888", static_cast<int>(info.format));
            return HJErrInvalidParams;
        }

        void* pixels = nullptr;
        if (AndroidBitmap_lockPixels(env, i_bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS || !pixels)
        {
            HJFLoge("bitmapToRGBFrame AndroidBitmap_lockPixels failed");
            return HJErrInvalidParams;
        }

        const int width = static_cast<int>(info.width);
        const int height = static_cast<int>(info.height);
        HJSPBuffer::Ptr rgbBuffer = HJSPBuffer::create((size_t)width * (size_t)height * 3);
        if (!rgbBuffer || !rgbBuffer->getBuf())
        {
            AndroidBitmap_unlockPixels(env, i_bitmap);
            HJFLoge("bitmapToRGBFrame alloc rgb buffer failed");
            return HJErrFatal;
        }

        int ret = libyuv::ABGRToRAW(
                static_cast<const uint8_t*>(pixels), static_cast<int>(info.stride),
                rgbBuffer->getBuf(), width * 3,
                width, height);
        AndroidBitmap_unlockPixels(env, i_bitmap);

        if (ret != 0)
        {
            HJFLoge("bitmapToRGBFrame ABGRToRAW failed ret:{}", ret);
            return HJErrFatal;
        }

        unsigned char* rgbData[4] = { rgbBuffer->getBuf(), nullptr, nullptr, nullptr };
        int rgbPitch[4] = { width * 3, 0, 0, 0 };
        o_rgbFrame = HJTransferMediaDataRGB::Create<HJTransferMediaDataRGB>(
                rgbData, rgbPitch, width, height, HJCurrentSteadyMS());
        if (!o_rgbFrame || !o_rgbFrame->getData(0))
        {
            HJFLoge("bitmapToRGBFrame create rgb frame failed");
            return HJErrFatal;
        }
        return HJ_OK;
    }

    void setJavaProcessMeta(
            JNIEnv* env,
            jobject thiz,
            int faceDetectTime,
            int srTime,
            int srOriginX, int srOriginY, int srOriginW, int srOriginH,
            int srOriginScaleW, int srOriginScaleH,
            int srTargetDisplayW, int srTargetDisplayH,
            int srPadLeft, int srPadRight, int srPadTop, int srPadBottom)
    {
        if (!env || !thiz)
        {
            return;
        }

        jclass cls = env->GetObjectClass(thiz);
        if (!cls)
        {
            return;
        }

        auto setIntField = [&](const char* name, int value)
        {
            jfieldID fid = env->GetFieldID(cls, name, "I");
            if (fid)
            {
                env->SetIntField(thiz, fid, static_cast<jint>(value));
            }
        };

        setIntField("m_faceDetectTime", faceDetectTime);
        setIntField("m_srTime", srTime);
        setIntField("m_srOriginX", srOriginX);
        setIntField("m_srOriginY", srOriginY);
        setIntField("m_srOriginW", srOriginW);
        setIntField("m_srOriginH", srOriginH);
        setIntField("m_srOriginScaleW", srOriginScaleW);
        setIntField("m_srOriginScaleH", srOriginScaleH);
        setIntField("m_srTargetDisplayW", srTargetDisplayW);
        setIntField("m_srTargetDisplayH", srTargetDisplayH);
        setIntField("m_srPadLeft", srPadLeft);
        setIntField("m_srPadRight", srPadRight);
        setIntField("m_srPadTop", srPadTop);
        setIntField("m_srPadBottom", srPadBottom);
    }

    jobject transferMediaToJavaBitmap(JNIEnv* env, const HJTransferMediaData::Ptr& i_media)
    {
        if (!env || !i_media || i_media->getWidth() <= 0 || i_media->getHeight() <= 0)
        {
            return nullptr;
        }

        HJTransferMediaData::Ptr rgba = i_media;
        if (rgba->getConvertType() != HJConvertDataType_RGBA)
        {
            rgba = HJTransferMediaData::create(rgba, HJConvertDataType_RGBA);
        }
        if (!rgba || !rgba->getData(0) || rgba->getStride(0) <= 0)
        {
            HJFLoge("transferMediaToJavaBitmap invalid rgba media");
            return nullptr;
        }

        jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
        jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
        if (!bitmapClass || !bitmapConfigClass)
        {
            HJFLoge("transferMediaToJavaBitmap find class failed");
            return nullptr;
        }

        jfieldID argb8888Field = env->GetStaticFieldID(
                bitmapConfigClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        jmethodID createBitmapMethod = env->GetStaticMethodID(
                bitmapClass, "createBitmap",
                "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
        if (!argb8888Field || !createBitmapMethod)
        {
            HJFLoge("transferMediaToJavaBitmap get method/field failed");
            return nullptr;
        }

        jobject configARGB8888 = env->GetStaticObjectField(bitmapConfigClass, argb8888Field);
        if (!configARGB8888)
        {
            HJFLoge("transferMediaToJavaBitmap get ARGB_8888 config failed");
            return nullptr;
        }

        jobject bitmapObj = env->CallStaticObjectMethod(
                bitmapClass,
                createBitmapMethod,
                static_cast<jint>(rgba->getWidth()),
                static_cast<jint>(rgba->getHeight()),
                configARGB8888);
        if (!bitmapObj)
        {
            HJFLoge("transferMediaToJavaBitmap createBitmap failed");
            return nullptr;
        }

        AndroidBitmapInfo info;
        std::memset(&info, 0, sizeof(info));
        if (AndroidBitmap_getInfo(env, bitmapObj, &info) != ANDROID_BITMAP_RESULT_SUCCESS)
        {
            HJFLoge("transferMediaToJavaBitmap AndroidBitmap_getInfo failed");
            return nullptr;
        }
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 || info.stride == 0)
        {
            HJFLoge("transferMediaToJavaBitmap unsupported dst bitmap format:{}", static_cast<int>(info.format));
            return nullptr;
        }

        void* dstPixels = nullptr;
        if (AndroidBitmap_lockPixels(env, bitmapObj, &dstPixels) != ANDROID_BITMAP_RESULT_SUCCESS || !dstPixels)
        {
            HJFLoge("transferMediaToJavaBitmap lock dst pixels failed");
            return nullptr;
        }

        const int w = rgba->getWidth();
        const int h = rgba->getHeight();
        //HJFLogi("SR transferMediaToJavaBitmap w:{} h:{}",w, h);
        const int srcStride = rgba->getStride(0);
        const int dstStride = static_cast<int>(info.stride);
        const int rowBytes = w * 4;
        const unsigned char* src = rgba->getData(0);
        unsigned char* dst = static_cast<unsigned char*>(dstPixels);
        libyuv::CopyPlane(src, srcStride, dst, dstStride, rowBytes, h);
        AndroidBitmap_unlockPixels(env, bitmapObj);

        return bitmapObj;
    }
} // namespace

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInitFaceSR)(
        JNIEnv* env,
        jobject thiz,
        jstring i_modelPath,
        jint i_faceDetectType,
        jobject i_faceDetectOption,
        jint i_srType,
        jobject i_srOption)
{
    int i_err = HJ_OK;

    const char* modelPath = HJJNIField::JStringToString(env, i_modelPath);
    do
    {
        if (!modelPath || !modelPath[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeInitFaceSR invalid modelPath");
            break;
        }

        HJInferenceJNI* handle = ensureHandle(env, thiz);
        if (!handle)
        {
            i_err = HJErrNotInited;
            HJFLoge("nativeInitFaceSR create handle failed");
            break;
        }

        HJFaceDetectWrapperOption faceDetectOption;
        HJVideoSRWrapperOption srOption;

        if (i_faceDetectOption)
        {
            i_err = HJReflCppJNIField::deserial(env, i_faceDetectOption, faceDetectOption);
            if (i_err < 0)
            {
                HJFLoge("nativeInitFaceSR deserial face option failed i_err:{}", i_err);
                break;
            }
        }

        if (i_srOption)
        {
            i_err = HJReflCppJNIField::deserial(env, i_srOption, srOption);
            if (i_err < 0)
            {
                HJFLoge("nativeInitFaceSR deserial sr option failed i_err:{}", i_err);
                break;
            }
        }

        const std::string modelRoot = modelPath;
        HJFLogi("nativeInitFaceSR modelPath:{} (root:{}), faceType:{}, srType:{}",
                modelPath, modelRoot, static_cast<int>(i_faceDetectType), static_cast<int>(i_srType));

        i_err = initFaceSRInternal(
                handle,
                modelRoot,
                static_cast<int>(i_faceDetectType),
                faceDetectOption,
                static_cast<int>(i_srType),
                srOption);
    } while (false);

    HJJNIField::JStringDestroy(env, modelPath, i_modelPath);
    return i_err;
}

JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeDone)(
        JNIEnv* env,
        jobject thiz)
{
    HJInferenceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> guard(handle->mutex);
        if (handle->faceSRWrapper)
        {
            handle->faceSRWrapper->done();
        }
        handle->faceSRWrapper.reset();
    }

    delete handle;
    HJJNIField::SetLongField(env, thiz, 0, HJJNIField::m_handleName);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcess)(
        JNIEnv* env,
        jobject thiz,
        jobject i_bitmap,
        jobject i_processOption)
{
    int i_err = HJ_OK;
    HJFaceSRProcessResult processRet;
    int faceDetectTime = 0;
    int srOriginX = 0;
    int srOriginY = 0;
    int srOriginW = 0;
    int srOriginH = 0;
    int srOriginScaleW = 0;
    int srOriginScaleH = 0;
    int srTargetDisplayW = 0;
    int srTargetDisplayH = 0;
    int srPadLeft = 0;
    int srPadRight = 0;
    int srPadTop = 0;
    int srPadBottom = 0;

    do
    {
        if (!i_bitmap)
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcess invalid bitmap");
            break;
        }

        HJInferenceJNI* handle = getHandle(env, thiz);
        if (!handle)
        {
            i_err = HJErrNotInited;
            HJFLoge("nativeProcess handle not initialized");
            break;
        }

        std::lock_guard<std::mutex> guard(handle->mutex);
        if (!handle->faceSRWrapper)
        {
            i_err = HJErrNotInited;
            HJFLoge("nativeProcess wrapper not initialized");
            break;
        }

        handle->lastSRTextureId = 0;
        handle->lastSRTextureWidth = 0;
        handle->lastSRTextureHeight = 0;

        HJTransferMediaData::Ptr rgbFrame = nullptr;
        i_err = bitmapToRGBFrame(env, i_bitmap, rgbFrame);
        if (i_err < 0)
        {
            break;
        }

        HJFaceSRProcessOption processOption;
        if (i_processOption)
        {
            i_err = HJReflCppJNIField::deserial(env, i_processOption, processOption);
            if (i_err < 0)
            {
                HJFLoge("nativeProcess deserial process option failed i_err:{}", i_err);
                break;
            }
        }
        HJFPERLog5i("nativeProcess option mode:{} preScale:{}x{} compose:{}x{} bFeather:{} postResize:{} bMixedEnable:{} mixAlphaRatio:{:.3f} policy:{}",
                (int)processOption.mode,
                processOption.preScaleWidth,
                processOption.preScaleHeight,
                processOption.composeCanvasWidth,
                processOption.composeCanvasHeight,
                processOption.bFeather,
                processOption.bEnablePostSRDisplayResize,
                processOption.bMixedEnable,
                processOption.mixAlphaRatio,
                (int)processOption.policy);
        i_err = handle->faceSRWrapper->process(rgbFrame, processOption, processRet);
        if (i_err == HJ_OK) {
            faceDetectTime = static_cast<int>(processRet.faceDetectElapsedMs);
            srPadLeft = processRet.padLeft;
            srPadRight = processRet.padRight;
            srPadTop = processRet.padTop;
            srPadBottom = processRet.padBottom;

            if (processOption.mode == HJFaceSRProcessMode_Full || processOption.mode == HJFaceSRProcessMode_FullScale)
            {
                srOriginX = 0;
                srOriginY = 0;
                srOriginW = rgbFrame->getWidth();
                srOriginH = rgbFrame->getHeight();
                srOriginScaleW = (processOption.mode == HJFaceSRProcessMode_FullScale) ? processRet.scaleW : srOriginW;
                srOriginScaleH = (processOption.mode == HJFaceSRProcessMode_FullScale) ? processRet.scaleH : srOriginH;
            }
            else
            {
                srOriginX = processRet.faceRect.x;
                srOriginY = processRet.faceRect.y;
                srOriginW = processRet.faceRect.w;
                srOriginH = processRet.faceRect.h;
                if (processOption.mode == HJFaceSRProcessMode_FaceScale)
                {
                    srOriginScaleW = processRet.scaleW;
                    srOriginScaleH = processRet.scaleH;
                }
                else
                {
                    srOriginScaleW = srOriginW;
                    srOriginScaleH = srOriginH;
                }
                srTargetDisplayW = processRet.faceTargetDisplayWidth;
                srTargetDisplayH = processRet.faceTargetDisplayHeight;
            }
        }
        else if (i_err < 0)
        {
            HJFLoge("nativeProcess error:{}", i_err);
            break;
        }
        else if (i_err == HJ_WOULD_BLOCK)
        {
            HJFLogi("nativeProcess no face detected");
            break;
        }
        //HJFLogi("nativeProcess done faces:{}, ret:{}", processRet.faceCount, HJ_OK);
    } while (false);

    if (i_err == HJ_OK)
    {
        setJavaProcessMeta(env, thiz, faceDetectTime, static_cast<int>(processRet.srElapsedMs), srOriginX, srOriginY, srOriginW, srOriginH, srOriginScaleW, srOriginScaleH,
            srTargetDisplayW, srTargetDisplayH,
            srPadLeft, srPadRight, srPadTop, srPadBottom);
    }
    else if (i_err == HJ_WOULD_BLOCK)
    {
        setJavaProcessMeta(env, thiz, faceDetectTime, 0, srOriginX, srOriginY, srOriginW, srOriginH, srOriginScaleW, srOriginScaleH,
            srTargetDisplayW, srTargetDisplayH,
            srPadLeft, srPadRight, srPadTop, srPadBottom);
    }
    else
    {
        setJavaProcessMeta(env, thiz, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    if (i_err < 0)
    {
        return i_err;
    }
    return static_cast<jint>(i_err);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcessTexture)(
        JNIEnv* env,
        jobject thiz,
        jint i_textureId,
        jint i_inputWidth,
        jint i_inputHeight)
{
    int i_err = HJ_OK;
    HJFaceSRProcessResult processRet;
    int faceDetectTime = 0;
    int srOriginX = 0;
    int srOriginY = 0;
    int srOriginW = 0;
    int srOriginH = 0;
    int srOriginScaleW = 0;
    int srOriginScaleH = 0;
    int srTargetDisplayW = 0;
    int srTargetDisplayH = 0;
    int srPadLeft = 0;
    int srPadRight = 0;
    int srPadTop = 0;
    int srPadBottom = 0;

    do
    {
        if (i_textureId <= 0 || i_inputWidth <= 0 || i_inputHeight <= 0)
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcessTexture invalid input tex:{} w:{} h:{}", (int)i_textureId, (int)i_inputWidth, (int)i_inputHeight);
            break;
        }

        HJInferenceJNI* handle = getHandle(env, thiz);
        if (!handle)
        {
            i_err = HJErrNotInited;
            HJFLoge("nativeProcessTexture handle not initialized");
            break;
        }

        std::lock_guard<std::mutex> guard(handle->mutex);
        if (!handle->faceSRWrapper)
        {
            i_err = HJErrNotInited;
            HJFLoge("nativeProcessTexture wrapper not initialized");
            break;
        }

        uint32_t outputTextureId = 0;
        int outputWidth = 0;
        int outputHeight = 0;
        i_err = handle->faceSRWrapper->process(
                static_cast<uint32_t>(i_textureId),
                static_cast<int>(i_inputWidth),
                static_cast<int>(i_inputHeight),
                outputTextureId,
                outputWidth,
                outputHeight,
                processRet);
        if (i_err < 0)
        {
            HJFLoge("nativeProcessTexture error:{}", i_err);
            break;
        }

        handle->lastSRTextureId = outputTextureId;
        handle->lastSRTextureWidth = outputWidth;
        handle->lastSRTextureHeight = outputHeight;

        faceDetectTime = static_cast<int>(processRet.faceDetectElapsedMs);
        srOriginX = 0;
        srOriginY = 0;
        srOriginW = static_cast<int>(i_inputWidth);
        srOriginH = static_cast<int>(i_inputHeight);
        srOriginScaleW = processRet.scaleW;
        srOriginScaleH = processRet.scaleH;
        srPadLeft = processRet.padLeft;
        srPadRight = processRet.padRight;
        srPadTop = processRet.padTop;
        srPadBottom = processRet.padBottom;
    } while (false);

    if (i_err == HJ_OK)
    {
        setJavaProcessMeta(env, thiz, faceDetectTime, static_cast<int>(processRet.srElapsedMs),
                srOriginX, srOriginY, srOriginW, srOriginH, srOriginScaleW, srOriginScaleH,
                srTargetDisplayW, srTargetDisplayH,
                srPadLeft, srPadRight, srPadTop, srPadBottom);
    }
    else
    {
        HJInferenceJNI* handle = getHandle(env, thiz);
        if (handle)
        {
            std::lock_guard<std::mutex> guard(handle->mutex);
            handle->lastSRTextureId = 0;
            handle->lastSRTextureWidth = 0;
            handle->lastSRTextureHeight = 0;
        }
        setJavaProcessMeta(env, thiz, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    return static_cast<jint>(i_err);
}

JNIEXPORT jobject JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRBitmap)(
        JNIEnv* env,
        jobject thiz)
{
    HJInferenceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        HJFLoge("nativeGetLastSRBitmap handle not initialized");
        return nullptr;
    }

    HJTransferMediaData::Ptr srData = nullptr;
    {
        std::lock_guard<std::mutex> guard(handle->mutex);
        if (handle->faceSRWrapper)
        {
            srData = handle->faceSRWrapper->takeLastOutput();
        }
    }
    if (!srData)
    {
        return nullptr;
    }

    jobject bitmapObj = transferMediaToJavaBitmap(env, srData);
    if (!bitmapObj)
    {
        HJFLoge("nativeGetLastSRBitmap transferMediaToJavaBitmap failed");
        return nullptr;
    }
    return bitmapObj;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRTextureId)(
        JNIEnv* env,
        jobject thiz)
{
    HJInferenceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        return 0;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    return static_cast<jint>(handle->lastSRTextureId);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRTextureWidth)(
        JNIEnv* env,
        jobject thiz)
{
    HJInferenceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        return 0;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    return static_cast<jint>(handle->lastSRTextureWidth);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeGetLastSRTextureHeight)(
        JNIEnv* env,
        jobject thiz)
{
    HJInferenceJNI* handle = getHandle(env, thiz);
    if (!handle)
    {
        return 0;
    }
    std::lock_guard<std::mutex> guard(handle->mutex);
    return static_cast<jint>(handle->lastSRTextureHeight);
}
