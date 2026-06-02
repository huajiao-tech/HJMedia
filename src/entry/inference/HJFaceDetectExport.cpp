#include "HJFaceDetectExport.h"
#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseFaceDetect.h"
#include "HJUtilitys.h"
#include "HJThreadPool.h"
#include "HJTransferMediaData.h"

NS_HJ_USING

HJFaceDetectWrapper::HJFaceDetectWrapper()
{
	m_threadPool = std::make_shared<HJThreadPool>();
}
HJFaceDetectWrapper::~HJFaceDetectWrapper()
{
	HJFLogi("HJFaceDetectWrapper ~HJFaceDetectWrapper enter");
	if (m_threadPool)
	{
		m_threadPool->done();
		m_threadPool = nullptr;
	}
	HJFLogi("HJFaceDetectWrapper ~HJFaceDetectWrapper end");
}

int HJFaceDetectWrapper::init(HJFaceDetectConciseCb i_cb, HJFaceDetectWrapperType i_type, const std::string& i_modelUrl, const HJFaceDetectWrapperOption& i_option)
{
	int i_err = HJ_OK;
	HJFLogi("HJFaceDetectWrapper init type:{} modelUrl:{}", (int)i_type, i_modelUrl);
	do
	{
		m_conciseOutCb = i_cb;
		HJFaceDetectType type = HJFaceDetectType_TNN_FACEALIGNER;
		switch (i_type)
		{
		case HJFaceDetectWrapperType_TNNBLAZE:
			type = HJFaceDetectType_TNN_BLAZEFACE;
			break;
		case HJFaceDetectWrapperType_TNNALIGNER:
			type = HJFaceDetectType_TNN_FACEALIGNER;
			break;
		case HJFaceDetectWrapperType_NCNNRETINAFACE:
            type = HJFaceDetectType_NCNN_RETINAFACE;
			break;
			case HJFaceDetectWrapperType_NCNNSCRFD:
				type = HJFaceDetectType_NCNN_SCRFD;
				break;
	        case HJFaceDetectWrapperType_COREMLRETINAFACE:
	            type = HJFaceDetectType_IOS_COREML_RETINAFACE;
	            HJFLogi("HJFaceDetectWrapper init using iOS CoreML retinaface backend");
	            break;
	        case HJFaceDetectWrapperType_IOSVISIONRECT:
	            type = HJFaceDetectType_IOS_VISION_RECT;
	            HJFLogi("HJFaceDetectWrapper init using iOS Vision rect backend, rect-only output for FaceSR/crop flows");
            break;
		default:
			break;
		}
		m_faceDetect = HJBaseFaceDetect::createFaceDetect(type);
		if (!m_faceDetect)
		{
            HJFLoge("HJFaceDetectWrapper init");
			i_err = HJErrNotInited;
			break;
		}
		HJBaseParam::Ptr param = HJBaseParam::Create();
		HJ_CatchMapPlainSetVal(param, std::string, HJBaseFaceDetect::s_parammodelPath, i_modelUrl);

		HJFaceDetectOption::Ptr optionPtr = HJFaceDetectOption::Create();

#define COPY_OPTION(var) \
	optionPtr->var = i_option.var;\
	HJFLogi("copy option {}:{}", #var, optionPtr->var);
	
		COPY_OPTION(tnnFaceAlignThreshold);
		COPY_OPTION(tnnFaceAlignMinFaceSize);
		COPY_OPTION(ncnnRetinaFaceProbThreshold);
		COPY_OPTION(ncnnRetinaFaceNmsThreshold);
		COPY_OPTION(ncnnRetinaFaceThreadNums);
		COPY_OPTION(retinaFaceTargetSize);
		COPY_OPTION(ncnnRetinaFaceUseGPU);

		COPY_OPTION(ncnnScrfdEqualScale);
		COPY_OPTION(ncnnScrfdTargetSize);
			COPY_OPTION(ncnnScrfdProbThreshold);
			COPY_OPTION(ncnnScrfdNmsThreshold);
			COPY_OPTION(ncnnScrfdThreadNums);
			COPY_OPTION(ncnnScrfdUseGPU);
	        COPY_OPTION(coreMLRetinaFaceComputeMode);
	        COPY_OPTION(visionRectTargetSize);
	        COPY_OPTION(visionRectComputeMode);
	#undef COPY_OPTION

		HJ_CatchMapSetVal(param, HJFaceDetectOption::Ptr, optionPtr);
		i_err = m_faceDetect->init(param);
		if (i_err < 0)
		{
            HJFLoge("HJFaceDetectWrapper init error:{}", i_err);
			break;
		}
		i_err = m_threadPool->start();
		if (i_err < 0)
		{
            HJFLoge("m_threadPool start error:{}", i_err);
			break;
		}
	} while (false);
	HJFLogi("HJFaceDetectWrapper init i_err:{}", i_err);
	return i_err;
}
int HJFaceDetectWrapper::priDetect(std::shared_ptr<HJ::HJTransferMediaData> i_mediaData, bool i_bSmooth,  bool i_bDebugImg)
{
	int i_err = HJ_OK;
	do
	{
		HJFaceDetectRet o_faceRet;
		i_err = m_faceDetect->detect(i_mediaData, o_faceRet);
		if (i_err < 0)
		{
			HJFLoge("HJFaceDetectWrapper detect error:{}", i_err);
			break;
		}

		if (m_conciseOutCb)
		{
			std::string facePointInfo = m_faceDetect->cvtConcisePoints(o_faceRet, i_bSmooth);
			m_conciseOutCb(facePointInfo);
		}
		if (i_bDebugImg && o_faceRet.m_bContainFace)
		{
			static int ii = 0;
			std::string saveUrl = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), HJFMT("detect_{}__{}.png", ii++, o_faceRet.m_elapseMs));
			m_faceDetect->dumpImage(i_mediaData, o_faceRet, saveUrl);
		}
		HJFPERLog5i("HJFaceDetectWrapper detect bContainFace:{} width:{} height:{} elapseMs:{}", o_faceRet.m_bContainFace, o_faceRet.m_width, o_faceRet.m_height, o_faceRet.m_elapseMs);
	} while (false);
	
    return i_err;
}
int HJFaceDetectWrapper::detect(std::shared_ptr<HJUnifyWrapperData> i_input, bool i_bSmooth, bool i_bSync, bool i_bDebugPoints, bool i_bDebugImg)
{
	int i_err = HJ_OK;
	do
	{
		if (!m_faceDetect)
		{
			i_err = HJErrAlreadyDone;
			break;
		}
		if (!i_input)
		{
			i_err = HJErrInvalidParams;
			break;
		}
		if (m_bError)
		{
			i_err = HJErrInvalid;
			break;
		}
		m_faceDetect->setDebugPoints(i_bDebugPoints);

		HJ::HJTransferMediaData::Ptr data = nullptr;
		HJConvertDataType type = HJConvertDataType_None;
		switch (i_input->dataType)
		{
		case HJUnifyWrapperDataType_NV12:
			type = HJConvertDataType_YUVNV12;
			data = i_bSync
				? HJ::HJTransferMediaDataYUVNV12::CreateView(i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp)
				: HJ::HJTransferMediaDataYUVNV12::Create<HJ::HJTransferMediaDataYUVNV12>(i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
			break;
		case HJUnifyWrapperDataType_RGB:
			type = HJConvertDataType_RGB;
			data = i_bSync
				? HJ::HJTransferMediaDataRGB::CreateView(i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp)
				: HJ::HJTransferMediaDataRGB::Create<HJ::HJTransferMediaDataRGB>(i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
			break;
		default:
            i_err = HJErrInvalidParams;
			return i_err;
		//case HJUnifyWrapperDataType_RGBA:
		//	type = HJConvertDataType_RGBA;
		//	break;
		}
		if (!data)
		{
            i_err = HJErrInvalidParams;
            break;
		}

		if (i_bSync)
		{
            i_err = priDetect(data, i_bSmooth, i_bDebugImg);
			if (i_err < 0)
			{
                HJFLoge("HJFaceDetectWrapper detect error:{}", i_err);
				break;
			}
		}
		else
		{
			if (m_threadPool)
			{
				m_threadPool->asyncClear([this, data, i_bSmooth, i_bDebugImg]()
					{
						int ret = priDetect(data, i_bSmooth, i_bDebugImg);
						if (ret < 0)
						{
                            HJFLoge("HJFaceDetectWrapper detect error:{}", ret);
							m_bError = true;
						}
						return ret;
					}, 888);
			}
		}
	} while (false);
	return i_err;
}
