#include "HJSRCoreMLRealESRGAN.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"
#include "libyuv.h"

#import <CoreImage/CoreImage.h>
#import <CoreML/CoreML.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

NS_HJ_BEGIN

namespace
{
enum CoreMLImageOutputExperiment
{
    CoreMLImageOutputExperiment_A_Default = 0,
    CoreMLImageOutputExperiment_B_NoExpand = 1,
    CoreMLImageOutputExperiment_C_AltChannelOrder = 2,
    CoreMLImageOutputExperiment_D_CIImageBridge = 3,
};

static constexpr CoreMLImageOutputExperiment kCoreMLImageOutputExperiment = CoreMLImageOutputExperiment_A_Default;

struct CoreMLPixelSample
{
    int x = 0;
    int y = 0;
    unsigned char c0 = 0;
    unsigned char c1 = 0;
    unsigned char c2 = 0;
    unsigned char c3 = 0;
};

struct CoreMLPixelBufferAnalysis
{
    bool valid = false;
    bool lowRange = false;
    int maxRGB = 0;
    int width = 0;
    int height = 0;
    int stride = 0;
    OSType pixelFormat = 0;
    std::vector<CoreMLPixelSample> samples;
};

static NSString* fourCCText(OSType value)
{
    char chars[5] = {
        (char)((value >> 24) & 0xFF),
        (char)((value >> 16) & 0xFF),
        (char)((value >> 8) & 0xFF),
        (char)(value & 0xFF),
        0
    };
    for (int i = 0; i < 4; ++i)
    {
        if ((unsigned char)chars[i] < 32 || (unsigned char)chars[i] > 126)
        {
            chars[i] = '.';
        }
    }
    return [NSString stringWithUTF8String:chars];
}

static NSString* attachmentString(CVPixelBufferRef buffer, CFStringRef key)
{
    if (!buffer || !key)
    {
        return @"";
    }
    CFTypeRef value = CVBufferGetAttachment(buffer, key, nullptr);
    if (!value)
    {
        return @"";
    }
    return [NSString stringWithFormat:@"%@", (__bridge id)value];
}

static CIContext* sharedCoreMLImageCIContext()
{
    static CIContext *context = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSDictionary *options = @{
            kCIContextUseSoftwareRenderer : @NO,
            kCIContextPriorityRequestLow : @YES,
        };
        context = [CIContext contextWithOptions:options];
    });
    return context;
}

static inline float HJHalfToFloat(uint16_t h)
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
            const uint32_t exp32 = exp + (127 - 15);
            out = sign | (exp32 << 23) | (mant << 13);
        }
    }
    else if (exp == 0x1F)
    {
        out = sign | 0x7F800000 | (mant << 13);
    }
    else
    {
        const uint32_t exp32 = exp + (127 - 15);
        out = sign | (exp32 << 23) | (mant << 13);
    }
    float f = 0.0f;
    memcpy(&f, &out, sizeof(float));
    return f;
}

static NSString* toNSString(const std::string& value)
{
    return [NSString stringWithUTF8String:value.c_str()];
}

static std::string toStdString(NSString* value)
{
    return value.length > 0 ? std::string(value.UTF8String) : std::string();
}

static NSString* featureTypeText(MLFeatureType type)
{
    switch (type)
    {
        case MLFeatureTypeImage: return @"image";
        case MLFeatureTypeMultiArray: return @"multiArray";
        case MLFeatureTypeDictionary: return @"dictionary";
        case MLFeatureTypeInt64: return @"int64";
        case MLFeatureTypeDouble: return @"double";
        case MLFeatureTypeString: return @"string";
        default: return @"unknown";
    }
}

static NSString* computeModeText(int mode)
{
    switch (mode)
    {
        case 0: return @"All";
        case 1: return @"CPU+GPU";
        case 2: return @"CPU+ANE";
        case 3: return @"CPUOnly";
        default: return @"Unknown";
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

static NSString* firstImageFeatureName(NSDictionary<NSString *, MLFeatureDescription *> *descriptions)
{
    for (NSString *name in descriptions.allKeys)
    {
        MLFeatureDescription *desc = descriptions[name];
        if (desc.type == MLFeatureTypeImage)
        {
            return name;
        }
    }
    return @"";
}

static NSString* firstMultiArrayFeatureName(NSDictionary<NSString *, MLFeatureDescription *> *descriptions)
{
    for (NSString *name in descriptions.allKeys)
    {
        MLFeatureDescription *desc = descriptions[name];
        if (desc.type == MLFeatureTypeMultiArray)
        {
            return name;
        }
    }
    return @"";
}

static NSArray<NSString *> *buildModelCandidates(NSString *preferredName, int width, int height)
{
    NSMutableOrderedSet<NSString *> *candidates = [NSMutableOrderedSet orderedSet];
    if (preferredName.length > 0)
    {
        [candidates addObject:preferredName];
    }
    if (width > 0 && height > 0)
    {
        [candidates addObject:[NSString stringWithFormat:@"realesr-general-merge05-x4v3-fixed-%dx%d-image-ios16", width, height]];
        [candidates addObject:[NSString stringWithFormat:@"realesr-general-merge05-x4v3-fixed-%dx%d-ios15", width, height]];
        [candidates addObject:[NSString stringWithFormat:@"realesr-general-merge05-x4v3-fixed-%dx%d-ios16", width, height]];
        if (width == 360 && height == 640)
        {
            [candidates addObject:@"realesr-general-merge05-x4v3-fixed-360x640-neural-ios14-int8"];
        }
    }
    [candidates addObjectsFromArray:@[
        @"realesr-general-merge05-x4v3-dynamic-tensor-ios15",
        @"realesr-general-merge05-x4v3-dynamic-tensor",
        @"realesr-general-merge05-x4v3"
    ]];
    return candidates.array;
}

static NSArray<NSString *> *coreMLModelSearchDirs(NSString *baseModelDir)
{
    if (baseModelDir.length == 0)
    {
        return @[];
    }
    NSString *realesrDir = [baseModelDir stringByAppendingPathComponent:@"CoreML/realesr"];
    return @[
        [realesrDir stringByAppendingPathComponent:@"fullScale"],
        [realesrDir stringByAppendingPathComponent:@"faceScale"],
        realesrDir
    ];
}

static NSURL* findCompiledModelURL(NSString *baseModelDir, NSString *preferredName, int width, int height)
{
    if (baseModelDir.length == 0)
    {
        return nil;
    }
    NSFileManager *fm = [NSFileManager defaultManager];
    for (NSString *coreMLDir in coreMLModelSearchDirs(baseModelDir))
    {
        for (NSString *name in buildModelCandidates(preferredName, width, height))
        {
            NSString *path = [coreMLDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.%@", name, @"mlmodelc"]];
            if ([fm fileExistsAtPath:path])
            {
                return [NSURL fileURLWithPath:path];
            }
        }
    }
    return nil;
}

static NSURL* findSourceModelURL(NSString *baseModelDir, NSString *preferredName, int width, int height)
{
    if (baseModelDir.length == 0)
    {
        return nil;
    }
    NSFileManager *fm = [NSFileManager defaultManager];
    for (NSString *coreMLDir in coreMLModelSearchDirs(baseModelDir))
    {
        for (NSString *name in buildModelCandidates(preferredName, width, height))
        {
            for (NSString *ext in @[ @"mlpackage", @"mlmodel" ])
            {
                NSString *path = [coreMLDir stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.%@", name, ext]];
                if ([fm fileExistsAtPath:path])
                {
                    return [NSURL fileURLWithPath:path];
                }
            }
        }
    }
    return nil;
}

static CVPixelBufferRef newPixelBufferFromRGB(const unsigned char *rgb, int width, int height)
{
    if (!rgb || width <= 0 || height <= 0)
    {
        return NULL;
    }
    NSDictionary *attrs = @{
        (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
        (NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
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
    const int64_t convertStartMs = HJCurrentSteadyMS();
    CVPixelBufferLockBaseAddress(buffer, 0);
    unsigned char *base = (unsigned char *)CVPixelBufferGetBaseAddress(buffer);
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

static CoreMLPixelBufferAnalysis analyzePixelBuffer(CVPixelBufferRef buffer)
{
    CoreMLPixelBufferAnalysis analysis;
    if (!buffer)
    {
        return analysis;
    }
    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    const int width = (int)CVPixelBufferGetWidth(buffer);
    const int height = (int)CVPixelBufferGetHeight(buffer);
    const int stride = (int)CVPixelBufferGetBytesPerRow(buffer);
    const unsigned char *base = (const unsigned char *)CVPixelBufferGetBaseAddress(buffer);
    if (!base || width <= 0 || height <= 0 || stride < width * 4)
    {
        CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        return analysis;
    }
    analysis.valid = true;
    analysis.width = width;
    analysis.height = height;
    analysis.stride = stride;
    analysis.pixelFormat = CVPixelBufferGetPixelFormatType(buffer);
    int maxRGB = 0;
    const int stepY = (height > 512) ? 8 : 4;
    const int stepX = (width > 512) ? 8 : 4;
    for (int y = 0; y < height && maxRGB <= 2; y += stepY)
    {
        const unsigned char *row = base + (size_t)y * (size_t)stride;
        for (int x = 0; x < width; x += stepX)
        {
            const unsigned char *p = row + (size_t)x * 4;
            maxRGB = (std::max)(maxRGB, (int)p[0]);
            maxRGB = (std::max)(maxRGB, (int)p[1]);
            maxRGB = (std::max)(maxRGB, (int)p[2]);
            if (maxRGB > 2)
            {
                break;
            }
        }
    }
    std::vector<std::pair<int, int>> samplePoints = {
        { 0, 0 },
        { width / 2, height / 2 },
        { (std::max)(0, width - 1), (std::max)(0, height - 1) },
    };
    for (const auto& pt : samplePoints)
    {
        const unsigned char *p = base + (size_t)pt.second * (size_t)stride + (size_t)pt.first * 4;
        CoreMLPixelSample sample;
        sample.x = pt.first;
        sample.y = pt.second;
        sample.c0 = p[0];
        sample.c1 = p[1];
        sample.c2 = p[2];
        sample.c3 = p[3];
        analysis.samples.push_back(sample);
    }
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    analysis.maxRGB = maxRGB;
    analysis.lowRange = (maxRGB <= 2);
    return analysis;
}

static CVPixelBufferRef expandLowRangeRGBIfNeeded(CVPixelBufferRef buffer, const CoreMLPixelBufferAnalysis& analysis)
{
    if (!buffer)
    {
        return NULL;
    }
    if (!analysis.valid || !analysis.lowRange)
    {
        return (CVPixelBufferRef)CFRetain(buffer);
    }
    if (analysis.pixelFormat != kCVPixelFormatType_32BGRA)
    {
        return (CVPixelBufferRef)CFRetain(buffer);
    }
    NSDictionary *attrs = @{
        (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
        (NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
    };
    CVPixelBufferRef dst = NULL;
    CVReturn ret = CVPixelBufferCreate(kCFAllocatorDefault,
                                       CVPixelBufferGetWidth(buffer),
                                       CVPixelBufferGetHeight(buffer),
                                       kCVPixelFormatType_32BGRA,
                                       (__bridge CFDictionaryRef)attrs,
                                       &dst);
    if (ret != kCVReturnSuccess || !dst)
    {
        return (CVPixelBufferRef)CFRetain(buffer);
    }
    const int64_t expandStartMs = HJCurrentSteadyMS();
    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    CVPixelBufferLockBaseAddress(dst, 0);
    const int width = (int)CVPixelBufferGetWidth(buffer);
    const int height = (int)CVPixelBufferGetHeight(buffer);
    const int srcStride = (int)CVPixelBufferGetBytesPerRow(buffer);
    const int dstStride = (int)CVPixelBufferGetBytesPerRow(dst);
    const unsigned char *srcBase = (const unsigned char *)CVPixelBufferGetBaseAddress(buffer);
    unsigned char *dstBase = (unsigned char *)CVPixelBufferGetBaseAddress(dst);
    for (int y = 0; y < height; ++y)
    {
        const unsigned char *srow = srcBase + (size_t)y * (size_t)srcStride;
        unsigned char *drow = dstBase + (size_t)y * (size_t)dstStride;
        for (int x = 0; x < width; ++x)
        {
            const unsigned char *s = srow + (size_t)x * 4;
            unsigned char *d = drow + (size_t)x * 4;
            d[0] = (unsigned char)((int)s[0] * 255 > 255 ? 255 : (int)s[0] * 255);
            d[1] = (unsigned char)((int)s[1] * 255 > 255 ? 255 : (int)s[1] * 255);
            d[2] = (unsigned char)((int)s[2] * 255 > 255 ? 255 : (int)s[2] * 255);
            d[3] = s[3];
        }
    }
    CVPixelBufferUnlockBaseAddress(dst, 0);
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    return dst;
}

static void copyBGRABytesToRGB(const unsigned char *base, int width, int height, int stride, unsigned char *dstRgb, bool altChannelOrder)
{
    for (int y = 0; y < height; ++y)
    {
        const unsigned char *srcRow = base + (size_t)y * (size_t)stride;
        unsigned char *dstRow = dstRgb + (size_t)y * (size_t)width * 3;
        for (int x = 0; x < width; ++x)
        {
            const unsigned char *s = srcRow + (size_t)x * 4;
            unsigned char *d = dstRow + (size_t)x * 3;
            if (altChannelOrder)
            {
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
            else
            {
                d[0] = s[2];
                d[1] = s[1];
                d[2] = s[0];
            }
        }
    }
}

static int copyPixelBufferToRGBViaCIContext(CVPixelBufferRef buffer, HJSPBuffer::Ptr& outRgb, int& outWidth, int& outHeight)
{
    if (!buffer)
    {
        return HJErrInvalidParams;
    }
    outWidth = (int)CVPixelBufferGetWidth(buffer);
    outHeight = (int)CVPixelBufferGetHeight(buffer);
    if (outWidth <= 0 || outHeight <= 0)
    {
        return HJErrInvalidParams;
    }
    NSDictionary *attrs = @{
        (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
        (NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
    };
    CVPixelBufferRef bridge = NULL;
    CVReturn ret = CVPixelBufferCreate(kCFAllocatorDefault,
                                       outWidth,
                                       outHeight,
                                       kCVPixelFormatType_32BGRA,
                                       (__bridge CFDictionaryRef)attrs,
                                       &bridge);
    if (ret != kCVReturnSuccess || !bridge)
    {
        return HJErrFatal;
    }
    const int64_t renderStartMs = HJCurrentSteadyMS();
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:buffer];
    CGRect rect = CGRectMake(0, 0, outWidth, outHeight);
    CIContext *context = sharedCoreMLImageCIContext();
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    [context render:ciImage toCVPixelBuffer:bridge bounds:rect colorSpace:colorSpace];
    CGColorSpaceRelease(colorSpace);
    CoreMLPixelBufferAnalysis analysis = analyzePixelBuffer(bridge);
    outRgb = HJSPBuffer::create((size_t)outWidth * (size_t)outHeight * 3);
    if (!outRgb || !outRgb->getBuf())
    {
        CFRelease(bridge);
        return HJErrFatal;
    }
    CVPixelBufferLockBaseAddress(bridge, kCVPixelBufferLock_ReadOnly);
    const unsigned char *base = (const unsigned char *)CVPixelBufferGetBaseAddress(bridge);
    if (!base)
    {
        CVPixelBufferUnlockBaseAddress(bridge, kCVPixelBufferLock_ReadOnly);
        CFRelease(bridge);
        return HJErrFatal;
    }
    copyBGRABytesToRGB(base, outWidth, outHeight, analysis.stride, outRgb->getBuf(), false);
    CVPixelBufferUnlockBaseAddress(bridge, kCVPixelBufferLock_ReadOnly);
    CFRelease(bridge);
    return HJ_OK;
}

static int copyPixelBufferToRGB(CVPixelBufferRef buffer, HJSPBuffer::Ptr& outRgb, int& outWidth, int& outHeight, CoreMLImageOutputExperiment experiment)
{
    if (!buffer)
    {
        return HJErrInvalidParams;
    }
    const int64_t totalStartMs = HJCurrentSteadyMS();
    if (experiment == CoreMLImageOutputExperiment_D_CIImageBridge)
    {
        const int ciRet = copyPixelBufferToRGBViaCIContext(buffer, outRgb, outWidth, outHeight);
        return ciRet;
    }
    const CoreMLPixelBufferAnalysis sourceAnalysis = analyzePixelBuffer(buffer);
    const bool allowExpand = (experiment != CoreMLImageOutputExperiment_B_NoExpand);
    const int64_t expandStartMs = HJCurrentSteadyMS();
    CVPixelBufferRef expanded = allowExpand ? expandLowRangeRGBIfNeeded(buffer, sourceAnalysis) : (CVPixelBufferRef)CFRetain(buffer);
    const int64_t expandElapsedMs = HJCurrentSteadyMS() - expandStartMs;
    CVPixelBufferRef useBuffer = expanded ? expanded : buffer;
    const int64_t lockStartMs = HJCurrentSteadyMS();
    CVPixelBufferLockBaseAddress(useBuffer, kCVPixelBufferLock_ReadOnly);
    outWidth = (int)CVPixelBufferGetWidth(useBuffer);
    outHeight = (int)CVPixelBufferGetHeight(useBuffer);
    const int stride = (int)CVPixelBufferGetBytesPerRow(useBuffer);
    const OSType pixelFormat = CVPixelBufferGetPixelFormatType(useBuffer);
    const unsigned char *base = (const unsigned char *)CVPixelBufferGetBaseAddress(useBuffer);
    const int64_t lockElapsedMs = HJCurrentSteadyMS() - lockStartMs;
    if (!base || outWidth <= 0 || outHeight <= 0)
    {
        CVPixelBufferUnlockBaseAddress(useBuffer, kCVPixelBufferLock_ReadOnly);
        if (expanded) CFRelease(expanded);
        return HJErrFatal;
    }
    bool altChannelOrder = (experiment == CoreMLImageOutputExperiment_C_AltChannelOrder);
    outRgb = HJSPBuffer::create((size_t)outWidth * (size_t)outHeight * 3);
    if (!outRgb || !outRgb->getBuf())
    {
        CVPixelBufferUnlockBaseAddress(useBuffer, kCVPixelBufferLock_ReadOnly);
        if (expanded) CFRelease(expanded);
        return HJErrFatal;
    }
    const int64_t convertStartMs = HJCurrentSteadyMS();
    switch (pixelFormat)
    {
        case kCVPixelFormatType_32BGRA:
            copyBGRABytesToRGB(base, outWidth, outHeight, stride, outRgb->getBuf(), altChannelOrder);
            break;
        default:
            CVPixelBufferUnlockBaseAddress(useBuffer, kCVPixelBufferLock_ReadOnly);
            if (expanded) CFRelease(expanded);
            HJFLoge("CoreML unsupported image pixel format:{}('{}') experiment:{}",
                    (uint32_t)pixelFormat,
                    toStdString(fourCCText(pixelFormat)),
                    (int)experiment);
            return HJErrInvalidParams;
    }
    const int64_t convertElapsedMs = HJCurrentSteadyMS() - convertStartMs;
    const int64_t unlockStartMs = HJCurrentSteadyMS();
    CVPixelBufferUnlockBaseAddress(useBuffer, kCVPixelBufferLock_ReadOnly);
    const int64_t unlockElapsedMs = HJCurrentSteadyMS() - unlockStartMs;
    if (expanded) CFRelease(expanded);
    return HJ_OK;
}

static int copyPixelBufferToRGBA(CVPixelBufferRef buffer, HJSPBuffer::Ptr& outRgba, int& outWidth, int& outHeight)
{
    if (!buffer)
    {
        return HJErrInvalidParams;
    }

    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    outWidth = (int)CVPixelBufferGetWidth(buffer);
    outHeight = (int)CVPixelBufferGetHeight(buffer);
    const int stride = (int)CVPixelBufferGetBytesPerRow(buffer);
    const OSType pixelFormat = CVPixelBufferGetPixelFormatType(buffer);
    const unsigned char *base = (const unsigned char *)CVPixelBufferGetBaseAddress(buffer);
    if (!base || outWidth <= 0 || outHeight <= 0)
    {
        CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        return HJErrFatal;
    }

    outRgba = HJSPBuffer::create((size_t)outWidth * (size_t)outHeight * 4);
    if (!outRgba || !outRgba->getBuf())
    {
        CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        return HJErrFatal;
    }

    int ret = HJ_OK;
    switch (pixelFormat)
    {
        case kCVPixelFormatType_32BGRA:
            ret = libyuv::ARGBToABGR(base, stride, outRgba->getBuf(), outWidth * 4, outWidth, outHeight);
            break;
        default:
            HJFLoge("CoreML unsupported rgba pixel format:{}('{}')",
                    (uint32_t)pixelFormat,
                    toStdString(fourCCText(pixelFormat)));
            ret = HJErrInvalidParams;
            break;
    }

    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    if (ret != 0)
    {
        outRgba = nullptr;
        return (ret == HJErrInvalidParams) ? ret : HJErrFatal;
    }
    return HJ_OK;
}

static int copyMultiArrayToRGB(MLMultiArray *arr, HJSPBuffer::Ptr& outRgb, int& outWidth, int& outHeight)
{
    if (!arr || arr.shape.count < 3)
    {
        return HJErrInvalidParams;
    }
    NSInteger n = 1;
    NSInteger c = 0;
    NSInteger h = 0;
    NSInteger w = 0;
    if (arr.shape.count == 4)
    {
        n = arr.shape[0].integerValue;
        c = arr.shape[1].integerValue;
        h = arr.shape[2].integerValue;
        w = arr.shape[3].integerValue;
    }
    else if (arr.shape.count == 3)
    {
        c = arr.shape[0].integerValue;
        h = arr.shape[1].integerValue;
        w = arr.shape[2].integerValue;
    }
    if (n != 1 || c < 3 || h <= 0 || w <= 0)
    {
        return HJErrInvalidParams;
    }

    NSInteger sN = 0;
    NSInteger sC = 0;
    NSInteger sH = 0;
    NSInteger sW = 0;
    if (arr.strides.count == 4)
    {
        sN = arr.strides[0].integerValue;
        sC = arr.strides[1].integerValue;
        sH = arr.strides[2].integerValue;
        sW = arr.strides[3].integerValue;
    }
    else if (arr.strides.count == 3)
    {
        sC = arr.strides[0].integerValue;
        sH = arr.strides[1].integerValue;
        sW = arr.strides[2].integerValue;
    }
    else
    {
        return HJErrInvalidParams;
    }

    double *dp = (double *)arr.dataPointer;
    float *fp32 = (float *)arr.dataPointer;
    uint16_t *fp16 = (uint16_t *)arr.dataPointer;
    BOOL isDouble = (arr.dataType == MLMultiArrayDataTypeDouble);
    BOOL isFloat32 = (arr.dataType == MLMultiArrayDataTypeFloat32);
    BOOL isFloat16 = NO;
    if (@available(iOS 16.0, *))
    {
        isFloat16 = (arr.dataType == MLMultiArrayDataTypeFloat16);
    }
    if (!isDouble && !isFloat32 && !isFloat16)
    {
        return HJErrInvalidParams;
    }

    const int64_t totalStartMs = HJCurrentSteadyMS();
    double sampleMaxAbs = 0.0;
    const NSInteger sampleStepY = h > 64 ? (h / 32) : 1;
    const NSInteger sampleStepX = w > 64 ? (w / 32) : 1;
    for (NSInteger y = 0; y < h; y += sampleStepY)
    {
        for (NSInteger x = 0; x < w; x += sampleStepX)
        {
            NSInteger base = sN * 0 + sH * y + sW * x;
            double rv = 0.0, gv = 0.0, bv = 0.0;
            if (isDouble)
            {
                rv = dp[base + sC * 0];
                gv = dp[base + sC * 1];
                bv = dp[base + sC * 2];
            }
            else if (isFloat32)
            {
                rv = fp32[base + sC * 0];
                gv = fp32[base + sC * 1];
                bv = fp32[base + sC * 2];
            }
            else
            {
                rv = HJHalfToFloat(fp16[base + sC * 0]);
                gv = HJHalfToFloat(fp16[base + sC * 1]);
                bv = HJHalfToFloat(fp16[base + sC * 2]);
            }
            sampleMaxAbs = (std::max)(sampleMaxAbs, std::fabs(rv));
            sampleMaxAbs = (std::max)(sampleMaxAbs, std::fabs(gv));
            sampleMaxAbs = (std::max)(sampleMaxAbs, std::fabs(bv));
        }
    }
    const int64_t sampleElapsedMs = HJCurrentSteadyMS() - totalStartMs;
    const double scaleFactor = (sampleMaxAbs <= 4.0) ? 255.0 : 1.0;
    outWidth = (int)w;
    outHeight = (int)h;
    outRgb = HJSPBuffer::create((size_t)outWidth * (size_t)outHeight * 3);
    if (!outRgb || !outRgb->getBuf())
    {
        return HJErrFatal;
    }
    const int64_t convertStartMs = HJCurrentSteadyMS();
    for (NSInteger y = 0; y < h; ++y)
    {
        for (NSInteger x = 0; x < w; ++x)
        {
            NSInteger base = sN * 0 + sH * y + sW * x;
            double rv = 0.0, gv = 0.0, bv = 0.0;
            if (isDouble)
            {
                rv = dp[base + sC * 0];
                gv = dp[base + sC * 1];
                bv = dp[base + sC * 2];
            }
            else if (isFloat32)
            {
                rv = fp32[base + sC * 0];
                gv = fp32[base + sC * 1];
                bv = fp32[base + sC * 2];
            }
            else
            {
                rv = HJHalfToFloat(fp16[base + sC * 0]);
                gv = HJHalfToFloat(fp16[base + sC * 1]);
                bv = HJHalfToFloat(fp16[base + sC * 2]);
            }
            if (!std::isfinite(rv)) rv = 0.0;
            if (!std::isfinite(gv)) gv = 0.0;
            if (!std::isfinite(bv)) bv = 0.0;
            rv = (std::min)(255.0, (std::max)(0.0, rv * scaleFactor));
            gv = (std::min)(255.0, (std::max)(0.0, gv * scaleFactor));
            bv = (std::min)(255.0, (std::max)(0.0, bv * scaleFactor));
            unsigned char *dst = outRgb->getBuf() + ((size_t)y * (size_t)outWidth + (size_t)x) * 3;
            dst[0] = (unsigned char)llround(rv);
            dst[1] = (unsigned char)llround(gv);
            dst[2] = (unsigned char)llround(bv);
        }
    }
    return HJ_OK;
}
} // namespace

struct HJSRCoreMLRealESRGAN::Impl
{
    MLModel *model = nil;
    NSString *baseModelDir = nil;
    NSString *preferredModelName = nil;
    NSString *modelPath = nil;
    NSString *inputName = nil;
    NSString *outputName = nil;
    CGSize inputFixedSize = CGSizeZero;
    bool dynamicInputEnabled = false;
    int computeMode = 0;
};

HJSRCoreMLRealESRGAN::HJSRCoreMLRealESRGAN()
    : m_impl(std::make_unique<Impl>())
{
}

HJSRCoreMLRealESRGAN::~HJSRCoreMLRealESRGAN() = default;

int HJSRCoreMLRealESRGAN::init(HJBaseParam::Ptr params)
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
        m_impl->baseModelDir = toNSString(m_modelUrls);
        m_impl->preferredModelName = toNSString(m_option ? m_option->coreMLModelName : std::string());
        m_impl->computeMode = m_option ? m_option->coreMLComputeMode : 0;
        if (m_impl->baseModelDir.length == 0)
        {
            i_err = HJErrInvalidParams;
            break;
        }
        HJFLogi("HJSRCoreMLRealESRGAN init modelDir:{} preferred:{} compute:{}",
                m_modelUrls,
                m_option ? m_option->coreMLModelName : std::string(),
                m_impl->computeMode);
    } while (false);
    return i_err;
}

int HJSRCoreMLRealESRGAN::ensureModelReady(int width, int height)
{
    if (!m_impl)
    {
        return HJErrNotInited;
    }
    if (m_impl->model)
    {
        if (m_impl->dynamicInputEnabled)
        {
            return HJ_OK;
        }
        if ((int)llround(m_impl->inputFixedSize.width) == width && (int)llround(m_impl->inputFixedSize.height) == height)
        {
            return HJ_OK;
        }
        if (m_impl->preferredModelName.length > 0)
        {
            HJFLoge("CoreML fixed model mismatch preferred:{} model:{}x{} input:{}x{}",
                    toStdString(m_impl->preferredModelName),
                    (int)llround(m_impl->inputFixedSize.width),
                    (int)llround(m_impl->inputFixedSize.height),
                    width,
                    height);
            return HJErrInvalidParams;
        }
    }

    NSError *error = nil;
    NSURL *modelURL = findCompiledModelURL(m_impl->baseModelDir, m_impl->preferredModelName, width, height);
    if (!modelURL)
    {
        NSURL *sourceURL = findSourceModelURL(m_impl->baseModelDir, m_impl->preferredModelName, width, height);
        if (sourceURL)
        {
            HJFLoge("CoreML requires precompiled .mlmodelc to avoid runtime compile memory spikes source:{} bundleDir:{}",
                    toStdString(sourceURL.path),
                    toStdString([m_impl->baseModelDir stringByAppendingPathComponent:@"CoreML/realesr"]));
        }
        else
        {
            HJFLoge("CoreML compiled model not found baseDir:{} preferred:{} input:{}x{}",
                    toStdString(m_impl->baseModelDir),
                    toStdString(m_impl->preferredModelName),
                    width,
                    height);
        }
        return HJErrInvalidFile;
    }

    MLModelConfiguration *config = [[MLModelConfiguration alloc] init];
    config.computeUnits = computeUnitsForMode(m_impl->computeMode);
    MLModel *model = [MLModel modelWithContentsOfURL:modelURL configuration:config error:&error];
    if (!model || error)
    {
        HJFLoge("CoreML model load failed path:{} err:{}",
                toStdString(modelURL.path),
                toStdString(error.localizedDescription ?: @"unknown"));
        return HJErrNotInited;
    }

    NSString *inputName = firstImageFeatureName(model.modelDescription.inputDescriptionsByName);
    if (inputName.length == 0)
    {
        HJFLoge("CoreML no image input path:{}", toStdString(modelURL.path));
        return HJErrNotInited;
    }
    NSString *outputName = firstImageFeatureName(model.modelDescription.outputDescriptionsByName);
    if (outputName.length == 0)
    {
        outputName = firstMultiArrayFeatureName(model.modelDescription.outputDescriptionsByName);
    }
    if (outputName.length == 0)
    {
        outputName = model.modelDescription.outputDescriptionsByName.allKeys.firstObject ?: @"";
    }
    if (outputName.length == 0)
    {
        HJFLoge("CoreML no output feature path:{}", toStdString(modelURL.path));
        return HJErrNotInited;
    }

    CGSize inputFixedSize = CGSizeZero;
    MLFeatureDescription *inDesc = model.modelDescription.inputDescriptionsByName[inputName];
    if (inDesc.type == MLFeatureTypeImage && inDesc.imageConstraint)
    {
        NSInteger w = inDesc.imageConstraint.pixelsWide;
        NSInteger h = inDesc.imageConstraint.pixelsHigh;
        if (w > 0 && h > 0)
        {
            inputFixedSize = CGSizeMake((CGFloat)w, (CGFloat)h);
        }
    }
    const bool dynamicInputEnabled = ([modelURL.path rangeOfString:@"dynamic" options:NSCaseInsensitiveSearch].location != NSNotFound);
    if (!dynamicInputEnabled && inputFixedSize.width >= 1.0 && inputFixedSize.height >= 1.0)
    {
        if ((int)llround(inputFixedSize.width) != width || (int)llround(inputFixedSize.height) != height)
        {
            HJFLoge("CoreML fixed model mismatch model:{} required:{}x{} input:{}x{}",
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
    m_impl->outputName = outputName;
    m_impl->inputFixedSize = inputFixedSize;
    m_impl->dynamicInputEnabled = dynamicInputEnabled;

    MLFeatureDescription *outDesc = model.modelDescription.outputDescriptionsByName[outputName];
    HJFLogi("CoreML ready model:{} input:{} output:{} type:{} compute:{} dynamic:{} fixed:{}x{}",
            toStdString(m_impl->modelPath),
            toStdString(m_impl->inputName),
            toStdString(m_impl->outputName),
            toStdString(featureTypeText(outDesc ? outDesc.type : MLFeatureTypeInvalid)),
            toStdString(computeModeText(m_impl->computeMode)),
            dynamicInputEnabled,
            (int)llround(inputFixedSize.width),
            (int)llround(inputFixedSize.height));
    return HJ_OK;
}

int HJSRCoreMLRealESRGAN::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
{
    @autoreleasepool
    {
        const int64_t totalStartMs = HJCurrentSteadyMS();
        if (!i_inputData)
        {
            return HJErrInvalidParams;
        }

        int i_width = 0;
        int i_height = 0;
        int64_t i_timestamp = 0;
        const unsigned char *inputRgb = nullptr;
        const int64_t prepareStartMs = HJCurrentSteadyMS();
        int i_err = prepareInputRgb(i_inputData, inputRgb, i_width, i_height, i_timestamp, "HJSRCoreMLRealESRGAN");
        const int64_t prepareElapsedMs = HJCurrentSteadyMS() - prepareStartMs;
        if (i_err < 0)
        {
            return i_err;
        }

        const int64_t ensureStartMs = HJCurrentSteadyMS();
        i_err = ensureModelReady(i_width, i_height);
        const int64_t ensureElapsedMs = HJCurrentSteadyMS() - ensureStartMs;
        if (i_err < 0)
        {
            return i_err;
        }

        const int64_t pixelStartMs = HJCurrentSteadyMS();
        CVPixelBufferRef pixel = newPixelBufferFromRGB(inputRgb, i_width, i_height);
        const int64_t pixelElapsedMs = HJCurrentSteadyMS() - pixelStartMs;
        if (!pixel)
        {
            return HJErrFatal;
        }

        NSError *innerError = nil;
        const int64_t providerStartMs = HJCurrentSteadyMS();
        MLFeatureValue *inputValue = [MLFeatureValue featureValueWithPixelBuffer:pixel];
        NSDictionary *dict = @{ m_impl->inputName : inputValue };
        id<MLFeatureProvider> provider = [[MLDictionaryFeatureProvider alloc] initWithDictionary:dict error:&innerError];
        const int64_t providerElapsedMs = HJCurrentSteadyMS() - providerStartMs;
        if (!provider || innerError)
        {
            CFRelease(pixel);
            HJFLoge("CoreML create feature provider failed err:{}", toStdString(innerError.localizedDescription ?: @"unknown"));
            return HJErrFatal;
        }

        const int64_t t0 = HJCurrentSteadyMS();
        id<MLFeatureProvider> output = [m_impl->model predictionFromFeatures:provider error:&innerError];
        const int64_t t1 = HJCurrentSteadyMS();
        CFRelease(pixel);
        if (!output || innerError)
        {
            HJFLoge("CoreML prediction failed err:{}", toStdString(innerError.localizedDescription ?: @"unknown"));
            return HJErrFatal;
        }

        CVPixelBufferRef outPixel = NULL;
        MLMultiArray *outArray = nil;
        if (m_impl->outputName.length > 0)
        {
            MLFeatureValue *fv = [output featureValueForName:m_impl->outputName];
            if (fv.type == MLFeatureTypeImage)
            {
                outPixel = fv.imageBufferValue;
            }
            else if (fv.type == MLFeatureTypeMultiArray)
            {
                outArray = fv.multiArrayValue;
            }
        }
        if (!outPixel && !outArray)
        {
            for (NSString *name in m_impl->model.modelDescription.outputDescriptionsByName.allKeys)
            {
                MLFeatureValue *fv = [output featureValueForName:name];
                if (fv.type == MLFeatureTypeImage)
                {
                    outPixel = fv.imageBufferValue;
                    break;
                }
                if (fv.type == MLFeatureTypeMultiArray)
                {
                    outArray = fv.multiArrayValue;
                    break;
                }
            }
        }

        HJSPBuffer::Ptr outRgb = nullptr;
        HJSPBuffer::Ptr outRgba = nullptr;
        int outWidth = 0;
        int outHeight = 0;
        const int64_t outputConvertStartMs = HJCurrentSteadyMS();
        if (outPixel)
        {
            i_err = copyPixelBufferToRGBA(outPixel, outRgba, outWidth, outHeight);
        }
        else if (outArray)
        {
            i_err = copyMultiArrayToRGB(outArray, outRgb, outWidth, outHeight);
        }
        else
        {
            i_err = HJErrFatal;
        }
        const int64_t outputConvertElapsedMs = HJCurrentSteadyMS() - outputConvertStartMs;
        if (i_err < 0 || (!outRgba && !outRgb) || outWidth <= 0 || outHeight <= 0)
        {
            HJFLoge("CoreML output convert failed model:{}", toStdString(m_impl->modelPath));
            return (i_err < 0) ? i_err : HJErrFatal;
        }

        o_ret.m_elapseMs = t1 - t0;
        o_ret.m_systemTime = t0;
        o_ret.m_timeStamp = i_timestamp;

        const int targetWidth = i_width * (std::max)(1, m_modelScale);
        const int targetHeight = i_height * (std::max)(1, m_modelScale);
        const int64_t finalizeStartMs = HJCurrentSteadyMS();
        if (outRgba)
        {
            i_err = finalizeSROutputRGBA(outRgba, outWidth, outHeight, targetWidth, targetHeight, i_timestamp, i_inputData, o_data, "HJSRCoreMLRealESRGAN");
        }
        else
        {
            i_err = finalizeSROutput(outRgb, outWidth, outHeight, targetWidth, targetHeight, i_timestamp, i_inputData, o_data, "HJSRCoreMLRealESRGAN");
        }
        const int64_t finalizeElapsedMs = HJCurrentSteadyMS() - finalizeStartMs;
        if (i_err < 0)
        {
            return i_err;
        }
        const int64_t totalElapsedMs = HJCurrentSteadyMS() - totalStartMs;
        HJFPERLog5i("HJSRCoreMLRealESRGAN process type:{} in:{}x{} out:{}x{} target:{}x{} elapseMs:{} model:{}",
                    (int)i_inputData->getConvertType(), i_width, i_height, outWidth, outHeight, targetWidth, targetHeight, (t1 - t0),
                    toStdString(m_impl->modelPath));
        HJFLogi("HJSRCoreMLRealESRGAN perf total:{} prepare:{} ensure:{} pixel:{} provider:{} predict:{} outputConvert:{} finalize:{} in:{}x{} out:{}x{} model:{}",
                totalElapsedMs,
                prepareElapsedMs,
                ensureElapsedMs,
                pixelElapsedMs,
                providerElapsedMs,
                (t1 - t0),
                outputConvertElapsedMs,
                finalizeElapsedMs,
                i_width,
                i_height,
                outWidth,
                outHeight,
                toStdString(m_impl->modelPath));
        return HJ_OK;
    }
}

NS_HJ_END
