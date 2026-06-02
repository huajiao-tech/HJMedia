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
#include "HJRenderCvt.h"
#include "HJTransferMediaData.h"
#include "HJMediaDataDraw.h"
#include "HJRteUtils.h"
#include "HJBaseUtils.h"
#include "HJFacePointsReal.h"

#define TEST_RESTART 0

NS_HJ_BEGIN

HJRteGraphConstructorType HJUIItemPlayerCom::s_configGraphType = HJRteGraphConstructorType_Test;// HJRteGraphConstructorType_PlaceHolder;// HJRteGraphConstructorType_Image;// HJRteGraphConstructorType_Image;// HJRteGraphConstructorType_Test;
bool HJUIItemPlayerCom::s_useSingleUI = true;
int HJUIItemPlayerCom::s_singleUIWidth = 720;
int HJUIItemPlayerCom::s_singleUIHeight = 1280;
bool HJUIItemPlayerCom::s_xMirror = false;
bool HJUIItemPlayerCom::s_customerFilter = false;

std::string HJUIItemPlayerCom::s_imageUrl = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/image/play.jpg");
std::string HJUIItemPlayerCom::s_pngSeqUrl = "harmony/entry/src/main/resources/resfile/pngseq/ShuangDanCaiShen";
std::string HJUIItemPlayerCom::s_playerUrl = "E:/video/chaofen/270538548_272x480.ts";//"E:/video/huajiaoline2_noaudio.flv"; //"E:/code/git/hjmedia/examples/harmony/entry/src/main/resources/resfile/ShuangDanCaiShen.mp4";  //"E:/video/huajiaoline2_noaudio.flv"; //"E:/code/git/hjmedia/examples/harmony/entry/src/main/resources/resfile/ShuangDanCaiShen.mp4"; //"E:/video/huajiaoline2_noaudio.flv"; //"E:/video/play.mp4";//"E:/video/huajiaoline2_noaudio.flv";
std::string HJUIItemPlayerCom::s_splitSreenGiftUrl = "harmony/entry/src/main/resources/resfile/ShuangDanCaiShen.mp4";
std::string HJUIItemPlayerCom::s_faceuUrl = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/faceu/60031_10");
std::string HJUIItemPlayerCom::s_imageSeq = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing");

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
		//ImGui::SetWindowSize(ImVec2(500, 200));
		//ImGui::SetWindowPos(ImVec2(30, 30));
		// Cache PlayerParams window pos/size for sibling window placement
		m_paramsPos = ImGui::GetWindowPos();
		m_paramsSize = ImGui::GetWindowSize();
		m_hasParamsInfo = true;
		char urls[512] = {};
		if (!s_playerUrl.empty())
		{
			memcpy(urls, s_playerUrl.c_str(), s_playerUrl.length());
		}

		if (ImGui::InputText("url", urls, IM_ARRAYSIZE(urls)))
		{
			s_playerUrl = urls;
		}

		if (ImGui::RadioButton("SplitScreen", s_configGraphType == HJRteGraphConstructorType_SplictScreen))
		{
			s_configGraphType = HJRteGraphConstructorType_SplictScreen;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Placeholder", s_configGraphType == HJRteGraphConstructorType_PlaceHolder))
		{
			s_configGraphType = HJRteGraphConstructorType_PlaceHolder;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Test", s_configGraphType == HJRteGraphConstructorType_Test))
		{
			s_configGraphType = HJRteGraphConstructorType_Test;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Image", s_configGraphType == HJRteGraphConstructorType_Image))
		{
			s_configGraphType = HJRteGraphConstructorType_Image;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("ImageSeq", s_configGraphType == HJRteGraphConstructorType_ImageSeq))
		{
			s_configGraphType = HJRteGraphConstructorType_ImageSeq;
		}

		if (s_configGraphType == HJRteGraphConstructorType_SplictScreen)
		{
			ImGui::Checkbox("xMirror", &s_xMirror);
		}
		else if (s_configGraphType == HJRteGraphConstructorType_PlaceHolder)
		{
			ImGui::Checkbox("customerfilter", &s_customerFilter);
		}

		const bool applyClicked = ImGui::Button("Apply");
		ImGui::SameLine();
		if (ImGui::Button("blur"))
		{
			if (m_renderCvt)
			{
				m_isBlure = !m_isBlure;
				m_renderCvt->setBlur(m_isBlure);
			}
		}
		ImGui::SameLine();
		const std::string denoiseBtnText = m_isDenoise ? "denoise: ON" : "denoise: OFF";
		if (ImGui::Button(denoiseBtnText.c_str()))
		{
			if (m_renderCvt)
			{
				m_isDenoise = !m_isDenoise;
				m_renderCvt->setDenoise(m_isDenoise, 0.2f);
			}
		}
		ImGui::SameLine();
		const std::string srBtnText = m_isSR ? "SR: ON" : "SR: OFF";
		if (ImGui::Button(srBtnText.c_str()))
		{
			if (m_renderCvt)
			{
				const bool nextSR = !m_isSR;
				if (m_renderCvt->setSR(nextSR, 1.0f, m_srMatch) >= 0)
				{
					m_isSR = nextSR;
				}
			}
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::SliderFloat("SR match", &m_srMatch, 0.0f, 1.0f, "%.2f"))
		{
			if (m_renderCvt)
			{
				m_renderCvt->setSR(m_isSR, 1.0f, m_srMatch);
			}
		}
		ImGui::SameLine();
		//faceu use bridge, so not work now;
		if (ImGui::Button("faceu"))
		{
			if (m_renderCvt)
			{
				m_isFaceu = !m_isFaceu;
				m_renderCvt->setFaceu(m_isFaceu, s_faceuUrl, true, true);
				if (!m_isFaceu)
				{
					m_renderCvt->setFaceInfo(HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData, "");
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("pngseq"))
		{
			if (m_renderCvt)
			{
				m_renderCvt->openPngseq(WorkDirResource + s_pngSeqUrl);
			}
		}

		if (applyClicked)
		{
			if (s_playerUrl.empty())
			{
				HJFLoge("param error");
				ImGui::End();
				return 0;
			}

			priDone();

			m_state = HJUIItemState_ready;
			m_renderCvt = nullptr;
		}

		if (!m_jsonConfig.empty())
		{
			ImGui::Separator();
			ImGui::Text("Config JSON:");
			ImGui::InputTextMultiline("##jsonConfig", (char*)m_jsonConfig.c_str(), m_jsonConfig.size() + 1, ImVec2(-1.0f, 100.0f), ImGuiInputTextFlags_ReadOnly);
			if (ImGui::Button("Copy JSON"))
			{
				ImGui::SetClipboardText(m_jsonConfig.c_str());
			}
		}

		if (m_isFaceu)
		{
			HJMoreFacePointsReal::Ptr morePtr = HJMoreFacePointsReal::getFakePoints();
			if (m_renderCvt)
			{
				m_renderCvt->setFaceInfo(HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData, morePtr->serial());
			}
		}

		ImGui::End();
	} while (false);	return i_err;
}

int HJUIItemPlayerCom::renderEveryStart()
{
	int i_err = 0;
	do
	{
#if TEST_RESTART
		if ((m_statIdx++) % m_randomVal == 0)
		{
			HJFLogi("restart close enter==============================");
			m_renderCvt = nullptr;
			m_randomVal = HJBaseUtils::getRandomValue(10, 100);
		}
#endif
		//HJFLogi("renderEveryStart");
		if (!m_renderCvt)
		{
			HJFLogi("restart start enter&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&");
			m_renderCvt = HJRenderCvt::Create();

			HJBaseParam::Ptr param = HJBaseParam::Create();
			HJTransferMediaDataGetCb getMDataCb = [this](std::shared_ptr<HJTransferMediaData>& o_data)
				{
					HJMediaFrame::Ptr hFrame = m_asyncCache.acquire();
					if (hFrame)
					{
						HJAVFrame::Ptr avFrame = hFrame->getMFrame();
						AVFrame* frame = avFrame->getAVFrame();
						int stride[4] = { frame->linesize[0],frame->linesize[1],frame->linesize[2], 0 };
						unsigned char* yuv[4] = { frame->data[0],frame->data[1],frame->data[2], nullptr };
						o_data = std::make_shared<HJTransferMediaDataYUVI420>(yuv, stride, frame->width, frame->height);
						m_asyncCache.recovery(hFrame);
					}
				};
			HJ_CatchMapSetVal(param, HJTransferMediaDataGetCb, getMDataCb);


			HJTransferMediaDataSetCb setRenderData = [this](std::shared_ptr<HJTransferMediaData> i_data)
				{
					m_asyncCacheRender.enqueue(std::move(i_data));
				};
			HJ_CatchMapSetVal(param, HJTransferMediaDataSetCb, setRenderData);



			HJ_CatchMapSetVal(param, HJRteGraphConstructorType, s_configGraphType);
			HJ_CatchMapPlainSetVal(param, bool, "IsMirror", s_xMirror);
			HJ_CatchMapPlainSetVal(param, bool, "UseCustomerFilter", s_customerFilter);
			HJ_CatchMapPlainSetVal(param, void*, "ParentWindow", getWindow());
			HJ_CatchMapPlainSetVal(param, bool, "IsUseSingleUI", s_useSingleUI);
			HJ_CatchMapPlainSetVal(param, int, "SingleUIWidth", s_singleUIWidth);
			HJ_CatchMapPlainSetVal(param, int, "SingleUIHeight", s_singleUIHeight);

			HJ_CatchMapPlainSetVal(param, std::string, "ImageUrl", s_imageUrl);

			if (s_configGraphType == HJRteGraphConstructorType_ImageSeq)
			{
				HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlImgSeq, s_imageSeq);
			}

			i_err = m_renderCvt->init(param);
			if (i_err < 0)
			{
				break;
			}

			std::string oConfig = "";
			HJ_CatchMapPlainGetVal(param, std::string, "graphConfigInfo", oConfig);
			if (!oConfig.empty())
			{
				m_jsonConfig = oConfig;
			}
		}
	} while (false);
	return i_err;
}

int HJUIItemPlayerCom::renderEveryEnd()
{
	return 0;
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

			std::string url = "";// WorkDirResource + s_splitSreenGiftUrl;
			if (s_configGraphType == HJRteGraphConstructorType_SplictScreen)
			{
				url = WorkDirResource + s_splitSreenGiftUrl;
			}
			else
			{
				url = s_playerUrl;
			}

			HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(url); //("E:/video/special_video/H264-265-264_RES.flv");// 
			HJ_CatchMapSetVal(param, HJMediaUrl::Ptr, mediaUrl);
			HJ_CatchMapSetVal(param, HJBaseNotify, (HJBaseNotify)notifyFun);
			(*param)["HJDeviceType"] = (int)HJPlayerVideoCodecType_SoftDefault;
			m_player->init(param);
		}

		//HJSleep(1000);
		priTryGetParam();

		HJTransferMediaData::Ptr data = m_asyncCacheRender.acquire();
		if (data)
		{
			if (!m_mediaDataDraw)
			{
				m_mediaDataDraw = HJMediaDataDraw::Create();
			}
			m_mediaDataDraw->update(data);
			// Place DrawImg to the right side of PlayerParams on appear.
			// Fallback to viewport work area if PlayerParams info unavailable.
			{
				ImVec2 targetPos;
				if (m_hasParamsInfo)
				{
					const float gap = 10.0f; // small horizontal gap
					targetPos = ImVec2(m_paramsPos.x + m_paramsSize.x + gap, m_paramsPos.y);
				}
				else
				{
					ImGuiViewport* vp = ImGui::GetMainViewport();
					targetPos = ImVec2(0.0f, ImGui::GetFrameHeight());
					if (vp)
					{
						targetPos = vp->WorkPos.y > 0.0f ? vp->WorkPos : ImVec2(vp->Pos.x, vp->Pos.y + ImGui::GetFrameHeight());
					}
				}
				ImGui::SetNextWindowPos(targetPos, ImGuiCond_Appearing);
			}
			ImGui::Begin("DrawImg");
			ImGui::SetWindowSize(ImVec2(data->getWidth(), data->getHeight()));
			ImGui::Image((void*)(intptr_t)m_mediaDataDraw->getTextureId(), ImVec2(data->getWidth(), data->getHeight()));
			ImGui::End();

			m_asyncCacheRender.recovery(data);
		}
	} while (false);
	return i_err;
}

void HJUIItemPlayerCom::priDone()
{
	if (m_player)
	{
		m_player->done();
		m_player = nullptr;
	}

	m_state = HJUIItemState_idle;
}

NS_HJ_END



