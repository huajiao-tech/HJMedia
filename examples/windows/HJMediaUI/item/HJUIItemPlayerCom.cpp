#pragma once

#include "HJUIItemPlayerCom.h"
#include "HJFLog.h"
#include "imgui.h"

#include "HJGraphComPlayer.h"
#include "HJMediaInfo.h"
#include "HJComEvent.h"
#include "HJMediaFrame.h"
#include "HJYuvTexCvt.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN


HJUIItemPlayerCom::HJUIItemPlayerCom()
{
	m_yuvTexCvt = HJYuvTexCvt::Create();
}

HJUIItemPlayerCom::~HJUIItemPlayerCom()
{
	priDone();
}

int HJUIItemPlayerCom::priTryGetParam()
{
	int i_err = 0;
	do
	{
		ImGui::Begin("PlayerParams");
		ImGui::SetWindowSize(ImVec2(500, 200));
		char urls[512] = {};
		if (!m_url.empty())
		{
			memcpy(urls, m_url.c_str(), m_url.length());
		}
		////lfs

		//if (ImGui::BeginDragDropTarget()) 
		//{
		//	if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_FILE"))
		//	{
		//		const char* dropped_file = (const char*)payload->Data;
		//		strncpy(urls, dropped_file, IM_ARRAYSIZE(urls) - 1);
		//		urls[IM_ARRAYSIZE(urls) - 1] = '\0'; 
		//	}
		//	ImGui::EndDragDropTarget();
		//}

		if (ImGui::InputText("url", urls, IM_ARRAYSIZE(urls)))
		{
			m_url = urls;
		}

		if (ImGui::Button("Apply"))
		{
			if (m_url.empty())
			{
				HJFLoge("param error");
				ImGui::End();
				return 0;
			}

			priDone();

			m_state = HJUIItemState_ready;
		}
		ImGui::End();
	} while (false);
	return i_err;
}




int HJUIItemPlayerCom::run()
{
	int i_err = 0;
	do
	{
		if (!m_player)
		{
			HJBaseNotify notifyFun = ([this](std::shared_ptr<HJBaseNotifyInfo> i_info)
				{
					if (i_info->getType() == HJCOMPLAYER_RENDER_FRAME)
					{
						HJMediaFrame::Ptr frame = nullptr;
						HJ_CatchMapGetVal(i_info, HJMediaFrame::Ptr, frame);
						if (frame)
						{
							m_asyncCache.enqueue(frame);
						}				
					}
				});

			m_player = HJGraphComPlayer::Create();
			HJBaseParam::Ptr param = HJBaseParam::Create();

			HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>("E:/video/play.mp4"); //("E:/video/special_video/H264-265-264_RES.flv");// 
			HJ_CatchMapSetVal(param, HJMediaUrl::Ptr, mediaUrl);
			HJ_CatchMapSetVal(param, HJBaseNotify, (HJBaseNotify)notifyFun);
			(*param)["HJDeviceType"] = (int)HJPlayerVideoCodecType_SoftDefault;
			m_player->init(param);
		}
		
		//HJSleep(1000);


		priTryGetParam();

		HJMediaFrame::Ptr hFrame = m_asyncCache.acquire();
		if (hFrame)
		{
			HJAVFrame::Ptr avFrame = hFrame->getMFrame();
			AVFrame* frame = avFrame->getAVFrame();
			//HJFLoge("video render frame: {}, {}, {}", frame->width, frame->height, frame->pts);

			HJYuvInfo::Ptr yuvInfo = HJYuvInfo::Create();
			yuvInfo->m_y = frame->data[0];
			yuvInfo->m_u = frame->data[1];
			yuvInfo->m_v = frame->data[2];
			yuvInfo->m_width = frame->width;
			yuvInfo->m_height = frame->height;
			yuvInfo->m_yLineSize = frame->linesize[0];
			yuvInfo->m_uLineSize = frame->linesize[1];
			yuvInfo->m_vLineSize = frame->linesize[2];
			m_yuvTexCvt->update(yuvInfo);


			ImGui::Begin("DrawImg");
			ImGui::SetWindowSize(ImVec2(yuvInfo->m_width, yuvInfo->m_height));
			ImGui::Image((void*)(intptr_t)m_yuvTexCvt->getTextureId(), ImVec2(yuvInfo->m_width, yuvInfo->m_height));
			ImGui::End();

			m_asyncCache.recovery(hFrame);
		}
		//if (m_state == HJUIItemState_ready)
		//{
		//	//std::string url = "E:/video/yuv/dance1_504_896.yuv";
		//	//int width = 504;
		//	//int height = 896;

		//	m_yuvReader = HJYuvReader::Create();
		//	i_err = m_yuvReader->init(m_yuvFilePath, m_width, m_height);
		//	if (i_err)
		//	{
		//		break;
		//	}
		//	if (!m_bCreateTexture)
		//	{
		//		m_bCreateTexture = true;
		//		glGenTextures(1, &m_textureId);
		//		glBindTexture(GL_TEXTURE_2D, m_textureId);

		//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//	}
		//	m_RGBBuffer = HJSPBuffer::create(m_width * m_height * 4);

		//	m_state = HJUIItemState_run;
		//}

		//if (m_state == HJUIItemState_run)
		//{
		//	unsigned char* pYuv = m_yuvReader->read();
		//	int w = m_yuvReader->getWidth();
		//	int h = m_yuvReader->getHeight();
		//	libyuv::I420ToABGR(m_yuvReader->getY(), w, m_yuvReader->getU(), w / 2, m_yuvReader->getV(), w / 2,
		//		m_RGBBuffer->getBuf(), w * 4,
		//		w, h);

		//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RGBBuffer->getBuf());

		//	ImGui::Begin("DrawImg");
		//	ImGui::SetWindowSize(ImVec2(w, h));
		//	ImGui::Image((void*)(intptr_t)m_textureId, ImVec2(w, h));
		//	ImGui::End();
		//}
	} while (false);
	return i_err;
}

void HJUIItemPlayerCom::priDone()
{
	if (m_player)
	{
		m_player->done();
		m_player.reset();
	}
	m_state = HJUIItemState_idle;
}

NS_HJ_END



