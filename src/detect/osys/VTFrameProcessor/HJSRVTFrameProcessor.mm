#include "HJSRVTFrameProcessor.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"
#include "HJUtilitys.h"
#include "libyuv.h"

#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#if __has_include(<VideoToolbox/VTFrameProcessor.h>) && __has_include(<VideoToolbox/VTFrameProcessorFrame.h>) && __has_include(<VideoToolbox/VTFrameProcessor_SuperResolutionScaler.h>)
#import <VideoToolbox/VTFrameProcessor.h>
#import <VideoToolbox/VTFrameProcessorFrame.h>
#import <VideoToolbox/VTFrameProcessor_SuperResolutionScaler.h>
#define HJ_HAS_VTFRAMEPROCESSOR_SR 1
#else
#define HJ_HAS_VTFRAMEPROCESSOR_SR 0
#endif

#if __has_include(<VideoToolbox/VTFrameProcessor_LowLatencySuperResolutionScaler.h>)
#import <VideoToolbox/VTFrameProcessor_LowLatencySuperResolutionScaler.h>
#define HJ_HAS_VTFRAMEPROCESSOR_LL_SR 1
#else
#define HJ_HAS_VTFRAMEPROCESSOR_LL_SR 0
#endif

#include <cmath>
#include <memory>
#include <string>

NS_HJ_BEGIN

namespace
{
static constexpr bool kPreferLowLatencyOnly = true;

static std::string toStdString(NSString* value)
{
    return value.length > 0 ? std::string(value.UTF8String) : std::string();
}

static int chooseSupportedScaleFactor(int requestedScale, NSArray<NSNumber *> *supportedScales)
{
    if (supportedScales.count <= 0)
    {
        return requestedScale;
    }

    int bestScale = 0;
    for (NSNumber *scaleNum in supportedScales)
    {
        const int scale = scaleNum.intValue;
        if (scale <= 0)
        {
            continue;
        }
        if (scale == requestedScale)
        {
            return scale;
        }
        if (scale > requestedScale)
        {
            if (bestScale <= 0 || scale < bestScale)
            {
                bestScale = scale;
            }
            continue;
        }
        if (bestScale <= 0)
        {
            bestScale = scale;
        }
        else if (bestScale < requestedScale && scale > bestScale)
        {
            bestScale = scale;
        }
    }
    return bestScale > 0 ? bestScale : requestedScale;
}

static NSDictionary<NSString *, id> *buildPixelBufferAttributes(
    int width,
    int height,
    NSDictionary<NSString *, id> *baseAttrs)
{
    NSMutableDictionary<NSString *, id> *attrs = [NSMutableDictionary dictionary];
    if (baseAttrs)
    {
        [attrs addEntriesFromDictionary:baseAttrs];
    }
    attrs[(NSString *)kCVPixelBufferWidthKey] = @(width);
    attrs[(NSString *)kCVPixelBufferHeightKey] = @(height);
    if (!attrs[(NSString *)kCVPixelBufferPixelFormatTypeKey])
    {
        attrs[(NSString *)kCVPixelBufferPixelFormatTypeKey] = @(kCVPixelFormatType_32BGRA);
    }
    if (!attrs[(NSString *)kCVPixelBufferIOSurfacePropertiesKey])
    {
        attrs[(NSString *)kCVPixelBufferIOSurfacePropertiesKey] = @{};
    }
    return attrs;
}

static OSType getPixelFormat(NSDictionary<NSString *, id> *attrs, OSType fallback)
{
    NSNumber *pixelFormat = attrs[(NSString *)kCVPixelBufferPixelFormatTypeKey];
    return pixelFormat ? (OSType)pixelFormat.unsignedIntValue : fallback;
}

static CVPixelBufferRef newPixelBuffer(int width, int height, NSDictionary<NSString *, id> *baseAttrs)
{
    if (width <= 0 || height <= 0)
    {
        return NULL;
    }

    CVPixelBufferRef buffer = NULL;
    NSDictionary<NSString *, id> *attrs = buildPixelBufferAttributes(width, height, baseAttrs);
    const OSType pixelFormat = getPixelFormat(attrs, kCVPixelFormatType_32BGRA);
    CVReturn ret = CVPixelBufferCreate(kCFAllocatorDefault,
                                       width,
                                       height,
                                       pixelFormat,
                                       (__bridge CFDictionaryRef)attrs,
                                       &buffer);
    if (ret != kCVReturnSuccess || !buffer)
    {
        return NULL;
    }
    return buffer;
}

static CVPixelBufferRef newPixelBufferFromRGB(
    const unsigned char *rgb,
    int width,
    int height,
    NSDictionary<NSString *, id> *baseAttrs)
{
    if (!rgb)
    {
        return NULL;
    }

    CVPixelBufferRef buffer = newPixelBuffer(width, height, baseAttrs);
    if (!buffer)
    {
        return NULL;
    }

    const OSType pixelFormat = CVPixelBufferGetPixelFormatType(buffer);
    CVPixelBufferLockBaseAddress(buffer, 0);
    int ret = -1;
    if (pixelFormat == kCVPixelFormatType_32BGRA)
    {
        unsigned char *base = (unsigned char *)CVPixelBufferGetBaseAddress(buffer);
        const int stride = (int)CVPixelBufferGetBytesPerRow(buffer);
        if (!base || stride < width * 4)
        {
            CVPixelBufferUnlockBaseAddress(buffer, 0);
            CFRelease(buffer);
            return NULL;
        }
        ret = libyuv::RAWToARGB(rgb, width * 3, base, stride, width, height);
    }
    else if (pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ||
             pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
    {
        if (CVPixelBufferGetPlaneCount(buffer) < 2)
        {
            CVPixelBufferUnlockBaseAddress(buffer, 0);
            CFRelease(buffer);
            return NULL;
        }
        uint8_t *dstY = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(buffer, 0);
        uint8_t *dstUV = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(buffer, 1);
        const int dstStrideY = (int)CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
        const int dstStrideUV = (int)CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
        const int chromaWidth = (width + 1) / 2;
        const int chromaHeight = (height + 1) / 2;
        const size_t ySize = (size_t)width * (size_t)height;
        const size_t uSize = (size_t)chromaWidth * (size_t)chromaHeight;
        std::unique_ptr<uint8_t[]> i420(new (std::nothrow) uint8_t[ySize + uSize * 2]);
        if (!dstY || !dstUV || !i420)
        {
            CVPixelBufferUnlockBaseAddress(buffer, 0);
            CFRelease(buffer);
            return NULL;
        }
        uint8_t *planeY = i420.get();
        uint8_t *planeU = planeY + ySize;
        uint8_t *planeV = planeU + uSize;
        ret = libyuv::RAWToI420(rgb,
                                width * 3,
                                planeY,
                                width,
                                planeU,
                                chromaWidth,
                                planeV,
                                chromaWidth,
                                width,
                                height);
        if (ret == 0)
        {
            ret = libyuv::I420ToNV12(planeY,
                                     width,
                                     planeU,
                                     chromaWidth,
                                     planeV,
                                     chromaWidth,
                                     dstY,
                                     dstStrideY,
                                     dstUV,
                                     dstStrideUV,
                                     width,
                                     height);
        }
    }

    CVPixelBufferUnlockBaseAddress(buffer, 0);
    if (ret != 0)
    {
        HJFLoge("HJSRVTFrameProcessor unsupported input pixel format:{}", (uint32_t)pixelFormat);
        CFRelease(buffer);
        return NULL;
    }
    return buffer;
}

static CMTime makePresentationTime(int64_t timestampMs)
{
    int64_t safeTimestampMs = timestampMs > 0 ? timestampMs : HJCurrentSteadyMS();
    return CMTimeMake(safeTimestampMs, 1000);
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
    if (outWidth <= 0 || outHeight <= 0)
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

    int ret = -1;
    if (pixelFormat == kCVPixelFormatType_32BGRA)
    {
        if (!base || stride < outWidth * 4)
        {
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            return HJErrFatal;
        }
        ret = libyuv::ARGBToABGR(base, stride, outRgba->getBuf(), outWidth * 4, outWidth, outHeight);
    }
    else if (pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ||
             pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
    {
        if (CVPixelBufferGetPlaneCount(buffer) < 2)
        {
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            return HJErrFatal;
        }
        const uint8_t *srcY = (const uint8_t *)CVPixelBufferGetBaseAddressOfPlane(buffer, 0);
        const uint8_t *srcUV = (const uint8_t *)CVPixelBufferGetBaseAddressOfPlane(buffer, 1);
        const int srcStrideY = (int)CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
        const int srcStrideUV = (int)CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
        if (!srcY || !srcUV)
        {
            CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
            return HJErrFatal;
        }
        ret = libyuv::NV12ToABGR(srcY,
                                 srcStrideY,
                                 srcUV,
                                 srcStrideUV,
                                 outRgba->getBuf(),
                                 outWidth * 4,
                                 outWidth,
                                 outHeight);
    }
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    if (ret != 0)
    {
        HJFLoge("HJSRVTFrameProcessor unsupported output pixel format:{}", (uint32_t)pixelFormat);
        outRgba = nullptr;
        return HJErrFatal;
    }
    return HJ_OK;
}

#if HJ_HAS_VTFRAMEPROCESSOR_SR
static NSString* inputTypeText(VTSuperResolutionScalerConfigurationInputType inputType)
{
    switch (inputType)
    {
        case VTSuperResolutionScalerConfigurationInputTypeImage: return @"image";
        case VTSuperResolutionScalerConfigurationInputTypeVideo: return @"video";
        default: return @"unknown";
    }
}
#endif
} // namespace

struct HJSRVTFrameProcessor::Impl
{
    int inputWidth = 0;
    int inputHeight = 0;
    int requestedScaleFactor = 4;
    int sessionScaleFactor = 4;
    bool useLowLatency = false;
    std::string statusText;

#if HJ_HAS_VTFRAMEPROCESSOR_SR || HJ_HAS_VTFRAMEPROCESSOR_LL_SR
    VTFrameProcessor *processor = nil;
    id config = nil;
    NSDictionary<NSString *, id> *sourcePixelBufferAttributes = nil;
    NSDictionary<NSString *, id> *destinationPixelBufferAttributes = nil;
    VTFrameProcessorFrame *previousInputFrame = nil;
    VTFrameProcessorFrame *previousOutputFrame = nil;
#endif
};

HJSRVTFrameProcessor::HJSRVTFrameProcessor()
    : m_impl(std::make_unique<Impl>())
{
}

HJSRVTFrameProcessor::~HJSRVTFrameProcessor()
{
#if HJ_HAS_VTFRAMEPROCESSOR_SR || HJ_HAS_VTFRAMEPROCESSOR_LL_SR
    if (m_impl && m_impl->processor)
    {
        if (@available(iOS 26.0, *))
        {
            [m_impl->processor endSession];
        }
    }
#endif
}

int HJSRVTFrameProcessor::init(HJBaseParam::Ptr params)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJBaseVideoSR::init(params);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJSRVTFrameProcessor base init failed:{}", i_err);
            break;
        }

        m_modelScale = (m_option && m_option->ncnnScale > 0) ? m_option->ncnnScale : 4;
        m_impl->requestedScaleFactor = (std::max)(1, m_modelScale);
        m_impl->sessionScaleFactor = m_impl->requestedScaleFactor;
        HJFLogi("HJSRVTFrameProcessor init scale:{} modelDir:{}",
                m_impl->requestedScaleFactor,
                m_modelUrls);

#if !HJ_HAS_VTFRAMEPROCESSOR_SR && !HJ_HAS_VTFRAMEPROCESSOR_LL_SR
        HJFLoge("HJSRVTFrameProcessor unavailable: current SDK has no VTFrameProcessor SR headers");
        i_err = HJErrNotSupport;
        break;
#endif
    } while (false);
    return i_err;
}

int HJSRVTFrameProcessor::ensureSessionReady(int width, int height)
{
    if (!m_impl)
    {
        return HJErrNotInited;
    }

#if !HJ_HAS_VTFRAMEPROCESSOR_SR && !HJ_HAS_VTFRAMEPROCESSOR_LL_SR
    (void)width;
    (void)height;
    return HJErrNotSupport;
#else
    if (!@available(iOS 26.0, *))
    {
        HJFLoge("HJSRVTFrameProcessor requires iOS 26+");
        return HJErrNotSupport;
    }

    const bool sameSize = (m_impl->inputWidth == width && m_impl->inputHeight == height);
    if (sameSize && m_impl->processor && m_impl->config)
    {
        return HJ_OK;
    }

    if (!sameSize)
    {
        if (m_impl->processor)
        {
            [m_impl->processor endSession];
        }
        m_impl->processor = nil;
        m_impl->config = nil;
        m_impl->sourcePixelBufferAttributes = nil;
        m_impl->destinationPixelBufferAttributes = nil;
        m_impl->previousInputFrame = nil;
        m_impl->previousOutputFrame = nil;
        m_impl->inputWidth = width;
        m_impl->inputHeight = height;
        m_impl->useLowLatency = false;
    }

    const int requestedScale = (std::max)(1, m_impl->requestedScaleFactor);
    id cfg = nil;
    bool lowLatencySupported = false;

#if HJ_HAS_VTFRAMEPROCESSOR_SR
    if (!cfg && !kPreferLowLatencyOnly)
    {
        if (![VTSuperResolutionScalerConfiguration isSupported])
        {
            HJFLoge("HJSRVTFrameProcessor unsupported on current device");
            return HJErrNotSupport;
        }

        NSArray<NSNumber *> *supportedScales = VTSuperResolutionScalerConfiguration.supportedScaleFactors ?: @[];
        const int effectiveScale = chooseSupportedScaleFactor(requestedScale, supportedScales);
        if (![supportedScales containsObject:@(effectiveScale)])
        {
            HJFLoge("HJSRVTFrameProcessor scale unsupported scale:{} supported:{}",
                    requestedScale,
                    toStdString([supportedScales description]));
            return HJErrNotSupport;
        }
        if (effectiveScale != requestedScale)
        {
            HJFLogw("HJSRVTFrameProcessor fallback scale from {} to {} supported:{}",
                    requestedScale,
                    effectiveScale,
                    toStdString([supportedScales description]));
        }
        m_impl->sessionScaleFactor = effectiveScale;

        NSMutableArray<NSNumber *> *candidateRevisions = [NSMutableArray array];
        const NSInteger defaultRevision = (NSInteger)VTSuperResolutionScalerConfiguration.defaultRevision;
        if (defaultRevision > 0)
        {
            [candidateRevisions addObject:@(defaultRevision)];
        }
        NSIndexSet *supportedRevisions = VTSuperResolutionScalerConfiguration.supportedRevisions;
        [supportedRevisions enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL * _Nonnull stop) {
            NSNumber *rev = @((NSInteger)idx);
            if (![candidateRevisions containsObject:rev])
            {
                [candidateRevisions addObject:rev];
            }
            (void)stop;
        }];

        NSArray<NSNumber *> *candidateInputTypes = @[
            @(VTSuperResolutionScalerConfigurationInputTypeVideo),
            @(VTSuperResolutionScalerConfigurationInputTypeImage)
        ];

        VTSuperResolutionScalerConfiguration *srCfg = nil;
        NSInteger usedRevision = 0;
        VTSuperResolutionScalerConfigurationInputType usedInputType = VTSuperResolutionScalerConfigurationInputTypeImage;
        for (NSNumber *typeNum in candidateInputTypes)
        {
            const VTSuperResolutionScalerConfigurationInputType inputType =
                (VTSuperResolutionScalerConfigurationInputType)typeNum.integerValue;
            for (NSNumber *revNum in candidateRevisions)
            {
                srCfg = [[VTSuperResolutionScalerConfiguration alloc]
                    initWithFrameWidth:width
                           frameHeight:height
                           scaleFactor:m_impl->sessionScaleFactor
                             inputType:inputType
                    usePrecomputedFlow:NO
                 qualityPrioritization:VTSuperResolutionScalerConfigurationQualityPrioritizationNormal
                              revision:(VTSuperResolutionScalerConfigurationRevision)revNum.integerValue];
                if (srCfg)
                {
                    usedRevision = revNum.integerValue;
                    usedInputType = inputType;
                    break;
                }
            }
            if (srCfg)
            {
                break;
            }
        }

        if (!srCfg)
        {
            HJFLoge("HJSRVTFrameProcessor config init failed in:{}x{} scale:{}",
                    width,
                    height,
                    m_impl->sessionScaleFactor);
            return HJErrNotInited;
        }

        const VTSuperResolutionScalerConfigurationModelStatus modelStatus = srCfg.configurationModelStatus;
        if (modelStatus == VTSuperResolutionScalerConfigurationModelStatusDownloadRequired ||
            modelStatus == VTSuperResolutionScalerConfigurationModelStatusDownloading)
        {
            HJFLogi("HJSRVTFrameProcessor model download begin progress:{}",
                    srCfg.configurationModelPercentageAvailable);
            dispatch_semaphore_t sem = dispatch_semaphore_create(0);
            __block NSError *downloadError = nil;
            [srCfg downloadConfigurationModelWithCompletionHandler:^(NSError * _Nullable error) {
                downloadError = error;
                dispatch_semaphore_signal(sem);
            }];
            dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
            if (downloadError)
            {
                HJFLoge("HJSRVTFrameProcessor model download failed err:{}",
                        toStdString(downloadError.localizedDescription ?: @"unknown"));
                return HJErrDownloadFailed;
            }
        }

        m_impl->useLowLatency = false;
        m_impl->sourcePixelBufferAttributes = srCfg.sourcePixelBufferAttributes;
        m_impl->destinationPixelBufferAttributes = srCfg.destinationPixelBufferAttributes;
        m_impl->statusText = HJFMT("ready {}x{} x{} mode:super-resolution type:{} rev:{}",
                                   width,
                                   height,
                                   m_impl->sessionScaleFactor,
                                   toStdString(inputTypeText(usedInputType)),
                                   usedRevision);
        cfg = srCfg;
    }
#endif

#if HJ_HAS_VTFRAMEPROCESSOR_LL_SR
    if (!cfg)
    {
        if ([VTLowLatencySuperResolutionScalerConfiguration isSupported])
        {
            lowLatencySupported = true;
            NSArray<NSNumber *> *llSupportedScales =
                [VTLowLatencySuperResolutionScalerConfiguration supportedScaleFactorsForFrameWidth:width frameHeight:height] ?: @[];
            const int llScale = chooseSupportedScaleFactor(requestedScale, llSupportedScales);
            if ([llSupportedScales containsObject:@(llScale)])
            {
                if (llScale != requestedScale)
                {
                    HJFLogw("HJSRVTFrameProcessor low-latency fallback scale from {} to {} supported:{}",
                            requestedScale,
                            llScale,
                            toStdString([llSupportedScales description]));
                }
                VTLowLatencySuperResolutionScalerConfiguration *llCfg =
                    [[VTLowLatencySuperResolutionScalerConfiguration alloc] initWithFrameWidth:width
                                                                                   frameHeight:height
                                                                                   scaleFactor:(float)llScale];
                if (llCfg)
                {
                    m_impl->sessionScaleFactor = llScale;
                    m_impl->useLowLatency = true;
                    m_impl->sourcePixelBufferAttributes = llCfg.sourcePixelBufferAttributes;
                    m_impl->destinationPixelBufferAttributes = llCfg.destinationPixelBufferAttributes;
                    m_impl->statusText = HJFMT("ready {}x{} x{} mode:low-latency",
                                               width,
                                               height,
                                               m_impl->sessionScaleFactor);
                    cfg = llCfg;
                }
            }
        }
    }
#endif

    if (!cfg && kPreferLowLatencyOnly)
    {
#if HJ_HAS_VTFRAMEPROCESSOR_LL_SR
        const char *reason = lowLatencySupported ? "config-unavailable" : "not-supported";
        HJFLoge("HJSRVTFrameProcessor low-latency only mode unavailable in:{}x{} reqScale:{} reason:{}",
                width,
                height,
                requestedScale,
                reason);
#else
        HJFLoge("HJSRVTFrameProcessor low-latency only mode unavailable: SDK has no low-latency SR headers");
#endif
        return HJErrNotSupport;
    }

    if (!cfg)
    {
        HJFLoge("HJSRVTFrameProcessor no usable VTFrameProcessor SR configuration in:{}x{} scale:{}",
                width,
                height,
                requestedScale);
        return HJErrNotSupport;
    }

    VTFrameProcessor *processor = [[VTFrameProcessor alloc] init];
    NSError *startError = nil;
    const BOOL ok = [processor startSessionWithConfiguration:cfg error:&startError];
    if (!ok || startError)
    {
        HJFLoge("HJSRVTFrameProcessor start session failed err:{}",
                toStdString(startError.localizedDescription ?: @"unknown"));
        return HJErrNotInited;
    }

    m_impl->processor = processor;
    m_impl->config = cfg;
    m_impl->previousInputFrame = nil;
    m_impl->previousOutputFrame = nil;
    m_impl->inputWidth = width;
    m_impl->inputHeight = height;
    HJFLogi("HJSRVTFrameProcessor session ready {}", m_impl->statusText);
    return HJ_OK;
#endif
}

int HJSRVTFrameProcessor::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
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
        int i_err = prepareInputRgb(i_inputData, inputRgb, i_width, i_height, i_timestamp, "HJSRVTFrameProcessor");
        const int64_t prepareElapsedMs = HJCurrentSteadyMS() - prepareStartMs;
        if (i_err < 0)
        {
            return i_err;
        }

        const int64_t ensureStartMs = HJCurrentSteadyMS();
        i_err = ensureSessionReady(i_width, i_height);
        const int64_t ensureElapsedMs = HJCurrentSteadyMS() - ensureStartMs;
        if (i_err < 0)
        {
            return i_err;
        }

#if !HJ_HAS_VTFRAMEPROCESSOR_SR && !HJ_HAS_VTFRAMEPROCESSOR_LL_SR
        (void)prepareElapsedMs;
        (void)ensureElapsedMs;
        return HJErrNotSupport;
#else
        if (@available(iOS 26.0, *))
        {
            const int64_t inputBufferStartMs = HJCurrentSteadyMS();
            CVPixelBufferRef src = newPixelBufferFromRGB(inputRgb, i_width, i_height, m_impl->sourcePixelBufferAttributes);
            const int64_t inputBufferElapsedMs = HJCurrentSteadyMS() - inputBufferStartMs;
            if (!src)
            {
                HJFLoge("HJSRVTFrameProcessor input pixel buffer alloc failed");
                return HJErrFatal;
            }

            const int outTargetWidth = i_width * (std::max)(1, m_impl->sessionScaleFactor);
            const int outTargetHeight = i_height * (std::max)(1, m_impl->sessionScaleFactor);
            const int64_t outputBufferStartMs = HJCurrentSteadyMS();
            CVPixelBufferRef dst = newPixelBuffer(outTargetWidth, outTargetHeight, m_impl->destinationPixelBufferAttributes);
            const int64_t outputBufferElapsedMs = HJCurrentSteadyMS() - outputBufferStartMs;
            if (!dst)
            {
                CFRelease(src);
                HJFLoge("HJSRVTFrameProcessor output pixel buffer alloc failed");
                return HJErrFatal;
            }

            const CMTime pts = makePresentationTime(i_timestamp);
            VTFrameProcessorFrame *srcFrame = [[VTFrameProcessorFrame alloc] initWithBuffer:src presentationTimeStamp:pts];
            VTFrameProcessorFrame *dstFrame = [[VTFrameProcessorFrame alloc] initWithBuffer:dst presentationTimeStamp:pts];
            if (!srcFrame || !dstFrame)
            {
                CFRelease(src);
                CFRelease(dst);
                HJFLoge("HJSRVTFrameProcessor frame wrapper init failed");
                return HJErrFatal;
            }

            id params = nil;
#if HJ_HAS_VTFRAMEPROCESSOR_LL_SR
            if (m_impl->useLowLatency)
            {
                params = [[VTLowLatencySuperResolutionScalerParameters alloc]
                    initWithSourceFrame:srcFrame
                        destinationFrame:dstFrame];
            }
#endif
#if HJ_HAS_VTFRAMEPROCESSOR_SR
            if (!params)
            {
                const VTSuperResolutionScalerParametersSubmissionMode mode = m_impl->previousInputFrame
                    ? VTSuperResolutionScalerParametersSubmissionModeSequential
                    : VTSuperResolutionScalerParametersSubmissionModeRandom;
                params = [[VTSuperResolutionScalerParameters alloc]
                    initWithSourceFrame:srcFrame
                          previousFrame:m_impl->previousInputFrame
                    previousOutputFrame:m_impl->previousOutputFrame
                            opticalFlow:nil
                         submissionMode:mode
                       destinationFrame:dstFrame];
            }
#endif
            if (!params)
            {
                CFRelease(src);
                CFRelease(dst);
                HJFLoge("HJSRVTFrameProcessor params init failed");
                return HJErrFatal;
            }

            NSError *processError = nil;
            const int64_t predictStartMs = HJCurrentSteadyMS();
            const BOOL ok = [m_impl->processor processWithParameters:params error:&processError];
            const int64_t predictElapsedMs = HJCurrentSteadyMS() - predictStartMs;
            if (!ok || processError)
            {
                CFRelease(src);
                CFRelease(dst);
                HJFLoge("HJSRVTFrameProcessor process failed err:{}",
                        toStdString(processError.localizedDescription ?: @"unknown"));
                m_impl->previousInputFrame = nil;
                m_impl->previousOutputFrame = nil;
                return HJErrFatal;
            }

            HJSPBuffer::Ptr outRgba = nullptr;
            int outWidth = 0;
            int outHeight = 0;
            const int64_t convertStartMs = HJCurrentSteadyMS();
            i_err = copyPixelBufferToRGBA(dst, outRgba, outWidth, outHeight);
            const int64_t convertElapsedMs = HJCurrentSteadyMS() - convertStartMs;
            if (i_err < 0 || !outRgba || outWidth <= 0 || outHeight <= 0)
            {
                CFRelease(src);
                CFRelease(dst);
                HJFLoge("HJSRVTFrameProcessor output convert failed");
                return (i_err < 0) ? i_err : HJErrFatal;
            }

            if (m_impl->useLowLatency)
            {
                m_impl->previousInputFrame = nil;
                m_impl->previousOutputFrame = nil;
            }
            else
            {
                m_impl->previousInputFrame = srcFrame;
                m_impl->previousOutputFrame = dstFrame;
            }
            CFRelease(src);
            CFRelease(dst);

            o_ret.m_elapseMs = predictElapsedMs;
            o_ret.m_systemTime = predictStartMs;
            o_ret.m_timeStamp = i_timestamp;

            const int64_t finalizeStartMs = HJCurrentSteadyMS();
            i_err = finalizeSROutputRGBA(outRgba,
                                         outWidth,
                                         outHeight,
                                         outTargetWidth,
                                         outTargetHeight,
                                         i_timestamp,
                                         i_inputData,
                                         o_data,
                                         "HJSRVTFrameProcessor");
            const int64_t finalizeElapsedMs = HJCurrentSteadyMS() - finalizeStartMs;
            if (i_err < 0)
            {
                return i_err;
            }

            const int64_t totalElapsedMs = HJCurrentSteadyMS() - totalStartMs;
            HJFPERLog5i("HJSRVTFrameProcessor process type:{} in:{}x{} out:{}x{} target:{}x{} elapseMs:{} status:{}",
                        (int)i_inputData->getConvertType(),
                        i_width,
                        i_height,
                        outWidth,
                        outHeight,
                        outTargetWidth,
                        outTargetHeight,
                        predictElapsedMs,
                        m_impl->statusText);
            HJFLogi("HJSRVTFrameProcessor perf total:{} prepare:{} ensure:{} inputBuffer:{} outputBuffer:{} predict:{} outputConvert:{} finalize:{} in:{}x{} out:{}x{} status:{}",
                    totalElapsedMs,
                    prepareElapsedMs,
                    ensureElapsedMs,
                    inputBufferElapsedMs,
                    outputBufferElapsedMs,
                    predictElapsedMs,
                    convertElapsedMs,
                    finalizeElapsedMs,
                    i_width,
                    i_height,
                    outWidth,
                    outHeight,
                    m_impl->statusText);
            return HJ_OK;
        }

        return HJErrNotSupport;
#endif
    }
}

NS_HJ_END
