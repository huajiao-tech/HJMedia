#pragma once

#include "HJUIItemSharedMemory.h"
#include "HJSharedMemory.h"
#include "HJFLog.h"
#include "imgui.h"
#include "HJFileData.h"
#include "libYuv.h"
#include "HJSPBuffer.h"
#include "HJYuvTexCvt.h"
#include "HJMediaFrame.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN


HJUIItemSharedMemory::HJUIItemSharedMemory()
{
	m_yuvTexCvt = HJYuvTexCvt::Create();
}

HJUIItemSharedMemory::~HJUIItemSharedMemory()
{
	priDone();
}
int HJUIItemSharedMemory::priTryGetParam()
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
void HJUIItemSharedMemory::priDraw()
{
	unsigned char* pYuv = m_pBuffer->getBuf();
	int w = m_cacheWidth;
	int h = m_cacheHeight;
	if (!m_pYUV420)
	{
		m_pYUV420 = HJSPBuffer::create(w * h * 3 / 2);
	}
	libyuv::NV12ToI420(pYuv, w, pYuv + w * h, w, 
		m_pYUV420->getBuf(), w, 
		m_pYUV420->getBuf() + w * h, w / 2, 
		m_pYUV420->getBuf() + w * h * 5 / 4, w / 2,
		w, h);  //I420ToNV12

	HJYuvInfo::Ptr yuvInfo = HJYuvInfo::Create();
	yuvInfo->m_y = m_pYUV420->getBuf();
	yuvInfo->m_u = m_pYUV420->getBuf() + w * h;
	yuvInfo->m_v = m_pYUV420->getBuf() + w * h * 5 / 4;
	yuvInfo->m_width = w;
	yuvInfo->m_height = h;
	yuvInfo->m_yLineSize = w;
	yuvInfo->m_uLineSize = w / 2;
	yuvInfo->m_vLineSize = w / 2;
	m_yuvTexCvt->update(yuvInfo);


	ImGui::Begin("consumber");
	ImGui::SetWindowSize(ImVec2(w, h));
	ImGui::Image((void*)(intptr_t)m_yuvTexCvt->getTextureId(), ImVec2(w, h));
	ImGui::End();
}
int HJUIItemSharedMemory::priProcProducer()
{
	int i_err = SHAREDMEM_RET_OK;
	do
	{
		if (!m_producer)
		{
			break;
		}
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

		HJMediaFrame::Ptr frame = HJMediaFrame::makeVideoFrame(nullptr);
		AVFrame* vFrame = hj_make_avframe_fromy_yuv420p(w, h, m_yuvReader->getY(), m_yuvReader->getU(), m_yuvReader->getV());
		frame->setMFrame(vFrame);

		i_err = m_producer->write(frame);
		if (i_err != SHAREDMEM_RET_OK)
		{
			break;
		}
	} while (false);
	return i_err;
}
int HJUIItemSharedMemory::priProcConsumer()
{
	int i_err = SHAREDMEM_RET_OK;
	do
	{
		if (!m_consumer)
		{
			break;
		}
		i_err = m_consumer->read(m_pBuffer->getSize(), m_pBuffer->getBuf());
		if (i_err == SHAREDMEM_RET_OK)
		{
			priDraw();
		}
		else if (i_err == SHAREDMEM_RET_NO_READY)
		{

		}
		else if (i_err == SHAREDMEM_RET_RESOLUTION_CHANGED)
		{
			//m_width = m_pSharedMemory->getWidth();
			//m_height = m_pSharedMemory->getHeight();
			//m_pBuffer = SL::SLBuffer::create(m_width * m_height * 3 / 2);
			//m_pI420Buffer = SL::SLBuffer::create(m_width * m_height * 3 / 2);
		}
		else if (i_err == SHAREDMEM_RET_INVALID_PARAM)
		{

		}
	} while (false);
	return i_err;
}
int HJUIItemSharedMemory::run()
{
	int i_err = 0;
	do
	{
		priTryGetParam();

		if (m_state == HJUIItemState_ready)
		{
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
#if 1
			if (!m_producer)
			{
				m_producer = HJSharedMemoryProducer::Create();
                i_err = m_producer->init(m_width, m_height, 30);
                if (i_err != SHAREDMEM_RET_OK)
                {
                    HJFLoge("init producer error i_err:{}", i_err);
                    break;
                }
			}
#endif

			if (!m_consumer)
			{
				m_consumer = HJSharedMemoryConsumer::Create();
				i_err = m_consumer->init();
				if (i_err != SHAREDMEM_RET_OK)
				{
					m_consumer = nullptr;
					HJFLoge("init consumer error i_err:{}", i_err);
					break;
				}
				m_cacheWidth = m_consumer->getWidth();
				m_cacheHeight = m_consumer->getHeight();
				m_pBuffer = HJSPBuffer::create(m_cacheWidth * m_cacheHeight * 3 / 2);
			}

			i_err = priProcProducer();

			i_err = priProcConsumer();
			
		}

		

		


#if 0
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
#endif
	} while (false);
	return i_err;
}

void HJUIItemSharedMemory::priDone()
{
	m_yuvReader = nullptr;
	m_state = HJUIItemState_idle;

	//m_yuvFilePath = "";
	//m_width = 0;
	//m_height = 0;
}

NS_HJ_END



