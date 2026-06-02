#include "HJFaceAcquireWrapper.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJTransferMediaData.h"
#include "HJFacePointsReal.h"

#include <algorithm>
#include <cmath>

NS_HJ_USING

HJFaceAcquireWrapper::HJFaceAcquireWrapper()
{
}

HJFaceAcquireWrapper::~HJFaceAcquireWrapper()
{
    done();
}

int HJFaceAcquireWrapper::init(const std::string& modelDir, HJFaceDetectWrapperType faceDetectType, const HJFaceDetectWrapperOption& option)
{
    m_modelDir = modelDir;
    m_faceDetectType = faceDetectType;
    m_option = option;
    return ensureInit();
}

int HJFaceAcquireWrapper::ensureInit()
{
    if (m_inited && m_faceDetect)
    {
        return HJ_OK;
    }

    if (m_modelDir.empty())
    {
        return HJErrInvalidParams;
    }

    m_faceDetect = std::make_shared<::HJFaceDetectWrapper>();
    if (!m_faceDetect)
    {
        return HJErrNotInited;
    }

    int ret = m_faceDetect->init(
        [this](const std::string& i_faceInfo)
        {
            std::lock_guard<std::mutex> lk(m_faceInfoMutex);
            m_faceInfo = i_faceInfo;
        },
        m_faceDetectType,
        m_modelDir,
        m_option);
    if (ret < 0)
    {
        m_faceDetect = nullptr;
        return ret;
    }

    m_inited = true;
    return HJ_OK;
}

int HJFaceAcquireWrapper::clampPositiveRect(int frameW, int frameH, int& x, int& y, int& w, int& h)
{
    if (frameW <= 0 || frameH <= 0 || w <= 0 || h <= 0)
    {
        return HJErrInvalidParams;
    }

    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }

    if (x >= frameW || y >= frameH)
    {
        return HJErrInvalidParams;
    }

    const int right = (std::min)(frameW, x + w);
    const int bottom = (std::min)(frameH, y + h);
    w = right - x;
    h = bottom - y;
    if (w <= 0 || h <= 0)
    {
        return HJErrInvalidParams;
    }
    return HJ_OK;
}

int HJFaceAcquireWrapper::acquireFaces(const std::shared_ptr<HJTransferMediaData>& input, std::vector<HJFaceAcquireWrapperItem>& outFaces)
{
    outFaces.clear();
    const int64_t acquireStartMs = HJCurrentSteadyMS();
    if (!input)
    {
        return HJErrInvalidParams;
    }

    const int frameW = input->getWidth();
    const int frameH = input->getHeight();
    if (frameW <= 0 || frameH <= 0)
    {
        return HJErrInvalidParams;
    }

    int ret = ensureInit();
    if (ret < 0)
    {
        return ret;
    }

    std::shared_ptr<HJUnifyWrapperData> detectInput = std::make_shared<HJUnifyWrapperData>();
    if (input->getConvertType() == HJConvertDataType_YUVNV12)
    {
        if (!input->getData(0) || !input->getData(1))
        {
            return HJErrInvalidParams;
        }
        detectInput->dataType = HJUnifyWrapperDataType_NV12;
        detectInput->data[0] = input->getData(0);
        detectInput->data[1] = input->getData(1);
        detectInput->nPitch[0] = input->getStride(0);
        detectInput->nPitch[1] = input->getStride(1);
    }
    else if (input->getConvertType() == HJConvertDataType_RGB)
    {
        if (!input->getData(0) || input->getStride(0) <= 0)
        {
            return HJErrInvalidParams;
        }
        detectInput->dataType = HJUnifyWrapperDataType_RGB;
        detectInput->data[0] = input->getData(0);
        detectInput->nPitch[0] = input->getStride(0);
    }
    else
    {
        return HJErrInvalidParams;
    }
    detectInput->width = input->getWidth();
    detectInput->height = input->getHeight();
    detectInput->timeStamp = input->getTimeStamp();
    detectInput->nElapseCount = 1;

    {
        std::lock_guard<std::mutex> lk(m_faceInfoMutex);
        m_faceInfo.clear();
    }

    const int64_t detectCallStartMs = HJCurrentSteadyMS();
    ret = m_faceDetect->detect(detectInput, false, true, false, false);
    const int64_t detectCallElapsedMs = HJCurrentSteadyMS() - detectCallStartMs;
    if (ret < 0)
    {
        return ret;
    }

    std::string faceInfo;
    {
        std::lock_guard<std::mutex> lk(m_faceInfoMutex);
        faceInfo = m_faceInfo;
    }
    if (faceInfo.empty())
    {
        return HJ_OK;
    }

    HJMoreFacePointsReal::Ptr points = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>();
    const int64_t deserialStartMs = HJCurrentSteadyMS();
    ret = points ? points->deserial(faceInfo) : HJErrNotInited;
    const int64_t deserialElapsedMs = HJCurrentSteadyMS() - deserialStartMs;
    if (ret < 0 || !points || !points->isContainFace())
    {
        return HJ_OK;
    }
    const int64_t detectElapsedMs = points->getElapsedTime();
	const int64_t detectSystemTime = points->getSystemTime();

    const bool isNv12 = (input->getConvertType() == HJConvertDataType_YUVNV12);
    const bool isRgb = (input->getConvertType() == HJConvertDataType_RGB);
    if (!isNv12 && !isRgb)
    {
        return HJErrInvalidParams;
    }

    const int srcStride0 = input->getStride(0);
    const int srcStride1 = input->getStride(1);
    unsigned char* srcBase0 = input->getData(0);
    unsigned char* srcBase1 = input->getData(1);
    if (!srcBase0 || srcStride0 <= 0 || (isNv12 && (!srcBase1 || srcStride1 <= 0)))
    {
        return HJErrInvalidParams;
    }

    const int64_t cropBuildStartMs = HJCurrentSteadyMS();
    for (const auto& oneFace : points->getPoints())
    {
        if (!oneFace)
        {
            continue;
        }

        const HJRectf& r = oneFace->getFaceRect();
        int x = (int)std::floor(r.x);
        int y = (int)std::floor(r.y);
        int w = (int)std::ceil(r.w);
        int h = (int)std::ceil(r.h);
        if (clampPositiveRect(frameW, frameH, x, y, w, h) < 0)
        {
            continue;
        }

        HJTransferMediaData::Ptr faceCrop = nullptr;
        if (isNv12)
        {
            x &= ~1;
            y &= ~1;
            w &= ~1;
            h &= ~1;
            if (w < 2 || h < 2)
            {
                continue;
            }
            if (x + w > frameW)
            {
                w = (frameW - x) & ~1;
            }
            if (y + h > frameH)
            {
                h = (frameH - y) & ~1;
            }
            if (w < 2 || h < 2)
            {
                continue;
            }

            unsigned char* cropData[4] = {
                srcBase0 + (size_t)y * srcStride0 + (size_t)x,
                srcBase1 + (size_t)(y / 2) * srcStride1 + (size_t)x,
                nullptr, nullptr
            };
            int cropStride[4] = { srcStride0, srcStride1, 0, 0 };
            faceCrop = HJTransferMediaDataYUVNV12::Create<HJTransferMediaDataYUVNV12>(
                cropData, cropStride, w, h, input->getTimeStamp());
        }
        else
        {
            unsigned char* cropData[4] = {
                srcBase0 + (size_t)y * (size_t)srcStride0 + (size_t)x * 3,
                nullptr, nullptr, nullptr
            };
            int cropStride[4] = { srcStride0, 0, 0, 0 };
            faceCrop = HJTransferMediaDataRGB::Create<HJTransferMediaDataRGB>(
                cropData, cropStride, w, h, input->getTimeStamp());
        }
        if (!faceCrop)
        {
            continue;
        }
        faceCrop->copymap(*input);

        HJFaceAcquireWrapperItem item;
        item.faceData = faceCrop;
        item.faceRect = HJUnifyWrapperRect{ x, y, w, h };
        item.m_elapsedTime = detectElapsedMs;
		item.m_systemTime = detectSystemTime;
        outFaces.push_back(std::move(item));
    }

    const int64_t cropBuildElapsedMs = HJCurrentSteadyMS() - cropBuildStartMs;
    const int64_t acquireTotalElapsedMs = HJCurrentSteadyMS() - acquireStartMs;
    HJFLogi("HJFaceAcquireWrapper acquireFaces total:{} detectCall:{} backend:{} deserial:{} cropBuild:{} faces:{} input:{}x{}",
            acquireTotalElapsedMs,
            detectCallElapsedMs,
            detectElapsedMs,
            deserialElapsedMs,
            cropBuildElapsedMs,
            (int)outFaces.size(),
            frameW,
            frameH);

    return HJ_OK;
}

void HJFaceAcquireWrapper::done()
{
    m_inited = false;
    m_faceDetect = nullptr;
    {
        std::lock_guard<std::mutex> lk(m_faceInfoMutex);
        m_faceInfo.clear();
    }
}
