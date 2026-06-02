#pragma once

#include "HJUIItemNodeEditor.h"
#include "HJFLog.h"
#include "imgui.h"
#include "implot.h"
#include "HJNodePipelineEditor.h"
#include "HJRenderCvt.h"
#include "HJComUtils.h"
#include "HJUtilitys.h"
#include "HJRteUtils.h"
#include "HJTransferMediaData.h"
#include "VisualDataBase.h"
#include "HJCommonInterface.h"
#include "HJFaceDetectExport.h"
#include "HJFacePointsReal.h"

//#include "HJBaseFaceDetect.h"
#include "libyuv.h"
#include "stb_image.h"
//#define STB_IMAGE_WRITE_STATIC
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include "HJDataConvert.h"
#include "HJInferenceContextExport.h"

NS_HJ_BEGIN

//std::string HJUIItemNodeEditor::s_imageSeq = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/imgseq/sing");
int HJUIItemNodeEditor::s_uiWidth = 720;
int HJUIItemNodeEditor::s_uiHeight = 1280;

//std::string HJUIItemNodeEditor::s_pngGiftSeq = "E:/video/gift/ShuangDanCaiShen";

HJUIItemNodeEditor::HJUIItemNodeEditor()
{
	m_exportJsonPreview.push_back('\0');

	HJEntryContextInfo infoInference;
	infoInference.logIsValid = true;
	infoInference.logDir = "e:/infoInference";
	infoInference.logLevel = HJLOGLevel_INFO;
	infoInference.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
	infoInference.logMaxFileSize = 5 * 1024 * 1024;
	infoInference.logMaxFileNum = 5;
	inferenceContextInit(infoInference);
}

HJUIItemNodeEditor::~HJUIItemNodeEditor()
{
	priDone();
}

void HJUIItemNodeEditor::priApplyNodeParams(const NodeData& node)
{
    if (!m_renderCvt || node.role == NodeRole::Link)
        return;

    HJBaseParam::Ptr param = nullptr;
    if (node.classStyle == "HJNodeClass_FilterDenoise")
    {
        param = HJBaseParam::Create();
        HJ_CatchMapPlainSetVal(param, float, "denoiseStrength", node.denoiseStrength);
    }
    else if (node.classStyle == "HJNodeClass_FilterSR")
    {
        param = HJBaseParam::Create();
        HJ_CatchMapPlainSetVal(param, float, "sharpness", node.sharpness);
        HJ_CatchMapPlainSetVal(param, float, "match", node.match);
    }

    if (param)
        m_renderCvt->nodeSetParam(node.classStyle, node.insName, param);
}

std::shared_ptr<HJFaceDetectWrapper> HJUIItemNodeEditor::getOrCreateFaceDetect(const std::string& insName)
{
	std::lock_guard<std::mutex> lock(m_faceDetectMutex);
	auto it = m_faceDetectStates.find(insName);
	if (it != m_faceDetectStates.end() && it->second.detect)
		return it->second.detect;

	FaceDetectState state;
	state.detect = std::make_shared<::HJFaceDetectWrapper>();
	if (!state.detect)
	{
		HJFLoge("FaceDetect create failed node:{}", insName);
		return nullptr;
	}
	HJFaceDetectWrapperOption option;
	option.ncnnScrfdEqualScale = true;
	option.ncnnScrfdTargetSize = 320;
	option.ncnnScrfdProbThreshold = 0.3f;
	option.ncnnScrfdNmsThreshold = 0.45f;
	option.ncnnScrfdThreadNums = 1;
	option.ncnnScrfdUseGPU = false;

	std::string modelPath = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/model");
	const std::string nodeName = insName;
	uint64_t generation = 0;
	{
		std::lock_guard<std::mutex> lifeLock(m_faceDetectLifeMutex);
		generation = ++m_faceDetectGenerationByIns[insName];
	}
	state.generation = generation;
	int ret = state.detect->init([this, nodeName, generation](const std::string& faceInfo)
		{
			{
				std::lock_guard<std::mutex> lifeLock(m_faceDetectLifeMutex);
				auto itGen = m_faceDetectGenerationByIns.find(nodeName);
				if (itGen == m_faceDetectGenerationByIns.end() || itGen->second != generation)
					return;
			}

			if (m_renderCvt)
			{
				m_renderCvt->setFaceInfo(nodeName, faceInfo);
				HJFacePointsReal faceIns;
				faceIns.init(faceInfo);
				const double elapsedMs = static_cast<double>(faceIns.getElapsedTime());
				{
					std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
					m_faceDetectElapsedMsByIns[nodeName] = elapsedMs;
				}
			}
		},
		HJFaceDetectWrapperType_NCNNSCRFD,
		modelPath,
		option);
	if (ret < 0)
	{
		HJFLoge("FaceDetect init failed node:{} ret:{}", insName, ret);
		return nullptr;
	}

	m_faceDetectStates[insName] = state;
	return state.detect;
}

void HJUIItemNodeEditor::installBridgeCallback(const NodeData& node, const std::shared_ptr<VisualDataBase>& visualData)
{
	if (!visualData || !m_renderCvt)
		return;

	std::shared_ptr<HJFaceDetectWrapper> detectPtr = nullptr;
	if (node.bridgeFaceDetect)
		detectPtr = getOrCreateFaceDetect(node.insName);
	HJBaseParam::Ptr bridgeParam = HJBaseParam::Create();
	std::string nodeName = node.insName;
	HJTransferMediaDataGetCb getCb = [this, visualData, detectPtr, nodeName](HJTransferMediaData::Ptr& outData)
		{
			outData = visualData->acquire();
			if (outData)
			{
				visualData->recovery(outData);
			}
			
			if (!outData)
				return;

			auto input = std::make_shared<HJUnifyWrapperData>();
			const auto type = outData->getConvertType();
			if (type == HJConvertDataType_YUVNV12)
			{
				input->dataType = HJUnifyWrapperDataType_NV12;
				input->data[0] = outData->getData(0);
				input->data[1] = outData->getData(1);
				input->nPitch[0] = outData->getStride(0);
				input->nPitch[1] = outData->getStride(1);
			}
			else if (type == HJConvertDataType_RGBA)
			{
				input->dataType = HJUnifyWrapperDataType_RGBA;
				input->data[0] = outData->getData(0);
				input->nPitch[0] = outData->getStride(0);
			}
			else
			{
				return;
			}
			input->width = outData->getWidth();
			input->height = outData->getHeight();
			input->timeStamp = outData->getTimeStamp();

			if (detectPtr && input->dataType == HJUnifyWrapperDataType_NV12)
			{
				int ret = detectPtr->detect(input, false, true, false, false);
				if (ret < 0)
				{
					return;
				}	
			}
		};
	HJ_CatchMapSetVal(bridgeParam, HJTransferMediaDataGetCb, getCb);
	m_renderCvt->nodeSetParam(node.classStyle, node.insName, bridgeParam);
}

void HJUIItemNodeEditor::enqueueLinkRenderEvent(const std::string& linkId, const std::string& fromInsName, const std::string& toInsName, bool i_bOnlyCopy)
{
	std::lock_guard<std::mutex> lock(m_linkRenderMutex);
	m_pendingLinkRenderEvents.push_back({ linkId, fromInsName, toInsName, i_bOnlyCopy });
}

void HJUIItemNodeEditor::flushLinkRenderEvents()
{
	if (!m_nodePipelineEditor)
		return;

	std::vector<LinkRenderEvent> events;
	{
		std::lock_guard<std::mutex> lock(m_linkRenderMutex);
		if (m_pendingLinkRenderEvents.empty())
			return;
		events.swap(m_pendingLinkRenderEvents);
	}
	for (const auto& e : events)
	{
		m_nodePipelineEditor->notifyLinkRendered(e.linkId, e.fromInsName, e.toInsName, e.bOnlyCopy);
	}
}

void HJUIItemNodeEditor::enqueueFrameStatEvent(int64_t timeMs, double fps, double renderAvgMs)
{
	std::lock_guard<std::mutex> lock(m_frameStatMutex);
	m_pendingFrameStatEvents.push_back({ timeMs, fps, renderAvgMs });
}

void HJUIItemNodeEditor::flushFrameStatEvents()
{
	std::vector<FrameStatEvent> events;
	{
		std::lock_guard<std::mutex> lock(m_frameStatMutex);
		if (m_pendingFrameStatEvents.empty())
			return;
		events.swap(m_pendingFrameStatEvents);
	}
	for (const auto& e : events)
	{
		(void)e.timeMs;
		m_latestRenderFps = e.fps;
		m_latestRenderAvgMs = e.renderAvgMs;
	}
}

void HJUIItemNodeEditor::pushStatSample(double x, double frameMs, double fps)
{
	m_statXs.push_back(x);
	m_statFrameMs.push_back(frameMs);
	m_statFps.push_back(fps);

	{
		std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
		std::unordered_map<std::string, bool> active;
		for (const auto& kv : m_faceDetectElapsedMsByIns)
		{
			active[kv.first] = true;
			auto& series = m_statFaceDetectSeriesByIns[kv.first];
			if (series.size() + 1 < m_statXs.size())
				series.resize(m_statXs.size() - 1, 0.0);
			series.push_back(kv.second);
		}
		for (auto it = m_statFaceDetectSeriesByIns.begin(); it != m_statFaceDetectSeriesByIns.end();)
		{
			if (active.find(it->first) == active.end())
				it = m_statFaceDetectSeriesByIns.erase(it);
			else
				++it;
		}
	}

	if (m_statXs.size() > m_statMaxSamples)
	{
		m_statXs.erase(m_statXs.begin());
		m_statFrameMs.erase(m_statFrameMs.begin());
		m_statFps.erase(m_statFps.begin());
		for (auto& kv : m_statFaceDetectSeriesByIns)
		{
			if (!kv.second.empty())
				kv.second.erase(kv.second.begin());
		}
	}
}

void HJUIItemNodeEditor::drawStatPlot()
{
	if (!m_showStatsPlot)
		return;
	if (m_statXs.empty())
		return;

	ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(4.0f, 4.0f));
	ImPlot::PushStyleVar(ImPlotStyleVar_LegendInnerPadding, ImVec2(3.0f, 2.0f));
	ImPlot::PushStyleVar(ImPlotStyleVar_LegendSpacing, ImVec2(6.0f, 2.0f));
	if (ImPlot::BeginPlot("Render/Detect/FPS", ImVec2(-1, 280.0f)))
	{
		ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
		const int count = static_cast<int>(m_statXs.size());
		const double xMax = m_statXs.back();
		const double xMin = std::max(0.0, xMax - 10.0); // rolling 10s window
		double yMax = 1.0;
		for (int i = 0; i < count; ++i)
		{
			yMax = std::max(yMax, m_statFrameMs[i]);
			yMax = std::max(yMax, m_statFps[i]);
		}
		{
			std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
			for (const auto& kv : m_statFaceDetectSeriesByIns)
			{
				for (const auto v : kv.second)
					yMax = std::max(yMax, v);
			}
		}

		ImPlot::SetupAxes("time(s)", "value", 0, ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxisLimits(ImAxis_X1, xMin, xMax, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, yMax * 1.2, ImGuiCond_Always);
		ImPlot::PlotLine("Frame(ms)", m_statXs.data(), m_statFrameMs.data(), count);
		ImPlot::PlotLine("FPS", m_statXs.data(), m_statFps.data(), count);
		{
			std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
			for (const auto& kv : m_statFaceDetectSeriesByIns)
			{
				const auto& series = kv.second;
				if (series.empty())
					continue;
				const int n = static_cast<int>(std::min(series.size(), m_statXs.size()));
				if (n <= 0)
					continue;
				const int xOffset = static_cast<int>(m_statXs.size()) - n;
				const std::string label = "FaceDetect(ms)-" + kv.first;
				ImPlot::PlotLine(label.c_str(), m_statXs.data() + xOffset, series.data(), n);
			}
		}
		ImPlot::EndPlot();
	}
	ImPlot::PopStyleVar(3);
}

std::string HJUIItemNodeEditor::nodeRoleToString(NodeRole role) const
{
    switch (role)
    {
    case NodeRole::Source: 
		return HJRteGraphConfig::HJNodeRole_Source;
    case NodeRole::Filter: 
		return HJRteGraphConfig::HJNodeRole_Filter;
    case NodeRole::Target:
		return HJRteGraphConfig::HJNodeRole_Target;
    default: 
		return HJRteGraphConfig::HJNodeRole_Source;
    }
}

HJRteJsonLinkInfo HJUIItemNodeEditor::makeLinkInfo(const LinkData& link) const
{
    HJRteJsonLinkInfo linkInfo;
    linkInfo.setRenderMode(link.viewport.renderMode);
    linkInfo.setX(link.viewport.x);
    linkInfo.setY(link.viewport.y);
    linkInfo.setW(link.viewport.w);
    linkInfo.setH(link.viewport.h);
    linkInfo.setXMirror(link.viewport.xMirror);
    linkInfo.setYMirror(link.viewport.yMirror);
    linkInfo.setLinkId(link.viewport.linkId);
    return linkInfo;
}
//void HJUIItemNodeEditor::priSave(unsigned char *i_pY, unsigned char *i_pUV, int w, int h)
//{
//	if (!i_pY || !i_pUV || w <= 0 || h <= 0)
//	{
//		return;
//	}
//
//	std::vector<unsigned char> rgb(w * h * 3);
//	//libyuv::NV12ToRAW to r g b;    libyuv::NV12ToRGB24 to b g r;
//	if (libyuv::NV12ToRAW(i_pY, w, i_pUV, w, rgb.data(), w * 3, w, h) != 0)
//	{
//		HJFLoge("NV12ToRGB24 failed");
//		return;
//	}
//
//	static int s_idx = 0;
//	std::string filename = HJUtilitys::formatString("nv12_dump_%04d.png", s_idx++);
//	std::string filepath = HJUtilitys::concatenatePath(HJUtilitys::exeDir(), filename);
//	if (stbi_write_png(filepath.c_str(), w, h, 3, rgb.data(), w * 3) == 0)
//	{
//		HJFLoge("write png failed {}", filepath);
//	}
//	else
//	{
//		HJFLogi("saved {}", filepath);
//	}
//}
void HJUIItemNodeEditor::priTest()
{
}

int HJUIItemNodeEditor::run()
{
    int i_err = 0;
    do
    {
		priTest();

        if (!m_nodePipelineEditor)
        {
            m_nodePipelineEditor = HJNodePipelineEditor::Create();
            m_nodePipelineEditor->setOnNodeCreated([this](const NodeData& node)
                {
                    if (m_renderCvt)
                    {
                        if (node.role == NodeRole::Link)
                            return;
                        const bool enable = node.enable.value_or(true);
                        const bool isFaceu = node.classStyle == "HJNodeClass_SourceFaceu";
                        const bool createEnable = isFaceu ? false : enable;
                        m_renderCvt->nodeCreate(node.classStyle, node.insName, nodeRoleToString(node.role), createEnable, node.isMainSource, node.resourceUrl, node.dependsOn);
                        priApplyNodeParams(node);
                        if (isFaceu)
                        {
                            m_renderCvt->nodeEnable(node.classStyle, node.insName, enable, node.resourceUrl);
                        }

						if (node.classStyle == "HJNodeClass_SourceBridgeMediaData" || node.classStyle == "HJNodeClass_SplitScreenMediaData")
						{
							auto visualData = m_nodePipelineEditor->getBridgeVisualData(node.classStyle, node.insName);
							installBridgeCallback(node, visualData);
						}
                    }
                });
            m_nodePipelineEditor->setOnNodeDeleted([this](const NodeData& node)
                {
					if (node.role == NodeRole::Link)
						return;
					{
						std::lock_guard<std::mutex> lifeLock(m_faceDetectLifeMutex);
						m_faceDetectGenerationByIns.erase(node.insName);
					}
                    if (m_renderCvt)
                    {
                        m_renderCvt->nodeDelete(node.classStyle, node.insName, m_relinkOnDelete);
                    }
					{
						std::lock_guard<std::mutex> lock(m_faceDetectMutex);
						m_faceDetectStates.erase(node.insName);
					}
					{
						std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
						m_faceDetectElapsedMsByIns.erase(node.insName);
						m_statFaceDetectSeriesByIns.erase(node.insName);
					}
                });
            m_nodePipelineEditor->setOnNodeEnableChanged([this](const NodeData& node)
                {
                    if (m_renderCvt)
                    {
                        if (node.role == NodeRole::Link)
                            return;
                        const bool enable = node.enable.value_or(true);
                        m_renderCvt->nodeEnable(node.classStyle, node.insName, enable, node.resourceUrl);
                    }
                });
            m_nodePipelineEditor->setOnNodeParamChanged([this](const NodeData& node)
                {
                    priApplyNodeParams(node);
                });
            m_nodePipelineEditor->setOnLinkCreated([this](const NodeData& src, const NodeData& dst, const LinkData& link)
                {
                    if (m_renderCvt)
                    {
                        m_renderCvt->nodeConnect(src.classStyle, src.insName, dst.classStyle, dst.insName, link.shaderType, makeLinkInfo(link));
                    }
                });
            m_nodePipelineEditor->setOnLinkDeleted([this](const NodeData& src, const NodeData& dst, const std::string& linkId)
                {
                    if (m_renderCvt)
                    {
                        m_renderCvt->nodeDisconnect(src.classStyle, src.insName, dst.classStyle, dst.insName, linkId);
                    }
                });
            m_nodePipelineEditor->setOnLinkInfoChanged([this](const NodeData& src, const NodeData& dst, const LinkData& link)
                {
                    if (m_renderCvt)
                    {
                        m_renderCvt->nodeLinkInfoChange(src.classStyle, src.insName, dst.classStyle, dst.insName, makeLinkInfo(link));
                    }
                });
            m_nodePipelineEditor->OnStart();
			m_nodePipelineEditor->setRelinkAfterDelete(m_relinkOnDelete);
        }

		if (m_nodePipelineEditor)
		{
			flushLinkRenderEvents();
			flushFrameStatEvents();

			ImGui::Begin("Pipeline Export");

			ImGui::Text("Import:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("##jsonI", m_importPath, sizeof(m_importPath));
			ImGui::SameLine();
			if (ImGui::Button("apply"))
			{
				if (m_nodePipelineEditor->ImportGraph(m_importPath))
				{
					HJFLogi("import success {}", m_importPath);
				}
				else
				{
					HJFLoge("import failed {}", m_importPath);
				}
			}

			ImGui::Text("Export:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("##jsonE", m_exportPath, sizeof(m_exportPath));
			ImGui::SameLine();
			if (ImGui::Button("save"))
			{
				if (m_nodePipelineEditor->ExportGraph(m_exportPath))
				{
					HJFLogi("success {}", m_exportPath);
				}
				else
				{
					HJFLogi("failed {}", m_exportPath);
				}
			}
			if (ImGui::Button("reset"))
			{
				std::string json;
				if (m_nodePipelineEditor->ExportGraphToString(json))
				{
					m_exportJsonPreview.assign(json.begin(), json.end());
					m_exportJsonPreview.push_back('\0');

					m_renderCvt = nullptr;
					m_renderCvt = HJRenderCvt::Create();
					m_latestRenderAvgMs = 0.0;
					m_latestRenderFps = 0.0;
					m_statXs.clear();
					m_statFrameMs.clear();
					m_statFps.clear();
					{
						std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
						m_statFaceDetectSeriesByIns.clear();
					}
					HJBaseParam::Ptr param = HJBaseParam::Create();
					HJ_CatchMapPlainSetVal(param, void*, "ParentWindow", getWindow());
					HJ_CatchMapPlainSetVal(param, bool, "IsUseSingleUI", true);
					HJ_CatchMapPlainSetVal(param, int, "SingleUIWidth", s_uiWidth);
					HJ_CatchMapPlainSetVal(param, int, "SingleUIHeight", s_uiHeight);
					HJ_CatchMapPlainSetVal(param, std::string, "UserConfig", json);
					HJ_CatchMapPlainSetVal(param, bool, "bMainSourceRenderForce", false);
					if (m_renderCvt->init(param) == HJ_OK)
					{
						m_renderCvt->setLinkRenderCallback([this](const std::string& linkId, const std::string& from, const std::string& to, bool i_bOnlyCopy)
							{
								enqueueLinkRenderEvent(linkId, from, to, i_bOnlyCopy);
							});
						m_renderCvt->setFrameStatCallback([this](int64_t timeMs, double fps, double renderAvgMs)
							{
								enqueueFrameStatEvent(timeMs, fps, renderAvgMs);
							});
						m_nodePipelineEditor->forEachBridgeNode([this](const NodeData& node, const std::shared_ptr<VisualDataBase>& visualData)
							{
								installBridgeCallback(node, visualData);
							});
                        m_nodePipelineEditor->forEachNode([this](const NodeData& node)
                            {
                                priApplyNodeParams(node);
                            });
						HJFLogi("success");
					}
					else
					{
						HJFLoge("failed");
					}
				}
				else
				{
					HJFLoge("error");
				}
			}
			ImGui::Checkbox("Relink on delete", &m_relinkOnDelete);
			m_nodePipelineEditor->setRelinkAfterDelete(m_relinkOnDelete);

			if (ImGui::Button(m_showStatsPlot ? "stat(N)" : "stat(Y)"))
				m_showStatsPlot = !m_showStatsPlot;

			ImGui::Separator();
			ImGui::Text("Notes:");
			static char s_faceuNotes[] =
				"1. Faceu can only be used when source is\n"
				"HJNodeClass_SourceBridgeMediaData.\n"
				"Only this source can enable face detection;\n"
				"other sources are not supported.\n"
				"2. If UI viewport is changed, Faceu viewport\n"
				"should be kept consistent; otherwise Faceu\n"
				"may not follow the face correctly.\n"
				"3. If filter is linked to filter, \n"
				"link must use empty, filter has own shader.\n"
				"4. HJRteComSourceBridgeMediaData use texture \n"
				"TextureMatrixYFlip, so Filter chain should insert \n"
				"HJNodeClass_FilterCopy2D after source first,\n"
				"for example before denoise/sr; otherwise\n"
				"denoise disable may cause Y-flip display error.\n";
			ImGui::InputTextMultiline("##faceu_notes", s_faceuNotes, IM_ARRAYSIZE(s_faceuNotes),
				ImVec2(-1.0f, ImGui::GetTextLineHeight() * 7.0f),
				ImGuiInputTextFlags_ReadOnly);

			ImGui::Separator();
			ImGui::Text("Current Graph JSON:");
			ImGui::InputTextMultiline("##pipeline_json_preview",
				m_exportJsonPreview.data(),
				m_exportJsonPreview.size(),
				ImVec2(-1.0f, ImGui::GetTextLineHeight() * 12.0f),
				ImGuiInputTextFlags_ReadOnly);

			ImGui::End();

			if (m_showStatsPlot)
			{
				if (ImGui::Begin("Render Stats", &m_showStatsPlot))
				{
					drawStatPlot();
				}
				ImGui::End();
			}
			m_nodePipelineEditor->OnFrame();
		}
		
	} while (false);
	pushStatSample(ImGui::GetTime(), m_latestRenderAvgMs, m_latestRenderFps);
	return i_err;
}

void HJUIItemNodeEditor::priDone()
{
	if (m_nodePipelineEditor)
	{
		m_nodePipelineEditor->OnStop();
		m_nodePipelineEditor = nullptr;
	}
	m_renderCvt = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_faceDetectMutex);
		m_faceDetectStates.clear();
	}
	{
		std::lock_guard<std::mutex> lifeLock(m_faceDetectLifeMutex);
		m_faceDetectGenerationByIns.clear();
	}
	{
		std::lock_guard<std::mutex> statLock(m_faceDetectStatMutex);
		m_faceDetectElapsedMsByIns.clear();
		m_statFaceDetectSeriesByIns.clear();
	}
	{
		std::lock_guard<std::mutex> lock(m_frameStatMutex);
		m_pendingFrameStatEvents.clear();
	}
	m_latestRenderAvgMs = 0.0;
	m_latestRenderFps = 0.0;
	m_statXs.clear();
	m_statFrameMs.clear();
	m_statFps.clear();
}

NS_HJ_END



