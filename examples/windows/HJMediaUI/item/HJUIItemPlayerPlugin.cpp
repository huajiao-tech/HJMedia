#pragma once

#include "HJUIItemPlayerPlugin.h"
#include "HJFLog.h"
#include "imgui.h"

#include "HJGraphLivePlayer.h"
#include "HJGraphVodPlayer.h"
#include "HJNotify.h"
#include "HJMediaInfo.h"
#include "HJComEvent.h"
#include "HJMediaFrame.h"
#include "HJYuvTexCvt.h"
#include "HJFFHeaders.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

HJUIItemPlayerPlugin::HJUIItemPlayerPlugin()
{
	m_yuvTexCvt = HJYuvTexCvt::Create();
}

HJUIItemPlayerPlugin::~HJUIItemPlayerPlugin()
{
	priDone();
	if (m_vq) {
		video_queue_close(m_vq);
	}
}
int HJUIItemPlayerPlugin::priTryGetParam()
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




int HJUIItemPlayerPlugin::run()
{
	int i_err = 0;
	do
	{
		if (!m_player)
		{
			auto param = std::make_shared<HJKeyStorage>();

//			std::string url = "https://live-pull-3.huajiao.com/main/HJ_0_tc_3_main__h265_180277754_1764313668502_1106_O.flv?txSecret=497c3a676f441d8cb9aefb2b7ebb5852&txTime=692A9F2B";
			std::string url = /*"F:/Sequence/1FF51A933E8ED536.mp3";/*/"F:/Sequence/0.mp4";
			HJGraphType graphType = HJGraphType_LIVESTREAM;
			if ((url.compare(0, 8, "https://") == 0) || (url.compare(0, 7, "http://") == 0))
			{
				graphType = HJGraphType_LIVESTREAM;

				m_statCtx = HJStatContext::Create();
				i_err = m_statCtx->init(123, "windowsDevice", "sn", 10, 0, [](const std::string& i_name, int i_nType, const std::string& i_info)
					{
						HJFLogi("StatStart callback name:{}, type:{}, info:{}", i_name, i_nType, i_info);
					});
				std::weak_ptr<HJStatContext> statCtx = m_statCtx;
				(*param)["HJStatContext"] = statCtx;
			}
			else
			{
				graphType = HJGraphType_VOD;
			}

			m_player = HJGraphPlayer::createGraph(graphType, 0);

			HJListener playerListener = [&](const HJNotification::Ptr ntf) -> int {
				if (ntf == nullptr) {
					return HJ_OK;
				}

				if (ntf->getID() == HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME) {
					auto frame = ntf->getValue<HJMediaFrame::Ptr>("frame");
					if (frame) {
						m_asyncCache.enqueue(frame);
					}				
				}
				else if (ntf->getID() == HJ_PLUGIN_NOTIFY_EOF) {

				}

				return HJ_OK;
			};
			(*param)["playerListener"] = playerListener;

			auto audioInfo = std::make_shared<HJAudioInfo>();
			audioInfo->m_samplesRate = 48000;
			audioInfo->setChannels(2);
			audioInfo->m_sampleFmt = 1;
			audioInfo->m_bytesPerSample = 2;
			(*param)["audioInfo"] = audioInfo;
#if defined (WINDOWS)
			LPWSTR deviceName = L"扬声器 (Realtek(R) Audio)";
			int utf8Size = WideCharToMultiByte(
				CP_UTF8,                // 目标编码：UTF-8
				0,                      // 标志（一般填 0）
				deviceName,			    // 输入的宽字符串
				-1,                     // 自动计算长度（-1 表示以 NULL 结尾）
				NULL,                   // 输出缓冲区（NULL 表示计算所需大小）
				0,                      // 输出缓冲区大小（0 表示计算）
				NULL,                   // 默认字符（一般 NULL）
				NULL                    // 是否使用默认字符（一般 NULL）
			);
			if (utf8Size > 0) {
				// 分配足够的内存存储 UTF-8 字符串
				char* utf8Name = (char*)malloc(utf8Size);
				if (utf8Name) {
					WideCharToMultiByte(
						CP_UTF8, 0, deviceName, -1,
						utf8Name, utf8Size, NULL, NULL
					);

					std::string audioDeviceName = utf8Name;
					free(utf8Name);
//					(*param)["audioDeviceName"] = audioDeviceName;
				}
			}
#endif
			m_player->init(param);
			HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(url);
			m_player->openURL(mediaUrl);
			m_vq = video_queue_open();
		}

		priTryGetParam();

		if (m_vq) {
			auto state = video_queue_state(m_vq);
			if (state != m_prevState)
			{
				m_prevState = state;
				if (state == SHARED_QUEUE_STATE_STOPPING)
				{
					if (m_vq) {
						video_queue_close(m_vq);
						m_vq = nullptr;
					}
				}
				else if (state == SHARED_QUEUE_STATE_INVALID)
				{
				}
			}

			if (state == SHARED_QUEUE_STATE_READY)
			{
				void* data{};
				uint64_t ts;
				int width{}, height{};
				if (video_queue_read2(m_vq, &data, &ts, &width, &height)) {
					size_t size = width * height;
					HJYuvInfo::Ptr yuvInfo = HJYuvInfo::Create();
					yuvInfo->m_y = (uint8_t*)data;
					yuvInfo->m_u = (uint8_t*)data + size;
					yuvInfo->m_v = (uint8_t*)data + size * 5 / 4;
					yuvInfo->m_width = width;
					yuvInfo->m_height = height;
					yuvInfo->m_yLineSize = width;
					yuvInfo->m_uLineSize = width / 2;
					yuvInfo->m_vLineSize = width / 2;
					m_yuvTexCvt->update(yuvInfo);

					ImGui::Begin("DrawImg");
					ImGui::SetWindowSize(ImVec2(yuvInfo->m_width, yuvInfo->m_height));
					ImGui::Image((void*)(intptr_t)m_yuvTexCvt->getTextureId(), ImVec2(yuvInfo->m_width, yuvInfo->m_height));
					ImGui::End();
				}
				else {
					int a = 0;
				}
			}
			else {
				int a = 0;
			}
		}
/*
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
*/
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

void HJUIItemPlayerPlugin::priDone()
{
	if (m_player)
	{
		m_player->done();
		m_player.reset();
	}
	m_state = HJUIItemState_idle;
}

NS_HJ_END



