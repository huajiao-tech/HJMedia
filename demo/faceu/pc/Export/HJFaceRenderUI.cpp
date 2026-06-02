#pragma once

#include "HJFaceRenderUI.h"
#include "imgui.h"
#include "HJRenderFaceuExport.h"
#include <glad/gl.h> 
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>
#include "HJFacePointsMadeup.h"
#include "HJBaseUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define HJFLoge
#define HJFLogi

std::string HJFaceRenderUI::m_imgSeqUrl = std::string(EXPORT_RESOURCE_ROOT) + "/imgseq/sing";
std::string HJFaceRenderUI::m_imgSeqUrlPrefix = "sing";
std::string HJFaceRenderUI::m_faceuUrl0 = std::string(EXPORT_RESOURCE_ROOT) + "/faceu/XianWeng";
std::string HJFaceRenderUI::m_faceuUrl1 = std::string(EXPORT_RESOURCE_ROOT) + "/faceu/60031_10";
std::string HJFaceRenderUI::m_faceuUrl2 = std::string(EXPORT_RESOURCE_ROOT) + "/faceu/90237_1";
static void priHJFaceuCallback(const char* i_uniqueKey, int i_type)
{
	HJFLogi("HJFaceuCallback: {} i_type:{}", i_uniqueKey, i_type);
	if (i_type == HJ_FACEU_NOTIFY_COMPLETE)
	{
		HJFLogi("the faceu:{} is complete", i_uniqueKey);
	}
}
HJFaceRenderUI::HJFaceRenderUI()
{
    if (!m_faceuUrl0.empty()) snprintf(m_urls[0], sizeof(m_urls[0]), "%s", m_faceuUrl0.c_str());
    if (!m_faceuUrl1.empty()) snprintf(m_urls[1], sizeof(m_urls[1]), "%s", m_faceuUrl1.c_str());
    if (!m_faceuUrl2.empty()) snprintf(m_urls[2], sizeof(m_urls[2]), "%s", m_faceuUrl2.c_str());
}
HJFaceRenderUI::~HJFaceRenderUI()
{
    if (m_bCreateTexture)
    {
        textureDestroy(m_textureFaceuId);
        textureDestroy(m_textureImgSeqId);
        m_bCreateTexture = false;
    }
    if (m_handle)
    {
        faceuDone(m_handle);
        m_handle = nullptr;
    }

}
void HJFaceRenderUI::priDrawOneAndAlpha(int i_width, int i_height)
{
    //default use (alpha, 1-alpha), must use (1, 1-alpha)
	//ImGui::Image((void*)(intptr_t)m_textureFaceuId, ImVec2(width, height));
	
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddCallback([](const ImDrawList*, const ImDrawCmd*)
		{
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}, nullptr);

	ImGui::Image((void*)(intptr_t)m_textureFaceuId, ImVec2(i_width, i_height));

	draw_list->AddCallback([](const ImDrawList*, const ImDrawCmd*)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}, nullptr);
}
int HJFaceRenderUI::run()
{
    int i_err = 0;
    
    do
    {
        if (ImGui::Begin("Faceu Manager"))
        {
            if (ImGui::Button("open"))
            {
                m_bDraw = true;
                m_handle = faceuInit(priHJFaceuCallback, true);
            }
            ImGui::Checkbox("IsContainFace", &m_bContainFace);
            ImGui::SameLine();
            ImGui::Checkbox("IsDebugPoint", &m_bDebugPoint);
            for (int i = 0; i < 3; ++i)
            {
                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Url:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(350.0f);
                ImGui::InputText("##URL", m_urls[i], 256);
                ImGui::SameLine();
                if (ImGui::Button("Add"))
                {
                    std::string key = "face_" + std::to_string(i);
                    GLFWwindow* old_ctx = glfwGetCurrentContext();
                    i_err = faceuAdd(m_handle, key.c_str(), m_urls[i], m_bDebugPoint);
                    glfwMakeContextCurrent(old_ctx);
                    if (i_err != 0)
                    {
                        HJFLoge("faceuAdd failed: {}", i_err);
                        break;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    std::string key = "face_" + std::to_string(i);
                    GLFWwindow* old_ctx = glfwGetCurrentContext();
                    i_err = faceuRemove(m_handle, key.c_str()); 
                    glfwMakeContextCurrent(old_ctx);
                    if (i_err != 0)
                    {
                        HJFLoge("faceuRemove failed: {}", i_err);
                        break;
                    }     
                }
                ImGui::PopID();
            }

            if (ImGui::Button("RemoveAll"))
            {
                GLFWwindow* old_ctx = glfwGetCurrentContext();
                i_err = faceuRemoveAll(m_handle);
                glfwMakeContextCurrent(old_ctx);
                if (i_err != 0)
                {
                    HJFLoge("faceuRemoveAll failed: {}", i_err);
                    break;
                }
            }

            ImGui::Separator();
            static char imgSeqPath[512] = { 0 };
            if (imgSeqPath[0] == 0)
            {
                snprintf(imgSeqPath, sizeof(imgSeqPath), "%s", m_imgSeqUrl.c_str());
            }
            ImGui::InputText("ImgSeqUrl", imgSeqPath, 512);
            if (ImGui::Button("ApplySeqUrl"))
            {
                m_imgSeqUrl = imgSeqPath;
                HJFLogi("m_imgSeqUrl updated to: {}", m_imgSeqUrl);
                m_imgSeqPaths.clear();
                m_imgSeqPaths = parseConfig(m_imgSeqUrl);				
            }
            ImGui::SameLine();
            ImGui::Checkbox("IsDrawImgSeq", &m_bDrawImgSeq);
        }
        ImGui::End();

        if (m_bDraw)
        {
            static std::shared_ptr<HJFacePointsMadeup> makeup = std::make_shared<HJFacePointsMadeup>();
            
            int oIndex = 0;
            std::shared_ptr<HJFacePointsReal> value = makeup->getFacePoints(-1, oIndex);
            
            unsigned char* oRGBA = nullptr;
            // Save current context (Main Window)
            GLFWwindow* old_ctx = glfwGetCurrentContext();
            int width = makeup->getWidth();
            int height = makeup->getHeight();

			HJFacePointInfo morePoints;
			float offsety = 0.f;
			morePoints.width = width;
			morePoints.height = height;
			morePoints.faceCount = 2;
			if (morePoints.faceCount > 0)
			{
				morePoints.faces = new HJSingleFaceInfo[morePoints.faceCount];
				for (int i = 0; i < morePoints.faceCount; i++)
				{
					if (i == 1)
					{
						offsety = 400.f;
					}

					for (int j = 0; j < 5; j++)
					{
						morePoints.faces[i].points[j].x = value->getFilterPt()[j].x;
						morePoints.faces[i].points[j].y = value->getFilterPt()[j].y + offsety;
					}
					morePoints.faces[i].rect.x = value->getFilterPt()[5].x;
					morePoints.faces[i].rect.y = value->getFilterPt()[5].y + offsety;
					morePoints.faces[i].rect.w = value->getFilterPt()[8].x - value->getFilterPt()[5].x;
					morePoints.faces[i].rect.h = value->getFilterPt()[8].y - value->getFilterPt()[5].y;
				}
			}

            i_err = faceuRender(m_handle, &morePoints, oRGBA);
			if (morePoints.faceCount > 0)
			{
				delete[] morePoints.faces;
			}
            
            glfwMakeContextCurrent(old_ctx);
			if (i_err < 0)
			{
				HJFLoge("faceuRender failed: {}", i_err);
				break;
			}
			
            if (oRGBA)
            {
                HJFLogi("HJFaceRenderUI::run() oWidth:{}, oHeight:{}", width, height);
                if (!m_bCreateTexture)
                {
                    m_textureFaceuId = textureCreate();
                    m_textureImgSeqId = textureCreate();
                    m_bCreateTexture = true;
                }
                textureUploadRGBA(m_textureFaceuId, width, height, oRGBA);
                ImGui::Begin("FaceuImg");


                ImGui::SetWindowSize(ImVec2(width, height));

                int imgIdx = oIndex - 1; //pbo delay one frame;
                if (m_bDrawImgSeq && (imgIdx >= 0 && imgIdx < m_imgSeqPaths.size()))
                {
                    int width = 0, height = 0, nrComponents = 0;
                    unsigned char* data = stbi_load(m_imgSeqPaths[imgIdx].c_str(), &width, &height, &nrComponents, 0);
                    if (data)
                    {
                        GLenum internalFormat;
                        GLenum dataFormat;
                        if (nrComponents == 3)
                        {
                            internalFormat = GL_RGB;
                            dataFormat = GL_RGB;
                        }
                        else if (nrComponents == 4)
                        {
                            internalFormat = GL_RGBA;
                            dataFormat = GL_RGBA;
                        }
                        textureUpload(m_textureImgSeqId, internalFormat, width, height, dataFormat, data);

                        //if more level draw, must restore cursor pos; else the top image is bottom right, not draw success;
                        ImVec2 cursor_pos = ImGui::GetCursorPos();
                        ImGui::Image((void*)(intptr_t)m_textureImgSeqId, ImVec2(width, height));
                        ImGui::SetCursorPos(cursor_pos);
                    }
                    stbi_image_free(data);
                }

                priDrawOneAndAlpha(width, height);
                ImGui::End();
            }
            else
            {
                HJFLogi("HJFaceRenderUI::run() not result");
            }
        }
    } while (false);
    return i_err;
}

std::vector<std::string> HJFaceRenderUI::parseConfig(const std::string& i_path)
{
    std::vector<std::string> urls;
    do
    {
        std::string configUrl = HJBaseUtils::combineUrl(i_path, "config.json");
        if (configUrl.empty())
        {
            HJFLoge("configUrl empty error");
            break;
        }
        std::string config = HJBaseUtils::readFileToString(configUrl);
        if (config.empty())
        {
            HJFLoge("config empty error");
            break;
        }
        urls = HJBaseUtils::getSortedFiles(i_path, m_imgSeqUrlPrefix);
    } while (false);
    return urls;
}

GLuint HJFaceRenderUI::textureCreate(uint32_t target)
{
	GLuint textureId = 0;
	glGenTextures(1, &textureId);

	glBindTexture(target, textureId);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(target, 0);
	return textureId;
}

void HJFaceRenderUI::textureDestroy(GLuint texture)
{
	glDeleteTextures(1, &texture);
}

void HJFaceRenderUI::textureUpload(GLuint texture, GLenum internalFormat, GLsizei width, GLsizei height, GLenum dataFormat, unsigned char* data)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void HJFaceRenderUI::textureUploadRGBA(GLuint texture, GLsizei width, GLsizei height, unsigned char* data)
{
	textureUpload(texture, GL_RGBA, width, height, GL_RGBA, data);
}

