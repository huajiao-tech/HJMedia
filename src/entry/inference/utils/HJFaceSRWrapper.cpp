#include "HJFaceSRWrapper.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJTransferMediaData.h"
#include "HJTextureDenoiseSRRunner.h"
#include "libyuv.h"
#include "libyuv/scale_rgb.h"
#include "HJSRUtils.h"
#include "stb_image_resize2.h"
#include <utility>
#include "HJUtilitys.h"
#include "HJDataConvert.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

NS_HJ_USING

namespace
{
constexpr float kFaceSRFeatherRatio = 0.08f;
constexpr float kPi = 3.14159265358979323846f;

static HJUnifyWrapperDataType toWrapperDataType(HJConvertDataType i_type)
{
    switch (i_type)
    {
    case HJConvertDataType_YUVNV12:
        return HJUnifyWrapperDataType_NV12;
    case HJConvertDataType_RGB:
        return HJUnifyWrapperDataType_RGB;
    case HJConvertDataType_RGBA:
        return HJUnifyWrapperDataType_RGBA;
    default:
        return HJUnifyWrapperDataType_Unknown;
    }
}

static std::shared_ptr<HJUnifyWrapperData> createWrapperOutputView(const HJTransferMediaData::Ptr& i_output)
{
    if (!i_output)
    {
        return nullptr;
    }

    const HJUnifyWrapperDataType dataType = toWrapperDataType(i_output->getConvertType());
    if (dataType == HJUnifyWrapperDataType_Unknown)
    {
        HJFLoge("HJFaceSRWrapper unsupported output convert type:{}", (int)i_output->getConvertType());
        return nullptr;
    }

    auto data = std::make_shared<HJUnifyWrapperData>();
    data->dataType = dataType;
    data->width = i_output->getWidth();
    data->height = i_output->getHeight();
    data->timeStamp = i_output->getTimeStamp();

    const int planeCount = (dataType == HJUnifyWrapperDataType_NV12) ? 2 : 1;
    for (int i = 0; i < planeCount; ++i)
    {
        unsigned char* src = i_output->getData(i);
        const int stride = i_output->getStride(i);
        if (!src || stride <= 0)
        {
            HJFLoge("HJFaceSRWrapper invalid output plane:{} stride:{}", i, stride);
            return nullptr;
        }
        data->data[i] = src;
        data->nPitch[i] = stride;
    }
    return data;
}

static HJTransferMediaData::Ptr createTransferInput(const std::shared_ptr<HJUnifyWrapperData>& i_input)
{
    if (!i_input)
    {
        return nullptr;
    }

    switch (i_input->dataType)
    {
    case HJUnifyWrapperDataType_NV12:
        return HJTransferMediaDataYUVNV12::CreateView(
            i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
    case HJUnifyWrapperDataType_RGB:
        return HJTransferMediaDataRGB::CreateView(
            i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
    case HJUnifyWrapperDataType_RGBA:
        return HJTransferMediaDataRGBA::CreateView(
            i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
    default:
        return nullptr;
    }
}

static float sinc(float x)
{
    if (std::fabs(x) < 1e-6f)
    {
        return 1.0f;
    }
    const float pix = kPi * x;
    return std::sin(pix) / pix;
}

static float lanczosKernel(float x, float radius)
{
    const float ax = std::fabs(x);
    if (ax >= radius)
    {
        return 0.0f;
    }
    return sinc(x) * sinc(x / radius);
}

static float lanczos3Kernel(float x, float scale, void* user_data)
{
    (void)scale;
    (void)user_data;
    return lanczosKernel(x, 3.0f);
}

static float lanczos4Kernel(float x, float scale, void* user_data)
{
    (void)scale;
    (void)user_data;
    return lanczosKernel(x, 4.0f);
}

static float lanczos3Support(float scale, void* user_data)
{
    (void)scale;
    (void)user_data;
    return 3.0f;
}

static float lanczos4Support(float scale, void* user_data)
{
    (void)scale;
    (void)user_data;
    return 4.0f;
}

static const char* toProcPolicyName(HJFaceSRProcessPolicy policy)
{
    switch (policy)
    {
        case HJFaceSRProcessPolicy_Mipmap:
            return "Mipmap";
		case HJFaceSRProcessPolicy_Bilinear:
			return "Bilinear";
		case HJFaceSRProcessPolicy_Bicubic:
            return "Bicubic";
        case HJFaceSRProcessPolicy_Lanczos3:
            return "Lanczos3";
        case HJFaceSRProcessPolicy_Lanczos4:
            return "Lanczos4";
    }
    return "Lanczos3";
}

static int scaleARGBWithPolicy(
    const unsigned char* srcArgb,
    int srcArgbStride,
    int srcW,
    int srcH,
    unsigned char* dstArgb,
    int dstArgbStride,
    int dstW,
    int dstH,
    HJFaceSRProcessPolicy i_procPolicy)
{
    if (!srcArgb || !dstArgb || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
    {
        return -1;
    }

    if (srcW == dstW && srcH == dstH)
    {
        return libyuv::ARGBCopy(srcArgb, srcArgbStride, dstArgb, dstArgbStride, srcW, srcH);
    }

    if (i_procPolicy == HJFaceSRProcessPolicy_Mipmap)
    {
        std::vector<HJSPBuffer::Ptr> mipChain;
        HJSPBuffer::Ptr srcCopy = HJSPBuffer::create((size_t)srcArgbStride * (size_t)srcH);
        if (!srcCopy || !srcCopy->getBuf())
        {
            return -1;
        }
        if (libyuv::ARGBCopy(srcArgb, srcArgbStride, srcCopy->getBuf(), srcArgbStride, srcW, srcH) != 0)
        {
            return -1;
        }
        mipChain.push_back(srcCopy);

        const float scale = (float)srcW / (float)dstW;
        const int maxLevel = (int)std::ceil(std::log2((std::max)(1.0f, scale)));
        for (int i = 0; i < maxLevel; ++i)
        {
            const int prevW = (std::max)(1, srcW >> i);
            const int prevH = (std::max)(1, srcH >> i);
            const int nextW = (std::max)(1, srcW >> (i + 1));
            const int nextH = (std::max)(1, srcH >> (i + 1));
            if (nextW == prevW && nextH == prevH)
            {
                break;
            }

            const int nextStride = nextW * 4;
            HJSPBuffer::Ptr nextBuf = HJSPBuffer::create((size_t)nextStride * (size_t)nextH);
            if (!nextBuf || !nextBuf->getBuf())
            {
                return -1;
            }

            if (libyuv::ARGBScale(mipChain.back()->getBuf(), prevW * 4, prevW, prevH,
                    nextBuf->getBuf(), nextStride, nextW, nextH, libyuv::kFilterBox) != 0)
            {
                return -1;
            }
            mipChain.push_back(nextBuf);
        }

        const float logScale = std::log2((std::max)(1.0f, scale));
        const int level0 = (std::max)(0, (std::min)((int)std::floor(logScale), (int)mipChain.size() - 1));
        const int level1 = (std::max)(level0, (std::min)(level0 + 1, (int)mipChain.size() - 1));
        const float t = logScale - (float)level0;

        HJSPBuffer::Ptr res0 = HJSPBuffer::create((size_t)dstArgbStride * (size_t)dstH);
        HJSPBuffer::Ptr res1 = HJSPBuffer::create((size_t)dstArgbStride * (size_t)dstH);
        if (!res0 || !res0->getBuf() || !res1 || !res1->getBuf())
        {
            return -1;
        }

        const int lw0 = (std::max)(1, srcW >> level0);
        const int lh0 = (std::max)(1, srcH >> level0);
        if (libyuv::ARGBScale(mipChain[level0]->getBuf(), lw0 * 4, lw0, lh0,
                res0->getBuf(), dstArgbStride, dstW, dstH, libyuv::kFilterBilinear) != 0)
        {
            return -1;
        }

        const int lw1 = (std::max)(1, srcW >> level1);
        const int lh1 = (std::max)(1, srcH >> level1);
        if (libyuv::ARGBScale(mipChain[level1]->getBuf(), lw1 * 4, lw1, lh1,
                res1->getBuf(), dstArgbStride, dstW, dstH, libyuv::kFilterBilinear) != 0)
        {
            return -1;
        }

        unsigned char* p0 = res0->getBuf();
        unsigned char* p1 = res1->getBuf();
        const int w1 = (int)(t * 256.0f);
        const int w0 = 256 - w1;
        for (size_t i = 0; i < (size_t)dstArgbStride * (size_t)dstH; ++i)
        {
            dstArgb[i] = (unsigned char)((p0[i] * w0 + p1[i] * w1) >> 8);
        }
        return 0;
    }

    if (i_procPolicy == HJFaceSRProcessPolicy_Bicubic)
    {
        void* resizeRet = stbir_resize(
            srcArgb, srcW, srcH, srcArgbStride,
            dstArgb, dstW, dstH, dstArgbStride,
            STBIR_4CHANNEL, STBIR_TYPE_UINT8,
            STBIR_EDGE_CLAMP, STBIR_FILTER_CATMULLROM);
        return (resizeRet != nullptr) ? 0 : -1;
    }

    if (i_procPolicy == HJFaceSRProcessPolicy_Lanczos3 || i_procPolicy == HJFaceSRProcessPolicy_Lanczos4)
    {
        STBIR_RESIZE resize;
        stbir_resize_init(
            &resize,
            srcArgb, srcW, srcH, srcArgbStride,
            dstArgb, dstW, dstH, dstArgbStride,
            STBIR_4CHANNEL, STBIR_TYPE_UINT8);

        int stbirRet = stbir_set_edgemodes(&resize, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP);
        if (stbirRet)
        {
            if (i_procPolicy == HJFaceSRProcessPolicy_Lanczos3)
            {
                stbirRet = stbir_set_filter_callbacks(&resize, lanczos3Kernel, lanczos3Support, lanczos3Kernel, lanczos3Support);
            }
            else
            {
                stbirRet = stbir_set_filter_callbacks(&resize, lanczos4Kernel, lanczos4Support, lanczos4Kernel, lanczos4Support);
            }
        }
        if (stbirRet)
        {
            stbirRet = stbir_resize_extended(&resize);
        }
        return stbirRet ? 0 : -1;
    }

    return libyuv::ARGBScale(
        srcArgb, srcArgbStride,
        srcW, srcH,
        dstArgb, dstArgbStride,
        dstW, dstH,
        libyuv::kFilterBilinear);
}

static int scaleRGBWithPolicy(
    const unsigned char* srcRgb,
    int srcRgbStride,
    int srcW,
    int srcH,
    unsigned char* dstRgb,
    int dstRgbStride,
    int dstW,
    int dstH,
    HJFaceSRProcessPolicy i_procPolicy)
{
    if (!srcRgb || !dstRgb || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
    {
        return -1;
    }

    if (srcW == dstW && srcH == dstH)
    {
        for (int y = 0; y < srcH; ++y)
        {
            memcpy(dstRgb + (size_t)y * (size_t)dstRgbStride,
                   srcRgb + (size_t)y * (size_t)srcRgbStride,
                   (size_t)srcW * 3);
        }
        return 0;
    }

    if (i_procPolicy == HJFaceSRProcessPolicy_Mipmap)
    {
        const int dstRowBytes = dstW * 3;
        std::vector<HJSPBuffer::Ptr> mipChain;
        HJSPBuffer::Ptr srcCopy = HJSPBuffer::create((size_t)srcRgbStride * (size_t)srcH);
        if (!srcCopy || !srcCopy->getBuf())
        {
            return -1;
        }
        for (int y = 0; y < srcH; ++y)
        {
            memcpy(srcCopy->getBuf() + (size_t)y * (size_t)srcRgbStride,
                   srcRgb + (size_t)y * (size_t)srcRgbStride,
                   (size_t)srcW * 3);
        }
        mipChain.push_back(srcCopy);

        const float scale = (float)srcW / (float)dstW;
        const int maxLevel = (int)std::ceil(std::log2((std::max)(1.0f, scale)));
        for (int i = 0; i < maxLevel; ++i)
        {
            const int prevW = (std::max)(1, srcW >> i);
            const int prevH = (std::max)(1, srcH >> i);
            const int nextW = (std::max)(1, srcW >> (i + 1));
            const int nextH = (std::max)(1, srcH >> (i + 1));
            if (nextW == prevW && nextH == prevH)
            {
                break;
            }

            const int nextStride = nextW * 3;
            HJSPBuffer::Ptr nextBuf = HJSPBuffer::create((size_t)nextStride * (size_t)nextH);
            if (!nextBuf || !nextBuf->getBuf())
            {
                return -1;
            }

            if (libyuv::RGBScale(mipChain.back()->getBuf(), prevW * 3, prevW, prevH,
                    nextBuf->getBuf(), nextStride, nextW, nextH, libyuv::kFilterBox) != 0)
            {
                return -1;
            }
            mipChain.push_back(nextBuf);
        }

        const float logScale = std::log2((std::max)(1.0f, scale));
        const int level0 = (std::max)(0, (std::min)((int)std::floor(logScale), (int)mipChain.size() - 1));
        const int level1 = (std::max)(level0, (std::min)(level0 + 1, (int)mipChain.size() - 1));
        const float t = logScale - (float)level0;

        HJSPBuffer::Ptr res0 = HJSPBuffer::create((size_t)dstRowBytes * (size_t)dstH);
        HJSPBuffer::Ptr res1 = HJSPBuffer::create((size_t)dstRowBytes * (size_t)dstH);
        if (!res0 || !res0->getBuf() || !res1 || !res1->getBuf())
        {
            return -1;
        }

        const int lw0 = (std::max)(1, srcW >> level0);
        const int lh0 = (std::max)(1, srcH >> level0);
        if (libyuv::RGBScale(mipChain[level0]->getBuf(), lw0 * 3, lw0, lh0,
                res0->getBuf(), dstRowBytes, dstW, dstH, libyuv::kFilterBilinear) != 0)
        {
            return -1;
        }

        const int lw1 = (std::max)(1, srcW >> level1);
        const int lh1 = (std::max)(1, srcH >> level1);
        if (libyuv::RGBScale(mipChain[level1]->getBuf(), lw1 * 3, lw1, lh1,
                res1->getBuf(), dstRowBytes, dstW, dstH, libyuv::kFilterBilinear) != 0)
        {
            return -1;
        }

        const int w1 = (int)(t * 256.0f);
        const int w0 = 256 - w1;
        for (int y = 0; y < dstH; ++y)
        {
            unsigned char* dstRow = dstRgb + (size_t)y * (size_t)dstRgbStride;
            unsigned char* p0Row = res0->getBuf() + (size_t)y * (size_t)dstRowBytes;
            unsigned char* p1Row = res1->getBuf() + (size_t)y * (size_t)dstRowBytes;
            for (int x = 0; x < dstRowBytes; ++x)
            {
                dstRow[x] = (unsigned char)((p0Row[x] * w0 + p1Row[x] * w1) >> 8);
            }
        }
        return 0;
    }

    if (i_procPolicy == HJFaceSRProcessPolicy_Bicubic)
    {
        void* resizeRet = stbir_resize(
            srcRgb, srcW, srcH, srcRgbStride,
            dstRgb, dstW, dstH, dstRgbStride,
            (stbir_pixel_layout)STBIR_RGB, STBIR_TYPE_UINT8,
            STBIR_EDGE_CLAMP, STBIR_FILTER_CATMULLROM);
        return (resizeRet != nullptr) ? 0 : -1;
    }

    if (i_procPolicy == HJFaceSRProcessPolicy_Lanczos3 || i_procPolicy == HJFaceSRProcessPolicy_Lanczos4)
    {
        STBIR_RESIZE resize;
        stbir_resize_init(
            &resize,
            srcRgb, srcW, srcH, srcRgbStride,
            dstRgb, dstW, dstH, dstRgbStride,
            (stbir_pixel_layout)STBIR_RGB, STBIR_TYPE_UINT8);

        int stbirRet = stbir_set_edgemodes(&resize, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP);
        if (stbirRet)
        {
            if (i_procPolicy == HJFaceSRProcessPolicy_Lanczos3)
            {
                stbirRet = stbir_set_filter_callbacks(&resize, lanczos3Kernel, lanczos3Support, lanczos3Kernel, lanczos3Support);
            }
            else
            {
                stbirRet = stbir_set_filter_callbacks(&resize, lanczos4Kernel, lanczos4Support, lanczos4Kernel, lanczos4Support);
            }
        }
        if (stbirRet)
        {
            stbirRet = stbir_resize_extended(&resize);
        }
        return stbirRet ? 0 : -1;
    }

    return libyuv::RGBScale(
        srcRgb, srcRgbStride,
        srcW, srcH,
        dstRgb, dstRgbStride,
        dstW, dstH,
        libyuv::kFilterBilinear);
}

static HJTransferMediaData::Ptr createOwningRGBDataFromBuffer(
    const HJSPBuffer::Ptr& rgbBuffer,
    int width,
    int height,
    int64_t timestamp,
    const HJTransferMediaData::Ptr& metaSource)
{
    if (!rgbBuffer || !rgbBuffer->getBuf() || width <= 0 || height <= 0)
    {
        return nullptr;
    }
    HJTransferMediaData::Ptr data = std::make_shared<HJTransferMediaDataRGB>();
    if (!data)
    {
        return nullptr;
    }
    data->setBuffer(rgbBuffer);
    data->setStorageMode(HJTransferMediaStorageMode_Owning);
    data->setData(0, rgbBuffer->getBuf());
    data->setStride(0, width * 3);
    data->setWidth(width);
    data->setHeight(height);
    data->setTimeStamp(timestamp);
    if (metaSource)
    {
        data->copymap(*metaSource);
    }
    return data;
}

static HJTransferMediaData::Ptr createOwningRGBADataFromBuffer(
    const HJSPBuffer::Ptr& rgbaBuffer,
    int width,
    int height,
    int64_t timestamp,
    const HJTransferMediaData::Ptr& metaSource)
{
    if (!rgbaBuffer || !rgbaBuffer->getBuf() || width <= 0 || height <= 0)
    {
        return nullptr;
    }
    HJTransferMediaData::Ptr data = std::make_shared<HJTransferMediaDataRGBA>();
    if (!data)
    {
        return nullptr;
    }
    data->setBuffer(rgbaBuffer);
    data->setStorageMode(HJTransferMediaStorageMode_Owning);
    data->setData(0, rgbaBuffer->getBuf());
    data->setStride(0, width * 4);
    data->setWidth(width);
    data->setHeight(height);
    data->setTimeStamp(timestamp);
    if (metaSource)
    {
        data->copymap(*metaSource);
    }
    return data;
}

static HJTransferMediaData::Ptr createOwningNV12DataFromBuffer(
    const HJSPBuffer::Ptr& nv12Buffer,
    int width,
    int height,
    int64_t timestamp,
    const HJTransferMediaData::Ptr& metaSource)
{
    if (!nv12Buffer || !nv12Buffer->getBuf() || width <= 0 || height <= 0)
    {
        return nullptr;
    }
    HJTransferMediaData::Ptr data = std::make_shared<HJTransferMediaDataYUVNV12>();
    if (!data)
    {
        return nullptr;
    }
    data->setBuffer(nv12Buffer);
    data->setStorageMode(HJTransferMediaStorageMode_Owning);
    data->setData(0, nv12Buffer->getBuf());
    data->setData(1, nv12Buffer->getBuf() + (size_t)width * (size_t)height);
    data->setStride(0, width);
    data->setStride(1, width);
    data->setWidth(width);
    data->setHeight(height);
    data->setTimeStamp(timestamp);
    if (metaSource)
    {
        data->copymap(*metaSource);
    }
    return data;
}

static std::shared_ptr<HJTransferMediaData> cropRGBAFrame(
    const std::shared_ptr<HJTransferMediaData>& rgbaFrame,
    int cropX,
    int cropY,
    int cropW,
    int cropH)
{
    if (!rgbaFrame || rgbaFrame->getConvertType() != HJConvertDataType_RGBA || !rgbaFrame->getData(0))
    {
        return nullptr;
    }

    if (cropX < 0 || cropY < 0 || cropW <= 0 || cropH <= 0)
    {
        return nullptr;
    }

    if (cropX + cropW > rgbaFrame->getWidth() || cropY + cropH > rgbaFrame->getHeight())
    {
        return nullptr;
    }

    unsigned char* cropData[4] = {
        rgbaFrame->getData(0) + (size_t)cropY * (size_t)rgbaFrame->getStride(0) + (size_t)cropX * 4,
        nullptr, nullptr, nullptr
    };
    int cropStride[4] = { rgbaFrame->getStride(0), 0, 0, 0 };
    HJTransferMediaData::Ptr crop = HJTransferMediaDataRGBA::CreateView(
        cropData, cropStride, cropW, cropH, rgbaFrame->getTimeStamp());
    if (crop)
    {
        crop->copymap(*rgbaFrame);
    }
    return crop;
}

static float clampSRAlphaRatio(float srAlphaRatio)
{
    return (std::max)(0.0f, (std::min)(1.0f, srAlphaRatio));
}

static void applyFeatherAndGlobalAlphaToRGBA(
    const HJTransferMediaData::Ptr& rgbaFrame,
    bool applyFeatherAlpha,
    float srAlphaRatio,
    int& outFeatherPx)
{
    outFeatherPx = 0;
    if (!rgbaFrame || rgbaFrame->getConvertType() != HJConvertDataType_RGBA || !rgbaFrame->getData(0))
    {
        return;
    }

    const int width = rgbaFrame->getWidth();
    const int height = rgbaFrame->getHeight();
    const int stride = rgbaFrame->getStride(0);
    if (width <= 1 || height <= 1 || stride < width * 4)
    {
        return;
    }

    int feather = 0;
    if (applyFeatherAlpha)
    {
        feather = (std::max)(1, (int)std::lround((std::min)(width, height) * kFaceSRFeatherRatio));
        const int maxFeather = (std::min)(width, height) / 2;
        if (maxFeather > 0)
        {
            feather = (std::min)(feather, maxFeather);
        }
    }
    outFeatherPx = feather;

    const int alphaScale = (int)std::lround(clampSRAlphaRatio(srAlphaRatio) * 255.0f);
    const bool applyGlobalAlpha = alphaScale < 255;
    if (!applyFeatherAlpha && !applyGlobalAlpha)
    {
        return;
    }

    unsigned char* rgba = rgbaFrame->getData(0);
    for (int y = 0; y < height; ++y)
    {
        unsigned char* row = rgba + (size_t)y * (size_t)stride;
        const int dy = (std::min)(y, height - 1 - y);
        for (int x = 0; x < width; ++x)
        {
            int alpha = 255;
            if (applyFeatherAlpha && feather > 0)
            {
                const int dx = (std::min)(x, width - 1 - x);
                const int d = (std::min)(dx, dy);
                alpha = d >= feather ? 255 : (d * 255) / feather;
            }
            if (applyGlobalAlpha)
            {
                alpha = (alpha * alphaScale + 127) / 255;
            }
            row[(size_t)x * 4 + 3] = (unsigned char)alpha;
        }
    }
}
}

HJFaceSRWrapper::HJFaceSRWrapper()
{
}

HJFaceSRWrapper::~HJFaceSRWrapper()
{
    done();
}

int HJFaceSRWrapper::init(
    const std::string& modelDir,
    HJFaceDetectWrapperType faceDetectType,
    HJVideoSRWrapperType srType,
    const HJFaceDetectWrapperOption& faceDetectOption,
    const HJVideoSRWrapperOption& srOption)
{
    if (modelDir.empty())
    {
        return HJErrInvalidParams;
    }

    done();
    std::lock_guard<std::mutex> guard(m_mutex);

#if defined(WIN32_LIB) || defined(ANDROID_LIB)
    if (srType == HJVideoSRWrapperType_TEXTUREFSR)
    {
        m_textureDenoiseSR = std::make_unique<HJTextureDenoiseSRRunner>();
        if (!m_textureDenoiseSR)
        {
            return HJErrNotInited;
        }
        int ret = m_textureDenoiseSR->init(
            srOption.textureFsrDenoiseStrength,
            srOption.textureFsrSharpness,
            srOption.textureFsrScale,
            srOption.textureFsrEnableExtraSharpen,
            srOption.textureFsrExtraSharpenBoost);
        if (ret < 0)
        {
            m_textureDenoiseSR.reset();
            return ret;
        }
        m_srType = srType;
        m_inited = true;
        return HJ_OK;
    }
#endif

    m_faceAcquireWrapper = std::make_unique<HJFaceAcquireWrapper>();
    m_srWrapper = std::make_unique<HJVideoSRWrapper>();
    if (!m_faceAcquireWrapper || !m_srWrapper)
    {
        m_faceAcquireWrapper.reset();
        m_srWrapper.reset();
        return HJErrNotInited;
    }

    int ret = m_faceAcquireWrapper->init(modelDir, faceDetectType, faceDetectOption);
    if (ret < 0)
    {
        m_faceAcquireWrapper.reset();
        m_srWrapper.reset();
        return ret;
    }

    ret = m_srWrapper->init(
        [this](std::shared_ptr<HJ::HJTransferMediaData> i_output, const HJ::HJSRRet& i_ret)
        {
            std::lock_guard<std::mutex> lk(m_srMutex);
            m_srOutput = i_output;
            m_srElapseMs = i_ret.m_elapseMs;
        },
        srType,
        modelDir,
        srOption);
    if (ret < 0)
    {
        m_faceAcquireWrapper.reset();
        m_srWrapper.reset();
        return ret;
    }

    {
        std::lock_guard<std::mutex> lk(m_srMutex);
        m_srOutput = nullptr;
        m_srElapseMs = -1;
    }
    m_srType = srType;
    m_inited = true;
    return HJ_OK;
}

std::shared_ptr<HJTransferMediaData> HJFaceSRWrapper::preScaleInputIfNeeded(
    const std::shared_ptr<HJTransferMediaData>& inputData,
    int preScaleWidth,
    int preScaleHeight,
    HJFaceSRProcessPolicy i_procPolicy,
    bool forceCanvasSize,
    HJFaceSRPreScaleMeta& outMeta)
{
    outMeta = HJFaceSRPreScaleMeta();
    if (!inputData)
    {
        return nullptr;
    }
    if (preScaleWidth <= 0 || preScaleHeight <= 0)
    {
        return inputData;
    }

    const int inputW = inputData->getWidth();
    const int inputH = inputData->getHeight();
    if (inputW <= 0 || inputH <= 0)
    {
        return inputData;
    }

    if (inputW == preScaleWidth && inputH == preScaleHeight)
    {
        return inputData;
    }

    if (!forceCanvasSize && inputW <= preScaleWidth && inputH <= preScaleHeight)
    {
        return inputData;
    }

    HJVec4i vec = HJDataConvert::cvtGetScaleOffset(HJDataScaleType::Fit, inputW, inputH, preScaleWidth, preScaleHeight);
    const int contentW = vec.x;
    const int contentH = vec.y;
    const int offsetX = vec.z;
    const int offsetY = vec.w;
    if (contentW <= 0 || contentH <= 0 || offsetX < 0 || offsetY < 0)
    {
        return inputData;
    }

    int64_t t0 = HJCurrentSteadyMS();
    if (inputData->getConvertType() == HJConvertDataType_RGB)
    {
        if (!inputData->getData(0) || inputData->getStride(0) <= 0)
        {
            return inputData;
        }

        HJSPBuffer::Ptr rgbCanvas = HJSPBuffer::create((size_t)preScaleWidth * (size_t)preScaleHeight * 3);
        if (!rgbCanvas || !rgbCanvas->getBuf())
        {
            return inputData;
        }
        std::memset(rgbCanvas->getBuf(), 0, (size_t)preScaleWidth * (size_t)preScaleHeight * 3);

        if (scaleRGBWithPolicy(
                inputData->getData(0),
                inputData->getStride(0),
                inputW,
                inputH,
                rgbCanvas->getBuf() + ((size_t)offsetY * (size_t)preScaleWidth + (size_t)offsetX) * 3,
                preScaleWidth * 3,
                contentW,
                contentH,
                i_procPolicy) == 0)
        {
            HJTransferMediaData::Ptr newScaleFace = createOwningRGBDataFromBuffer(
                rgbCanvas, preScaleWidth, preScaleHeight, inputData->getTimeStamp(), inputData);
            if (newScaleFace)
            {
                outMeta.canvasW = preScaleWidth;
                outMeta.canvasH = preScaleHeight;
                outMeta.contentW = contentW;
                outMeta.contentH = contentH;
                outMeta.offsetX = offsetX;
                outMeta.offsetY = offsetY;
                outMeta.applied = true;
                int64_t t1 = HJCurrentSteadyMS();
                HJFPERLog5i("HJFaceSRWrapper preScaleInputIfNeeded fastRGB (policy:{}) from {}x{} to canvas {}x{} fit {}x{} off:{}x{}, elapse {} ms",
                    toProcPolicyName(i_procPolicy), inputW, inputH, preScaleWidth, preScaleHeight, contentW, contentH, offsetX, offsetY, t1 - t0);
                return newScaleFace;
            }
        }
    }
    else if (inputData->getConvertType() == HJConvertDataType_YUVNV12 &&
             preScaleWidth > 0 && preScaleHeight > 0 &&
             (i_procPolicy == HJFaceSRProcessPolicy_Bilinear || i_procPolicy == HJFaceSRProcessPolicy_Mipmap))
    {
        if (!inputData->getData(0) || !inputData->getData(1) || inputData->getStride(0) <= 0 || inputData->getStride(1) <= 0)
        {
            return inputData;
        }

        int canvasW = preScaleWidth;
        int canvasH = preScaleHeight;
        int nv12ContentW = contentW;
        int nv12ContentH = contentH;
        int nv12OffsetX = offsetX;
        int nv12OffsetY = offsetY;
        if (((canvasW | canvasH | nv12ContentW | nv12ContentH | nv12OffsetX | nv12OffsetY) & 1) == 0)
        {
            HJSPBuffer::Ptr nv12Canvas = HJSPBuffer::create((size_t)canvasW * (size_t)canvasH * 3 / 2);
            if (!nv12Canvas || !nv12Canvas->getBuf())
            {
                return inputData;
            }
            std::memset(nv12Canvas->getBuf(), 16, (size_t)canvasW * (size_t)canvasH);
            std::memset(nv12Canvas->getBuf() + (size_t)canvasW * (size_t)canvasH, 128, (size_t)canvasW * (size_t)canvasH / 2);

            const libyuv::FilterMode filter = (i_procPolicy == HJFaceSRProcessPolicy_Mipmap)
                ? libyuv::kFilterBox
                : libyuv::kFilterBilinear;
            const int scaleRet = libyuv::NV12Scale(
                inputData->getData(0), inputData->getStride(0),
                inputData->getData(1), inputData->getStride(1),
                inputW, inputH,
                nv12Canvas->getBuf() + (size_t)nv12OffsetY * (size_t)canvasW + (size_t)nv12OffsetX, canvasW,
                nv12Canvas->getBuf() + (size_t)canvasW * (size_t)canvasH + ((size_t)nv12OffsetY * (size_t)canvasW) / 2 + (size_t)nv12OffsetX, canvasW,
                nv12ContentW, nv12ContentH,
                filter);
            if (scaleRet == 0)
            {
                HJTransferMediaData::Ptr newScaleFace = createOwningNV12DataFromBuffer(
                    nv12Canvas, canvasW, canvasH, inputData->getTimeStamp(), inputData);
                if (newScaleFace)
                {
                    outMeta.canvasW = canvasW;
                    outMeta.canvasH = canvasH;
                    outMeta.contentW = nv12ContentW;
                    outMeta.contentH = nv12ContentH;
                    outMeta.offsetX = nv12OffsetX;
                    outMeta.offsetY = nv12OffsetY;
                    outMeta.applied = true;
                    int64_t t1 = HJCurrentSteadyMS();
                    HJFPERLog5i("HJFaceSRWrapper preScaleInputIfNeeded fastNV12 (policy:{}) from {}x{} to canvas {}x{} fit {}x{} off:{}x{}, elapse {} ms",
                        toProcPolicyName(i_procPolicy), inputW, inputH, canvasW, canvasH, nv12ContentW, nv12ContentH, nv12OffsetX, nv12OffsetY, t1 - t0);
                    return newScaleFace;
                }
            }
        }
    }

    const int srcArgbStride = inputW * 4;
    HJSPBuffer::Ptr srcArgbBuffer = HJSPBuffer::create((size_t)srcArgbStride * (size_t)inputH);
    if (!srcArgbBuffer || !srcArgbBuffer->getBuf())
    {
        return inputData;
    }

    int libyuvRet = -1;
    if (inputData->getConvertType() == HJConvertDataType_RGB)
    {
        if (!inputData->getData(0) || inputData->getStride(0) <= 0)
        {
            return inputData;
        }
        libyuvRet = libyuv::RAWToARGB(
            inputData->getData(0), inputData->getStride(0),
            srcArgbBuffer->getBuf(), srcArgbStride,
            inputW, inputH);
    }
    else if (inputData->getConvertType() == HJConvertDataType_YUVNV12)
    {
        if (!inputData->getData(0) || !inputData->getData(1) || inputData->getStride(0) <= 0 || inputData->getStride(1) <= 0)
        {
            return inputData;
        }
        libyuvRet = libyuv::NV12ToARGB(
            inputData->getData(0), inputData->getStride(0),
            inputData->getData(1), inputData->getStride(1),
            srcArgbBuffer->getBuf(), srcArgbStride,
            inputW, inputH);
    }
    else if (inputData->getConvertType() == HJConvertDataType_RGBA)
    {
        if (!inputData->getData(0) || inputData->getStride(0) <= 0)
        {
            return inputData;
        }
        libyuvRet = libyuv::ARGBCopy(
            inputData->getData(0), inputData->getStride(0),
            srcArgbBuffer->getBuf(), srcArgbStride,
            inputW, inputH);
    }
    if (libyuvRet != 0)
    {
        HJFLoge("HJFaceSRWrapper preScaleInputIfNeeded inputToARGB failed type:{} ret:{}",
            (int)inputData->getConvertType(), libyuvRet);
        return inputData;
    }

    const int scaledArgbStride = contentW * 4;
    HJSPBuffer::Ptr scaledArgb = HJSPBuffer::create((size_t)scaledArgbStride * (size_t)contentH);
    if (!scaledArgb || !scaledArgb->getBuf())
    {
        return inputData;
    }

    if (scaleARGBWithPolicy(
            srcArgbBuffer->getBuf(),
            srcArgbStride,
            inputW,
            inputH,
            scaledArgb->getBuf(),
            scaledArgbStride,
            contentW,
            contentH,
            i_procPolicy) != 0)
    {
        HJFLoge("HJFaceSRWrapper preScaleInputIfNeeded ARGBScale failed policy:{}", toProcPolicyName(i_procPolicy));
        return inputData;
    }

    const int canvasArgbStride = preScaleWidth * 4;
    HJSPBuffer::Ptr canvasArgb = HJSPBuffer::create((size_t)canvasArgbStride * (size_t)preScaleHeight);
    if (!canvasArgb || !canvasArgb->getBuf())
    {
        return inputData;
    }
    std::memset(canvasArgb->getBuf(), 0, (size_t)canvasArgbStride * (size_t)preScaleHeight);
    if (libyuv::ARGBCopy(
            scaledArgb->getBuf(),
            scaledArgbStride,
            canvasArgb->getBuf() + (size_t)offsetY * (size_t)canvasArgbStride + (size_t)offsetX * 4,
            canvasArgbStride,
            contentW,
            contentH) != 0)
    {
        return inputData;
    }

    HJSPBuffer::Ptr rgbBuffer = HJSPBuffer::create((size_t)preScaleWidth * (size_t)preScaleHeight * 3);
    if (!rgbBuffer || !rgbBuffer->getBuf())
    {
        return inputData;
    }
    libyuvRet = libyuv::ARGBToRAW(
        canvasArgb->getBuf(), canvasArgbStride,
        rgbBuffer->getBuf(), preScaleWidth * 3,
        preScaleWidth, preScaleHeight);
    if (libyuvRet != 0)
    {
        HJFLoge("HJFaceSRWrapper preScaleInputIfNeeded ARGBToRAW failed ret:{}", libyuvRet);
        return inputData;
    }

    unsigned char* rgbData[4] = { rgbBuffer->getBuf(), nullptr, nullptr, nullptr };
    int rgbStride[4] = { preScaleWidth * 3, 0, 0, 0 };
    HJTransferMediaData::Ptr newScaleFace = HJTransferMediaDataRGB::Create<HJTransferMediaDataRGB>(
        rgbData, rgbStride, preScaleWidth, preScaleHeight, inputData->getTimeStamp());
    if (!newScaleFace)
    {
        return inputData;
    }
    newScaleFace->copymap(*inputData);

    outMeta.canvasW = preScaleWidth;
    outMeta.canvasH = preScaleHeight;
    outMeta.contentW = contentW;
    outMeta.contentH = contentH;
    outMeta.offsetX = offsetX;
    outMeta.offsetY = offsetY;
    outMeta.applied = true;

    int64_t t1 = HJCurrentSteadyMS();
    HJFPERLog5i("HJFaceSRWrapper preScaleInputIfNeeded scaled (policy:{}) from {}x{} to canvas {}x{} fit {}x{} off:{}x{}, elapse {} ms",
        toProcPolicyName(i_procPolicy), inputW, inputH, preScaleWidth, preScaleHeight, contentW, contentH, offsetX, offsetY, t1 - t0);
    return newScaleFace;
}

int HJFaceSRWrapper::runSRLocked(
    const std::shared_ptr<HJTransferMediaData>& input,
    HJFaceSRProcessResult& outResult,
    int64_t& outSrElapsedMs,
    bool applyFeatherAlpha,
    float srAlphaRatio,
    int postTargetWidth,
    int postTargetHeight,
    const HJFaceSRPreScaleMeta* preScaleMeta)
{
    const int64_t totalStartMs = HJCurrentSteadyMS();
    if (!input)
    {
        return HJErrInvalidParams;
    }

    const int64_t packStartMs = HJCurrentSteadyMS();
    std::shared_ptr<HJUnifyWrapperData> srInput = std::make_shared<HJUnifyWrapperData>();
    if (input->getConvertType() == HJConvertDataType_RGB)
    {
        if (!input->getData(0) || input->getStride(0) <= 0)
        {
            return HJErrInvalidParams;
        }
        srInput->dataType = HJUnifyWrapperDataType_RGB;
        srInput->data[0] = input->getData(0);
        srInput->nPitch[0] = input->getStride(0);
    }
    else if (input->getConvertType() == HJConvertDataType_YUVNV12)
    {
        if (!input->getData(0) || !input->getData(1) || input->getStride(0) <= 0 || input->getStride(1) <= 0)
        {
            return HJErrInvalidParams;
        }
        srInput->dataType = HJUnifyWrapperDataType_NV12;
        srInput->data[0] = input->getData(0);
        srInput->data[1] = input->getData(1);
        srInput->nPitch[0] = input->getStride(0);
        srInput->nPitch[1] = input->getStride(1);
    }
    else
    {
        return HJErrInvalidParams;
    }
    srInput->width = input->getWidth();
    srInput->height = input->getHeight();
    srInput->timeStamp = input->getTimeStamp();
    srInput->nElapseCount = 1;
    const int64_t packElapsedMs = HJCurrentSteadyMS() - packStartMs;

    {
        std::lock_guard<std::mutex> lk(m_srMutex);
        m_srOutput = nullptr;
        m_srElapseMs = -1;
    }

    const int64_t wrapperProcessStartMs = HJCurrentSteadyMS();
    int ret = m_srWrapper->process(srInput, true);
    const int64_t wrapperProcessElapsedMs = HJCurrentSteadyMS() - wrapperProcessStartMs;
    if (ret < 0)
    {
        return ret;
    }

    const int64_t takeOutputStartMs = HJCurrentSteadyMS();
    HJTransferMediaData::Ptr srOutput = nullptr;
    int64_t srElapsedMs = -1;
    {
        std::lock_guard<std::mutex> lk(m_srMutex);
        srOutput = m_srOutput;
        srElapsedMs = m_srElapseMs;
    }
    const int64_t takeOutputElapsedMs = HJCurrentSteadyMS() - takeOutputStartMs;
    if (!srOutput)
    {
        return HJErrFatal;
    }
    outResult.postResizeInputWidth = srOutput->getWidth();
    outResult.postResizeInputHeight = srOutput->getHeight();
    outResult.postResizeOutputWidth = srOutput->getWidth();
    outResult.postResizeOutputHeight = srOutput->getHeight();

    int64_t cropElapsedMs = 0;
    if (preScaleMeta && preScaleMeta->applied && preScaleMeta->canvasW > 0 && preScaleMeta->canvasH > 0 &&
        preScaleMeta->contentW > 0 && preScaleMeta->contentH > 0 &&
        (preScaleMeta->offsetX > 0 || preScaleMeta->offsetY > 0 ||
         preScaleMeta->contentW != preScaleMeta->canvasW || preScaleMeta->contentH != preScaleMeta->canvasH))
    {
        const int64_t cropStartMs = HJCurrentSteadyMS();
        if (srOutput->getConvertType() != HJConvertDataType_RGBA)
        {
            srOutput = HJTransferMediaData::create(srOutput, HJConvertDataType_RGBA);
        }
        if (!srOutput)
        {
            return HJErrFatal;
        }

        const int outW = srOutput->getWidth();
        const int outH = srOutput->getHeight();
        int cropX = (int)std::lround((double)preScaleMeta->offsetX * (double)outW / (double)preScaleMeta->canvasW);
        int cropY = (int)std::lround((double)preScaleMeta->offsetY * (double)outH / (double)preScaleMeta->canvasH);
        int cropW = (int)std::lround((double)preScaleMeta->contentW * (double)outW / (double)preScaleMeta->canvasW);
        int cropH = (int)std::lround((double)preScaleMeta->contentH * (double)outH / (double)preScaleMeta->canvasH);
        cropX = (std::max)(0, cropX);
        cropY = (std::max)(0, cropY);
        cropW = (std::max)(1, cropW);
        cropH = (std::max)(1, cropH);
        if (cropX + cropW > outW)
        {
            cropW = outW - cropX;
        }
        if (cropY + cropH > outH)
        {
            cropH = outH - cropY;
        }
        HJTransferMediaData::Ptr cropped = cropRGBAFrame(srOutput, cropX, cropY, cropW, cropH);
        if (!cropped)
        {
            HJFLoge("HJFaceSRWrapper crop padded SR output failed out:{}x{} crop:{} {} {} {}", outW, outH, cropX, cropY, cropW, cropH);
            return HJErrFatal;
        }
        if (cropped->isView())
        {
            cropped = HJTransferMediaData::makeOwning(cropped);
        }
        if (!cropped)
        {
            HJFLoge("HJFaceSRWrapper crop padded SR output owning clone failed out:{}x{} crop:{} {} {} {}", outW, outH, cropX, cropY, cropW, cropH);
            return HJErrFatal;
        }
        srOutput = cropped;
        {
            std::lock_guard<std::mutex> lk(m_srMutex);
            m_srOutput = srOutput;
        }
        cropElapsedMs = HJCurrentSteadyMS() - cropStartMs;
        outResult.postResizeInputWidth = srOutput->getWidth();
        outResult.postResizeInputHeight = srOutput->getHeight();
        outResult.postResizeOutputWidth = srOutput->getWidth();
        outResult.postResizeOutputHeight = srOutput->getHeight();
    }

    int64_t postResizeElapsedMs = 0;
    if (postTargetWidth > 0 && postTargetHeight > 0 &&
        (postTargetWidth != srOutput->getWidth() || postTargetHeight != srOutput->getHeight()))
    {
        const int64_t postResizeStartMs = HJCurrentSteadyMS();
        outResult.postResizeInputWidth = srOutput->getWidth();
        outResult.postResizeInputHeight = srOutput->getHeight();
        if (srOutput->getConvertType() != HJConvertDataType_RGBA)
        {
            srOutput = HJTransferMediaData::create(srOutput, HJConvertDataType_RGBA);
        }
        if (!srOutput || !srOutput->getData(0) || srOutput->getStride(0) <= 0)
        {
            return HJErrFatal;
        }

        const int dstStride = postTargetWidth * 4;
        HJSPBuffer::Ptr rgbaBuffer = HJSPBuffer::create((size_t)dstStride * (size_t)postTargetHeight);
        if (!rgbaBuffer || !rgbaBuffer->getBuf())
        {
            return HJErrFatal;
        }

        if (scaleARGBWithPolicy(
                srOutput->getData(0),
                srOutput->getStride(0),
                srOutput->getWidth(),
                srOutput->getHeight(),
                rgbaBuffer->getBuf(),
                dstStride,
                postTargetWidth,
                postTargetHeight,
                HJFaceSRProcessPolicy_Lanczos3) != 0)
        {
            HJFLoge("HJFaceSRWrapper post resize failed src:{}x{} dst:{}x{}",
                    srOutput->getWidth(), srOutput->getHeight(), postTargetWidth, postTargetHeight);
            return HJErrFatal;
        }

        srOutput = createOwningRGBADataFromBuffer(
            rgbaBuffer,
            postTargetWidth,
            postTargetHeight,
            srOutput->getTimeStamp(),
            srOutput);
        if (!srOutput)
        {
            return HJErrFatal;
        }
        {
            std::lock_guard<std::mutex> lk(m_srMutex);
            m_srOutput = srOutput;
        }
        postResizeElapsedMs = HJCurrentSteadyMS() - postResizeStartMs;
        outResult.postResizeOutputWidth = srOutput->getWidth();
        outResult.postResizeOutputHeight = srOutput->getHeight();
    }
    outResult.postResizeElapsedMs = postResizeElapsedMs;

    const float clampedSRAlphaRatio = clampSRAlphaRatio(srAlphaRatio);
    const bool needAlphaProcess = applyFeatherAlpha || clampedSRAlphaRatio < 0.999f;
    int64_t rgbaConvertElapsedMs = 0;
    int64_t featherElapsedMs = 0;
    int64_t globalAlphaElapsedMs = 0;
    int64_t alphaElapsedMs = 0;
    if (needAlphaProcess)
    {
        int64_t tstart = HJCurrentSteadyMS();
        if (srOutput->getConvertType() != HJConvertDataType_RGBA)
        {
            const int64_t rgbaConvertStartMs = HJCurrentSteadyMS();
            srOutput = HJTransferMediaData::create(srOutput, HJConvertDataType_RGBA);
            rgbaConvertElapsedMs = HJCurrentSteadyMS() - rgbaConvertStartMs;
        }
        if (!srOutput)
        {
            return HJErrFatal;
        }

        int featherPx = 0;
        applyFeatherAndGlobalAlphaToRGBA(srOutput, applyFeatherAlpha, clampedSRAlphaRatio, featherPx);
        {
            std::lock_guard<std::mutex> lk(m_srMutex);
            m_srOutput = srOutput;
        }
        alphaElapsedMs = HJCurrentSteadyMS() - tstart;
        featherElapsedMs = applyFeatherAlpha ? alphaElapsedMs : 0;
        globalAlphaElapsedMs = clampedSRAlphaRatio < 0.999f ? alphaElapsedMs : 0;
    }

    outSrElapsedMs = srElapsedMs;
    const int64_t totalElapsedMs = HJCurrentSteadyMS() - totalStartMs;
    HJFLogi("HJFaceSRWrapper runSRLocked total:{} wrapper:{} backend:{} crop:{} postResize:{} alpha:{} rgbaConvert:{} feather:{} globalAlpha:{} input:{}x{} output:{}x{} preScale:{}",
            totalElapsedMs,
            wrapperProcessElapsedMs,
            srElapsedMs,
            cropElapsedMs,
            postResizeElapsedMs,
            alphaElapsedMs,
            rgbaConvertElapsedMs,
            featherElapsedMs,
            globalAlphaElapsedMs,
            input->getWidth(),
            input->getHeight(),
            srOutput ? srOutput->getWidth() : 0,
            srOutput ? srOutput->getHeight() : 0,
            (preScaleMeta && preScaleMeta->applied) ? 1 : 0);
    return HJ_OK;
}

int HJFaceSRWrapper::process(
    const std::shared_ptr<HJTransferMediaData>& input,
    const HJFaceSRProcessOption& i_processOption,
    HJFaceSRProcessResult& outResult)
{
    outResult = HJFaceSRProcessResult();
    const int64_t processStartMs = HJCurrentSteadyMS();
    if (!input)
    {
        return HJErrInvalidParams;
    }

    std::lock_guard<std::mutex> guard(m_mutex);
    if (!m_inited || !m_faceAcquireWrapper || !m_srWrapper)
    {
        return HJErrNotInited;
    }

    const bool bIsFullSR = (i_processOption.mode == HJFaceSRProcessMode_Full);
    const bool bIsFullScale = (i_processOption.mode == HJFaceSRProcessMode_FullScale);
    const bool bFaceScale = (i_processOption.mode == HJFaceSRProcessMode_FaceScale);
    const bool forceCanvasSize = (m_srType == HJVideoSRWrapperType_COREMLREALESRGAN);
    const int preScaleWidth = i_processOption.preScaleWidth;
    const int preScaleHeight = i_processOption.preScaleHeight;
    const int composeCanvasWidth = i_processOption.composeCanvasWidth;
    const int composeCanvasHeight = i_processOption.composeCanvasHeight;
    const bool bEnableFeatherAlpha = i_processOption.bFeather;
    const bool bEnablePostSRDisplayResize = i_processOption.bEnablePostSRDisplayResize;
    const float srAlphaRatio = i_processOption.bMixedEnable ? i_processOption.mixAlphaRatio : 1.0f;
    HJFaceSRProcessPolicy policy = i_processOption.policy;

    if (bIsFullSR || bIsFullScale)
    {
        outResult.faceCount = 1;
        outResult.faceRect = HJUnifyWrapperRect{ 0, 0, input->getWidth(), input->getHeight() };
        outResult.faceDetectElapsedMs = 0;
        HJFaceSRPreScaleMeta preScaleMeta;
        const bool shouldPreScaleFullInput =
            bIsFullScale || (bIsFullSR && preScaleWidth > 0 && preScaleHeight > 0);
        HJTransferMediaData::Ptr srInput = shouldPreScaleFullInput
            ? preScaleInputIfNeeded(input, preScaleWidth, preScaleHeight, policy, forceCanvasSize, preScaleMeta)
            : input;
        if (!srInput)
        {
            return HJErrInvalidParams;
        }
        outResult.scaleW = srInput->getWidth();
        outResult.scaleH = srInput->getHeight();
        outResult.preScaleInputWidth = input->getWidth();
        outResult.preScaleInputHeight = input->getHeight();
        outResult.preScaleOutputWidth = srInput->getWidth();
        outResult.preScaleOutputHeight = srInput->getHeight();
        if (preScaleMeta.applied)
        {
            outResult.padLeft = preScaleMeta.offsetX;
            outResult.padRight = (std::max)(0, preScaleMeta.canvasW - preScaleMeta.contentW - preScaleMeta.offsetX);
            outResult.padTop = preScaleMeta.offsetY;
            outResult.padBottom = (std::max)(0, preScaleMeta.canvasH - preScaleMeta.contentH - preScaleMeta.offsetY);
        }
        return runSRLocked(srInput, outResult, outResult.srElapsedMs, false, srAlphaRatio, 0, 0, preScaleMeta.applied ? &preScaleMeta : nullptr);
    }

    std::vector<HJFaceAcquireWrapperItem> faces;
    const int64_t acquireStartMs = HJCurrentSteadyMS();
    int ret = m_faceAcquireWrapper->acquireFaces(input, faces);
    const int64_t acquireElapsedMs = HJCurrentSteadyMS() - acquireStartMs;
    if (ret < 0)
    {
        return ret;
    }

    outResult.faceCount = static_cast<int>(faces.size());
    if (faces.empty())
    {
        return HJ_WOULD_BLOCK;
    }
    if (!faces[0].faceData)
    {
        return HJ_WOULD_BLOCK;
    }

    outResult.faceRect = faces[0].faceRect;
    outResult.faceDetectElapsedMs = faces[0].m_elapsedTime;
    if (composeCanvasWidth > 0 && composeCanvasHeight > 0 && input->getWidth() > 0)
    {
        outResult.faceTargetDisplayWidth = (outResult.faceRect.w * composeCanvasWidth + (input->getWidth() / 2)) / input->getWidth();
        outResult.faceTargetDisplayHeight = (outResult.faceRect.h * composeCanvasWidth + (input->getWidth() / 2)) / input->getWidth();
        outResult.faceTargetDisplayWidth = (std::max)(0, (std::min)(composeCanvasWidth, outResult.faceTargetDisplayWidth));
        outResult.faceTargetDisplayHeight = (std::max)(0, (std::min)(composeCanvasHeight, outResult.faceTargetDisplayHeight));
    }

    HJFaceSRPreScaleMeta preScaleMeta;
    const int64_t preScaleStartMs = HJCurrentSteadyMS();
    HJTransferMediaData::Ptr srInput = bFaceScale
        ? preScaleInputIfNeeded(faces[0].faceData, preScaleWidth, preScaleHeight, policy, forceCanvasSize, preScaleMeta)
        : faces[0].faceData;
    const int64_t preScaleElapsedMs = HJCurrentSteadyMS() - preScaleStartMs;
    outResult.preScaleElapsedMs = preScaleElapsedMs;
    if (!srInput)
    {
        return HJErrInvalidParams;
    }
    outResult.preScaleInputWidth = faces[0].faceData ? faces[0].faceData->getWidth() : 0;
    outResult.preScaleInputHeight = faces[0].faceData ? faces[0].faceData->getHeight() : 0;
    outResult.preScaleOutputWidth = srInput->getWidth();
    outResult.preScaleOutputHeight = srInput->getHeight();
    outResult.scaleW = srInput->getWidth();
    outResult.scaleH = srInput->getHeight();
    if (preScaleMeta.applied)
    {
        outResult.padLeft = preScaleMeta.offsetX;
        outResult.padRight = (std::max)(0, preScaleMeta.canvasW - preScaleMeta.contentW - preScaleMeta.offsetX);
        outResult.padTop = preScaleMeta.offsetY;
        outResult.padBottom = (std::max)(0, preScaleMeta.canvasH - preScaleMeta.contentH - preScaleMeta.offsetY);
    }

    const int64_t runSRStartMs = HJCurrentSteadyMS();
    ret = runSRLocked(
        srInput,
        outResult,
        outResult.srElapsedMs,
        bEnableFeatherAlpha,
        srAlphaRatio,
        bEnablePostSRDisplayResize ? outResult.faceTargetDisplayWidth : 0,
        bEnablePostSRDisplayResize ? outResult.faceTargetDisplayHeight : 0,
        preScaleMeta.applied ? &preScaleMeta : nullptr);
    const int64_t runSRElapsedMs = HJCurrentSteadyMS() - runSRStartMs;
    if (ret < 0)
    {
        return ret;
    }

    const int64_t processElapsedMs = HJCurrentSteadyMS() - processStartMs;
    HJFLogi("HJFaceSRWrapper process total:{} acquire:{} detectBackend:{} preScale:{} {}x{}->{}x{} runSR:{} postResize:{} {}x{}->{}x{} faces:{} faceRect:{}x{}+{}+{} target:{}x{} scale:{}x{} pad:{} {} {} {}",
            processElapsedMs,
            acquireElapsedMs,
            outResult.faceDetectElapsedMs,
            preScaleElapsedMs,
            outResult.preScaleInputWidth,
            outResult.preScaleInputHeight,
            outResult.preScaleOutputWidth,
            outResult.preScaleOutputHeight,
            runSRElapsedMs,
            outResult.postResizeElapsedMs,
            outResult.postResizeInputWidth,
            outResult.postResizeInputHeight,
            outResult.postResizeOutputWidth,
            outResult.postResizeOutputHeight,
            outResult.faceCount,
            outResult.faceRect.w,
            outResult.faceRect.h,
            outResult.faceRect.x,
            outResult.faceRect.y,
            outResult.faceTargetDisplayWidth,
            outResult.faceTargetDisplayHeight,
            outResult.scaleW,
            outResult.scaleH,
            outResult.padLeft,
            outResult.padRight,
            outResult.padTop,
            outResult.padBottom);

    return HJ_OK;
}

int HJFaceSRWrapper::process(
    const std::shared_ptr<HJUnifyWrapperData>& input,
    const HJFaceSRProcessOption& i_processOption,
    HJFaceSRProcessResult& outResult,
    const HJFaceSROutputCb& outCb)
{
    if (!input || !outCb)
    {
        outResult = HJFaceSRProcessResult();
        return HJErrInvalidParams;
    }

    HJTransferMediaData::Ptr transferInput = createTransferInput(input);
    if (!transferInput)
    {
        outResult = HJFaceSRProcessResult();
        return HJErrInvalidParams;
    }

    const int ret = process(transferInput, i_processOption, outResult);
    if (ret != HJ_OK)
    {
        return ret;
    }

    HJTransferMediaData::Ptr srOutput = takeLastOutput();
    if (!srOutput)
    {
        return HJErrFatal;
    }

    std::shared_ptr<HJUnifyWrapperData> outputView = createWrapperOutputView(srOutput);
    if (!outputView)
    {
        return HJErrFatal;
    }
    outCb(outputView);
    return HJ_OK;
}

int HJFaceSRWrapper::process(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    uint32_t& outputTextureId,
    int& outputWidth,
    int& outputHeight,
    HJFaceSRProcessResult& outResult)
{
    outResult = HJFaceSRProcessResult();
    outputTextureId = 0;
    outputWidth = 0;
    outputHeight = 0;

    std::lock_guard<std::mutex> guard(m_mutex);
    if (!m_inited || m_srType != HJVideoSRWrapperType_TEXTUREFSR || !m_textureDenoiseSR)
    {
        return HJErrNotInited;
    }

    outResult.faceCount = 1;
    outResult.faceRect = HJUnifyWrapperRect{ 0, 0, inputWidth, inputHeight };
    int ret = m_textureDenoiseSR->process(inputTextureId, inputWidth, inputHeight, outputTextureId, outputWidth, outputHeight, &outResult.srElapsedMs);
    if (ret >= 0)
    {
        outResult.scaleW = outputWidth;
        outResult.scaleH = outputHeight;
    }
    return ret;
}

std::shared_ptr<HJTransferMediaData> HJFaceSRWrapper::takeLastOutput()
{
    std::lock_guard<std::mutex> lk(m_srMutex);
    HJTransferMediaData::Ptr output = m_srOutput;
    m_srOutput = nullptr;
    m_srElapseMs = -1;
    return output;
}

void HJFaceSRWrapper::done()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_inited = false;
    if (m_faceAcquireWrapper)
    {
        m_faceAcquireWrapper->done();
    }
    m_faceAcquireWrapper.reset();
    m_srWrapper.reset();
    if (m_textureDenoiseSR)
    {
        m_textureDenoiseSR->done();
    }
    m_textureDenoiseSR.reset();
    {
        std::lock_guard<std::mutex> lk(m_srMutex);
        m_srOutput = nullptr;
        m_srElapseMs = -1;
    }
}
