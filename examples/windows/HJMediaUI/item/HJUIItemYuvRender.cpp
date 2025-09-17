#pragma once

#include "HJUIItemYuvRender.h"
#include "HJFLog.h"
#include "imgui.h"
#include "HJFileData.h"
#include "libYuv.h"
#include "HJSPBuffer.h"
#include "HJYuvTexCvt.h"

NS_HJ_BEGIN


HJUIItemYuvRender::HJUIItemYuvRender()
{
	m_yuvTexCvt = HJYuvTexCvt::Create();
}

HJUIItemYuvRender::~HJUIItemYuvRender()
{
	priDone();
}
int HJUIItemYuvRender::priTryGetParam()
{
	int i_err = 0;
	do
	{
		ImGui::Begin("YuvParams");
		ImGui::SetWindowSize(ImVec2(500, 200));
		char urls[512] = {};
		if (!m_yuvFilePath.empty())
		{
			memcpy(urls, m_yuvFilePath.c_str(), m_yuvFilePath.length());
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
			m_yuvFilePath = urls;
		}
		ImGui::PushItemWidth(150.0f);
		if (ImGui::InputInt("width", &m_width))
		{
		}
		if (ImGui::InputInt("height", &m_height))
		{
		}
		ImGui::PopItemWidth();

		if (ImGui::Button("Apply"))
		{
			if (m_yuvFilePath.empty() || (m_width <= 0) || (m_height <= 0))
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
int HJUIItemYuvRender::run()
{
	int i_err = 0;
	do
	{
		priTryGetParam();

		if (m_state == HJUIItemState_ready)
		{
			//std::string url = "E:/video/yuv/dance1_504_896.yuv";
			//int width = 504;
			//int height = 896;

			m_yuvReader = HJYuvReader::Create();
			i_err = m_yuvReader->init(m_yuvFilePath, m_width, m_height);
			if (i_err)
			{
				break;
			}
			

			m_state = HJUIItemState_run;
		}

		if (m_state == HJUIItemState_run)
		{
			unsigned char* pYuv = m_yuvReader->read();
			int w = m_yuvReader->getWidth();
			int h = m_yuvReader->getHeight();

			HJYuvInfo::Ptr yuvInfo = HJYuvInfo::Create();
			yuvInfo->m_y = m_yuvReader->getY();
			yuvInfo->m_u = m_yuvReader->getU();
			yuvInfo->m_v = m_yuvReader->getV();
			yuvInfo->m_width = w;
			yuvInfo->m_height = h;
			yuvInfo->m_yLineSize = w;
			yuvInfo->m_uLineSize = w / 2;
			yuvInfo->m_vLineSize = w / 2;
			m_yuvTexCvt->update(yuvInfo);
			

			ImGui::Begin("DrawImg");
			ImGui::SetWindowSize(ImVec2(w, h));
			ImGui::Image((void*)(intptr_t)m_yuvTexCvt->getTextureId(), ImVec2(w, h));
			ImGui::End();
		}
	} while (false);
	return i_err;
}

void HJUIItemYuvRender::priDone()
{
	m_yuvReader = nullptr;
	m_state = HJUIItemState_idle;

	//m_yuvFilePath = "";
	//m_width = 0;
	//m_height = 0;
}

NS_HJ_END



