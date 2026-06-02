#if defined(HJ_LOG_TAG)
#undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJInferenceEntryJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJFLog.h"
#include "HJVideoSRExport.h"
#include "HJTransferMediaData.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "HJSRUtils.h"
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <mutex>
#include "HJTime.h"
NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_INFERENCE_DECL(HJInferenceEntryJni, sig)

extern "C"
{
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInitSR)(
    JNIEnv* env,
    jobject thiz,
    jstring i_modelPath,
    jint i_srType,
    jstring i_modeType,
    jboolean i_bCpu,
    jint i_threadNum,
    jint i_nScale);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcessSR)(
    JNIEnv* env,
    jobject thiz,
    jstring i_url,
    jstring o_url);

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcess)(
    JNIEnv* env,
    jobject thiz,
    jstring i_modelPath,
    jint i_srType,
    jstring i_modeType,
    jboolean i_bCpu,
    jint i_threadNum,
    jint i_nScale,
    jstring i_url,
    jstring o_url);
}

namespace
{
std::mutex g_srMutex;
std::unique_ptr<HJVideoSRWrapper> g_srWrapper;
HJTransferMediaData::Ptr g_srOutput = nullptr;
int64_t g_srElapseMs = -1;

std::string normalizeModelRoot(const char* modelPath)
{
    if (!modelPath)
    {
        return "";
    }
    std::string root(modelPath);
    std::string slashPath = root;
    for (char& c : slashPath)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }

    const std::string suffix = "/NCNN/realesr";
    if (slashPath.size() >= suffix.size())
    {
        const size_t pos = slashPath.size() - suffix.size();
        if (slashPath.compare(pos, suffix.size(), suffix) == 0)
        {
            slashPath.erase(pos);
            if (!slashPath.empty())
            {
                return slashPath;
            }
        }
    }
    return root;
}

int saveSrJpg(const HJTransferMediaData::Ptr& media, const std::string& outPath)
{
    if (!media || media->getWidth() <= 0 || media->getHeight() <= 0 || !media->getData(0))
    {
        return HJErrInvalidParams;
    }

    const int w = media->getWidth();
    const int h = media->getHeight();
    const int stride = media->getStride(0);
    if (stride <= 0)
    {
        return HJErrInvalidParams;
    }

    std::vector<unsigned char> rgb;
    const unsigned char* src = media->getData(0);
    int comp = 3;

    switch (media->getConvertType())
    {
    case HJConvertDataType_RGBA:
    {
        rgb.resize((size_t)w * h * 3);
        for (int y = 0; y < h; y++)
        {
            const unsigned char* row = src + (size_t)y * stride;
            unsigned char* dst = rgb.data() + (size_t)y * w * 3;
            for (int x = 0; x < w; x++)
            {
                dst[x * 3 + 0] = row[x * 4 + 0];
                dst[x * 3 + 1] = row[x * 4 + 1];
                dst[x * 3 + 2] = row[x * 4 + 2];
            }
        }
        src = rgb.data();
        break;
    }
    case HJConvertDataType_RGB:
        comp = 3;
        break;
    default:
        return HJErrInvalidParams;
    }

    int ok = stbi_write_jpg(outPath.c_str(), w, h, comp, src, 95);
    return ok ? HJ_OK : HJErrFatal;
}

int initSRInternal(const std::string& modelRoot, int srType, const std::string& modeType, bool bCpu, int threadNums, int nScale)
{
    std::lock_guard<std::mutex> guard(g_srMutex);

    g_srWrapper = std::make_unique<HJVideoSRWrapper>();
    g_srOutput = nullptr;
    g_srElapseMs = -1;

    HJVideoSRWrapperOption option;
    option.ncnnRealESRGANType = modeType;
    option.ncnnRealCUGANType = "conservative";
    option.ncnnUseGPU = !bCpu;
    option.ncnnThreadNums = threadNums > 0 ? threadNums : 4;
    option.ncnnScale = nScale > 0 ? nScale : 1;

    HJVideoSRWrapperType wrapperType = HJVideoSRWrapperType_NCNNREALCUGAN;
    if (srType == 0)
    {
        wrapperType = HJVideoSRWrapperType_NCNNREALESRGAN;
    }

    int i_err = g_srWrapper->init(
        [&](std::shared_ptr<HJ::HJTransferMediaData> i_output, const HJ::HJSRRet& i_ret)
        {
            g_srOutput = i_output;
            g_srElapseMs = i_ret.m_elapseMs;
            HJFLogi("nativeProcessSR sr callback elapse:{}ms", g_srElapseMs);
        },
        wrapperType,
        modelRoot,
        option);
    if (i_err < 0)
    {
        g_srWrapper.reset();
    }
    return i_err;
}

int processSRInternal(const std::string& url, const std::string& outUrl, int64_t& srElapseMs)
{
    int i_err = HJ_OK;
    srElapseMs = -1;

    int w = 0;
    int h = 0;
    int c = 0;
    unsigned char* decoded = stbi_load(url.c_str(), &w, &h, &c, 4);
    if (!decoded || w <= 0 || h <= 0)
    {
        HJFLoge("stbi_load failed url:{}", url);
        return HJErrInvalidFile;
    }

    unsigned char* rgbaData[4] = { decoded, nullptr, nullptr, nullptr };
    int rgbaPitch[4] = { w * 4, 0, 0, 0 };
    HJTransferMediaData::Ptr rgbaFrame = HJTransferMediaDataRGBA::Create<HJTransferMediaDataRGBA>(
        rgbaData, rgbaPitch, w, h, 0);
    if (!rgbaFrame)
    {
        stbi_image_free(decoded);
        HJFLoge("create rgba frame failed");
        return HJErrInvalidParams;
    }

    HJTransferMediaData::Ptr nv12Frame = HJTransferMediaData::create(rgbaFrame, HJConvertDataType_YUVNV12);
    if (!nv12Frame || !nv12Frame->getData(0) || !nv12Frame->getData(1))
    {
        stbi_image_free(decoded);
        HJFLoge("convert rgba to nv12 failed");
        return HJErrFatal;
    }

    std::shared_ptr<HJUnifyWrapperData> input = std::make_shared<HJUnifyWrapperData>();
    input->dataType = HJUnifyWrapperDataType_NV12;
    input->width = w;
    input->height = h;
    input->data[0] = nv12Frame->getData(0);
    input->data[1] = nv12Frame->getData(1);
    input->nPitch[0] = nv12Frame->getStride(0);
    input->nPitch[1] = nv12Frame->getStride(1);
    input->timeStamp = 0;

    HJTransferMediaData::Ptr srOutput = nullptr;
    {
        std::lock_guard<std::mutex> guard(g_srMutex);
        if (!g_srWrapper)
        {
            stbi_image_free(decoded);
            HJFLoge("processSRInternal wrapper not initialized");
            return HJErrNotInited;
        }

        g_srOutput = nullptr;
        g_srElapseMs = -1;
        i_err = g_srWrapper->process(input, true);
        srOutput = g_srOutput;
        srElapseMs = g_srElapseMs;
    }

    stbi_image_free(decoded);
    if (i_err < 0)
    {
        HJFLoge("videoSR.process failed i_err:{}", i_err);
        return i_err;
    }
    if (!srOutput)
    {
        HJFLoge("videoSR output empty");
        return HJErrFatal;
    }

    i_err = saveSrJpg(srOutput, outUrl);
    if (i_err < 0)
    {
        HJFLoge("saveSrJpg failed i_err:{}", i_err);
        return i_err;
    }
    return HJ_OK;
}
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeInitSR)(
    JNIEnv* env,
    jobject thiz,
    jstring i_modelPath,
    jint i_srType,
    jstring i_modeType,
    jboolean i_bCpu,
    jint i_threadNum,
    jint i_nScale)
{
    (void)thiz;
    int i_err = HJ_OK;

    const char* modelPath = HJJNIField::JStringToString(env, i_modelPath);
    const char* modeType = HJJNIField::JStringToString(env, i_modeType);
    do
    {
        if (!modelPath || !modelPath[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeInitSR invalid modelPath");
            break;
        }
        if (!modeType || !modeType[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeInitSR invalid modeType");
            break;
        }

        const bool bCpu = (i_bCpu == JNI_TRUE);
        const int srType = static_cast<int>(i_srType);
        const int threadNums = i_threadNum > 0 ? static_cast<int>(i_threadNum) : 4;
        const int nScale = i_nScale > 0 ? static_cast<int>(i_nScale) : 1;
        const std::string modelRoot = normalizeModelRoot(modelPath);

        HJFLogi("nativeInitSR modelPath:{} (root:{}), srType:{}, modeType:{}, cpu:{}, threads:{}, scale:{}",
            modelPath, modelRoot, srType, modeType, bCpu, threadNums, nScale);
        i_err = initSRInternal(modelRoot, srType, modeType, bCpu, threadNums, nScale);
    } while (false);

    HJJNIField::JStringDestroy(env, modelPath, i_modelPath);
    HJJNIField::JStringDestroy(env, modeType, i_modeType);
    return i_err;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcessSR)(
    JNIEnv* env,
    jobject thiz,
    jstring i_url,
    jstring o_url)
{
    (void)thiz;
    int i_err = HJ_OK;
    int64_t srElapseMs = -1;

    const char* url = HJJNIField::JStringToString(env, i_url);
    const char* outUrl = HJJNIField::JStringToString(env, o_url);
    do
    {
        if (!url || !url[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcessSR invalid url");
            break;
        }
        if (!outUrl || !outUrl[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcessSR invalid outUrl");
            break;
        }

        i_err = processSRInternal(url, outUrl, srElapseMs);
        if (i_err < 0)
        {
            break;
        }

        HJFLogi("nativeProcessSR done, save:{}, sr_elapse:{}ms", outUrl, srElapseMs);
    } while (false);

    HJJNIField::JStringDestroy(env, url, i_url);
    HJJNIField::JStringDestroy(env, outUrl, o_url);

    if (i_err < 0)
    {
        return i_err;
    }
    if (srElapseMs < 0 || srElapseMs > std::numeric_limits<jint>::max())
    {
        return 0;
    }
    return static_cast<jint>(srElapseMs);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeProcess)(
    JNIEnv* env,
    jobject thiz,
    jstring i_modelPath,
    jint i_srType,
    jstring i_modeType,
    jboolean i_bCpu,
    jint i_threadNum,
    jint i_nScale,
    jstring i_url,
    jstring o_url)
{
    (void)thiz;
    int i_err = HJ_OK;
    int64_t srElapseMs = -1;

    const char* modelPath = HJJNIField::JStringToString(env, i_modelPath);
    const char* modeType = HJJNIField::JStringToString(env, i_modeType);
    const char* url = HJJNIField::JStringToString(env, i_url);
    const char* outUrl = HJJNIField::JStringToString(env, o_url);
    do
    {
        if (!modelPath || !modelPath[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcess invalid modelPath");
            break;
        }
        if (!modeType || !modeType[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcess invalid modeType");
            break;
        }
        if (!url || !url[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcess invalid url");
            break;
        }
        if (!outUrl || !outUrl[0])
        {
            i_err = HJErrInvalidParams;
            HJFLoge("nativeProcess invalid outUrl");
            break;
        }

        const bool bCpu = (i_bCpu == JNI_TRUE);
        const int srType = static_cast<int>(i_srType);
        const int threadNums = i_threadNum > 0 ? static_cast<int>(i_threadNum) : 4;
        const int nScale = i_nScale > 0 ? static_cast<int>(i_nScale) : 1;

        const std::string modelRoot = normalizeModelRoot(modelPath);
        HJFLogi(
            "nativeProcess modelPath:{} (root:{}), srType:{}, modeType:{}, cpu:{}, threads:{}, scale:{}, in:{}, out:{}",
            modelPath,
            modelRoot,
            srType,
            modeType,
            bCpu,
            threadNums,
            nScale,
            url,
            outUrl);
        int64_t t0 = HJCurrentSteadyMS();
        i_err = initSRInternal(modelRoot, srType, modeType, bCpu, threadNums, nScale);
        if (i_err < 0)
        {
            HJFLoge("nativeProcess initSRInternal failed i_err:{}", i_err);
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();
        i_err = processSRInternal(url, outUrl, srElapseMs);
        int64_t t2 = HJCurrentSteadyMS();
        HJFLogi("nativeProcess init:{} process elapse:{} total:{}", t1 - t0, t2 - t1, t2 - t0);
        if (i_err < 0)
        {
            break;
        }
        HJFLogi("nativeProcess done, save:{}, sr_elapse:{}ms", outUrl, srElapseMs);
    } while (false);
    HJJNIField::JStringDestroy(env, modelPath, i_modelPath);
    HJJNIField::JStringDestroy(env, modeType, i_modeType);
    HJJNIField::JStringDestroy(env, url, i_url);
    HJJNIField::JStringDestroy(env, outUrl, o_url);

    if (i_err < 0)
    {
        return i_err;
    }
    if (srElapseMs < 0 || srElapseMs > std::numeric_limits<jint>::max())
    {
        return 0;
    }
    return static_cast<jint>(srElapseMs);
}
