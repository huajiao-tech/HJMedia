#include "HJFaceDetectCoreMLRetinaface.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"
#include "HJRetinaFaceDecodeUtils.h"
#include "libyuv.h"

#import <CoreML/CoreML.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <cmath>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

NS_HJ_BEGIN

namespace
{
static NSString* toNSString(const std::string& value)
{
    return [NSString stringWithUTF8String:value.c_str()];
}

static std::string toStdString(NSString* value)
{
    return value.length > 0 ? std::string(value.UTF8String) : std::string();
}

static NSString* computeModeText(int mode)
{
    switch (mode)
    {
        case 1: return @"CPU+GPU";
        case 2: return @"CPU+ANE";
        case 3: return @"CPUOnly";
        case 0:
        default: return @"All";
    }
}

static MLComputeUnits computeUnitsForMode(int mode)
{
    switch (mode)
    {
        case 1: return MLComputeUnitsCPUAndGPU;
        case 2:
            if (@available(iOS 16.0, *))
            {
                return MLComputeUnitsCPUAndNeuralEngine;
            }
            return MLComputeUnitsAll;
        case 3: return MLComputeUnitsCPUOnly;
        case 0:
        default:
            return MLComputeUnitsAll;
    }
}

static NSArray<NSString*>* buildModelCandidates(int width, int height)
{
    NSMutableOrderedSet<NSString*>* candidates = [NSMutableOrderedSet orderedSet];
    if (width > 0 && height > 0)
    {
        [candidates addObject:[NSString stringWithFormat:@"retinaface-mnet25-fixed-%dx%d", width, height]];
        [candidates addObject:[NSString stringWithFormat:@"retinaface-mnet25-%dx%d", width, height]];
        [candidates addObject:[NSString stringWithFormat:@"retinaface-fixed-%dx%d", width, height]];
        [candidates addObject:[NSString stringWithFormat:@"retinaface-%dx%d", width, height]];
    }
    [candidates addObjectsFromArray:@[
        @"retinaface-mnet25",
        @"retinaface"
    ]];
    return candidates.array;
}

static NSString* retinaFaceCoreMLDir(NSString* baseModelDir)
{
    if (baseModelDir.length == 0)
    {
        return @"";
    }
    return [baseModelDir stringByAppendingPathComponent:@"CoreML/retinaface"];
}

static NSURL* findCompiledModelURL(NSString* baseModelDir, int width, int height)
{
    NSString* coreMLDir = retinaFaceCoreMLDir(baseModelDir);
    if (coreMLDir.length == 0)
    {
        return nil;
    }
    NSFileManager* fm = [NSFileManager defaultManager];
    for (NSString* name in buildModelCandidates(width, height))
    {
        NSString* path = [coreMLDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.mlmodelc", name]];
        if ([fm fileExistsAtPath:path])
        {
            return [NSURL fileURLWithPath:path];
        }
    }
    return nil;
}

static NSURL* findSourceModelURL(NSString* baseModelDir, int width, int height)
{
    NSString* coreMLDir = retinaFaceCoreMLDir(baseModelDir);
    if (coreMLDir.length == 0)
    {
        return nil;
    }
    NSFileManager* fm = [NSFileManager defaultManager];
    for (NSString* name in buildModelCandidates(width, height))
    {
        for (NSString* ext in @[ @"mlpackage", @"mlmodel" ])
        {
            NSString* path = [coreMLDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.%@", name, ext]];
            if ([fm fileExistsAtPath:path])
            {
                return [NSURL fileURLWithPath:path];
            }
        }
    }
    return nil;
}

static CVPixelBufferRef newPixelBufferFromRGB(const unsigned char* rgb, int width, int height)
{
    if (!rgb || width <= 0 || height <= 0)
    {
        return NULL;
    }
    NSDictionary* attrs = @{
        (NSString*)kCVPixelBufferCGImageCompatibilityKey: @YES,
        (NSString*)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
    };
    CVPixelBufferRef buffer = NULL;
    CVReturn ret = CVPixelBufferCreate(kCFAllocatorDefault,
                                       width,
                                       height,
                                       kCVPixelFormatType_32BGRA,
                                       (__bridge CFDictionaryRef)attrs,
                                       &buffer);
    if (ret != kCVReturnSuccess || !buffer)
    {
        return NULL;
    }
    CVPixelBufferLockBaseAddress(buffer, 0);
    unsigned char* base = (unsigned char*)CVPixelBufferGetBaseAddress(buffer);
    const size_t stride = CVPixelBufferGetBytesPerRow(buffer);
    if (!base || stride < (size_t)width * 4 ||
        libyuv::RAWToARGB(rgb, width * 3, base, (int)stride, width, height) != 0)
    {
        CVPixelBufferUnlockBaseAddress(buffer, 0);
        CFRelease(buffer);
        return NULL;
    }
    CVPixelBufferUnlockBaseAddress(buffer, 0);
    return buffer;
}

static bool parseMultiArrayAsCHW(MLMultiArray* arr, HJRetinaFaceTensor& outTensor)
{
    outTensor = HJRetinaFaceTensor();
    if (!arr || arr.shape.count < 3 || arr.shape.count > 4 || !arr.dataPointer)
    {
        return false;
    }

    NSInteger n = 1;
    NSInteger c = 0;
    NSInteger h = 0;
    NSInteger w = 0;
    NSInteger sN = 0;
    NSInteger sC = 0;
    NSInteger sH = 0;
    NSInteger sW = 0;
    if (arr.shape.count == 4)
    {
        n = arr.shape[0].integerValue;
        c = arr.shape[1].integerValue;
        h = arr.shape[2].integerValue;
        w = arr.shape[3].integerValue;
        sN = arr.strides[0].integerValue;
        sC = arr.strides[1].integerValue;
        sH = arr.strides[2].integerValue;
        sW = arr.strides[3].integerValue;
    }
    else
    {
        c = arr.shape[0].integerValue;
        h = arr.shape[1].integerValue;
        w = arr.shape[2].integerValue;
        sC = arr.strides[0].integerValue;
        sH = arr.strides[1].integerValue;
        sW = arr.strides[2].integerValue;
    }
    if (n != 1 || c <= 0 || h <= 0 || w <= 0)
    {
        return false;
    }

    outTensor.channels = (int)c;
    outTensor.height = (int)h;
    outTensor.width = (int)w;
    outTensor.data.resize((size_t)c * (size_t)h * (size_t)w);

    const bool isDouble = (arr.dataType == MLMultiArrayDataTypeDouble);
    const bool isFloat32 = (arr.dataType == MLMultiArrayDataTypeFloat32);
    BOOL isFloat16 = NO;
    if (@available(iOS 16.0, *))
    {
        isFloat16 = (arr.dataType == MLMultiArrayDataTypeFloat16);
    }
    if (!isDouble && !isFloat32 && !isFloat16)
    {
        return false;
    }

    const double* dp = (const double*)arr.dataPointer;
    const float* fp32 = (const float*)arr.dataPointer;
    const uint16_t* fp16 = (const uint16_t*)arr.dataPointer;
    auto halfToFloat = [](uint16_t h) -> float
    {
        const uint32_t sign = (uint32_t)(h & 0x8000) << 16;
        uint32_t exp = (h >> 10) & 0x1F;
        uint32_t mant = h & 0x03FF;
        uint32_t out = 0;
        if (exp == 0)
        {
            if (mant == 0)
            {
                out = sign;
            }
            else
            {
                exp = 1;
                while ((mant & 0x0400) == 0)
                {
                    mant <<= 1;
                    exp--;
                }
                mant &= 0x03FF;
                out = sign | ((exp + (127 - 15)) << 23) | (mant << 13);
            }
        }
        else if (exp == 0x1F)
        {
            out = sign | 0x7F800000 | (mant << 13);
        }
        else
        {
            out = sign | ((exp + (127 - 15)) << 23) | (mant << 13);
        }
        float value = 0.0f;
        memcpy(&value, &out, sizeof(float));
        return value;
    };

    for (NSInteger channel = 0; channel < c; ++channel)
    {
        for (NSInteger row = 0; row < h; ++row)
        {
            for (NSInteger col = 0; col < w; ++col)
            {
                const NSInteger base = sN * 0 + sC * channel + sH * row + sW * col;
                float value = 0.0f;
                if (isDouble)
                {
                    value = (float)dp[base];
                }
                else if (isFloat32)
                {
                    value = fp32[base];
                }
                else
                {
                    value = halfToFloat(fp16[base]);
                }
                outTensor.data[((size_t)channel * (size_t)h + (size_t)row) * (size_t)w + (size_t)col] = value;
            }
        }
    }
    return true;
}

static bool reorderScoreTensorAnchorMajorToClassMajor(HJRetinaFaceTensor& tensor)
{
    if (!tensor.valid() || tensor.channels <= 0 || (tensor.channels % 2) != 0)
    {
        return false;
    }

    const int numAnchors = tensor.channels / 2;
    const size_t plane = (size_t)tensor.height * (size_t)tensor.width;
    std::vector<float> reordered(tensor.data.size(), 0.0f);
    for (int anchorIndex = 0; anchorIndex < numAnchors; ++anchorIndex)
    {
        const int srcBg = anchorIndex * 2 + 0;
        const int srcFg = anchorIndex * 2 + 1;
        const int dstBg = anchorIndex;
        const int dstFg = anchorIndex + numAnchors;
        std::copy_n(tensor.data.data() + (size_t)srcBg * plane,
                    plane,
                    reordered.data() + (size_t)dstBg * plane);
        std::copy_n(tensor.data.data() + (size_t)srcFg * plane,
                    plane,
                    reordered.data() + (size_t)dstFg * plane);
    }
    tensor.data.swap(reordered);
    return true;
}

static float maxForegroundScore(const HJRetinaFaceTensor& tensor)
{
    if (!tensor.valid() || tensor.channels <= 0 || (tensor.channels % 2) != 0)
    {
        return 0.0f;
    }
    const int numAnchors = tensor.channels / 2;
    const size_t plane = (size_t)tensor.height * (size_t)tensor.width;
    float maxScore = 0.0f;
    for (int anchorIndex = 0; anchorIndex < numAnchors; ++anchorIndex)
    {
        const float* fg = tensor.data.data() + (size_t)(anchorIndex + numAnchors) * plane;
        for (size_t idx = 0; idx < plane; ++idx)
        {
            if (fg[idx] > maxScore)
            {
                maxScore = fg[idx];
            }
        }
    }
    return maxScore;
}

static std::string describeFeatureSet(const HJRetinaFaceFeatureSet& feature)
{
    std::ostringstream oss;
    oss << "s" << feature.stride
        << " score:" << feature.score.channels << "x" << feature.score.height << "x" << feature.score.width
        << " bbox:" << feature.bbox.channels << "x" << feature.bbox.height << "x" << feature.bbox.width
        << " landmark:" << feature.landmark.channels
        << " maxFg:" << std::fixed << std::setprecision(4) << maxForegroundScore(feature.score);
    return oss.str();
}

static std::string describeFaceObjects(const std::vector<HJRetinaFaceObject>& faces,
                                       int maxCount,
                                       int offx,
                                       int offy,
                                       int scaleW,
                                       int scaleH,
                                       int inputWidth,
                                       int inputHeight)
{
    std::ostringstream oss;
    const int count = (std::min)(maxCount, (int)faces.size());
    for (int i = 0; i < count; ++i)
    {
        const auto& face = faces[(size_t)i];
        const float x0 = (face.x - offx) * inputWidth / scaleW;
        const float y0 = (face.y - offy) * inputHeight / scaleH;
        const float x1 = (face.x + face.w - offx) * inputWidth / scaleW;
        const float y1 = (face.y + face.h - offy) * inputHeight / scaleH;
        if (i > 0)
        {
            oss << ";";
        }
        oss << "#" << i
            << " raw:" << (int)std::lround(face.x) << "," << (int)std::lround(face.y) << "," << (int)std::lround(face.w) << "," << (int)std::lround(face.h)
            << " map:" << (int)std::lround(x0) << "," << (int)std::lround(y0) << "," << (int)std::lround(x1 - x0) << "," << (int)std::lround(y1 - y0)
            << " p:" << std::fixed << std::setprecision(3) << face.prob;
    }
    return oss.str();
}

static int parseStrideFromName(NSString* name)
{
    NSString* lower = name.lowercaseString ?: @"";
    if ([lower containsString:@"stride32"] || [lower containsString:@"s32"])
    {
        return 32;
    }
    if ([lower containsString:@"stride16"] || [lower containsString:@"s16"])
    {
        return 16;
    }
    if ([lower containsString:@"stride8"] || [lower containsString:@"s8"])
    {
        return 8;
    }
    return 0;
}
}

struct HJFaceDetectCoreMLRetinaface::Impl
{
    MLModel* model = nil;
    NSString* baseModelDir = nil;
    NSString* modelPath = nil;
    NSString* inputName = nil;
    CGSize inputFixedSize = CGSizeZero;
    int computeMode = 0;
};

HJFaceDetectCoreMLRetinaface::HJFaceDetectCoreMLRetinaface()
    : m_impl(std::make_unique<Impl>())
{
}

HJFaceDetectCoreMLRetinaface::~HJFaceDetectCoreMLRetinaface() = default;

int HJFaceDetectCoreMLRetinaface::init(HJBaseParam::Ptr params)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJBaseFaceDetect::init(params);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJBaseFaceDetect init error:{}", i_err);
            break;
        }
        m_impl->baseModelDir = toNSString(m_modelUrls);
        m_impl->computeMode = m_option ? m_option->coreMLRetinaFaceComputeMode : 0;
        if (m_impl->baseModelDir.length == 0)
        {
            i_err = HJErrInvalidParams;
            break;
        }
        HJFLogi("HJFaceDetectCoreMLRetinaface init modelDir:{} target:{} compute:{}",
                m_modelUrls,
                m_option ? m_option->retinaFaceTargetSize : 0,
                toStdString(computeModeText(m_impl->computeMode)));
    } while (false);
    return i_err;
}

int HJFaceDetectCoreMLRetinaface::ensureModelReady(int width, int height)
{
    if (!m_impl)
    {
        return HJErrNotInited;
    }
    if (m_impl->model)
    {
        if ((int)llround(m_impl->inputFixedSize.width) == width &&
            (int)llround(m_impl->inputFixedSize.height) == height)
        {
            return HJ_OK;
        }
    }

    NSError* error = nil;
    NSURL* modelURL = findCompiledModelURL(m_impl->baseModelDir, width, height);
    if (!modelURL)
    {
        NSURL* sourceURL = findSourceModelURL(m_impl->baseModelDir, width, height);
        if (sourceURL)
        {
            HJFLoge("CoreML retinaface requires precompiled .mlmodelc source:{} bundleDir:{}",
                    toStdString(sourceURL.path),
                    toStdString(retinaFaceCoreMLDir(m_impl->baseModelDir)));
        }
        else
        {
            HJFLoge("CoreML retinaface compiled model not found baseDir:{} input:{}x{} searchDir:{}",
                    toStdString(m_impl->baseModelDir),
                    width,
                    height,
                    toStdString(retinaFaceCoreMLDir(m_impl->baseModelDir)));
        }
        return HJErrInvalidFile;
    }

    MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
    config.computeUnits = computeUnitsForMode(m_impl->computeMode);
    MLModel* model = [MLModel modelWithContentsOfURL:modelURL configuration:config error:&error];
    if (!model || error)
    {
        HJFLoge("CoreML retinaface model load failed path:{} err:{}",
                toStdString(modelURL.path),
                toStdString(error.localizedDescription ?: @"unknown"));
        return HJErrNotInited;
    }

    NSString* inputName = @"";
    for (NSString* name in model.modelDescription.inputDescriptionsByName)
    {
        MLFeatureDescription* desc = model.modelDescription.inputDescriptionsByName[name];
        if (desc.type == MLFeatureTypeImage)
        {
            inputName = name;
            break;
        }
    }
    if (inputName.length == 0)
    {
        HJFLoge("CoreML retinaface no image input path:{}", toStdString(modelURL.path));
        return HJErrNotInited;
    }

    MLFeatureDescription* inDesc = model.modelDescription.inputDescriptionsByName[inputName];
    CGSize inputFixedSize = CGSizeZero;
    if (inDesc.type == MLFeatureTypeImage && inDesc.imageConstraint)
    {
        const NSInteger w = inDesc.imageConstraint.pixelsWide;
        const NSInteger h = inDesc.imageConstraint.pixelsHigh;
        if (w > 0 && h > 0)
        {
            inputFixedSize = CGSizeMake((CGFloat)w, (CGFloat)h);
        }
    }
    if (inputFixedSize.width >= 1.0 && inputFixedSize.height >= 1.0)
    {
        if ((int)llround(inputFixedSize.width) != width || (int)llround(inputFixedSize.height) != height)
        {
            HJFLoge("CoreML retinaface fixed model mismatch model:{} required:{}x{} input:{}x{}",
                    toStdString(modelURL.lastPathComponent),
                    (int)llround(inputFixedSize.width),
                    (int)llround(inputFixedSize.height),
                    width,
                    height);
            return HJErrInvalidParams;
        }
    }

    m_impl->model = model;
    m_impl->modelPath = modelURL.path;
    m_impl->inputName = inputName;
    m_impl->inputFixedSize = inputFixedSize;

    HJFLogi("CoreML retinaface ready model:{} input:{} compute:{} fixed:{}x{} outputs:{}",
            toStdString(m_impl->modelPath),
            toStdString(m_impl->inputName),
            toStdString(computeModeText(m_impl->computeMode)),
            (int)llround(m_impl->inputFixedSize.width),
            (int)llround(m_impl->inputFixedSize.height),
            toStdString([model.modelDescription.outputDescriptionsByName.allKeys componentsJoinedByString:@","]));
    return HJ_OK;
}

int HJFaceDetectCoreMLRetinaface::detect(std::shared_ptr<HJTransferMediaData> i_inputData, HJFaceDetectRet& o_ret)
{
    @autoreleasepool
    {
        int i_err = HJ_OK;
        if (!i_inputData)
        {
            return HJErrInvalidParams;
        }

        const int inputWidth = i_inputData->getWidth();
        const int inputHeight = i_inputData->getHeight();
        const int64_t inputTimestamp = i_inputData->getTimeStamp();
        if (inputWidth <= 0 || inputHeight <= 0)
        {
            return HJErrInvalidParams;
        }

        const int targetSize = m_option ? m_option->retinaFaceTargetSize : 0;
        int targetWidth = inputWidth;
        int targetHeight = inputHeight;
        if (targetSize > 0)
        {
            // CoreML RetinaFace uses fixed-size square models; keep preprocessing on a square canvas
            // and rely on fit+padding so box restoration stays equivalent to the NCNN path.
            targetWidth = targetSize;
            targetHeight = targetSize;
        }
        setTargetWidth(targetWidth);
        setTargetHeight(targetHeight);

        HJVec4i vec = { 0, 0, 0, 0 };
        const int64_t totalStartMs = HJCurrentSteadyMS();
        i_err = HJBaseFaceDetect::convert(i_inputData, HJDataScaleType::Fit, vec);
        if (i_err < 0)
        {
            HJFLoge("CoreML retinaface convert error:{}", i_err);
            return HJErrFatal;
        }
        const int scaleW = vec.x;
        const int scaleH = vec.y;
        const int offx = vec.z;
        const int offy = vec.w;

        i_err = ensureModelReady(m_targetWidth, m_targetHeight);
        if (i_err < 0)
        {
            return i_err;
        }

        CVPixelBufferRef pixel = newPixelBufferFromRGB(m_scaleRgb ? m_scaleRgb->getBuf() : nullptr, m_targetWidth, m_targetHeight);
        if (!pixel)
        {
            return HJErrFatal;
        }

        NSError* innerError = nil;
        MLFeatureValue* inputValue = [MLFeatureValue featureValueWithPixelBuffer:pixel];
        NSDictionary* dict = @{ m_impl->inputName : inputValue };
        id<MLFeatureProvider> provider = [[MLDictionaryFeatureProvider alloc] initWithDictionary:dict error:&innerError];
        if (!provider || innerError)
        {
            CFRelease(pixel);
            HJFLoge("CoreML retinaface create feature provider failed err:{}",
                    toStdString(innerError.localizedDescription ?: @"unknown"));
            return HJErrFatal;
        }

        const int64_t predictStartMs = HJCurrentSteadyMS();
        id<MLFeatureProvider> output = [m_impl->model predictionFromFeatures:provider error:&innerError];
        const int64_t predictElapsedMs = HJCurrentSteadyMS() - predictStartMs;
        CFRelease(pixel);
        if (!output || innerError)
        {
            HJFLoge("CoreML retinaface prediction failed err:{}",
                    toStdString(innerError.localizedDescription ?: @"unknown"));
            return HJErrFatal;
        }

        std::map<int, HJRetinaFaceFeatureSet> featureMap;
        for (NSString* name in m_impl->model.modelDescription.outputDescriptionsByName)
        {
            MLFeatureValue* featureValue = [output featureValueForName:name];
            if (!featureValue || featureValue.type != MLFeatureTypeMultiArray)
            {
                continue;
            }
            const int stride = parseStrideFromName(name);
            if (stride <= 0)
            {
                continue;
            }
            HJRetinaFaceTensor tensor;
            if (!parseMultiArrayAsCHW(featureValue.multiArrayValue, tensor))
            {
                continue;
            }
            NSString* lowerName = name.lowercaseString ?: @"";
            HJRetinaFaceFeatureSet& item = featureMap[stride];
            item.stride = stride;
            if ([lowerName containsString:@"landmark"])
            {
                item.landmark = std::move(tensor);
            }
            else if ([lowerName containsString:@"bbox"])
            {
                item.bbox = std::move(tensor);
            }
            else if ([lowerName containsString:@"cls"] || [lowerName containsString:@"score"] || [lowerName containsString:@"prob"])
            {
                reorderScoreTensorAnchorMajorToClassMajor(tensor);
                item.score = std::move(tensor);
            }
        }

        std::vector<HJRetinaFaceFeatureSet> features;
        std::string featureSummary;
        for (auto& entry : featureMap)
        {
            if (!entry.second.score.valid() || !entry.second.bbox.valid())
            {
                continue;
            }
            if (!featureSummary.empty())
            {
                featureSummary += ",";
            }
            featureSummary += describeFeatureSet(entry.second);
            features.push_back(std::move(entry.second));
        }
        std::sort(features.begin(), features.end(), [](const HJRetinaFaceFeatureSet& lhs, const HJRetinaFaceFeatureSet& rhs)
        {
            return lhs.stride > rhs.stride;
        });
        if (features.empty())
        {
            HJFLoge("CoreML retinaface no valid outputs model:{} outputs:{}",
                    toStdString(m_impl->modelPath),
                    toStdString([m_impl->model.modelDescription.outputDescriptionsByName.allKeys componentsJoinedByString:@","]));
            return HJErrFatal;
        }

        std::vector<HJRetinaFaceObject> faceobjects;
        i_err = HJRetinaFaceDecodeUtils::decodeWithPriors(features,
                                                          m_option ? m_option->ncnnRetinaFaceProbThreshold : 0.8f,
                                                          m_option ? m_option->ncnnRetinaFaceNmsThreshold : 0.4f,
                                                          m_targetWidth,
                                                          m_targetHeight,
                                                          faceobjects);
        if (i_err != 0)
        {
            HJFLoge("CoreML retinaface decode failed err:{} model:{}", i_err, toStdString(m_impl->modelPath));
            return HJErrFatal;
        }

        const int64_t totalElapsedMs = HJCurrentSteadyMS() - totalStartMs;
        o_ret.reset();
        o_ret.setSize(inputWidth, inputHeight);
        o_ret.m_elapseMs = totalElapsedMs;
        o_ret.m_systemTime = totalStartMs;
        o_ret.m_timeStamp = inputTimestamp;
        o_ret.m_bContainFace = !faceobjects.empty();
        for (const auto& face : faceobjects)
        {
            const float x0 = (face.x - offx) * inputWidth / scaleW;
            const float y0 = (face.y - offy) * inputHeight / scaleH;
            const float x1 = (face.x + face.w - offx) * inputWidth / scaleW;
            const float y1 = (face.y + face.h - offy) * inputHeight / scaleH;

            std::shared_ptr<HJSingleFaceDetectRet> singleFace = std::make_shared<HJSingleFaceDetectRet>();
            singleFace->setRect(x0, y0, x1 - x0, y1 - y0);
            if (face.hasLandmark)
            {
                for (int idx = 0; idx < 5; ++idx)
                {
                    const float kx = (face.landmark[idx][0] - offx) * inputWidth / scaleW;
                    const float ky = (face.landmark[idx][1] - offy) * inputHeight / scaleH;
                    singleFace->addKeyPoint(kx, ky);
                }
            }
            o_ret.add(singleFace);
        }
        HJFLogi("CoreML retinaface detect total:{} predict:{} input:{}x{} target:{}x{} scale:{}x{} off:{}x{} faces:{} compute:{} model:{}",
                totalElapsedMs,
                predictElapsedMs,
                inputWidth,
                inputHeight,
                m_targetWidth,
                m_targetHeight,
                scaleW,
                scaleH,
                offx,
                offy,
                (int)faceobjects.size(),
                toStdString(computeModeText(m_impl->computeMode)),
                toStdString(m_impl->modelPath));
        HJFLogi("CoreML retinaface feature summary:{}", featureSummary);
        HJFLogi("CoreML retinaface face samples:{}",
                describeFaceObjects(faceobjects, 3, offx, offy, scaleW, scaleH, inputWidth, inputHeight));
        return HJ_OK;
    }
}

NS_HJ_END
