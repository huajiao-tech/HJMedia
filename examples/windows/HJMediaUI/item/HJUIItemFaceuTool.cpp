#include <filesystem>
#include <algorithm>
#include <cctype>
#include "stb_image.h"
#include <fstream>
#include "HJUIItemTest.h"
#include "HJFLog.h"
#include "imgui.h"
#include "HJUIItemFaceuTool.h"
#include "HJRteGraphProc.h"
#include "HJRteGraphSetupInfo.h"
#include "HJGUICommon.h"
#include "HJFaceuInfo.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJFacePointsMadeup.h"
#include "HJFacePointsReal.h"

NS_HJ_BEGIN

std::string HJUIItemFaceuTool::s_imageUrl = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/image/play.jpg");//"resource/play.jpg";//"E: / video / image / play.jpg";
std::string HJUIItemFaceuTool::s_faceuUrl = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/60031_10");
std::string HJUIItemFaceuTool::s_imageSeq = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing"); //"E:/code/git/hjmedia/examples/resource/pngseg/sing";

HJUIItemFaceuTool::HJUIItemFaceuTool()
{
}

HJUIItemFaceuTool::~HJUIItemFaceuTool()
{
	priDoneAll();
}
void HJUIItemFaceuTool::priSave(const std::shared_ptr<HJFaceuInfo>& i_faceuInfo, const std::string& faceuPath, bool i_bUseBackUp)
{
	if (ImGui::Button("Save"))
	{
		if (i_bUseBackUp)
		{
			HJBaseUtils::backupFile(HJBaseUtils::getRealPath(faceuPath), "config", "config.backup");
		}

		i_faceuInfo->initSerialToFile(HJBaseUtils::getRealPath(faceuPath) + "config");
		m_lastSaveTime = ImGui::GetTime();
	}

	if (m_lastSaveTime > 0.0 && (ImGui::GetTime() - m_lastSaveTime) < 1.0)
	{
		ImGui::SameLine();
		ImGui::SetWindowFontScale(1.f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
		ImGui::Text("Config Save Success");
		ImGui::PopStyleColor();
		ImGui::SetWindowFontScale(1.0f);
	}
}
void HJUIItemFaceuTool::priFaceuOpen(const std::string& i_url)
{
	//HJBaseParam::Ptr param = HJBaseParam::Create();
	//HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, i_url);
	//HJ_CatchMapPlainSetVal(param, bool, "bUseFakePoints", true);
	//HJ_CatchMapPlainSetVal(param, bool, "bDebugPoint", true);
	//m_rteGraphProc->openFaceu(param);
	m_bUseFakePoints = true;
	m_rteGraphProc->nodeEnable("HJNodeClass_SourceFaceu", "HJNodeClass_SourceFaceu", true, i_url);
	m_faceuInfoWtr = m_rteGraphProc->getFaceuInfo();
}
void HJUIItemFaceuTool::priDoneAll()
{
	if (m_rteGraphProc)
	{
		m_rteGraphProc->done();
		m_rteGraphProc = nullptr;
	}
	if (m_uiWindow)
	{
		glfwDestroyWindow(m_uiWindow);
		m_uiWindow = nullptr;
	}
}
void HJUIItemFaceuTool::priAdjustFaceu()
{
	if (ImGui::Button("Reset"))
	{
		std::string configPath = HJBaseUtils::getRealPath(m_faceuPath) + "config";
		if (!std::filesystem::exists(configPath))
		{
			m_openErrorPopup = true;
		}
		else
		{
			priDoneAll();
			m_bUseImgSeq = false;
		}
	}
	ImGui::SameLine();

	if (m_openErrorPopup)
	{
		ImGui::OpenPopup("Error");
		m_openErrorPopup = false;
	}

	if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("not find config, please use CreateFaceu!!!");
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Preview"))
	{
		priDoneAll();
		m_bUseImgSeq = true;
	}

	HJFaceuInfo::Ptr faceuInfo = m_faceuInfoWtr.lock();
	if (faceuInfo)
	{
		if (faceuInfo->framerate != std::numeric_limits<int>::min())
		{
			//ImGui::PushID(0);
			ImGui::DragInt("framerate, if modify, must Save-Reset", &faceuInfo->framerate, 1.0f, 1, 60);
			//ImGui::PopID();
		}

		for (int i = 0; i < faceuInfo->texture.size(); i++)
		{
			auto& item = faceuInfo->texture[i];
			ImGui::PushID(i);
			if (ImGui::CollapsingHeader(item->imageName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Disable", &item->m_bDisable);

				if (item->radius_Type == HJFaceuConstants::RotateType_Fixed) //for example, background or other is not use eyes angle, use fixed angle <mradius>
				{
					ImGui::DragInt("mradius", &item->mradius, 1.0f);
				}
				if (item->mid_Type == HJFaceuConstants::AnchorAlignType_Fixed)
				{
					float mid_x = (float)item->mid_x;
					if (ImGui::DragFloat("mid_x", &mid_x, 0.1f))
					{
						item->mid_x = (double)mid_x;
					}
					float mid_y = (float)item->mid_y;
					if (ImGui::DragFloat("mid_y", &mid_y, 0.1f))
					{
						item->mid_y = (double)mid_y;
					}
				}
				//if (item->scale_Type == HJFaceuConstants::ScaleType_Fixed)
				//{
				float scale_ratio = (float)item->scale_ratio;
				if (ImGui::DragFloat("scale_ratio", &scale_ratio, 0.1f, 1.f, 500.f))
				{
					item->scale_ratio = (double)scale_ratio;
				}
				//}

				ImGui::DragInt("Anchor Offset X", &item->anchor_offset_x, 1.0f);
				ImGui::DragInt("Anchor Offset Y", &item->anchor_offset_y, 1.0f);
				//ImGui::DragInt("Scale Ratio", &item->scale_ratio, 1.0f);

				ImGui::Text("Frame Count: %d", item->mframeCount); ImGui::SameLine();
				ImGui::Text("Radius Type: %d", item->radius_Type); ImGui::SameLine();
				//ImGui::Text("Radius: %d", item->mradius); ImGui::SameLine();

				ImGui::Text("Mid Type: %d", item->mid_Type); ImGui::SameLine();
				ImGui::Text("Scale Type: %d", item->scale_Type); ImGui::SameLine();

				//ImGui::Text("Mid X: %.2f", item->mid_x); ImGui::SameLine();
				//ImGui::Text("Mid Y: %.2f", item->mid_y);

				ImGui::Text("ASize Offset X <width> : %d", item->asize_offset_x); ImGui::SameLine();
				ImGui::Text("ASize Offset Y <height>: %d", item->asize_offset_y);
			}
			ImGui::PopID();
		}
		priSave(faceuInfo, m_faceuPath, true);
	}
}
void HJUIItemFaceuTool::priBaseInfo()
{
	if (m_faceuPath[0] == 0)
	{
		size_t len = s_faceuUrl.length();
		if (len > 511) len = 511;
		memcpy(m_faceuPath, s_faceuUrl.c_str(), len);
		m_faceuPath[len] = 0;
	}
	ImGui::InputText("FaceuUrl", m_faceuPath, 512);
}
void HJUIItemFaceuTool::priCreateFaceu()
{
	if (ImGui::Button("parse"))
	{
		std::string rootPath = m_faceuPath;
		if (!rootPath.empty() && std::filesystem::exists(rootPath) && std::filesystem::is_directory(rootPath))
		{
			m_createFaceuInfo = HJFaceuInfo::Create();
			auto isImageFile = [](const std::filesystem::path& path) -> bool {
				std::string ext = path.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
			};
			for (const auto& entry : std::filesystem::directory_iterator(rootPath))
			{
				if (entry.is_directory())
				{
					auto texture = HJFaceuTextureInfo::Create();
					texture->imageName = entry.path().filename().string();

					int frameCount = 0;
					std::string firstImgPath;

					for (const auto& imgEntry : std::filesystem::directory_iterator(entry.path()))
					{
						if (imgEntry.is_regular_file() && isImageFile(imgEntry.path()))
						{
							frameCount++;
							if (firstImgPath.empty())
							{
								firstImgPath = imgEntry.path().string();
							}
						}
					}
					texture->mframeCount = frameCount;

					if (!firstImgPath.empty())
					{
						int width, height, nrComponents;
						unsigned char* data = stbi_load(firstImgPath.c_str(), &width, &height, &nrComponents, 0);
						if (data)
						{
							texture->asize_offset_x = width;
							texture->asize_offset_y = height;
							stbi_image_free(data);
						}
					}
					texture->scale_ratio = 100.0;

					//if (texture->mid_Type == HJFaceuConstants::AnchorAlignType_Eyebrows)
					//{
					texture->anchor_offset_x = -180;
					texture->anchor_offset_y = -280;
					//}
					//else if (texture->mid_Type == HJFaceuConstants::AnchorAlignType_Nose || texture->mid_Type == HJFaceuConstants::AnchorAlignType_Mouth)
					//{
					//	texture->anchor_offset_x = -40;
					//	texture->anchor_offset_y = -30;
					//}

					//if (texture->scale_Type == HJFaceuConstants::ScaleType_Fixed)
					//{
					//	texture->scale_ratio = 1;
					//}
					//else
					//{
					texture->scale_ratio = 100;
					//}

					texture->mradius = 0;

					m_createFaceuInfo->texture.push_back(texture);
				}
			}
			m_createFaceuInfo->framerate = 15;
			m_createFaceuInfo->initSerialToFile(HJBaseUtils::getRealPath(rootPath) + "config");
		}
	}


	if (m_createFaceuInfo)
	{
		ImGui::DragInt("framerate", &m_createFaceuInfo->framerate, 1.0f, 1, 60);

		for (int i = 0; i < m_createFaceuInfo->texture.size(); i++)
		{
			auto& item = m_createFaceuInfo->texture[i];
			ImGui::PushID(i);
			if (ImGui::CollapsingHeader(item->imageName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("rotate_type");
				ImGui::RadioButton("rotate_eyes", &item->radius_Type, HJFaceuConstants::RotateType_Eyes); ImGui::SameLine();
				ImGui::RadioButton("rotate_fixed", &item->radius_Type, HJFaceuConstants::RotateType_Fixed);

				if (item->radius_Type == HJFaceuConstants::RotateType_Fixed)
				{
					ImGui::DragInt("mradius", &item->mradius, 1.0f);
				}

				ImGui::Text("align_type");
				ImGui::RadioButton("align_eyebrows", &item->mid_Type, HJFaceuConstants::AnchorAlignType_Eyebrows); ImGui::SameLine();
				ImGui::RadioButton("align_nose", &item->mid_Type, HJFaceuConstants::AnchorAlignType_Nose); ImGui::SameLine();
				ImGui::RadioButton("align_mouth", &item->mid_Type, HJFaceuConstants::AnchorAlignType_Mouth); ImGui::SameLine();
				if (ImGui::RadioButton("align_fixed", &item->mid_Type, HJFaceuConstants::AnchorAlignType_Fixed))
				{
					item->anchor_offset_x = 0;
					item->anchor_offset_y = 0;
				}

				if (item->mid_Type == HJFaceuConstants::AnchorAlignType_Fixed)
				{
					float mid_x = (float)item->mid_x;
					if (ImGui::DragFloat("align_x", &mid_x, 0.1f))
					{
						item->mid_x = mid_x;
					}
					float mid_y = (float)item->mid_y;
					if (ImGui::DragFloat("align_y", &mid_y, 0.1f))
					{
						item->mid_y = mid_y;
					}
				}

				ImGui::Text("sacle_type");
				if (ImGui::RadioButton("scale_eyedistance", &item->scale_Type, HJFaceuConstants::ScaleType_EyeDistance))
				{
					item->scale_ratio = 100;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("scale_fixed", &item->scale_Type, HJFaceuConstants::ScaleType_Fixed))
				{
					item->scale_ratio = 1;
				}

				float scale_ratio = (float)item->scale_ratio;
				if (ImGui::DragFloat("scale_ratio", &scale_ratio, 0.1f, 1.f, 500.f))
				{
					item->scale_ratio = scale_ratio;
				}

				ImGui::DragInt("anchor_offset_x", &item->anchor_offset_x, 1.0f);
				ImGui::DragInt("anchor_offset_y", &item->anchor_offset_y, 1.0f);

				ImGui::Text("frame_count %d", item->mframeCount);
				ImGui::Text("resolution %d x %d", item->asize_offset_x, item->asize_offset_y);
			}
			ImGui::PopID();
		}

		priSave(m_createFaceuInfo, m_faceuPath, false);

	}

}
int HJUIItemFaceuTool::run()
{
	int i_err = 0;
	do
	{
		ImGui::SetNextWindowPos(ImVec2(0, 10), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(200, 50), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("BaseInfo"))
		{
			priBaseInfo();
		}
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(0, 80), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("CreateFaceu"))
		{
			priCreateFaceu();
		}
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(800, 80), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("AdjustFaceu"))
		{
			priAdjustFaceu();
		}
		ImGui::End();

	} while (false);
	return i_err;
}

int HJUIItemFaceuTool::renderEveryStart()
{
	int i_err = 0;
	do
	{
		int width = 720;
		int height = 1280;

		if (!m_rteGraphProc)
		{
			if (!m_uiWindow)
			{
				glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
				glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); //windows use WGL , other use EGL
				m_uiWindow = glfwCreateWindow(width, height, "ui", NULL, NULL);// also can use shared context with parent window NULL=>(GLFWwindow*)pWindow;

				if (!m_uiWindow)
				{
					glfwTerminate();
					i_err = -1;
					break;
				}
				HJGUICommon::allignMonitorTopRight(m_uiWindow, width, height);
			}

			HJBaseParam::Ptr param = HJBaseParam::Create();
			m_rteGraphProc = HJRteGraphProc::createRteGraphProc((HJRteGraphProcType)HJRteGraphProcType_CONFIG_SETUP);

			if (m_bUseImgSeq)
			{
				m_bUseFakePoints = false;
				HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlImgSeq, s_imageSeq);
				HJ_CatchMapSetVal(param, HJRteGraphConstructorType, HJRteGraphConstructorType_ImageSeq);
				HJ_CatchMapPlainSetVal(param, bool, "bDebugPoint", false);
				HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, m_faceuPath);

				HJRenderIndexCb idxCbFunc = [this](int i_idx)
					{
						m_imgSeqIndex = i_idx;
					};
				HJ_CatchMapSetVal(param, HJRenderIndexCb, idxCbFunc);

				m_facePointsMadeup = std::make_shared<HJFacePointsMadeup>();
				MoreFacePointAcquireFunc func = [this]()
					{
						int oIndx = 0;
						std::shared_ptr<HJFacePointsReal> pointReal = m_facePointsMadeup->getFacePoints(m_imgSeqIndex, oIndx);
						return HJMoreFacePointsReal::cvtFrom(pointReal);
					};
				HJ_CatchMapSetVal(param, MoreFacePointAcquireFunc, func);
			}
			else
			{
				HJ_CatchMapPlainSetVal(param, std::string, "ImageUrl", s_imageUrl);
				HJ_CatchMapSetVal(param, HJRteGraphConstructorType, HJRteGraphConstructorType_Image);
			}

			std::string configInfo = HJRteGraphConfigConstructor::constructGraph(param);
			HJ_CatchMapPlainSetVal(param, std::string, "graphConfigInfo", configInfo);

			HJRenderMakeCurrent makeCurrentCb = [this](bool i_bMake)
				{
					if (i_bMake)
					{
						glfwMakeContextCurrent(m_uiWindow);
					}
					else
					{
						glfwMakeContextCurrent(nullptr);
					}
					return HJ_OK;
				};
			HJ_CatchMapSetVal(param, HJRenderMakeCurrent, makeCurrentCb);

			i_err = m_rteGraphProc->init(param);
			if (i_err < 0)
			{
				HJFLoge("Init error:{}", i_err);
				break;
			}
			m_surfacePtr = HJOGEGLSurface::Create();
			m_surfacePtr->setInsName("windows_eglsurface");
			m_surfacePtr->setWindow(m_uiWindow);
			m_surfacePtr->setEGLSurface(m_uiWindow);
			m_surfacePtr->setTargetWidth(width);
			m_surfacePtr->setTargetHeight(height);
			m_surfacePtr->setSurfaceType(HJOGEGLSurfaceType_UI);
			m_surfacePtr->setFps(30);
			m_surfacePtr->setMakeCurrentCb([this]()
				{
					glfwMakeContextCurrent(m_uiWindow);
					return HJ_OK;
				});
			m_surfacePtr->setSwapCb([this]()
				{
					glfwSwapBuffers(m_uiWindow);
					return HJ_OK;
				});

			m_rteGraphProc->procEGLSurface(m_surfacePtr, HJOGEGLSurfaceType_UI, HJTargetState_Create);

			if (!m_bUseImgSeq)
			{
				priFaceuOpen(m_faceuPath);
			}

		}

		if (m_bUseFakePoints && m_rteGraphProc)
		{
			HJMoreFacePointsReal::Ptr morePtr = HJMoreFacePointsReal::getFakePoints(true);
			std::string sourceName = m_bUseImgSeq ? HJRteGraphConfig::HJNodeClass_SourceImageSeq : HJRteGraphConfig::HJNodeClass_SourceImage;
			m_rteGraphProc->setFaceInfo(sourceName, morePtr->serial());
		}

	} while (false);
	return i_err;
}
int HJUIItemFaceuTool::renderEveryEnd()
{
	return HJ_OK;
}

NS_HJ_END



