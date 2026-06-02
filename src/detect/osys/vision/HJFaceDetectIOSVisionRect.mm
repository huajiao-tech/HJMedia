#include "HJFaceDetectIOSVisionRect.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJComUtils.h"
#include "HJTransferMediaData.h"
#include "HJDataConvert.h"
#include "libyuv.h"
#include "libyuv/scale_rgb.h"
#include "HJTime.h"
#import <CoreFoundation/CoreFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>
#if __has_include(<CoreML/MLComputeDevice.h>)
#import <CoreML/MLComputeDevice.h>
#endif

#include <algorithm>
#include <cmath>
#include <vector>

NS_HJ_BEGIN

namespace
{
struct VisionTimingInfo
{
    int64_t pixelBufferCreateElapsedMs = 0;
    int64_t inputConvertElapsedMs = 0;
    int64_t inputCopyElapsedMs = 0;
    int64_t visionRequestElapsedMs = 0;
    int64_t resultCollectElapsedMs = 0;
    int64_t totalElapsedMs = 0;
};

struct VisionInputInfo
{
    int width = 0;
    int height = 0;
    int scaledWidth = 0;
    int scaledHeight = 0;
    int contentWidth = 0;
    int contentHeight = 0;
    int offsetX = 0;
    int offsetY = 0;
    bool scaled = false;
    HJConvertDataType type = HJConvertDataType_None;
};

struct VisionFaceItem
{
    HJFaceDetectRect rect;
    float confidence = 0.0f;
};

static std::string toStdString(NSString* value)
{
    return value.length > 0 ? std::string(value.UTF8String) : std::string();
}

static std::string boolString(bool value)
{
    return value ? "true" : "false";
}

static const char* visionComputeModeString(int mode)
{
    switch (mode)
    {
        case 1: return "ane";
        case 2: return "cpu";
        case 3: return "gpu";
        default: return "auto";
    }
}

#if defined(__APPLE__)
static std::string toStdString(id value)
{
    if (!value)
    {
        return std::string("nil");
    }
    if ([value isKindOfClass:[NSString class]])
    {
        return toStdString((NSString*)value);
    }
    return toStdString([value description]);
}
#endif

#if defined(__APPLE__)
static std::string joinNSStringArray(NSArray<NSString*>* values)
{
    if (!values || values.count == 0)
    {
        return std::string();
    }
    return toStdString([values componentsJoinedByString:@","]);
}

static NSArray<NSString*>* describeComputeDevices(NSArray<id<MLComputeDeviceProtocol>>* devices)
{
    NSMutableArray<NSString*>* names = [NSMutableArray array];
    for (id<MLComputeDeviceProtocol> device in devices)
    {
        if (!device)
        {
            continue;
        }
        NSString* desc = [device description];
        if (desc.length <= 0)
        {
            desc = NSStringFromClass([device class]);
        }
        [names addObject:desc ?: @"unknown"];
    }
    return names;
}

static bool matchesVisionComputeMode(id<MLComputeDeviceProtocol> device, int mode)
{
    if (!device)
    {
        return false;
    }
    switch (mode)
    {
        case 1:
            return [device isKindOfClass:NSClassFromString(@"MLNeuralEngineComputeDevice")];
        case 2:
            return [device isKindOfClass:NSClassFromString(@"MLCPUComputeDevice")];
        case 3:
            return [device isKindOfClass:NSClassFromString(@"MLGPUComputeDevice")];
        default:
            return false;
    }
}

static std::string configureVisionComputeDeviceIfNeeded(VNDetectFaceRectanglesRequest* request, int computeMode)
{
    if (!request || computeMode <= 0)
    {
        return std::string("auto");
    }
    if (@available(iOS 17.0, *))
    {
        NSError* deviceError = nil;
        if ([request respondsToSelector:@selector(supportedComputeStageDevicesAndReturnError:)] &&
            [request respondsToSelector:@selector(setComputeDevice:forComputeStage:)])
        {
            NSDictionary<VNComputeStage, NSArray<id<MLComputeDeviceProtocol>>*>* devices = [request supportedComputeStageDevicesAndReturnError:&deviceError];
            NSArray<id<MLComputeDeviceProtocol>>* mainDevices = devices[VNComputeStageMain];
            for (id<MLComputeDeviceProtocol> device in mainDevices)
            {
                if (!matchesVisionComputeMode(device, computeMode))
                {
                    continue;
                }
                [request setComputeDevice:device forComputeStage:VNComputeStageMain];
                return toStdString([device description]);
            }
            if (deviceError)
            {
                return HJFMT("auto(err:{})", toStdString(deviceError.localizedDescription ?: @"unknown"));
            }
            return HJFMT("auto(unsupported:{})", visionComputeModeString(computeMode));
        }
        return HJFMT("auto(unavailable:{})", visionComputeModeString(computeMode));
    }
    return HJFMT("auto(ios<17:{})", visionComputeModeString(computeMode));
}
#endif

static CFDictionaryRef createPixelBufferAttributes()
{
    const void* keys[] = {
        kCVPixelBufferCGImageCompatibilityKey,
        kCVPixelBufferCGBitmapContextCompatibilityKey,
        kCVPixelBufferIOSurfacePropertiesKey
    };
    CFTypeRef emptyDict = CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const void* values[] = {
        kCFBooleanTrue,
        kCFBooleanTrue,
        emptyDict
    };
    CFDictionaryRef attrs = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 3,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (emptyDict)
    {
        CFRelease(emptyDict);
    }
    return attrs;
}

static int createNV12PixelBufferCopy(unsigned char* yPlane,
                                     unsigned char* uvPlane,
                                     int yStride,
                                     int uvStride,
                                     int width,
                                     int height,
                                     CVPixelBufferRef* outBuffer,
                                     int64_t* outCreateElapsedMs,
                                     int64_t* outCopyElapsedMs)
{
    if (!outBuffer)
    {
        return HJErrInvalidParams;
    }
    *outBuffer = nullptr;
    if (!yPlane || !uvPlane || yStride < width || uvStride < width || width <= 0 || height <= 0)
    {
        return HJErrInvalidParams;
    }

    CFDictionaryRef attrs = createPixelBufferAttributes();
    const int64_t createStartMs = HJCurrentSteadyMS();
    CVReturn cvRet = CVPixelBufferCreate(kCFAllocatorDefault,
        width,
        height,
        kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
        attrs,
        outBuffer);
    if (attrs)
    {
        CFRelease(attrs);
    }
    if (outCreateElapsedMs)
    {
        *outCreateElapsedMs = HJCurrentSteadyMS() - createStartMs;
    }
    if (cvRet != kCVReturnSuccess || !*outBuffer)
    {
        return HJErrFatal;
    }

    cvRet = CVPixelBufferLockBaseAddress(*outBuffer, 0);
    if (cvRet != kCVReturnSuccess)
    {
        CFRelease(*outBuffer);
        *outBuffer = nullptr;
        return HJErrFatal;
    }

    unsigned char* dstY = static_cast<unsigned char*>(CVPixelBufferGetBaseAddressOfPlane(*outBuffer, 0));
    unsigned char* dstUV = static_cast<unsigned char*>(CVPixelBufferGetBaseAddressOfPlane(*outBuffer, 1));
    const size_t dstYStride = CVPixelBufferGetBytesPerRowOfPlane(*outBuffer, 0);
    const size_t dstUVStride = CVPixelBufferGetBytesPerRowOfPlane(*outBuffer, 1);
    if (!dstY || !dstUV || dstYStride < (size_t)width || dstUVStride < (size_t)width)
    {
        CVPixelBufferUnlockBaseAddress(*outBuffer, 0);
        CFRelease(*outBuffer);
        *outBuffer = nullptr;
        return HJErrFatal;
    }

    const int64_t copyStartMs = HJCurrentSteadyMS();
    for (int y = 0; y < height; ++y)
    {
        memcpy(dstY + (size_t)y * dstYStride, yPlane + (size_t)y * (size_t)yStride, (size_t)width);
    }
    for (int y = 0; y < height / 2; ++y)
    {
        memcpy(dstUV + (size_t)y * dstUVStride, uvPlane + (size_t)y * (size_t)uvStride, (size_t)width);
    }

    CVPixelBufferUnlockBaseAddress(*outBuffer, 0);
    if (outCopyElapsedMs)
    {
        *outCopyElapsedMs = HJCurrentSteadyMS() - copyStartMs;
    }
    return HJ_OK;
}

static int createRGBPixelBufferCopy(unsigned char* src,
                                    int srcStride,
                                    int width,
                                    int height,
                                    CVPixelBufferRef* outBuffer,
                                    int64_t* outCreateElapsedMs,
                                    int64_t* outConvertElapsedMs,
                                    int64_t* outCopyElapsedMs)
{
    if (!outBuffer)
    {
        return HJErrInvalidParams;
    }
    *outBuffer = nullptr;
    if (!src || srcStride < width * 3 || width <= 0 || height <= 0)
    {
        return HJErrInvalidParams;
    }

    CFDictionaryRef attrs = createPixelBufferAttributes();
    const int64_t createStartMs = HJCurrentSteadyMS();
    CVReturn cvRet = CVPixelBufferCreate(kCFAllocatorDefault,
        width,
        height,
        kCVPixelFormatType_32BGRA,
        attrs,
        outBuffer);
    if (attrs)
    {
        CFRelease(attrs);
    }
    if (outCreateElapsedMs)
    {
        *outCreateElapsedMs = HJCurrentSteadyMS() - createStartMs;
    }
    if (cvRet != kCVReturnSuccess || !*outBuffer)
    {
        return HJErrFatal;
    }

    cvRet = CVPixelBufferLockBaseAddress(*outBuffer, 0);
    if (cvRet != kCVReturnSuccess)
    {
        CFRelease(*outBuffer);
        *outBuffer = nullptr;
        return HJErrFatal;
    }

    unsigned char* dst = static_cast<unsigned char*>(CVPixelBufferGetBaseAddress(*outBuffer));
    const size_t dstStride = CVPixelBufferGetBytesPerRow(*outBuffer);
    const int dstArgbStride = width * 4;
    if (!dst || dstStride < (size_t)dstArgbStride)
    {
        CVPixelBufferUnlockBaseAddress(*outBuffer, 0);
        CFRelease(*outBuffer);
        *outBuffer = nullptr;
        return HJErrFatal;
    }

    const int64_t convertStartMs = HJCurrentSteadyMS();
    const int libyuvRet = libyuv::RAWToARGB(src, srcStride, dst, (int)dstStride, width, height);
    CVPixelBufferUnlockBaseAddress(*outBuffer, 0);
    if (outConvertElapsedMs)
    {
        *outConvertElapsedMs = HJCurrentSteadyMS() - convertStartMs;
    }
    if (outCopyElapsedMs)
    {
        *outCopyElapsedMs = 0;
    }
    if (libyuvRet != 0)
    {
        CFRelease(*outBuffer);
        *outBuffer = nullptr;
        return HJErrFatal;
    }
    return HJ_OK;
}

static void computeScaledExtent(int srcWidth, int srcHeight, int targetMaxSide, int& outWidth, int& outHeight)
{
    outWidth = srcWidth;
    outHeight = srcHeight;
    if (srcWidth <= 0 || srcHeight <= 0 || targetMaxSide <= 0)
    {
        return;
    }

    const int maxSide = (std::max)(srcWidth, srcHeight);
    if (maxSide <= targetMaxSide)
    {
        return;
    }

    const float scale = (float)targetMaxSide / (float)maxSide;
    outWidth = (std::max)(2, (int)std::round(srcWidth * scale));
    outHeight = (std::max)(2, (int)std::round(srcHeight * scale));
}

static bool shouldUseScaledVisionInput(const HJFaceDetectOption::Ptr& option, int srcWidth, int srcHeight, int& outWidth, int& outHeight)
{
    outWidth = srcWidth;
    outHeight = srcHeight;
    if (!option || option->visionRectTargetSize <= 0)
    {
        return false;
    }
    computeScaledExtent(srcWidth, srcHeight, option->visionRectTargetSize, outWidth, outHeight);
    return outWidth != srcWidth || outHeight != srcHeight;
}

static bool computeFitMapping(int srcWidth,
                              int srcHeight,
                              int canvasWidth,
                              int canvasHeight,
                              int& outContentWidth,
                              int& outContentHeight,
                              int& outOffsetX,
                              int& outOffsetY)
{
    outContentWidth = srcWidth;
    outContentHeight = srcHeight;
    outOffsetX = 0;
    outOffsetY = 0;
    if (srcWidth <= 0 || srcHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0)
    {
        return false;
    }

    HJVec4i fit = HJDataConvert::cvtGetScaleOffset(HJDataScaleType::Fit, srcWidth, srcHeight, canvasWidth, canvasHeight);
    outContentWidth = fit.x;
    outContentHeight = fit.y;
    outOffsetX = fit.z;
    outOffsetY = fit.w;
    if (outContentWidth <= 0 || outContentHeight <= 0)
    {
        return false;
    }
    return true;
}

static int prepareNV12Input(const std::shared_ptr<HJTransferMediaData>& input,
                            const HJFaceDetectOption::Ptr& option,
                            std::shared_ptr<HJSPBuffer>& scaledBuffer,
                            unsigned char*& outYPlane,
                            unsigned char*& outUVPlane,
                            int& outYStride,
                            int& outUVStride,
                            int& outWidth,
                            int& outHeight,
                            int& outContentWidth,
                            int& outContentHeight,
                            int& outOffsetX,
                            int& outOffsetY,
                            bool& outScaled,
                            int64_t* outConvertElapsedMs)
{
    if (!input)
    {
        return HJErrInvalidParams;
    }
    unsigned char* srcY = input->getData(0);
    unsigned char* srcUV = input->getData(1);
    const int srcYStride = input->getStride(0);
    const int srcUVStride = input->getStride(1);
    const int srcWidth = input->getWidth();
    const int srcHeight = input->getHeight();
    if (!srcY || !srcUV || srcYStride <= 0 || srcUVStride <= 0 || srcWidth <= 0 || srcHeight <= 0)
    {
        return HJErrInvalidParams;
    }

    outScaled = shouldUseScaledVisionInput(option, srcWidth, srcHeight, outWidth, outHeight);
    outContentWidth = outWidth;
    outContentHeight = outHeight;
    outOffsetX = 0;
    outOffsetY = 0;
    if (!outScaled)
    {
        outYPlane = srcY;
        outUVPlane = srcUV;
        outYStride = srcYStride;
        outUVStride = srcUVStride;
        if (outConvertElapsedMs)
        {
            *outConvertElapsedMs = 0;
        }
        return HJ_OK;
    }

    outWidth &= ~1;
    outHeight &= ~1;
    if (outWidth < 2 || outHeight < 2)
    {
        return HJErrInvalidParams;
    }

    const size_t bufferSize = (size_t)outWidth * (size_t)outHeight * 3 / 2;
    if (!scaledBuffer || scaledBuffer->getSize() != bufferSize)
    {
        scaledBuffer = HJSPBuffer::create(bufferSize);
    }
    if (!scaledBuffer || !scaledBuffer->getBuf())
    {
        return HJErrFatal;
    }

    if (!computeFitMapping(srcWidth, srcHeight, outWidth, outHeight, outContentWidth, outContentHeight, outOffsetX, outOffsetY))
    {
        return HJErrFatal;
    }
    outContentWidth &= ~1;
    outContentHeight &= ~1;
    outOffsetX &= ~1;
    outOffsetY &= ~1;
    if (outContentWidth < 2 || outContentHeight < 2)
    {
        return HJErrFatal;
    }

    memset(scaledBuffer->getBuf(), 16, (size_t)outWidth * (size_t)outHeight);
    memset(scaledBuffer->getBuf() + (size_t)outWidth * (size_t)outHeight, 128, (size_t)outWidth * (size_t)outHeight / 2);

    const int64_t convertStartMs = HJCurrentSteadyMS();
    const int scaleRet = libyuv::NV12Scale(srcY, srcYStride, srcUV, srcUVStride, srcWidth, srcHeight,
        scaledBuffer->getBuf() + (size_t)outOffsetY * (size_t)outWidth + (size_t)outOffsetX, outWidth,
        scaledBuffer->getBuf() + (size_t)outWidth * (size_t)outHeight + ((size_t)outOffsetY * (size_t)outWidth) / 2 + (size_t)outOffsetX, outWidth,
        outContentWidth, outContentHeight, libyuv::kFilterBilinear);
    if (outConvertElapsedMs)
    {
        *outConvertElapsedMs = HJCurrentSteadyMS() - convertStartMs;
    }
    if (scaleRet != 0)
    {
        return HJErrFatal;
    }

    outYPlane = scaledBuffer->getBuf();
    outUVPlane = scaledBuffer->getBuf() + (size_t)outWidth * (size_t)outHeight;
    outYStride = outWidth;
    outUVStride = outWidth;
    return HJ_OK;
}

static int prepareRGBInput(const std::shared_ptr<HJTransferMediaData>& input,
                           const HJFaceDetectOption::Ptr& option,
                           std::shared_ptr<HJSPBuffer>& scaledBuffer,
                           unsigned char*& outRgb,
                           int& outStride,
                           int& outWidth,
                           int& outHeight,
                           int& outContentWidth,
                           int& outContentHeight,
                           int& outOffsetX,
                           int& outOffsetY,
                           bool& outScaled,
                           int64_t* outConvertElapsedMs)
{
    if (!input)
    {
        return HJErrInvalidParams;
    }
    unsigned char* src = input->getData(0);
    const int srcStride = input->getStride(0);
    const int srcWidth = input->getWidth();
    const int srcHeight = input->getHeight();
    if (!src || srcStride <= 0 || srcWidth <= 0 || srcHeight <= 0)
    {
        return HJErrInvalidParams;
    }

    outScaled = shouldUseScaledVisionInput(option, srcWidth, srcHeight, outWidth, outHeight);
    outContentWidth = outWidth;
    outContentHeight = outHeight;
    outOffsetX = 0;
    outOffsetY = 0;
    if (!outScaled)
    {
        outRgb = src;
        outStride = srcStride;
        if (outConvertElapsedMs)
        {
            *outConvertElapsedMs = 0;
        }
        return HJ_OK;
    }

    const size_t bufferSize = (size_t)outWidth * (size_t)outHeight * 3;
    if (!scaledBuffer || scaledBuffer->getSize() != bufferSize)
    {
        scaledBuffer = HJSPBuffer::create(bufferSize);
    }
    if (!scaledBuffer || !scaledBuffer->getBuf())
    {
        return HJErrFatal;
    }

    if (!computeFitMapping(srcWidth, srcHeight, outWidth, outHeight, outContentWidth, outContentHeight, outOffsetX, outOffsetY))
    {
        return HJErrFatal;
    }

    memset(scaledBuffer->getBuf(), 0, bufferSize);

    const int64_t convertStartMs = HJCurrentSteadyMS();
    const int scaleRet = libyuv::RGBScale(src, srcStride, srcWidth, srcHeight,
        scaledBuffer->getBuf() + ((size_t)outOffsetY * (size_t)outWidth + (size_t)outOffsetX) * 3,
        outWidth * 3, outContentWidth, outContentHeight, libyuv::kFilterBilinear);
    if (outConvertElapsedMs)
    {
        *outConvertElapsedMs = HJCurrentSteadyMS() - convertStartMs;
    }
    if (scaleRet != 0)
    {
        return HJErrFatal;
    }

    outRgb = scaledBuffer->getBuf();
    outStride = outWidth * 3;
    return HJ_OK;
}

static bool convertVisionBoundingBoxToRect(CGRect box, int width, int height, HJFaceDetectRect& outRect)
{
    if (width <= 0 || height <= 0 || box.size.width <= 0.0 || box.size.height <= 0.0)
    {
        return false;
    }

    const float x0 = (std::max)(0.0f, (float)(box.origin.x * width));
    const float y0 = (std::max)(0.0f, (float)((1.0 - box.origin.y - box.size.height) * height));
    const float x1 = (std::min)((float)width, (float)((box.origin.x + box.size.width) * width));
    const float y1 = (std::min)((float)height, (float)((1.0 - box.origin.y) * height));
    if (x1 <= x0 || y1 <= y0)
    {
        return false;
    }

    outRect.set(x0, y0, x1 - x0, y1 - y0);
    return true;
}
} // namespace

HJFaceDetectIOSVisionRect::HJFaceDetectIOSVisionRect() = default;

HJFaceDetectIOSVisionRect::~HJFaceDetectIOSVisionRect()
{
#if defined(__APPLE__)
    if (m_request)
    {
        CFRelease((CFTypeRef)m_request);
        m_request = nullptr;
    }
#endif
}

int HJFaceDetectIOSVisionRect::init(HJBaseParam::Ptr params)
{
    int i_err = HJ_OK;
    do
    {
        HJ_CatchMapPlainGetVal(params, std::string, s_parammodelPath, m_modelUrls);
        HJ_CatchMapGetVal(params, HJFaceDetectOption::Ptr, m_option);
        if (!m_option)
        {
            m_option = HJFaceDetectOption::Create();
        }
#if defined(__APPLE__)
        VNDetectFaceRectanglesRequest* request = [[VNDetectFaceRectanglesRequest alloc] init];
        m_request = (__bridge_retained void*)request;
        std::string configuredCompute = "auto";
        if (request)
        {
            request.preferBackgroundProcessing = NO;
            configuredCompute = configureVisionComputeDeviceIfNeeded(request, m_option ? m_option->visionRectComputeMode : 0);
        }
        HJFLogi("HJFaceDetectIOSVisionRect::init request revision:{} defaultRevision:{} currentRevision:{} preferBackground:{} visionTarget:{} visionComputeMode:{} configuredMain:{}",
                request ? request.revision : 0,
                [VNDetectFaceRectanglesRequest defaultRevision],
                [VNDetectFaceRectanglesRequest currentRevision],
                request ? boolString((bool)request.preferBackgroundProcessing) : std::string("false"),
                m_option ? m_option->visionRectTargetSize : 0,
                m_option ? visionComputeModeString(m_option->visionRectComputeMode) : "auto",
                configuredCompute);
        NSError* deviceError = nil;
        if (request && [request respondsToSelector:@selector(supportedComputeStageDevicesAndReturnError:)])
        {
            NSDictionary<VNComputeStage, NSArray<id<MLComputeDeviceProtocol>>*>* devices = [request supportedComputeStageDevicesAndReturnError:&deviceError];
            if (devices)
            {
                NSArray<id<MLComputeDeviceProtocol>>* mainDevices = devices[VNComputeStageMain];
                NSArray<NSString*>* mainDeviceNames = describeComputeDevices(mainDevices);
                id<MLComputeDeviceProtocol> configuredMainDevice = nil;
                if ([request respondsToSelector:@selector(computeDeviceForComputeStage:)])
                {
                    configuredMainDevice = [request computeDeviceForComputeStage:VNComputeStageMain];
                }
                for (id key in devices)
                {
                    NSArray<id<MLComputeDeviceProtocol>>* stageDevices = devices[key];
                    (void)stageDevices;
                    (void)key;
                }
                HJFLogi("HJFaceDetectIOSVisionRect::init Vision compute support main:[{}] configuredMain:{} actualMain:unknown revision:{} preferBackground:{}",
                        joinNSStringArray(mainDeviceNames),
                        configuredMainDevice ? toStdString([configuredMainDevice description]) : std::string("auto"),
                        request.revision,
                        boolString((bool)request.preferBackgroundProcessing));
                HJFLogi("HJFaceDetectIOSVisionRect::init Vision compute note supported!=actual, actualMain is not exposed by Vision");
                HJFLogi("HJFaceDetectIOSVisionRect::init Vision compute devices available revision:{} preferBackground:{} devices:{}",
                        request.revision,
                        boolString((bool)request.preferBackgroundProcessing),
                        toStdString([devices description]));
            }
            else
            {
                HJFLogi("HJFaceDetectIOSVisionRect::init Vision compute device introspection unavailable err:{}",
                        deviceError ? toStdString(deviceError.localizedDescription ?: @"unknown") : std::string("unknown"));
            }
        }
#endif
        HJFLogi("HJFaceDetectIOSVisionRect::init modelDir:{} rect-only Vision backend visionTarget:{} visionComputeMode:{}",
                m_modelUrls,
                m_option ? m_option->visionRectTargetSize : 0,
                m_option ? visionComputeModeString(m_option->visionRectComputeMode) : "auto");
    } while (false);
    return i_err;
}

int HJFaceDetectIOSVisionRect::detect(std::shared_ptr<HJTransferMediaData> i_inputData, HJFaceDetectRet& o_ret)
{
    if (!i_inputData)
    {
        return HJErrInvalidParams;
    }

    const int width = i_inputData->getWidth();
    const int height = i_inputData->getHeight();
    const int64_t timestamp = i_inputData->getTimeStamp();
    if (width <= 0 || height <= 0)
    {
        return HJErrInvalidParams;
    }

    int i_err = HJ_OK;
    const int64_t t0 = HJCurrentSteadyMS();
    CVPixelBufferRef pixelBuffer = nullptr;
    VisionTimingInfo timing;
    VisionInputInfo inputInfo;
    inputInfo.width = width;
    inputInfo.height = height;
    inputInfo.type = i_inputData->getConvertType();
    do
    {
        const HJConvertDataType type = i_inputData->getConvertType();
        if (type == HJConvertDataType_YUVNV12)
        {
            unsigned char* yPlane = nullptr;
            unsigned char* uvPlane = nullptr;
            int yStride = 0;
            int uvStride = 0;
            i_err = prepareNV12Input(i_inputData, m_option, m_scaledNv12Buffer,
                                     yPlane, uvPlane, yStride, uvStride,
                                     inputInfo.scaledWidth, inputInfo.scaledHeight,
                                     inputInfo.contentWidth, inputInfo.contentHeight,
                                     inputInfo.offsetX, inputInfo.offsetY,
                                     inputInfo.scaled,
                                     &timing.inputConvertElapsedMs);
            if (i_err >= 0)
            {
                i_err = createNV12PixelBufferCopy(yPlane, uvPlane, yStride, uvStride,
                                                  inputInfo.scaledWidth, inputInfo.scaledHeight,
                                                  &pixelBuffer,
                                                  &timing.pixelBufferCreateElapsedMs,
                                                  &timing.inputCopyElapsedMs);
            }
        }
        else if (type == HJConvertDataType_RGB)
        {
            unsigned char* rgb = nullptr;
            int rgbStride = 0;
            i_err = prepareRGBInput(i_inputData, m_option, m_scaledRgbBuffer,
                                    rgb, rgbStride,
                                    inputInfo.scaledWidth, inputInfo.scaledHeight,
                                    inputInfo.contentWidth, inputInfo.contentHeight,
                                    inputInfo.offsetX, inputInfo.offsetY,
                                    inputInfo.scaled,
                                    &timing.inputConvertElapsedMs);
            if (i_err >= 0)
            {
                i_err = createRGBPixelBufferCopy(rgb, rgbStride, inputInfo.scaledWidth, inputInfo.scaledHeight,
                                                 &pixelBuffer,
                                                 &timing.pixelBufferCreateElapsedMs,
                                                 nullptr,
                                                 &timing.inputCopyElapsedMs);
            }
        }
        else
        {
            i_err = HJErrInvalidParams;
        }
        if (i_err < 0 || !pixelBuffer)
        {
            HJFLoge("HJFaceDetectIOSVisionRect create pixel buffer failed type:{} err:{}", (int)type, i_err);
            break;
        }

        HJFLogi("HJFaceDetectIOSVisionRect prepare input type:{} src:{}x{} scaled:{} scaledInput:{}x{} content:{}x{} offset:{}x{} visionTarget:{}",
                (int)type,
                width,
                height,
                boolString(inputInfo.scaled),
                inputInfo.scaledWidth,
                inputInfo.scaledHeight,
                inputInfo.contentWidth,
                inputInfo.contentHeight,
                inputInfo.offsetX,
                inputInfo.offsetY,
                m_option ? m_option->visionRectTargetSize : 0);

        NSError* innerError = nil;
#if defined(__APPLE__)
        VNDetectFaceRectanglesRequest* request = m_request
            ? (__bridge VNDetectFaceRectanglesRequest*)m_request
            : [[VNDetectFaceRectanglesRequest alloc] init];
#else
        VNDetectFaceRectanglesRequest* request = [[VNDetectFaceRectanglesRequest alloc] init];
#endif
        VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:pixelBuffer options:@{}];
        const int64_t requestStartMs = HJCurrentSteadyMS();
        const BOOL success = [handler performRequests:@[ request ] error:&innerError];
        timing.visionRequestElapsedMs = HJCurrentSteadyMS() - requestStartMs;
        if (!success)
        {
            HJFLoge("HJFaceDetectIOSVisionRect Vision request failed err:{}",
                innerError ? toStdString(innerError.localizedDescription ?: @"unknown") : std::string("unknown"));
            i_err = HJErrFatal;
            break;
        }

        const int64_t collectStartMs = HJCurrentSteadyMS();
        NSArray<VNFaceObservation*>* results = request.results ? request.results : @[];
        std::vector<VisionFaceItem> faces;
        faces.reserve((size_t)results.count);
        for (VNFaceObservation* obs in results)
        {
            if (!obs)
            {
                continue;
            }

            const int detectWidth = (inputInfo.scaled && inputInfo.scaledWidth > 0) ? inputInfo.scaledWidth : width;
            const int detectHeight = (inputInfo.scaled && inputInfo.scaledHeight > 0) ? inputInfo.scaledHeight : height;
            HJFaceDetectRect rect;
            if (!convertVisionBoundingBoxToRect(obs.boundingBox, detectWidth, detectHeight, rect))
            {
                continue;
            }

            VisionFaceItem item;
            item.rect = rect;
            item.confidence = obs.confidence;
            faces.push_back(item);
        }

        std::sort(faces.begin(), faces.end(), [](const VisionFaceItem& lhs, const VisionFaceItem& rhs)
        {
            if (lhs.confidence != rhs.confidence)
            {
                return lhs.confidence > rhs.confidence;
            }
            const float lhsArea = lhs.rect.width * lhs.rect.height;
            const float rhsArea = rhs.rect.width * rhs.rect.height;
            if (lhsArea != rhsArea)
            {
                return lhsArea > rhsArea;
            }
            if (lhs.rect.y != rhs.rect.y)
            {
                return lhs.rect.y < rhs.rect.y;
            }
            return lhs.rect.x < rhs.rect.x;
        });
        timing.resultCollectElapsedMs = HJCurrentSteadyMS() - collectStartMs;

        const int64_t t1 = HJCurrentSteadyMS();
        timing.totalElapsedMs = t1 - t0;
        o_ret.reset();
        o_ret.setSize(width, height);
        o_ret.m_elapseMs = timing.totalElapsedMs;
        o_ret.m_systemTime = t0;
        o_ret.m_timeStamp = timestamp;
        o_ret.m_bContainFace = !faces.empty();
        for (const auto& face : faces)
        {
            std::shared_ptr<HJSingleFaceDetectRet> singleFace = std::make_shared<HJSingleFaceDetectRet>();
            if (inputInfo.scaled &&
                inputInfo.contentWidth > 0 &&
                inputInfo.contentHeight > 0)
            {
                const float sx = (float)width / (float)inputInfo.contentWidth;
                const float sy = (float)height / (float)inputInfo.contentHeight;
                const float x = ((float)face.rect.x - (float)inputInfo.offsetX) * sx;
                const float y = ((float)face.rect.y - (float)inputInfo.offsetY) * sy;
                const float w = face.rect.width * sx;
                const float h = face.rect.height * sy;
                const float x0 = (std::max)(0.0f, x);
                const float y0 = (std::max)(0.0f, y);
                const float x1 = (std::min)((float)width, x + w);
                const float y1 = (std::min)((float)height, y + h);
                if (x1 <= x0 || y1 <= y0)
                {
                    continue;
                }
                singleFace->setRect(x0, y0, x1 - x0, y1 - y0);
            }
            else
            {
                singleFace->setRect(face.rect.x, face.rect.y, face.rect.width, face.rect.height);
            }
            o_ret.add(singleFace);
        }
        HJFLogi("HJFaceDetectIOSVisionRect detect faces:{} input:{}x{} scaled:{} scaledInput:{}x{} content:{}x{} offset:{}x{} total:{} pixelCreate:{} inputConvert:{} inputCopy:{} visionRequest:{} resultCollect:{} revision:{} preferBackground:{} explicitCompute:{}",
                (int)faces.size(),
                width, height,
                boolString(inputInfo.scaled),
                inputInfo.scaledWidth, inputInfo.scaledHeight,
                inputInfo.contentWidth, inputInfo.contentHeight,
                inputInfo.offsetX, inputInfo.offsetY,
                timing.totalElapsedMs,
                timing.pixelBufferCreateElapsedMs,
                timing.inputConvertElapsedMs,
                timing.inputCopyElapsedMs,
                timing.visionRequestElapsedMs,
                timing.resultCollectElapsedMs,
                request.revision,
                boolString((bool)request.preferBackgroundProcessing),
                ([request respondsToSelector:@selector(computeDeviceForComputeStage:)] &&
                 [request computeDeviceForComputeStage:VNComputeStageMain] != nil) ? "true" : "false");
#if defined(__APPLE__)
        if ([request respondsToSelector:@selector(supportedComputeStageDevicesAndReturnError:)])
        {
            NSError* detectDeviceError = nil;
            NSDictionary<VNComputeStage, NSArray<id<MLComputeDeviceProtocol>>*>* devices = [request supportedComputeStageDevicesAndReturnError:&detectDeviceError];
            NSArray<NSString*>* mainDeviceNames = describeComputeDevices(devices[VNComputeStageMain]);
            id<MLComputeDeviceProtocol> configuredMainDevice = nil;
            if ([request respondsToSelector:@selector(computeDeviceForComputeStage:)])
            {
                configuredMainDevice = [request computeDeviceForComputeStage:VNComputeStageMain];
            }
            HJFLogi("HJFaceDetectIOSVisionRect compute support main:[{}] configuredMain:{} actualMain:unknown",
                    joinNSStringArray(mainDeviceNames),
                    configuredMainDevice ? toStdString([configuredMainDevice description]) : std::string("auto"));
            if (detectDeviceError)
            {
                HJFLogi("HJFaceDetectIOSVisionRect compute introspection err:{}",
                        toStdString(detectDeviceError.localizedDescription ?: @"unknown"));
            }
        }
#endif
    } while (false);

    if (pixelBuffer)
    {
        CFRelease(pixelBuffer);
    }
    return i_err;
}

NS_HJ_END
