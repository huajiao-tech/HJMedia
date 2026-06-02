#include "HJBaseFaceDetect.h"
#include "HJFLog.h"
#include "HJError.h"
#include "HJComUtils.h"
#include "HJBaseUtils.h"
#include "HJDataConvert.h"
#include "libyuv.h"
#include "HJFaceDetectNCNNRetinaface.h"
#include "HJFaceDetectNCNNScrfd.h"
#include "HJFacePointsReal.h"
#include "HJTransferMediaData.h"
#include "HJPointSmooth.h"
#include "libyuv/scale_rgb.h"
#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif
#if defined(HJ_ENABLE_TNN)
    #include "HJFaceDetectTNNBlazeFace.h"
    #include "HJFaceDetectTNNAligner.h"
#endif
#if defined(__APPLE__) && TARGET_OS_IOS
    #include "osys/vision/HJFaceDetectIOSVisionRect.h"
    #include "osys/coreml/HJFaceDetectCoreMLRetinaface.h"
#endif

NS_HJ_BEGIN

std::string HJBaseFaceDetect::s_parammodelPath = "FACE_DETECT_MODEL_PATH";

int HJBaseFaceDetect::init(HJBaseParam::Ptr params)
{
	int i_err = HJ_OK;
	do
	{
        HJFLogi("HJBaseFaceDetect::init, modelUrls:{}", m_modelUrls);
		HJ_CatchMapPlainGetVal(params, std::string, s_parammodelPath, m_modelUrls);
		if (m_modelUrls.empty())
		{
			i_err = HJErrInvalidParams;
            HJLoge("HJBaseFaceDetect::init, modelUrls is empty");
            break;
		}
		if (!HJBaseUtils::isDirectoryExists(m_modelUrls))
		{
            i_err = HJErrInvalidFile;
            HJLoge("HJBaseFaceDetect::init, modelUrls is not exists");
            break;
		}
        HJ_CatchMapGetVal(params, HJFaceDetectOption::Ptr, m_option);
        if (!m_option)
        {
            m_option = HJFaceDetectOption::Create();
        }
	} while (false);
    HJFLogi("HJBaseFaceDetect::init, end i_err:{}", i_err);
	return i_err;
}

int HJBaseFaceDetect::convert(std::shared_ptr<HJTransferMediaData> i_inputData, HJDataScaleType i_scaleType, HJVec4i& o_vec)
{
    int i_err = HJ_OK;
    do
    {
        HJConvertDataType i_type = i_inputData->getConvertType();
        unsigned char* i_data[4] = {
            i_inputData->getData(0),
            i_inputData->getData(1),
            i_inputData->getData(2),
            i_inputData->getData(3)
        };
        int i_nPitch[4] = {
            i_inputData->getStride(0),
            i_inputData->getStride(1),
            i_inputData->getStride(2),
            i_inputData->getStride(3)
        };
        int i_width = i_inputData->getWidth();
        int i_height = i_inputData->getHeight();
        int scaleW = 0;
        int scaleH = 0;
        int offx = 0;
        int offy = 0;
		bool bUpdate = false;

        //if the input size change, ncnn retinface can change m_targetWidth and m_targetHeight, we need to reallocate the buffer
		if ((m_catchWidth != i_width) || (m_catchHeight != i_height))
		{
			m_catchWidth = i_width;
			m_catchHeight = i_height;
            bUpdate = true;
			HJFLogi("HJBaseFaceDetect convert update input wh {} {}, catch wh:{} {} target wh:{} {}", i_width, i_height, m_catchWidth, m_catchHeight, m_targetWidth, m_targetHeight);
		}

        if (i_type == HJConvertDataType_YUVNV12)
        {
            if (bUpdate)
            {
                m_scaleYuv = HJSPBuffer::create(m_targetWidth * m_targetHeight * 3 / 2);
                m_scaleRgb = HJSPBuffer::create(m_targetWidth * m_targetHeight * 3);
            }

            // Clear so padding stays black on every frame.  0,0,0 is green, 16,128,128 is black
            memset(m_scaleYuv->getBuf(), 16, m_targetWidth * m_targetHeight);
            memset(m_scaleYuv->getBuf() + m_targetWidth * m_targetHeight, 128, m_targetWidth * m_targetHeight / 2);

            HJVec4i vec = HJDataConvert::cvtGetScaleOffset(i_scaleType, i_width, i_height, m_targetWidth, m_targetHeight);
            // NV12 UV plane needs even alignment.
            scaleW = vec.x;
            scaleH = vec.y;

            if ((scaleW <= 0) || (scaleH <= 0))
            {
                HJFLoge("HJBaseFaceDetect detect scaleW:{}, scaleH:{}", scaleW, scaleH);
                i_err = HJErrFatal;
                break;
            }

            offx = vec.z;
            offy = vec.w;
            offx &= ~1;
            offy &= ~1;

            o_vec = vec;

            libyuv::NV12Scale(i_data[0], i_nPitch[0], i_data[1], i_nPitch[1], i_width, i_height,
                m_scaleYuv->getBuf() + offy * m_targetWidth + offx, m_targetWidth,
                m_scaleYuv->getBuf() + m_targetWidth * m_targetHeight + (offy * m_targetWidth) / 2 + offx, m_targetWidth,
                scaleW, scaleH, libyuv::kFilterBilinear);
#if 0
            {
                int pitches[4] = { m_targetWidth, m_targetWidth, 0, 0 };
                unsigned char* planes[4] = { m_scaleYuv->getBuf(), m_scaleYuv->getBuf() + m_targetWidth * m_targetHeight, nullptr, nullptr };
                HJDataConvert::cvtSaveToImage(HJConvertDataType_YUVNV12, planes, pitches, m_targetWidth, m_targetHeight, HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "scale.png"));
            }
#endif
            //libyuv::NV12ToRAW to r g b;    libyuv::NV12ToRGB24 to b g r;
            libyuv::NV12ToRAW(m_scaleYuv->getBuf(), m_targetWidth, m_scaleYuv->getBuf() + m_targetWidth * m_targetHeight, m_targetWidth,
                m_scaleRgb->getBuf(), m_targetWidth * 3, m_targetWidth, m_targetHeight);

#if 0           
            {
                int pitches[4] = { m_targetWidth * 3, 0, 0, 0 };
                unsigned char* planes[4] = { m_scaleRgb->getBuf(), nullptr, nullptr, nullptr };
                HJDataConvert::cvtSaveToImage(HJConvertDataType_RGB, planes, pitches, m_targetWidth, m_targetHeight, HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "scaleRGB.png"));
            }
#endif
        }
        else if (i_type == HJConvertDataType_RGB)
        {
            if (bUpdate || !m_scaleRgb)
            {
                m_scaleRgb = HJSPBuffer::create(m_targetWidth * m_targetHeight * 3);
            }
            if (!m_scaleRgb || !m_scaleRgb->getBuf())
            {
                HJFLoge("HJBaseFaceDetect RGB alloc failed target:{}x{}", m_targetWidth, m_targetHeight);
                i_err = HJErrFatal;
                break;
            }

            memset(m_scaleRgb->getBuf(), 0, m_targetWidth * m_targetHeight * 3);

            HJVec4i vec = HJDataConvert::cvtGetScaleOffset(i_scaleType, i_width, i_height, m_targetWidth, m_targetHeight);
            scaleW = vec.x;
            scaleH = vec.y;
            if ((scaleW <= 0) || (scaleH <= 0))
            {
                HJFLoge("HJBaseFaceDetect RGB detect scaleW:{}, scaleH:{}", scaleW, scaleH);
                i_err = HJErrFatal;
                break;
            }

            offx = vec.z;
            offy = vec.w;
            o_vec = vec;

            if (!i_data[0] || i_nPitch[0] <= 0)
            {
                HJFLoge("HJBaseFaceDetect RGB invalid plane/stride");
                i_err = HJErrInvalidParams;
                break;
            }

            const int ret = libyuv::RGBScale(
                i_data[0], i_nPitch[0],
                i_width, i_height,
                m_scaleRgb->getBuf() + ((size_t)offy * (size_t)m_targetWidth + (size_t)offx) * 3,
                m_targetWidth * 3,
                scaleW, scaleH,
                libyuv::kFilterBilinear);
            if (ret != 0)
            {
                HJFLoge("HJBaseFaceDetect RGBScale failed ret:{} src:{}x{} dst:{}x{} off:{}x{}",
                    ret, i_width, i_height, scaleW, scaleH, offx, offy);
                i_err = HJErrFatal;
                break;
            }
        }
        else
        {
            HJFLoge("HJBaseFaceDetect unsupported input type:{}", (int)i_type);
            i_err = HJErrInvalidParams;
            break;
        }
    } while (false);
    return i_err;
}
void HJBaseFaceDetect::dumpImage(std::shared_ptr<HJTransferMediaData> i_inputData, const HJFaceDetectRet& i_ret, const std::string& i_url)
{
    if (!i_ret.m_bContainFace)
    {
        return;
    }
    HJConvertDataType i_type = i_inputData->getConvertType();
    unsigned char* i_data[4] = {
        i_inputData->getData(0),
        i_inputData->getData(1),
        i_inputData->getData(2),
        i_inputData->getData(3)
    };
    int i_nPitch[4] = {
        i_inputData->getStride(0),
        i_inputData->getStride(1),
        i_inputData->getStride(2),
        i_inputData->getStride(3)
    };
    int i_width = i_inputData->getWidth();
    int i_height = i_inputData->getHeight();

    HJSPBuffer::Ptr orgRGBA = HJSPBuffer::create(i_width * i_height * 4);
    if (i_type == HJConvertDataType_YUVNV12)
    {
        libyuv::NV12ToABGR(i_data[0], i_nPitch[0], i_data[1], i_nPitch[1],
            orgRGBA->getBuf(), i_width * 4, i_width, i_height);
    }

    const std::vector<std::shared_ptr<HJSingleFaceDetectRet>>& faces = i_ret.getFaces();
    for (auto& face : faces)
    { 
        HJFaceDetectUtils::rectangle(orgRGBA->getBuf(), i_height, i_width, face->m_rect.x, face->m_rect.y, face->m_rect.x + face->m_rect.width, face->m_rect.y + face->m_rect.height, 1.f, 1.f);
        for (int idx = 0; idx < face->m_keyPoints.size(); ++idx)
        {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int px = static_cast<int>(face->m_keyPoints[idx].x) + dx;
                    int py = static_cast<int>(face->m_keyPoints[idx].y) + dy;
                    if (px >= 0 && px < i_width && py >= 0 && py < i_height) {
                        uint8_t* p = orgRGBA->getBuf() + (py * i_width + px) * 4;
                        p[0] = 255; p[1] = 0; p[2] = 0; p[3] = 255;
                    }
                }
            }
        }
    }
    int pitches[4] = { i_width * 4, 0, 0, 0 };
    unsigned char* planes[4] = { orgRGBA->getBuf(), nullptr, nullptr, nullptr };
    HJDataConvert::cvtSaveToImage(HJConvertDataType_RGBA, planes, pitches, i_width, i_height, i_url);

}
HJBaseFaceDetect::Ptr HJBaseFaceDetect::createFaceDetect(HJFaceDetectType i_type)
{
    HJBaseFaceDetect::Ptr detect = nullptr;
    HJFLogi("HJBaseFaceDetect createFaceDetect type:{}", (int)i_type);
    switch (i_type)
    {
#if defined(HJ_ENABLE_TNN)
    case HJFaceDetectType_TNN_BLAZEFACE:
        detect = HJFaceDetectTNNBlazeFace::Create();
        break;
    case HJFaceDetectType_TNN_FACEALIGNER:
        detect = HJFaceDetectTNNAligner::Create();
        break;
#endif
    case HJFaceDetectType_NCNN_RETINAFACE:
        detect = HJFaceDetectNCNNRetinaface::Create();
        break;
    case HJFaceDetectType_NCNN_SCRFD:
        detect = HJFaceDetectNCNNScrfd::Create();
        break;
    case HJFaceDetectType_IOS_VISION_RECT:
#if defined(__APPLE__) && TARGET_OS_IOS
        detect = HJFaceDetectIOSVisionRect::Create();
#else
        HJFLoge("HJBaseFaceDetect iOS Vision rect backend unavailable: iOS required");
#endif
        break;
    case HJFaceDetectType_IOS_COREML_RETINAFACE:
#if defined(__APPLE__) && TARGET_OS_IOS
        detect = HJFaceDetectCoreMLRetinaface::Create();
#else
        HJFLoge("HJBaseFaceDetect iOS CoreML retinaface backend unavailable: iOS required");
#endif
        break;
    case HJFaceDetectType_None:
		break;
    default:
        break;
    }
    if (detect)
    {
        detect->setType(i_type);
    }
    
    return detect;
}
std::shared_ptr<HJMoreFacePointsReal> HJBaseFaceDetect::priTrySmoothPoints(std::shared_ptr<HJMoreFacePointsReal> i_points, bool i_bSmooth)
{
	HJFaceDetectSmoothState curState = i_bSmooth ? HJFaceDetectSmoothState::Enable : HJFaceDetectSmoothState::Disable;
    if (m_smoothState != curState)
    {
        m_smoothState = curState;
        switch (m_smoothState)
        {
		case HJFaceDetectSmoothState::None:
            break;
		case HJFaceDetectSmoothState::Enable:
			m_facePointSmooth = HJMorePointSmooth::Create();
			break;
		case HJFaceDetectSmoothState::Disable:
			m_facePointSmooth = nullptr;
			break;
        default:
            break;
        }
    }

    if (m_facePointSmooth)
    {
        return m_facePointSmooth->smooth(i_points);
	}
    else
    {
        return i_points;
    }
}
std::string HJBaseFaceDetect::cvtConcisePoints(HJFaceDetectRet& i_ret, bool i_bSmooth)
{
    std::string json = "";
    HJMoreFacePointsReal::Ptr points = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(i_ret.m_width, i_ret.m_height, i_ret.m_bContainFace, i_ret.m_timeStamp);
    points->setIsDebugPoints(m_bDebugPoints);
    points->setSystemTime(i_ret.m_systemTime);
    points->setElapsedTime(i_ret.m_elapseMs);
    points->setTimestamp(i_ret.m_timeStamp);
    if (i_ret.m_bContainFace)
    {
        const std::vector<std::shared_ptr<HJSingleFaceDetectRet>>& faces = i_ret.getFaces();
        for (auto& face : faces)
        {
            std::shared_ptr<HJSingleFacePointsReal> single = std::make_shared<HJSingleFacePointsReal>();

            switch (m_type)
            {
#if defined(HJ_ENABLE_TNN)
            case HJFaceDetectType_TNN_BLAZEFACE:
            {
				single->add(face->getPoint(0));
				single->add(face->getPoint(1));
				single->add(face->getPoint(2));
				single->add(face->getPoint(3)); //not use left ear and right ear
				single->add(face->getPoint(3));
				break;
            }
#endif
            case HJFaceDetectType_NCNN_RETINAFACE:
            case HJFaceDetectType_NCNN_SCRFD:
            case HJFaceDetectType_IOS_COREML_RETINAFACE:
            {
                single->add(face->getPoint(0));
                single->add(face->getPoint(1));
                single->add(face->getPoint(2));
                single->add(face->getPoint(3));
                single->add(face->getPoint(4));
                break;
            }
#if defined(HJ_ENABLE_TNN)
            case HJFaceDetectType_TNN_FACEALIGNER:
            {
                single->add(face->getAvgPoint(std::vector<int>{153, 166}));
                single->add(face->getAvgPoint(std::vector<int>{177, 189}));
                single->add(face->getAvgPoint(std::vector<int>{33, 43}));
                single->add(face->getPoint(73));
                single->add(face->getPoint(68));
                break;
            }
#endif
            case HJFaceDetectType_IOS_VISION_RECT:
            {
                break;
            }

            default:
                return "";
            }
            single->setFaceRect(face->m_rect.x, face->m_rect.y, face->m_rect.width, face->m_rect.height);
            points->add(single);
        }
    }

    HJMoreFacePointsReal::Ptr out = priTrySmoothPoints(points, i_bSmooth);
    if (out)
    {
        return out->serial();
    }
    return "";
}

NS_HJ_END
