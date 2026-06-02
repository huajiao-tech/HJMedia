#pragma once

#include "HJFaceRenderUI.h"
#include "HJError.h"
#include "imgui.h"
#include "HJRenderFaceuExport.h"
#include "HJThreadPool.h"
#include "HJFLog.h"
#include <glad/gl.h> 
#include <GLFW/glfw3.h>
#include "HJUtilitys.h"
#include "HJFacePointsMadeup.h"
#include "HJFacePointMgr.h"
#include "HJOGCommon.h"
#include <filesystem>
#include <vector>
#include "HJBaseUtils.h"
#include "HJImgSeqInfo.h"
#include "stb_image.h"
#include "HJTime.h"
#include "HJFileData.h"
#include "libyuv.h"
#include "HJFacePointsReal.h"

NS_HJ_BEGIN

std::string HJFaceRenderUI::m_imgSeqUrl = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing");

std::string HJFaceRenderUI::m_faceuUrl0 = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/60031_10");
std::string HJFaceRenderUI::m_faceuUrl1 = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/XianWeng");
std::string HJFaceRenderUI::m_faceuUrl2 = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/90237_1");
void HJFaceRenderUI::priHJFaceuCallback(const char* i_uniqueKey, int i_type)
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
        HJOGCommon::textureDestroy(m_textureFaceuId);
        HJOGCommon::textureDestroy(m_textureFaceuId1);
        HJOGCommon::textureDestroy(m_textureImgSeqId);
        m_bCreateTexture = false;
    }
    if (m_handle)
    {
        faceuDone(m_handle);
        m_handle = nullptr;
    }
#if TWO_HANDLE
    if (m_handle1)
    {
        faceuDone(m_handle1);
        m_handle1 = nullptr;
    }
#endif
}
void HJFaceRenderUI::priDrawOneAndAlpha(GLuint i_textureId, int i_width, int i_height)
{
    //default use (alpha, 1-alpha), must use (1, 1-alpha)
	//ImGui::Image((void*)(intptr_t)m_textureFaceuId, ImVec2(width, height));
	
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddCallback([](const ImDrawList*, const ImDrawCmd*)
		{
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}, nullptr);

	ImGui::Image((void*)(intptr_t)i_textureId, ImVec2(i_width, i_height));

	draw_list->AddCallback([](const ImDrawList*, const ImDrawCmd*)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}, nullptr);
}


int HJFaceRenderUI::run()
{
    int i_err = HJ_OK;
    bool bEnd = false;
    do
    {
        if (ImGui::Begin("Faceu Manager"))
        {
            if (ImGui::Button("open"))
            {
                m_bDraw = true;
                if (m_handle)
                {
                    faceuDone(m_handle);
                    m_handle = nullptr;
                }
#if TWO_HANDLE
                if (m_handle1)
                {
                    faceuDone(m_handle1);
                    m_handle1 = nullptr;
                }
#endif
                m_handle = faceuInit(priHJFaceuCallback, true);
#if TWO_HANDLE
                m_handle1 = faceuInit(priHJFaceuCallback, true);
#endif
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
                    int addErr0 = faceuAdd(m_handle, key.c_str(), m_urls[i], m_bDebugPoint);
#if TWO_HANDLE
                    int addErr1 = faceuAdd(m_handle1, key.c_str(), m_urls[i], m_bDebugPoint);
#endif
                    glfwMakeContextCurrent(old_ctx);
                    if (addErr0 != HJ_OK)
                    {
                        i_err = HJErrInvalid;
                        HJFLoge("faceuAdd failed: h0:{} ", addErr0);
                        break;
                    }
#if TWO_HANDLE
					if (addErr1 != HJ_OK)
					{
                        i_err = HJErrInvalid;
						HJFLoge("faceuAdd failed:h1:{}", addErr1);
						break;
					}
#endif
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    std::string key = "face_" + std::to_string(i);
                    GLFWwindow* old_ctx = glfwGetCurrentContext();
                    int rmErr0 = faceuRemove(m_handle, key.c_str());
#if TWO_HANDLE
                    int rmErr1 = faceuRemove(m_handle1, key.c_str());
#endif
                    glfwMakeContextCurrent(old_ctx);
                    if (rmErr0 != HJ_OK)
                    {
                        i_err = HJErrInvalid;
                        HJFLoge("faceuAdd failed: h0:{} ", rmErr0);
                        break;
                    }
#if TWO_HANDLE
                    if (rmErr1 != HJ_OK)
                    {
                        i_err = HJErrInvalid;
                        HJFLoge("faceuAdd failed:h1:{}", rmErr1);
                        break;
                    }
#endif
                }
                ImGui::PopID();
            }

            if (ImGui::Button("RemoveAll"))
            {
                GLFWwindow* old_ctx = glfwGetCurrentContext();
                int rmAll0 = faceuRemoveAll(m_handle);
#if TWO_HANDLE
                int rmAll1 = faceuRemoveAll(m_handle1);
#endif
                glfwMakeContextCurrent(old_ctx);
                if (rmAll0 != HJ_OK)
                {
                    i_err = HJErrInvalid;
                    HJFLoge("faceuRemoveAll failed: h0:{}", rmAll0);
                    break;
                }

#if TWO_HANDLE
                if (rmAll1 != HJ_OK)
                {
                    i_err = HJErrInvalid;
                    HJFLoge("faceuAdd failed:h1:{}", rmAll1);
                    break;
                }
#endif
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
                HJImgSeqConfigInfo info;
                m_imgSeqPaths = HJImgSeqParse::parseConfig(m_imgSeqUrl, info);
                HJFLogi("m_imgSeqPaths size: {}", m_imgSeqPaths.size());
            }
            ImGui::SameLine();
            ImGui::Checkbox("IsDrawImgSeq", &m_bDrawImgSeq);
        }
        bEnd = true;
        ImGui::End();

        if (m_bDraw)
        {
            static HJFacePointsMadeup::Ptr makeup = HJFacePointsMadeup::Create();
            int oIndex = 0;
            std::shared_ptr<HJFacePointsReal> value = makeup->getFacePoints(-1, oIndex);
            //for (int i = 0; i < nCount; i++)
            //{
            //    points[i].x = value->getFilterPt()[i].x;
            //    points[i].y = value->getFilterPt()[i].y;
            //    offsetPoints[i] = points[i];
            //    offsetPoints[i].y += 400;
            //}

            unsigned char* oRGBA = nullptr;
            unsigned char* oRGBA1 = nullptr;
            // Save current context (Main Window)
            GLFWwindow* old_ctx = glfwGetCurrentContext();
            int width = makeup->getWidth();
            int height = makeup->getHeight();

            bool bContain = m_bContainFace;

            HJFacePointInfo morePoints;
            float offsety = 0.f;
            morePoints.width = width;
            morePoints.height = height;
            morePoints.faceCount = 2;
#if 0
			static int ii = 0;
			if (ii % 30 == 0)
			{
                morePoints.faceCount = 0;
			}
			ii++;
#endif
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
            int renderErr0 = faceuRender(m_handle, &morePoints, oRGBA);
#if TWO_HANDLE
            int renderErr1 = faceuRender(m_handle1, width, makeup->getHeight(), offsetPoints, nCount, m_bContainFace, oRGBA1);
#endif
            if (morePoints.faceCount > 0)
            {
                delete [] morePoints.faces;
            }

            glfwMakeContextCurrent(old_ctx);
            if (renderErr0 < 0)
            {
                i_err = HJErrInvalid;
                HJFLoge("faceuRender failed: h0:{}", renderErr0);
                break;
            }
            else if (renderErr0 == HJ_WOULD_BLOCK)
            {
                HJFLogi("faceuRender wouldblock h0:{} ", renderErr0);
            }
#if TWO_HANDLE
			if (renderErr1 < 0)
			{
				i_err = HJErrInvalid;
				HJFLoge("faceuRender failed: h1:{}", renderErr1);
				break;
			}
			else if (renderErr1 == HJ_WOULD_BLOCK)
			{
				HJFLogi("faceuRender wouldblock h1:{} ", renderErr1);
			}
#endif

            if (oRGBA || oRGBA1)
            {
                HJFLogi("HJFaceRenderUI::run() oWidth:{}, oHeight:{}", width, height);
                if (!m_bCreateTexture)
                {
                    m_textureFaceuId = HJOGCommon::textureCreate();
                    m_textureFaceuId1 = HJOGCommon::textureCreate();
                    m_textureImgSeqId = HJOGCommon::textureCreate();
                    m_bCreateTexture = true;
                }
                if (oRGBA)
                {
                    HJOGCommon::textureUploadRGBA(m_textureFaceuId, width, height, oRGBA);
                }
                if (oRGBA1)
                {
                    HJOGCommon::textureUploadRGBA(m_textureFaceuId1, width, height, oRGBA1);
                }
                ImGui::Begin("FaceuImg");


                ImGui::SetWindowSize(ImVec2(width, height * 2.0f + 60.0f));

                if (oRGBA)
                {
                    ImGui::TextUnformatted("Handle0");
                    int imgIdx = oIndex - 1; //pbo delay one frame;
                    if (m_bDrawImgSeq && (imgIdx >= 0 && imgIdx < m_imgSeqPaths.size()))
                    {
                        int seqWidth = 0, seqHeight = 0, nrComponents = 0;
                        int64_t tt0 = HJCurrentSteadyMS();
                        unsigned char* data = stbi_load(m_imgSeqPaths[imgIdx].c_str(), &seqWidth, &seqHeight, &nrComponents, 0);
                        if (data)
                        {
                            int64_t tt1 = HJCurrentSteadyMS();
                            HJFLogi("stbi_load time: {}", tt1 - tt0);
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


           //                 if (nrComponents == 3)
           //                 {
           //                     static HJYuvWriter::Ptr yuvFilter = nullptr;
								//if (!yuvFilter)
								//{
								//	yuvFilter = HJYuvWriter::Create();
								//	std::string path = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "test.yuv");
								//	yuvFilter->init(path, width, height);
								//}
								//HJSPBuffer::Ptr yuvBuffer = HJSPBuffer::create(width * height * 3 / 2);
								//libyuv::RGB24ToI420(data, width * 3,
           //                         yuvBuffer->getBuf(), width,
								//	yuvBuffer->getBuf() + width * height, width / 2,
								//	yuvBuffer->getBuf() + width * height * 5 / 4, width / 2,
								//	width, height);
								//yuvFilter->write(yuvBuffer->getBuf());
           //                 }

                            HJOGCommon::textureUpload(m_textureImgSeqId, internalFormat, seqWidth, seqHeight, dataFormat, data);

                            //if more level draw, must restore cursor pos; else the top image is bottom right, not draw success;
                            ImVec2 cursor_pos = ImGui::GetCursorPos();
                            ImGui::Image((void*)(intptr_t)m_textureImgSeqId, ImVec2(seqWidth, seqHeight));
                            ImGui::SetCursorPos(cursor_pos);
                        }
                        stbi_image_free(data);
                    }
                    ImVec2 cursor_pos = ImGui::GetCursorPos();
                    priDrawOneAndAlpha(m_textureFaceuId, width, height);
                    if (oRGBA1)
                    {
                        ImGui::SetCursorPos(cursor_pos);
                    }                
                }

                if (oRGBA1)
                {
                    //the last not set cursor pos
                    //ImVec2 cursor_pos = ImGui::GetCursorPos();
                    priDrawOneAndAlpha(m_textureFaceuId1, width, height);
                    //ImGui::SetCursorPos(cursor_pos);
                }

                ImGui::Button("test");

                ImGui::End();
            }
            else
            {
                HJFLogi("HJFaceRenderUI::run() not result");
            }
        }
    } while (false);

    if (!bEnd)
    {
        ImGui::End();
    }
    
    return i_err;
}

NS_HJ_END



