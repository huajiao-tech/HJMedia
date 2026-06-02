#if !defined(HJ_MEDIA_GRAPHIC_UI)

#include "HJVisualDataUI.h"
#include "HJFLog.h"
#include "imgui.h"
#include "HJTransferMediaData.h"
#include "HJUtilitys.h"
#include "VisualDataBase.h"
#include <cstring>

NS_HJ_BEGIN

namespace
{
	const char* kDefaultVideoUrl = "http://127.0.0.1:8080/live/test.flv";//"e:/video/chaofen/zctz.flv";//"e:/video/comparevid/204637931-720p.mp4";
	const char* kDefaultCameraUrl = "OBS Virtual Camera";
	const char* kDefaultImageSeqUrl = "E:/code/git/hjmedia/examples/resource/imgseq/sing";
	const char* kDefaultImageUrl = "E:/code/git/hjmedia/examples/resource/image/360x640_483.jpg"; 
}

HJVisualDataUI::HJVisualDataUI()
{
	std::strncpy(m_videoUrl, kDefaultVideoUrl, sizeof(m_videoUrl) - 1);
	std::strncpy(m_cameraUrl, kDefaultCameraUrl, sizeof(m_cameraUrl) - 1);
	std::strncpy(m_imgSeqUrl, kDefaultImageSeqUrl, sizeof(m_imgSeqUrl) - 1);
	std::strncpy(m_imageUrl, kDefaultImageUrl, sizeof(m_imageUrl) - 1);
}

HJVisualDataUI::~HJVisualDataUI()
{
	stop();
}

int HJVisualDataUI::applySource(HJConvertDataType i_dataType)
{
	int i_err = HJ_OK;
	do
	{
		stop();

		VisualDataType dataType = VisualDataType::VisualDataType_Video;
		const char* url = m_videoUrl;
		switch (m_sourceType)
		{
		case SourceType_Video:
			dataType = VisualDataType::VisualDataType_Video;
			url = m_videoUrl;
			break;
		case SourceType_Camera:
			dataType = VisualDataType::VisualDataType_Camera;
			url = m_cameraUrl;
			break;
		case SourceType_ImgSeq:
			dataType = VisualDataType::VisualDataType_ImgSeq;
			url = m_imgSeqUrl;
			break;
		case SourceType_Image:
			dataType = VisualDataType::VisualDataType_Image;
			url = m_imageUrl;
			break;
		default:
			break;
		}

		if (!url || std::strlen(url) == 0)
		{
			HJFLoge("VisualData source url is empty");
			i_err = HJErrInvalidParams;
			break;
		}

		m_visualData = VisualDataBase::createVisualData(dataType);
		if (!m_visualData)
		{
			i_err = HJErrFatal;
			break;
		}

		HJBaseParam::Ptr param = HJBaseParam::Create();
		HJ_CatchMapPlainSetVal(param, std::string, VisualDataBase::s_paramUrl, std::string(url));
		HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramFps, m_fps);
		HJ_CatchMapPlainSetVal(param, bool, VisualDataBase::s_paramLoop, true);
		HJ_CatchMapPlainSetVal(param, HJConvertDataType, VisualDataBase::s_paramOutputType, i_dataType);

		if (m_sourceType == SourceType_Camera)
		{
			HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramWidth, m_cameraWidth);
			HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramHeight, m_cameraHeight);
		}

		i_err = m_visualData->init(param);
		if (i_err < 0)
		{
			HJFLoge("VisualData init failed ret:{}", i_err);
			stop();
			break;
		}
	} while (false);
	return i_err;
}

int HJVisualDataUI::drawUI(const char* i_windowName)
{
	int i_err = HJ_OK;
	ImGui::Begin(i_windowName);

	ImGui::TextUnformatted("Source");
	ImGui::RadioButton("video", &m_sourceType, SourceType_Video);
	ImGui::SameLine(0.0f, 24.0f);
	ImGui::RadioButton("camera", &m_sourceType, SourceType_Camera);
	ImGui::SameLine(0.0f, 24.0f);
	ImGui::RadioButton("imgseq", &m_sourceType, SourceType_ImgSeq);
	ImGui::SameLine(0.0f, 24.0f);
	ImGui::RadioButton("image", &m_sourceType, SourceType_Image);

	ImGui::TextUnformatted("Input Url");
	switch (m_sourceType)
	{
	case SourceType_Video:
		ImGui::InputText("videoUrl", m_videoUrl, IM_ARRAYSIZE(m_videoUrl));
		break;
	case SourceType_Camera:
		ImGui::InputText("cameraUrl", m_cameraUrl, IM_ARRAYSIZE(m_cameraUrl));
		break;
	case SourceType_ImgSeq:
		ImGui::InputText("imgSeqUrl", m_imgSeqUrl, IM_ARRAYSIZE(m_imgSeqUrl));
		break;
	case SourceType_Image:
		ImGui::InputText("imageUrl", m_imageUrl, IM_ARRAYSIZE(m_imageUrl));
		break;
	default:
		break;
	}

	ImGui::TextUnformatted("FPS");
	ImGui::InputInt("fps", &m_fps);
	if (m_fps < 1)
		m_fps = 1;

	if (m_sourceType == SourceType_Camera)
	{
		ImGui::TextUnformatted("Camera Resolution");
		ImGui::InputInt("width", &m_cameraWidth);
		ImGui::InputInt("height", &m_cameraHeight);
		if (m_cameraWidth < 1)
			m_cameraWidth = 1;
		if (m_cameraHeight < 1)
			m_cameraHeight = 1;
	}
	ImGui::End();

	return i_err;
}

std::shared_ptr<HJTransferMediaData> HJVisualDataUI::acquire()
{
	if (!m_visualData)
		return nullptr;
	return m_visualData->acquire();
}

void HJVisualDataUI::recovery(std::shared_ptr<HJTransferMediaData> i_data)
{
	if (!m_visualData || !i_data)
		return;
	m_visualData->recovery(i_data);
}

std::shared_ptr<VisualDataBase> HJVisualDataUI::getVisualData() const
{
	return m_visualData;
}

void HJVisualDataUI::stop()
{
	m_visualData = nullptr;
}

NS_HJ_END

#endif

