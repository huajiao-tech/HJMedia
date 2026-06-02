#include "HJSRMindSporeRealESRGAN.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <thread>
#include <string_view>
#include <utility>
#include <vector>

#include "libyuv/scale_rgb.h"

#if defined(HJ_HAS_MINDSPORE_LITE)
#include "include/api/context.h"
#include "include/api/model.h"
#include "include/api/status.h"
#include "include/api/types.h"
#endif

NS_HJ_BEGIN

struct HJSRMindSporeRealESRGAN::Impl
{
#if defined(HJ_HAS_MINDSPORE_LITE)
    std::shared_ptr<mindspore::Context> context;
    std::shared_ptr<mindspore::Model> model;
#endif
    std::string modelPath;
    int modelInputWidth = 0;
    int modelInputHeight = 0;
    bool modelFixedShape = false;
    int threadNum = 1;
    bool requestAccelerator = false;
    std::vector<float> floatBuffer;
    std::vector<uint16_t> fp16Buffer;
};

#if defined(HJ_HAS_MINDSPORE_LITE)
namespace
{
enum class HJMindSporeTensorLayout
{
    NHWC = 0,
    NCHW = 1,
};

struct HJMindSporeTensorDesc
{
    bool valid = false;
    int batch = 1;
    int width = 0;
    int height = 0;
    int channels = 0;
    HJMindSporeTensorLayout layout = HJMindSporeTensorLayout::NHWC;
};

static bool isNativeLiteModelPath(const std::string& path)
{
    return path.find("-native.ms") != std::string::npos;
}

static void applyTensorSizeHint(HJMindSporeTensorDesc& desc, int preferWidth, int preferHeight)
{
    if (!desc.valid || preferWidth <= 0 || preferHeight <= 0)
    {
        return;
    }
    if (desc.width == preferWidth && desc.height == preferHeight)
    {
        return;
    }
    if (desc.width == preferHeight && desc.height == preferWidth)
    {
        std::swap(desc.width, desc.height);
    }
}

static HJMindSporeTensorDesc describeTensor(const std::vector<int64_t>& shape, int preferWidth = 0, int preferHeight = 0)
{
    HJMindSporeTensorDesc desc;
    if (shape.size() == 4)
    {
        desc.batch = (shape[0] > 0) ? (int)shape[0] : 1;
        if (shape[1] == 3 && shape[3] != 3)
        {
            desc.layout = HJMindSporeTensorLayout::NCHW;
            desc.channels = 3;
            desc.height = (int)shape[2];
            desc.width = (int)shape[3];
        }
        else
        {
            desc.layout = HJMindSporeTensorLayout::NHWC;
            desc.height = (int)shape[1];
            desc.width = (int)shape[2];
            desc.channels = (shape[3] > 0) ? (int)shape[3] : 3;
        }
    }
    else if (shape.size() == 3)
    {
        if (shape[0] == 3 && shape[2] != 3)
        {
            desc.layout = HJMindSporeTensorLayout::NCHW;
            desc.channels = 3;
            desc.height = (int)shape[1];
            desc.width = (int)shape[2];
        }
        else
        {
            desc.layout = HJMindSporeTensorLayout::NHWC;
            desc.height = (int)shape[0];
            desc.width = (int)shape[1];
            desc.channels = (shape[2] > 0) ? (int)shape[2] : 3;
        }
    }

    desc.valid = desc.width > 0 && desc.height > 0 && desc.channels == 3;
    applyTensorSizeHint(desc, preferWidth, preferHeight);
    return desc;
}

static HJMindSporeTensorDesc describeTensorForNativeModel(const std::vector<int64_t>& shape, int preferWidth, int preferHeight)
{
    HJMindSporeTensorDesc desc = describeTensor(shape, preferWidth, preferHeight);
    if (preferWidth > 0 && preferHeight > 0)
    {
        desc.layout = HJMindSporeTensorLayout::NCHW;
        desc.channels = 3;
        desc.width = preferWidth;
        desc.height = preferHeight;
        desc.batch = (!shape.empty() && shape[0] > 0) ? (int)shape[0] : 1;
        desc.valid = true;
    }
    return desc;
}

static std::vector<int64_t> buildInputShape(const std::vector<int64_t>& currentShape, int width, int height)
{
    if (currentShape.size() == 4)
    {
        if (currentShape[1] == 3 && currentShape[3] != 3)
        {
            return { 1, 3, height, width };
        }
        return { 1, height, width, 3 };
    }
    if (currentShape.size() == 3)
    {
        if (currentShape[0] == 3 && currentShape[2] != 3)
        {
            return { 3, height, width };
        }
        return { height, width, 3 };
    }
    return {};
}

static std::string shapeToString(const std::vector<int64_t>& shape)
{
    std::string text = "[";
    for (size_t i = 0; i < shape.size(); ++i)
    {
        if (i > 0)
        {
            text += ",";
        }
        text += std::to_string(shape[i]);
    }
    text += "]";
    return text;
}

static std::shared_ptr<mindspore::Context> createMindSporeContext(int threadNum, bool useKirinNpu)
{
    std::shared_ptr<mindspore::Context> context = std::make_shared<mindspore::Context>();
    if (!context)
    {
        return nullptr;
    }

    threadNum = (std::max)(1, threadNum);
    context->SetThreadNum(threadNum);
    context->SetInterOpParallelNum(1);
    context->SetThreadAffinity(0);
    context->SetEnableParallel(threadNum > 1);

    auto& deviceList = context->MutableDeviceInfo();
    deviceList.clear();

    if (useKirinNpu)
    {
        auto npuDevice = std::make_shared<mindspore::KirinNPUDeviceInfo>();
        if (npuDevice)
        {
            npuDevice->SetEnableFP16(true);
            npuDevice->SetFrequency(3);
            deviceList.push_back(npuDevice);
        }
    }

    auto cpuDevice = std::make_shared<mindspore::CPUDeviceInfo>();
    if (!cpuDevice)
    {
        return nullptr;
    }
    cpuDevice->SetEnableFP16(false);
    deviceList.push_back(cpuDevice);
    return context;
}

static uint16_t floatToHalf(float value)
{
    union
    {
        float f;
        uint32_t u;
    } src = { value };

    uint32_t sign = (src.u >> 16) & 0x8000u;
    uint32_t mantissa = src.u & 0x007fffffu;
    int32_t exponent = (int32_t)((src.u >> 23) & 0xffu) - 127 + 15;

    if (exponent <= 0)
    {
        if (exponent < -10)
        {
            return (uint16_t)sign;
        }
        mantissa = (mantissa | 0x00800000u) >> (uint32_t)(1 - exponent);
        return (uint16_t)(sign | ((mantissa + 0x00001000u) >> 13));
    }
    if (exponent >= 31)
    {
        return (uint16_t)(sign | 0x7c00u);
    }
    return (uint16_t)(sign | ((uint32_t)exponent << 10) | ((mantissa + 0x00001000u) >> 13));
}

static float halfToFloat(uint16_t value)
{
    uint32_t sign = ((uint32_t)value & 0x8000u) << 16;
    uint32_t exponent = ((uint32_t)value >> 10) & 0x1fu;
    uint32_t mantissa = (uint32_t)value & 0x03ffu;

    uint32_t out = 0;
    if (exponent == 0)
    {
        if (mantissa == 0)
        {
            out = sign;
        }
        else
        {
            exponent = 1;
            while ((mantissa & 0x0400u) == 0)
            {
                mantissa <<= 1;
                --exponent;
            }
            mantissa &= 0x03ffu;
            out = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);
        }
    }
    else if (exponent == 31)
    {
        out = sign | 0x7f800000u | (mantissa << 13);
    }
    else
    {
        out = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);
    }

    union
    {
        uint32_t u;
        float f;
    } dst = { out };
    return dst.f;
}

static uint8_t clampToByte(float value, bool normalized)
{
    float scaled = normalized ? value * 255.0f : value;
    if (scaled < 0.0f)
    {
        scaled = 0.0f;
    }
    else if (scaled > 255.0f)
    {
        scaled = 255.0f;
    }
    return (uint8_t)(scaled + 0.5f);
}

static void fillInputTensor(const unsigned char* rgb, int width, int height,
    const HJMindSporeTensorDesc& desc, mindspore::DataType dataType, void* dst,
    std::vector<float>& floatBuffer, std::vector<uint16_t>& fp16Buffer)
{
    const size_t pixelCount = (size_t)width * (size_t)height;
    if (desc.layout == HJMindSporeTensorLayout::NHWC)
    {
        if (dataType == mindspore::DataType::kNumberTypeFloat16)
        {
            fp16Buffer.resize(pixelCount * 3);
            for (size_t i = 0; i < pixelCount * 3; ++i)
            {
                fp16Buffer[i] = floatToHalf((float)rgb[i] / 255.0f);
            }
            memcpy(dst, fp16Buffer.data(), fp16Buffer.size() * sizeof(uint16_t));
        }
        else if (dataType == mindspore::DataType::kNumberTypeFloat32)
        {
            floatBuffer.resize(pixelCount * 3);
            for (size_t i = 0; i < pixelCount * 3; ++i)
            {
                floatBuffer[i] = (float)rgb[i] / 255.0f;
            }
            memcpy(dst, floatBuffer.data(), floatBuffer.size() * sizeof(float));
        }
        else
        {
            memcpy(dst, rgb, pixelCount * 3);
        }
        return;
    }

    if (dataType == mindspore::DataType::kNumberTypeFloat16)
    {
        fp16Buffer.resize(pixelCount * 3);
        for (int c = 0; c < 3; ++c)
        {
            for (size_t i = 0; i < pixelCount; ++i)
            {
                fp16Buffer[(size_t)c * pixelCount + i] = floatToHalf((float)rgb[i * 3 + c] / 255.0f);
            }
        }
        memcpy(dst, fp16Buffer.data(), fp16Buffer.size() * sizeof(uint16_t));
    }
    else if (dataType == mindspore::DataType::kNumberTypeFloat32)
    {
        floatBuffer.resize(pixelCount * 3);
        for (int c = 0; c < 3; ++c)
        {
            for (size_t i = 0; i < pixelCount; ++i)
            {
                floatBuffer[(size_t)c * pixelCount + i] = (float)rgb[i * 3 + c] / 255.0f;
            }
        }
        memcpy(dst, floatBuffer.data(), floatBuffer.size() * sizeof(float));
    }
    else
    {
        unsigned char* out = reinterpret_cast<unsigned char*>(dst);
        for (int c = 0; c < 3; ++c)
        {
            for (size_t i = 0; i < pixelCount; ++i)
            {
                out[(size_t)c * pixelCount + i] = rgb[i * 3 + c];
            }
        }
    }
}

static bool isNormalizedTensor(const void* data, mindspore::DataType dataType, size_t elementCount)
{
    const size_t sampleCount = (std::min)(elementCount, (size_t)64);
    float maxAbsValue = 0.0f;
    if (dataType == mindspore::DataType::kNumberTypeFloat16)
    {
        const uint16_t* ptr = reinterpret_cast<const uint16_t*>(data);
        for (size_t i = 0; i < sampleCount; ++i)
        {
            maxAbsValue = (std::max)(maxAbsValue, std::fabs(halfToFloat(ptr[i])));
        }
    }
    else if (dataType == mindspore::DataType::kNumberTypeFloat32)
    {
        const float* ptr = reinterpret_cast<const float*>(data);
        for (size_t i = 0; i < sampleCount; ++i)
        {
            maxAbsValue = (std::max)(maxAbsValue, std::fabs(ptr[i]));
        }
    }
    return maxAbsValue <= 2.0f;
}

static int addScaledBaseToRgb(const unsigned char* srcRgb, int srcWidth, int srcHeight,
    HJSPBuffer::Ptr& outRgb, int outWidth, int outHeight)
{
    if (!srcRgb || !outRgb || !outRgb->getBuf() || srcWidth <= 0 || srcHeight <= 0 || outWidth <= 0 || outHeight <= 0)
    {
        return HJErrInvalidParams;
    }

    HJSPBuffer::Ptr scaledBase = HJSPBuffer::create((int)((size_t)outWidth * (size_t)outHeight * 3));
    if (!scaledBase || !scaledBase->getBuf())
    {
        return HJErrFatal;
    }

    if (srcWidth == outWidth && srcHeight == outHeight)
    {
        memcpy(scaledBase->getBuf(), srcRgb, (size_t)outWidth * (size_t)outHeight * 3);
    }
    else
    {
        const int ret = libyuv::RGBScale(
            srcRgb, srcWidth * 3, srcWidth, srcHeight,
            scaledBase->getBuf(), outWidth * 3, outWidth, outHeight,
            libyuv::kFilterBilinear);
        if (ret != 0)
        {
            return HJErrFatal;
        }
    }

    unsigned char* dst = outRgb->getBuf();
    const unsigned char* base = scaledBase->getBuf();
    const size_t totalBytes = (size_t)outWidth * (size_t)outHeight * 3;
    for (size_t i = 0; i < totalBytes; ++i)
    {
        const int value = (int)dst[i] + (int)base[i];
        dst[i] = (unsigned char)(value > 255 ? 255 : value);
    }
    return HJ_OK;
}

static int convertOutputTensorToRgb(const mindspore::MSTensor& tensor, HJSPBuffer::Ptr& outRgb,
    int& outWidth, int& outHeight, int preferWidth = 0, int preferHeight = 0, bool forceNchw = false)
{
    const HJMindSporeTensorDesc desc = forceNchw
        ? describeTensorForNativeModel(tensor.Shape(), preferWidth, preferHeight)
        : describeTensor(tensor.Shape(), preferWidth, preferHeight);
    if (!desc.valid || desc.batch != 1)
    {
        return HJErrInvalidParams;
    }

    const std::shared_ptr<const void> dataHolder = tensor.Data();
    const void* src = dataHolder ? dataHolder.get() : nullptr;
    if (!src)
    {
        return HJErrFatal;
    }

    outWidth = desc.width;
    outHeight = desc.height;
    const size_t pixelCount = (size_t)outWidth * (size_t)outHeight;
    const size_t outBytes = pixelCount * 3;
    if (!outRgb || outRgb->getSize() != (int)outBytes)
    {
        outRgb = HJSPBuffer::create((int)outBytes);
    }
    if (!outRgb || !outRgb->getBuf())
    {
        return HJErrFatal;
    }

    const mindspore::DataType dataType = tensor.DataType();
    const bool normalized = isNormalizedTensor(src, dataType, pixelCount * 3);
    unsigned char* dst = outRgb->getBuf();

    if (desc.layout == HJMindSporeTensorLayout::NHWC)
    {
        for (size_t i = 0; i < pixelCount; ++i)
        {
            for (int c = 0; c < 3; ++c)
            {
                const size_t index = i * 3 + (size_t)c;
                if (dataType == mindspore::DataType::kNumberTypeFloat16)
                {
                    dst[index] = clampToByte(halfToFloat(reinterpret_cast<const uint16_t*>(src)[index]), normalized);
                }
                else if (dataType == mindspore::DataType::kNumberTypeFloat32)
                {
                    dst[index] = clampToByte(reinterpret_cast<const float*>(src)[index], normalized);
                }
                else
                {
                    dst[index] = reinterpret_cast<const uint8_t*>(src)[index];
                }
            }
        }
    }
    else
    {
        for (int c = 0; c < 3; ++c)
        {
            for (size_t i = 0; i < pixelCount; ++i)
            {
                const size_t srcIndex = (size_t)c * pixelCount + i;
                const size_t dstIndex = i * 3 + (size_t)c;
                if (dataType == mindspore::DataType::kNumberTypeFloat16)
                {
                    dst[dstIndex] = clampToByte(halfToFloat(reinterpret_cast<const uint16_t*>(src)[srcIndex]), normalized);
                }
                else if (dataType == mindspore::DataType::kNumberTypeFloat32)
                {
                    dst[dstIndex] = clampToByte(reinterpret_cast<const float*>(src)[srcIndex], normalized);
                }
                else
                {
                    dst[dstIndex] = reinterpret_cast<const uint8_t*>(src)[srcIndex];
                }
            }
        }
    }

    return HJ_OK;
}
}
#endif

HJSRMindSporeRealESRGAN::HJSRMindSporeRealESRGAN()
    : m_impl(std::make_unique<Impl>())
{
}

HJSRMindSporeRealESRGAN::~HJSRMindSporeRealESRGAN() = default;

#if defined(HJ_HAS_MINDSPORE_LITE)
namespace
{
static mindspore::ModelType detectModelType(const std::string& path)
{
    constexpr std::string_view kLiteSuffix = ".ms";
    constexpr std::string_view kMindIRSuffix = ".mindir";
    if (path.size() >= kLiteSuffix.size() && path.compare(path.size() - kLiteSuffix.size(), kLiteSuffix.size(), kLiteSuffix) == 0)
    {
        return mindspore::kMindIR_Lite;
    }
    if (path.size() >= kMindIRSuffix.size() && path.compare(path.size() - kMindIRSuffix.size(), kMindIRSuffix.size(), kMindIRSuffix) == 0)
    {
        return mindspore::kMindIR;
    }
    return mindspore::kMindIR;
}

static std::string buildFixedLiteModelPath(const std::string& basePath, int width, int height)
{
    return HJUtilitys::concatenatePath(basePath,
        fmt::format("MindSpore/realesr/realesr-general-merge05-x4v3-{}x{}.ms", width, height));
}

static std::string buildFixedNativeLiteModelPath(const std::string& basePath, int width, int height)
{
    return HJUtilitys::concatenatePath(basePath,
        fmt::format("MindSpore/realesr/realesr-general-merge05-x4v3-{}x{}-native.ms", width, height));
}

static std::vector<std::string> buildModelCandidates(const std::string& basePath, int width, int height)
{
    std::vector<std::string> candidates;
    if (width > 0 && height > 0)
    {
        const std::string fixedNativeMsPath = buildFixedNativeLiteModelPath(basePath, width, height);
        const std::string fixedMsPath = buildFixedLiteModelPath(basePath, width, height);
        if (HJBaseUtils::isFileExists(fixedNativeMsPath))
        {
            candidates.push_back(fixedNativeMsPath);
        }
        if (HJBaseUtils::isFileExists(fixedMsPath))
        {
            candidates.push_back(fixedMsPath);
        }
    }

//    const std::string msPath = HJUtilitys::concatenatePath(basePath, "MindSpore/realesr/realesr-general-merge05-x4v3.ms");
//    if (HJBaseUtils::isFileExists(msPath))
//    {
//        candidates.push_back(msPath);
//    }
//    if (HJBaseUtils::isFileExists(mindirPath))
//    {
//        candidates.push_back(mindirPath);
//    }
    return candidates;
}
}
#endif

int HJSRMindSporeRealESRGAN::init(HJBaseParam::Ptr params)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJBaseVideoSR::init(params);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJBaseVideoSR init error:{}", i_err);
            break;
        }

        m_modelScale = 4;
        const std::vector<std::string> modelCandidates = buildModelCandidates(m_modelUrls, 0, 0);
        if (modelCandidates.empty())
        {
            HJFLoge("mindspore realesr model not exists basePath:{}", m_modelUrls);
            i_err = HJErrInvalidFile;
            break;
        }
        m_impl->modelPath.clear();
        m_impl->modelInputWidth = 0;
        m_impl->modelInputHeight = 0;
        m_impl->modelFixedShape = false;

#if !defined(HJ_HAS_MINDSPORE_LITE)
        HJFLoge("MindSpore Lite runtime is not linked. Please add externals/harmony/mindspore.");
        i_err = HJErrNotInited;
        break;
#else
        m_impl->model = std::make_shared<mindspore::Model>();
        if (!m_impl->model)
        {
            i_err = HJErrNotInited;
            break;
        }

        int threadNum = m_option ? m_option->ncnnThreadNums : 0;
        if (threadNum <= 0)
        {
            const unsigned int hwThreads = std::thread::hardware_concurrency();
            threadNum = hwThreads > 0 ? (int)hwThreads : 4;
        }
        threadNum = (std::max)(1, threadNum);
        m_impl->threadNum = threadNum;
        m_impl->requestAccelerator = m_option ? m_option->ncnnUseGPU : false;
        m_impl->context = createMindSporeContext(m_impl->threadNum, false);
        if (!m_impl->context)
        {
            i_err = HJErrNotInited;
            break;
        }
        HJFLogi("HJSRMindSporeRealESRGAN init deferred basePath:{} scale:{} threadNum:{} parallel:{} reqAccel:{}",
            m_modelUrls, m_modelScale, threadNum, threadNum > 1, m_impl->requestAccelerator);
#endif
    } while (false);
    return i_err;
}

int HJSRMindSporeRealESRGAN::ensureModelReady(int width, int height)
{
#if !defined(HJ_HAS_MINDSPORE_LITE)
    (void)width;
    (void)height;
    return HJErrNotInited;
#else
    if (!m_impl || !m_impl->context)
    {
        return HJErrNotInited;
    }
    if (width <= 0 || height <= 0)
    {
        return HJErrInvalidParams;
    }
    if (m_impl->model && m_impl->modelInputWidth == width && m_impl->modelInputHeight == height)
    {
        return HJ_OK;
    }

    const std::vector<std::string> modelCandidates = buildModelCandidates(m_modelUrls, width, height);
    if (modelCandidates.empty())
    {
        HJFLoge("MindSpore model candidates empty for input:{}x{} basePath:{}", width, height, m_modelUrls);
        return HJErrInvalidFile;
    }

    const std::string fixedModelPath = buildFixedLiteModelPath(m_modelUrls, width, height);
    const std::string fixedNativeModelPath = buildFixedNativeLiteModelPath(m_modelUrls, width, height);
    const int64_t t0 = HJCurrentSteadyMS();
    for (const std::string& candidatePath : modelCandidates)
    {
        const mindspore::ModelType modelType = detectModelType(candidatePath);
        const bool tryNpuFirst = m_impl->requestAccelerator &&
            mindspore::Model::CheckModelSupport(mindspore::DeviceType::kKirinNPU, modelType);
        for (int attempt = 0; attempt < (tryNpuFirst ? 2 : 1); ++attempt)
        {
            const bool useKirinNpu = tryNpuFirst && attempt == 0;
            std::shared_ptr<mindspore::Context> buildContext = createMindSporeContext(m_impl->threadNum, useKirinNpu);
            std::shared_ptr<mindspore::Model> candidateModel = std::make_shared<mindspore::Model>();
            if (!buildContext || !candidateModel)
            {
                return HJErrNotInited;
            }

            HJFLogi("MindSpore build candidate:{} input:{}x{} modelType:{} useKirinNpu:{}",
                candidatePath, width, height, (int)modelType, useKirinNpu);
            if (candidateModel->Build(candidatePath, modelType, buildContext) != mindspore::kSuccess)
            {
                HJFLoge("MindSpore build model failed path:{} modelType:{} input:{}x{} useKirinNpu:{}",
                    candidatePath, (int)modelType, width, height, useKirinNpu);
                continue;
            }

            std::vector<mindspore::MSTensor> inputs = candidateModel->GetInputs();
            std::vector<mindspore::MSTensor> outputs = candidateModel->GetOutputs();
            if (inputs.empty() || outputs.empty())
            {
                HJFLoge("MindSpore model input/output empty path:{} useKirinNpu:{}", candidatePath, useKirinNpu);
                continue;
            }

            m_impl->context = buildContext;
            m_impl->model = candidateModel;
            m_impl->modelPath = candidatePath;
            m_impl->modelInputWidth = width;
            m_impl->modelInputHeight = height;
            m_impl->modelFixedShape = (candidatePath == fixedModelPath
                || candidatePath == fixedNativeModelPath);
            const int64_t t1 = HJCurrentSteadyMS();
            HJFLogi("MindSpore ready model:{} input:{}x{} fixed:{} useKirinNpu:{} elapse:{}",
                m_impl->modelPath, width, height, m_impl->modelFixedShape, useKirinNpu, t1 - t0);
            return HJ_OK;
        }
    }

    return HJErrNotInited;
#endif
}

int HJSRMindSporeRealESRGAN::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
{
    int i_err = HJ_OK;
    if (!i_inputData)
    {
        return HJErrInvalidParams;
    }

#if !defined(HJ_HAS_MINDSPORE_LITE)
    (void)o_data;
    (void)o_ret;
    return HJErrNotInited;
#else
    if (!m_impl || !m_impl->context)
    {
        return HJErrNotInited;
    }

    int i_width = 0;
    int i_height = 0;
    int64_t i_timestamp = 0;
    const unsigned char* inputRgb = nullptr;

    do
    {
        i_err = prepareInputRgb(i_inputData, inputRgb, i_width, i_height, i_timestamp, "HJSRMindSporeRealESRGAN");
        if (i_err < 0)
        {
            break;
        }

        i_err = ensureModelReady(i_width, i_height);
        if (i_err < 0)
        {
            break;
        }

        std::vector<mindspore::MSTensor> inputs = m_impl->model->GetInputs();
        if (inputs.empty())
        {
            i_err = HJErrNotInited;
            break;
        }

        std::vector<int64_t> newShape = buildInputShape(inputs[0].Shape(), i_width, i_height);
        if (newShape.empty())
        {
            HJFLoge("MindSpore input shape not supported");
            i_err = HJErrInvalidParams;
            break;
        }

        const bool forceNativeNchw = isNativeLiteModelPath(m_impl->modelPath);
        const HJMindSporeTensorDesc currentDesc = forceNativeNchw
            ? describeTensorForNativeModel(inputs[0].Shape(), i_width, i_height)
            : describeTensor(inputs[0].Shape(), i_width, i_height);
        if (!currentDesc.valid || currentDesc.width != i_width || currentDesc.height != i_height)
        {
            if (m_impl->modelFixedShape)
            {
                HJFLoge("MindSpore fixed model mismatch model:{} tensor:{}x{} input:{}x{}",
                    m_impl->modelPath, currentDesc.width, currentDesc.height, i_width, i_height);
                i_err = HJErrInvalidParams;
                break;
            }
            std::vector<std::vector<int64_t>> newShapes;
            newShapes.push_back(newShape);
            if (m_impl->model->Resize(inputs, newShapes) != mindspore::kSuccess)
            {
                HJFLoge("MindSpore resize failed to:{}x{}", i_width, i_height);
                i_err = HJErrFatal;
                break;
            }
            inputs = m_impl->model->GetInputs();
            if (inputs.empty())
            {
                i_err = HJErrNotInited;
                break;
            }
        }

        const HJMindSporeTensorDesc inputDesc = forceNativeNchw
            ? describeTensorForNativeModel(inputs[0].Shape(), i_width, i_height)
            : describeTensor(inputs[0].Shape(), i_width, i_height);
        if (!inputDesc.valid || inputDesc.width != i_width || inputDesc.height != i_height)
        {
            HJFLoge("MindSpore input tensor mismatch shape after resize");
            i_err = HJErrInvalidParams;
            break;
        }

        void* inputData = inputs[0].MutableData();
        if (!inputData)
        {
            HJFLoge("MindSpore input MutableData failed");
            i_err = HJErrFatal;
            break;
        }
        fillInputTensor(inputRgb, i_width, i_height, inputDesc, inputs[0].DataType(), inputData, m_impl->floatBuffer, m_impl->fp16Buffer);

        int64_t t0 = HJCurrentSteadyMS();
        std::vector<mindspore::MSTensor> outputs;
        if (m_impl->model->Predict(inputs, &outputs) != mindspore::kSuccess || outputs.empty())
        {
            HJFLoge("MindSpore predict failed");
            i_err = HJErrFatal;
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();
        HJFLogi("MindSpore output tensor shape:{} dtype:{} native:{} model:{}",
            shapeToString(outputs[0].Shape()), (int)outputs[0].DataType(), forceNativeNchw, m_impl->modelPath);

        HJSPBuffer::Ptr outRgb = nullptr;
        int outWidth = 0;
        int outHeight = 0;
        const int expectedOutWidth = i_width * m_modelScale;
        const int expectedOutHeight = i_height * m_modelScale;
        i_err = convertOutputTensorToRgb(outputs[0], outRgb, outWidth, outHeight,
            expectedOutWidth, expectedOutHeight, forceNativeNchw);
        if (i_err < 0)
        {
            HJFLoge("MindSpore output convert failed");
            break;
        }
        if (forceNativeNchw)
        {
            i_err = addScaledBaseToRgb(inputRgb, i_width, i_height, outRgb, outWidth, outHeight);
            if (i_err < 0)
            {
                HJFLoge("MindSpore native base add failed input:{}x{} output:{}x{}",
                    i_width, i_height, outWidth, outHeight);
                break;
            }
        }

        int targetScale = (std::max)(1, m_option ? m_option->ncnnScale : 1);
        targetScale = (std::min)(targetScale, m_modelScale);
        const int targetWidth = i_width * targetScale;
        const int targetHeight = i_height * targetScale;

        o_ret.m_elapseMs = t1 - t0;
        o_ret.m_systemTime = t0;
        o_ret.m_timeStamp = i_timestamp;
        i_err = HJBaseVideoSR::finalizeSROutput(outRgb, outWidth, outHeight, targetWidth, targetHeight, i_timestamp, i_inputData, o_data, "HJSRMindSporeRealESRGAN");
        if (i_err < 0)
        {
            break;
        }

        int64_t t2 = HJCurrentSteadyMS();
        HJFPERLog5i("HJSRMindSporeRealESRGAN process type:{} in:{}x{} out:{}x{} target:{}x{} native:{} elapseMs:{} model:{}",
            (int)i_inputData->getConvertType(), i_width, i_height, outWidth, outHeight,
            targetWidth, targetHeight, forceNativeNchw, (t2 - t0), m_impl->modelPath);
    } while (false);

    return i_err;
#endif
}

NS_HJ_END
