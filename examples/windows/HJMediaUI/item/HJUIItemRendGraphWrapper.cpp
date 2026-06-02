#pragma once

#include "HJUIItemRendGraphWrapper.h"
#include "HJFLog.h"
#include "imgui.h"
#include "HJFileData.h"
#include "libYuv.h"
#include "HJSPBuffer.h"

#include "HJRenderContextExport.h"
#include "HJRenderGraphExport.h"
#include "HJRenderCoreSingleton.h"
#include "HJTransferMediaData.h"
#include "HJResourceImgSeq.h"
#include "HJMediaData.h"
#include "HJMediaDataDraw.h"
#include "HJUtilitys.h"

#include "HJInferenceContextExport.h"
#include "HJFaceDetectExport.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
 
#include "HJFacePointsReal.h"
#include "HJMediaRenderConfig.h"
#include "HJRteGraphSetupInfo.h"

#include "HJFacePointsInfo.h"


#include "test.h"
#include "VisualDataBase.h"

NS_HJ_BEGIN

std::string HJUIItemRendGraphWrapper::m_faceuUrl0 = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/XianWeng");
std::string HJUIItemRendGraphWrapper::m_faceuUrl1 = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/60031_10");
std::string HJUIItemRendGraphWrapper::m_faceuUrl2 = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/90237_1");
//std::shared_ptr<VisualDataBase> p = nullptr;
HJUIItemRendGraphWrapper::HJUIItemRendGraphWrapper()
{
//	//test();
//	//int v0 = s_val0;
//	//int v1 = s_val1;
//	//int v2 = s_val2;
//	//int v3 = s_val3;
//
//	p = VisualDataBase::createVisualData(VisualDataType::VisualDataType_Image);
//	HJBaseParam::Ptr param = HJBaseParam::Create();
////	HJ_CatchMapPlainSetVal(param, std::string, VisualDataBase::s_paramUrl, "e:/video/comparevid/204637931-720p.mp4");
////	HJ_CatchMapPlainSetVal(param, std::string, VisualDataBase::s_paramUrl, "OBS Virtual Camera");
////	HJ_CatchMapPlainSetVal(param, std::string, VisualDataBase::s_paramUrl, "E:/code/git/hjmedia/examples/resource/imgseq/sing");
//	HJ_CatchMapPlainSetVal(param, std::string, VisualDataBase::s_paramUrl, "E:/code/git/hjmedia/examples/resource/image/play.jpg");
//
//	HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramWidth, 720);
//	HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramHeight, 720);
//	p->init(param);

	HJEntryContextInfo infoRender;
	infoRender.logIsValid = true;
	infoRender.logDir = "e:/infoRender";
	infoRender.logLevel = HJLOGLevel_INFO;
	infoRender.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
	infoRender.logMaxFileSize = 5 * 1024 * 1024;
	infoRender.logMaxFileNum = 5;
	renderContextInit(infoRender);

	HJEntryContextInfo infoInference;
	infoInference.logIsValid = true;
	infoInference.logDir = "e:/infoInference";
	infoInference.logLevel = HJLOGLevel_INFO;
	infoInference.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
	infoInference.logMaxFileSize = 5 * 1024 * 1024;
	infoInference.logMaxFileNum = 5;
	inferenceContextInit(infoInference);
}

HJUIItemRendGraphWrapper::~HJUIItemRendGraphWrapper()
{
	priDone();
	inferenceContextUnInit();
	renderContextUnInit();
}

void HJUIItemRendGraphWrapper::priRenderImg()
{
	HJTransferMediaData::Ptr rendData = m_asyncCacheRender.acquire();

	if (rendData)
	{
		if (!m_mediaDataDraw)
		{
			m_mediaDataDraw = HJMediaDataDraw::Create();
		}
		m_mediaDataDraw->update(rendData);

		ImGui::Begin("DrawImg");
		ImGui::SetWindowSize(ImVec2(rendData->getWidth(), rendData->getHeight()));
		ImGui::Image((void*)(intptr_t)m_mediaDataDraw->getTextureId(), ImVec2(rendData->getWidth(), rendData->getHeight()));
		ImGui::End();

		m_asyncCacheRender.recovery(rendData);
	}
}
int HJUIItemRendGraphWrapper::priCloseRender()
{
	int i_err = HJ_OK;
	do
	{
		m_renderGraph = nullptr;
	} while (false);
    return i_err;
}
int HJUIItemRendGraphWrapper::priCloseDetect()
{
	int i_err = HJ_OK;
	do
	{
		m_faceDetect = nullptr;

		m_elapseFaceDetectTime = 0;
		{
			HJ_LOCK(m_faceMutex);
			m_cacheFaceInfo = "";
		}

		if (m_renderGraph)
		{
			m_renderGraph->setFaceInfo(HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData, "");
		}
	} while (false);
	return i_err;
}
int HJUIItemRendGraphWrapper::priResetRender()
{
	int i_err = HJ_OK;
	do {
		priCloseRender();

		m_renderGraph = std::make_shared<HJRenderGraphWrapper>();
		i_err = m_renderGraph->init([this](int i_type, int i_ret, const std::string& i_msgInfo)
			{
				HJFLogi("notify type:{} ret:{} msg:{}", i_type, i_ret, i_msgInfo);
			},
			[this](std::shared_ptr<HJUnifyWrapperData> o_output)
			{
				HJTransferMediaData::Ptr mediaData = std::make_shared<HJTransferMediaDataRGBA>(o_output->data, o_output->nPitch, o_output->width, o_output->height);
				m_asyncCacheRender.enqueue(std::move(mediaData));
			}, 0, true, HJMediaRenderConfig::GetConfig(HJMediaRenderConfigType_SourceRender), HJUnifyWrapperDataType_RGBA);
		if (i_err < 0)
		{
			break;
		}	
	} while (false);
	return i_err;
}
int HJUIItemRendGraphWrapper::priResetFaceDetect()
{
	int i_err = HJ_OK;

	do
	{	
		priCloseDetect();

		m_faceDetect = std::make_shared<HJFaceDetectWrapper>();

		HJFaceDetectWrapperType detectType = HJFaceDetectWrapperType_NCNNSCRFD;
		switch (m_faceDetectType)
		{
		case 0:
			detectType = HJFaceDetectWrapperType_TNNBLAZE;
			break;
		case 1:
			detectType = HJFaceDetectWrapperType_TNNALIGNER;
			break;
		case 2:
			detectType = HJFaceDetectWrapperType_NCNNRETINAFACE;
			break;
		case 3:
			detectType = HJFaceDetectWrapperType_NCNNSCRFD;
			break;
		default:
			detectType = HJFaceDetectWrapperType_NCNNSCRFD;
			break;
		}
		HJFaceDetectWrapperOption option;
		option.tnnFaceAlignThreshold = 0.75f;
		option.tnnFaceAlignMinFaceSize = 20;
		option.ncnnRetinaFaceProbThreshold = 0.8f;
		option.ncnnRetinaFaceNmsThreshold = 0.4f;
		option.ncnnRetinaFaceThreadNums = m_ncnnRetinaFaceThread;
		option.retinaFaceTargetSize = 320;
		option.ncnnRetinaFaceUseGPU = (m_ncnnRetinaFaceUseGPU == 1);

		option.ncnnScrfdEqualScale = true;
		option.ncnnScrfdTargetSize = 320;
		option.ncnnScrfdProbThreshold = 0.3f;
		option.ncnnScrfdNmsThreshold = 0.45f;
		option.ncnnScrfdThreadNums = m_ncnnRetinaFaceThread; // 1-2
		option.ncnnScrfdUseGPU = (m_ncnnRetinaFaceUseGPU == 1);

		i_err = m_faceDetect->init([this](const std::string& i_faceInfo)
			{
				{
					HJ_LOCK(m_faceMutex);
					m_cacheFaceInfo = i_faceInfo;
				}
			},
			detectType,
			HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/model"), option);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
bool HJUIItemRendGraphWrapper::updateTitle()
{
	std::string info = HJFMT("Mouse: ({}, {}) FPS:{:.1f} GetImgData:{} DetectTime:{}", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y, ImGui::GetIO().Framerate, m_elapseImgGetTime, m_elapseFaceDetectTime);
	glfwSetWindowTitle((GLFWwindow*)getWindow(), info.c_str());
	return true;
}
int HJUIItemRendGraphWrapper::run()
{
	int i_err = 0;
	do
	{
		//HJTransferMediaData::Ptr data = p->acquire();
		HJTransferMediaData::Ptr data = HJResourceImgSeq::Instance().getImgDataYUVNV12(HJResourceImgSeqStyle_MIX);
		if (!data)
		{
			return 0;
		}
			
		std::shared_ptr<HJUnifyWrapperData> input = std::make_shared<HJUnifyWrapperData>();
		input->dataType = HJUnifyWrapperDataType_NV12;

		input->width = data->getWidth();
		input->height = data->getHeight();

		input->data[0] = data->getData(0);
		input->data[1] = data->getData(1);

		input->nPitch[0] = data->getStride(0);
		input->nPitch[1] = data->getStride(1);
		input->timeStamp = data->getTimeStamp() * 1000; 
		input->nElapseCount = 1;

		//HJTransferMediaData::Ptr data2 = HJTransferMediaData::Create();
		//data2->copymap(*data);
		//int64_t elapseTime = 0;
		//HJ_CatchMapPlainGetVal(data2, int64_t, "ImgDecodeElapse", elapseTime);

		HJ_CatchMapPlainGetVal(data, int64_t, "ImgDecodeElapse", m_elapseImgGetTime);

		if (m_faceDetect)
		{
			i_err = m_faceDetect->detect(input, m_faceDetectSmooth == 1, m_faceDetectSyncMode == 1, m_faceDetectDebugPoints == 1, m_faceDetectDebugImage == 1);
			if (i_err < 0)
			{
				//break;
                HJFLoge("detect err:{}", i_err);
				i_err = 0;
			}
		}
		if (m_renderGraph)
		{
			std::string strFaceInfo = "";
			{
				HJ_LOCK(m_faceMutex);
				strFaceInfo = m_cacheFaceInfo;				
			}
			if (!strFaceInfo.empty())
			{
				HJFacePointsReal faceInfo;
				faceInfo.init(strFaceInfo);
				//HJFLogi("faceoutput: w h <{} {}> bContain:{} timeStamp:{} bDebugPt:{} systemTime:{} elapseTime:{} ", faceInfo.width(), faceInfo.height(), faceInfo.isContainFace(), faceInfo.getTimestamp(), faceInfo.isDebugPoints(), faceInfo.getSystemTime(), faceInfo.getElapsedTime());
				m_elapseFaceDetectTime = faceInfo.getElapsedTime();

				m_renderGraph->setFaceInfo(HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData, strFaceInfo);
			}

			i_err = m_renderGraph->render(input);
			if (i_err < 0)
			{
				break;
			}
		}
		priRenderImg();

		ImGui::Begin("Render");

		if (ImGui::Button("ResetRender"))
		{
			priResetRender();
		}
		ImGui::SameLine();
		if (ImGui::Button("CloseRender"))
		{
            priCloseRender();
		}
		//ImGui::SameLine();
		if (ImGui::Button("FaceuOpen"))
		{
#if 0
			if (m_renderGraph)
			{
				m_renderGraph->nodeEnable("HJNodeClass_SourceFaceu", "HJNodeClass_SourceFaceu", true, m_faceuUrl1);
			}
#else
			if (m_renderGraph)
			{
				m_renderGraph->nodeCreate(HJRteGraphConfig::HJNodeClass_SourceFaceu, m_faceuUrl1, HJRteGraphConfig::HJNodeRole_Source, false, false, "", HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData);
				m_renderGraph->nodeConnect(HJRteGraphConfig::HJNodeClass_SourceFaceu, m_faceuUrl1, HJRteGraphConfig::HJNodeClass_TargetPBO_0, HJRteGraphConfig::HJNodeClass_TargetPBO_0,
					HJRteGraphConfig::HJNodeShaderType_Copy2D, "");
				m_renderGraph->nodeEnable(HJRteGraphConfig::HJNodeClass_SourceFaceu, m_faceuUrl1, true, m_faceuUrl1);
			}

#endif
		}
		ImGui::SameLine();
		if (ImGui::Button("FaceuClose"))
		{
#if 0
			if (m_renderGraph)
			{
				m_renderGraph->nodeEnable("HJNodeClass_SourceFaceu", "HJNodeClass_SourceFaceu", false, "");
			}
#else
			if (m_renderGraph)
			{
				m_renderGraph->nodeDelete(HJRteGraphConfig::HJNodeClass_SourceFaceu, m_faceuUrl1, false);
			}
#endif
		}
		ImGui::End();

		ImGui::Begin("FaceDetect");
		ImGui::TextUnformatted("Sync Mode");
		ImGui::RadioButton("sync", &m_faceDetectSyncMode, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("async", &m_faceDetectSyncMode, 0);

		ImGui::TextUnformatted("Smooth");
		ImGui::RadioButton("yes_smooth", &m_faceDetectSmooth, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("no_smooth", &m_faceDetectSmooth, 0);

		ImGui::TextUnformatted("Type");
		ImGui::RadioButton("tnnblaze", &m_faceDetectType, 0);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("tnnaligner", &m_faceDetectType, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("ncnnretinaface", &m_faceDetectType, 2);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("ncnnscrfd", &m_faceDetectType, 3);


		ImGui::TextUnformatted("DebugImage");
		ImGui::RadioButton("yes_", &m_faceDetectDebugImage, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("no_", &m_faceDetectDebugImage, 0);


		ImGui::TextUnformatted("DebugFacePoint");
		ImGui::RadioButton("yes", &m_faceDetectDebugPoints, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("no", &m_faceDetectDebugPoints, 0);

		ImGui::TextUnformatted("NCNN RetinaFace Threads");
		ImGui::SliderInt("RetinaFaceThread", &m_ncnnRetinaFaceThread, 1, 8);

		ImGui::TextUnformatted("NCNN RetinaFace Device");
		ImGui::RadioButton("cpu", &m_ncnnRetinaFaceUseGPU, 0);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("gpu", &m_ncnnRetinaFaceUseGPU, 1);
		

		if (ImGui::Button("ResetDetect"))
		{
			priResetFaceDetect();
		}
		ImGui::SameLine();
		if (ImGui::Button("CloseDetect"))
		{
			priCloseDetect();
		}
		ImGui::End();

		//p->recovery(data);

	} while (false);
	return i_err;
}

void HJUIItemRendGraphWrapper::priDone()
{
	m_faceDetect = nullptr;
	m_renderGraph = nullptr;
	//p = nullptr;
}

NS_HJ_END


