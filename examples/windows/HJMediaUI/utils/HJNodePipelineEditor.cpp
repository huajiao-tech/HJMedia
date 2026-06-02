#include "HJNodePipelineEditor.h"
#include "HJFLog.h"
#include "HJComUtils.h"
#include "VisualDataBase.h"
#include <chrono>

NS_HJ_BEGIN

std::string HJNodePipelineEditor::s_imgSeq = "E:/code/git/hjmedia/examples/resource/imgseq/sing";
std::string HJNodePipelineEditor::s_pngSeq = "E:/video/gift/pngseq/ShuangDanCaiShen";

namespace
{
	const char* kBridgeVideoUrl = "e:/video/comparevid/204637931-720p.mp4";
	const char* kBridgeCameraUrl = "OBS Virtual Camera";
	const char* kBridgeImgSeqUrl = "E:/code/git/hjmedia/examples/resource/imgseq/sing";
	const char* kBridgeImageUrl = "E:/code/git/hjmedia/examples/resource/image/play.jpg";
	const char* kSplitScreenMediaDefaultUrl = "E:/video/gift/ShuangDanCaiShen.mp4";//"PK_ZUOJIA.mp4";
	const char* kFaceuDefaultUrl = "E:/code/git/hjmedia/examples/resource/faceu/60031_10";
	const char* kFaceuDefaultDependsOn = "HJNodeClass_SourceBridgeMediaData";

	bool IsBridgeVisualSourceClass(const std::string& classStyle)
	{
		return classStyle == "HJNodeClass_SourceBridgeMediaData" || classStyle == "HJNodeClass_SplitScreenMediaData";
	}

	const char* BridgeMediaTypeToString(BridgeMediaType type)
	{
		switch (type)
		{
		case BridgeMediaType::Video:
			return "video";
		case BridgeMediaType::Camera:
			return "camera";
		case BridgeMediaType::ImgSeq:
			return "imgseq";
		case BridgeMediaType::Image:
			return "image";
		default:
			return "video";
		}
	}

	BridgeMediaType BridgeMediaTypeFromString(const std::string& value)
	{
		if (value == "camera")
			return BridgeMediaType::Camera;
		if (value == "imgseq")
			return BridgeMediaType::ImgSeq;
		if (value == "image")
			return BridgeMediaType::Image;
		return BridgeMediaType::Video;
	}

	const char* BridgeMediaTypeToDefaultUrl(BridgeMediaType type)
	{
		switch (type)
		{
		case BridgeMediaType::Video:
			return kBridgeVideoUrl;
		case BridgeMediaType::Camera:
			return kBridgeCameraUrl;
		case BridgeMediaType::ImgSeq:
			return kBridgeImgSeqUrl;
		case BridgeMediaType::Image:
			return kBridgeImageUrl;
		default:
			return kBridgeVideoUrl;
		}
	}

	VisualDataType BridgeMediaTypeToVisualDataType(BridgeMediaType type)
	{
		switch (type)
		{
		case BridgeMediaType::Video:
			return VisualDataType::VisualDataType_Video;
		case BridgeMediaType::Camera:
			return VisualDataType::VisualDataType_Camera;
		case BridgeMediaType::ImgSeq:
			return VisualDataType::VisualDataType_ImgSeq;
		case BridgeMediaType::Image:
			return VisualDataType::VisualDataType_Image;
		default:
			return VisualDataType::VisualDataType_Video;
		}
	}

	void ApplyBridgeDefaults(NodeData& data, BridgeMediaType type)
	{
		data.bridgeMediaType = type;
		data.captureUrl = BridgeMediaTypeToDefaultUrl(type);
		if (data.fps <= 0)
			data.fps = 30;
		if (type == BridgeMediaType::Camera)
		{
			if (data.width <= 0)
				data.width = 720;
			if (data.height <= 0)
				data.height = 1280;
		}
	}

	std::shared_ptr<VisualDataBase> CreateBridgeVisualData(const NodeData& data)
	{
		auto visualType = BridgeMediaTypeToVisualDataType(data.bridgeMediaType);
		auto visualData = VisualDataBase::createVisualData(visualType);
		if (!visualData)
			return nullptr;

		HJBaseParam::Ptr param = HJBaseParam::Create();
		const std::string& mediaUrl =
			(data.classStyle == "HJNodeClass_SourceBridgeMediaData" || data.classStyle == "HJNodeClass_SplitScreenMediaData")
			? data.captureUrl
			: data.resourceUrl;
		HJ_CatchMapPlainSetVal(param, std::string, VisualDataBase::s_paramUrl, mediaUrl);
		HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramFps, data.fps);
		if (data.bridgeMediaType == BridgeMediaType::Camera)
		{
			HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramWidth, data.width);
			HJ_CatchMapPlainSetVal(param, int, VisualDataBase::s_paramHeight, data.height);
		}
		visualData->init(param);
		return visualData;
	}
}

void HJNodePipelineEditor::OnStart()
{
	ed::Config config;
	config.SettingsFile = "PipelineEditor.json";
	m_Context = ed::CreateEditor(&config);

	BuildDemoNodes();
}

void HJNodePipelineEditor::OnStop()
{
	ed::DestroyEditor(m_Context);
}

void HJNodePipelineEditor::OnFrame()
{
	auto& io = ImGui::GetIO();

#if 0
	ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
	ImGui::Separator();
#endif

#if 0
	if (!m_Status.empty())
		ImGui::TextUnformatted(m_Status.c_str());

	if (ImGui::Button("Export Pipeline JSON"))
	{
		if (ExportGraph("PipelineGraph.json"))
			m_Status = "Exported to PipelineGraph.json";
		else
			m_Status = "Export failed (see log)";
	}
#endif

	ed::SetCurrentEditor(m_Context);

	ImGui::Begin("Pipeline Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::BeginChild("Palette", ImVec2(260, 0), true);
	DrawPalette();
	ImGui::EndChild();

	ImGui::SameLine();

	ed::Begin("Node Canvas", ImVec2(0.0f, 0.0f));

	DrawNodes();
	DrawLinks();
	HandleCreate();
	HandleDelete();
	HandleLinkContextDelete();

	if (m_FirstFrame)
	{
		for (auto& node : m_Nodes)
			ed::SetNodePosition(node.nodeId, node.position);
		//ed::NavigateToContent();
	}

	ed::End();

	ImGui::SameLine();
	ImGui::BeginChild("Reference", ImVec2(260, 0), true);
	DrawReferenceList();
	ImGui::EndChild();

	//DrawDuplicatePopup();

	ImGui::End();
	ed::SetCurrentEditor(nullptr);

	m_FirstFrame = false;
}

void HJNodePipelineEditor::forEachBridgeNode(const std::function<void(const NodeData&, const std::shared_ptr<VisualDataBase>&)>& cb) const
{
	if (!cb)
		return;
	for (const auto& node : m_Nodes)
	{
		if (IsBridgeVisualSourceClass(node.data.classStyle))
			cb(node.data, node.bridgeVisualData);
	}
}

std::shared_ptr<VisualDataBase> HJNodePipelineEditor::getBridgeVisualData(const std::string& classStyle, const std::string& insName) const
{
	for (const auto& node : m_Nodes)
	{
		if (node.data.classStyle == classStyle && node.data.insName == insName)
			return node.bridgeVisualData;
	}
	return nullptr;
}

void HJNodePipelineEditor::forEachNode(const std::function<void(const NodeData&)>& cb) const
{
	if (!cb)
		return;
	for (const auto& node : m_Nodes)
	{
		if (node.data.role != NodeRole::Link)
			cb(node.data);
	}
}



int HJNodePipelineEditor::NextId() { return m_NextId++; }

int HJNodePipelineEditor::CountNodesByRole(NodeRole role) const
{
	int count = 0;
	for (auto& node : m_Nodes)
		if (node.data.role == role)
			++count;
	return count;
}

bool HJNodePipelineEditor::HasMainSource() const
{
	for (auto& node : m_Nodes)
		if (node.data.role == NodeRole::Source && node.data.isMainSource)
			return true;
	return false;
}

ImVec2 HJNodePipelineEditor::SuggestPosition(NodeRole role, int roleIndex) const
{
	float baseX = role == NodeRole::Source ? -800.0f : role == NodeRole::Filter ? -460.0f : role == NodeRole::Link ? -280.0f : -160.0f;
	float baseY = 120.0f * roleIndex;
	return ImVec2(baseX, baseY);
}

void HJNodePipelineEditor::AddNode(const NodeData& inData)
{
	NodeData data = inData;
	if (data.classStyle.empty())
		return;

	if (data.insName.empty())
		data.insName = data.classStyle;
	if (data.classStyle == "HJNodeClass_SourceBridgeMediaData" && data.captureUrl.empty())
		data.captureUrl = BridgeMediaTypeToDefaultUrl(data.bridgeMediaType);
	if (data.classStyle == "HJNodeClass_SourceBridgeMediaData")
		data.resourceUrl.clear();
	if (data.classStyle == "HJNodeClass_SplitScreenMediaData" && data.captureUrl.empty())
		data.captureUrl = kSplitScreenMediaDefaultUrl;
	if (data.classStyle == "HJNodeClass_SourceFaceu" && data.resourceUrl.empty())
		data.resourceUrl = kFaceuDefaultUrl;
	if (data.classStyle == "HJNodeClass_SourceFaceu" && data.dependsOn.empty())
		data.dependsOn = kFaceuDefaultDependsOn;
	if (data.classStyle == "HJNodeClass_SplitScreenMediaData")
	{
		data.resourceUrl.clear();
		data.bridgeFaceDetect = false;
	}

	if (data.role == NodeRole::Source && data.isMainSource == false && !HasMainSource())
		data.isMainSource = true;
	if (data.role == NodeRole::Link)
		data.enable = std::nullopt;
	else if (!data.enable.has_value())
		data.enable = true;

	const bool hasInput = data.role != NodeRole::Source;
	const bool hasOutput = data.role != NodeRole::Target;

	auto isDuplicateNode = [this](const std::string& classStyle, const std::string& insName) -> bool
	{
		for (const auto& node : m_Nodes)
		{
			if (node.data.classStyle == classStyle && node.data.insName == insName)
				return true;
		}
		return false;
	};
	if (isDuplicateNode(data.classStyle, data.insName))
	{
		const std::string baseName = data.insName;
		int suffix = 1;
		std::string candidate = baseName + std::to_string(suffix);
		while (isDuplicateNode(data.classStyle, candidate))
		{
			++suffix;
			candidate = baseName + std::to_string(suffix);
		}
		data.insName = candidate;
	}

	const int roleIndex = CountNodesByRole(data.role);
	bool inPin = hasInput;
	bool outPin = hasOutput;
	if (data.role == NodeRole::Link)
	{
		inPin = true;
		outPin = true;
	}
	auto node = MakeNode(data, inPin, outPin, SuggestPosition(data.role, roleIndex));
	if (IsBridgeVisualSourceClass(data.classStyle))
		node.bridgeVisualData = CreateBridgeVisualData(data);
	if (data.role == NodeRole::Link && node.link.viewport.linkId.empty())
		node.link.viewport.linkId = "linkid_" + std::to_string(NextId());

	m_Nodes.push_back(node);

	if (m_Context)
		ed::SetNodePosition(node.nodeId, node.position);

	if (m_onNodeCreated)
	{
		m_onNodeCreated(node.data);
	}
}

void HJNodePipelineEditor::AddNode(NodeRole role, const char* classStyle)
{
	NodeData data;
	data.classStyle = classStyle;
	data.insName = classStyle;
	data.role = role;
	data.enable = role == NodeRole::Link ? std::optional<bool>() : std::optional<bool>(true);
	data.isMainSource = role == NodeRole::Source && !HasMainSource();
	AddNode(data);
}

NodeSpec HJNodePipelineEditor::MakeNode(const NodeData& data, bool hasInput, bool hasOutput, ImVec2 pos)
{
	NodeSpec node;
	node.nodeId = ed::NodeId(NextId());
	node.data = data;
	node.hasInput = hasInput;
	node.hasOutput = hasOutput;
	node.position = pos;

	if (hasInput)
	{
		node.inputPin.id = ed::PinId(NextId());
		node.inputPin.kind = ed::PinKind::Input;
	}
	if (hasOutput)
	{
		node.outputPin.id = ed::PinId(NextId());
		node.outputPin.kind = ed::PinKind::Output;
	}
	return node;
}

void HJNodePipelineEditor::BuildDemoNodes()
{
	// A minimal graph similar to the JSON provided by the user.
	NodeData source;
	source.classStyle = "HJNodeClass_SourceImageSeq";
	source.insName = "HJNodeClass_SourceImageSeq";
	source.role = NodeRole::Source;
	source.isMainSource = true;
	source.resourceUrl = s_imgSeq;

	NodeData filterCopy;
	filterCopy.classStyle = "HJNodeClass_FilterCopy2D";
	filterCopy.insName = "HJNodeClass_FilterCopy2D";
	filterCopy.role = NodeRole::Filter;

	NodeData target;
	target.classStyle = "HJNodeClass_TargetUI_0";
	target.insName = "HJNodeClass_TargetUI_0";
	target.role = NodeRole::Target;
	NodeData linkNode;
	linkNode.classStyle = "HJLink";
	linkNode.insName = "Link_0";
	linkNode.role = NodeRole::Link;
	linkNode.enable = std::nullopt;
	LinkData linkInfo;
	linkInfo.shaderType = "HJNodeShaderType_Copy2D";
	linkInfo.viewport.linkId = "ui_0_id";
	m_Nodes.clear();
	m_Links.clear();

	m_Nodes.push_back(MakeNode(source, false, true, ImVec2(-460, 50)));
	{
		NodeData linkToFilter;
		linkToFilter.classStyle = "HJLink";
		linkToFilter.insName = "Link_0";
		linkToFilter.role = NodeRole::Link;
		linkToFilter.enable = std::nullopt;
		NodeSpec linkSpec = MakeNode(linkToFilter, true, true, ImVec2(-420, 120));
		linkSpec.link.shaderType = "HJNodeShaderType_Copy2D";
		linkSpec.link.viewport.linkId = "linkid_0";
		m_Nodes.push_back(linkSpec);
	}
	m_Nodes.push_back(MakeNode(filterCopy, true, true, ImVec2(-360, 200)));
	{
		NodeSpec linkSpec = MakeNode(linkNode, true, true, ImVec2(-280, 260));
		linkSpec.link = linkInfo;
		m_Nodes.push_back(linkSpec);
	}
	m_Nodes.push_back(MakeNode(target, true, false, ImVec2(-200, 330)));

	// Pre-wire them.
	m_Links.push_back({ ed::LinkId(NextId()), m_Nodes[0].outputPin.id, m_Nodes[1].inputPin.id });
	m_Links.push_back({ ed::LinkId(NextId()), m_Nodes[1].outputPin.id, m_Nodes[2].inputPin.id });
	m_Links.push_back({ ed::LinkId(NextId()), m_Nodes[2].outputPin.id, m_Nodes[3].inputPin.id });
	m_Links.push_back({ ed::LinkId(NextId()), m_Nodes[3].outputPin.id, m_Nodes[4].inputPin.id });
}

void HJNodePipelineEditor::BeginPendingNode(NodeRole role, const char* classStyle)
{
	m_hasPendingNode = true;
	m_pendingNode = {};
	m_pendingNode.role = role;
	m_pendingNode.classStyle = classStyle ? classStyle : "";
	m_pendingNode.insName = m_pendingNode.classStyle;
	m_pendingNode.enable = role == NodeRole::Link ? std::optional<bool>() : std::optional<bool>(true);
	m_pendingNode.isMainSource = role == NodeRole::Source && !HasMainSource();
	if (m_pendingNode.role == NodeRole::Source)
	{
		if (m_pendingNode.classStyle == "HJNodeClass_SourceBridgeMediaData")
		{
			ApplyBridgeDefaults(m_pendingNode, BridgeMediaType::Video);
		}
		else if (m_pendingNode.classStyle == "HJNodeClass_SplitScreenMediaData")
		{
			m_pendingNode.captureUrl = kSplitScreenMediaDefaultUrl;
			m_pendingNode.bridgeFaceDetect = false;
		}
		else if (m_pendingNode.classStyle == "HJNodeClass_SourcePNGSeq")
			m_pendingNode.resourceUrl = s_pngSeq;
		else if (m_pendingNode.classStyle == "HJNodeClass_SourceImageSeq")
			m_pendingNode.resourceUrl = s_imgSeq;
		else if (m_pendingNode.classStyle == "HJNodeClass_SourceFaceu")
		{
			m_pendingNode.resourceUrl = kFaceuDefaultUrl;
			m_pendingNode.dependsOn = kFaceuDefaultDependsOn;
		}
	}
}

void HJNodePipelineEditor::DrawPendingCreate()
{
	if (!m_hasPendingNode)
		return;

	ImGui::Separator();
	ImGui::Text("New %s", m_pendingNode.classStyle.c_str());
	ImGui::TextUnformatted("insName");
	ImGui::PushItemWidth(-1.0f);
	InputTextField("##pending_insName", m_pendingNode.insName);
	ImGui::PopItemWidth();
	if (m_pendingNode.enable.has_value())
	{
		bool enabled = m_pendingNode.enable.value_or(true);
		ImGui::Checkbox("enable", &enabled);
		m_pendingNode.enable = enabled;
	}
	if (m_pendingNode.role == NodeRole::Source)
	{
		ImGui::SameLine();
		ImGui::Checkbox("Main", &m_pendingNode.isMainSource);
		if (m_pendingNode.classStyle == "HJNodeClass_SourceBridgeMediaData")
		{
			ImGui::TextUnformatted("mediaType");
			if (ImGui::RadioButton("video##pending_bridge_video", m_pendingNode.bridgeMediaType == BridgeMediaType::Video))
				ApplyBridgeDefaults(m_pendingNode, BridgeMediaType::Video);
			ImGui::SameLine();
			if (ImGui::RadioButton("camera##pending_bridge_camera", m_pendingNode.bridgeMediaType == BridgeMediaType::Camera))
				ApplyBridgeDefaults(m_pendingNode, BridgeMediaType::Camera);
			//ImGui::SameLine();
			if (ImGui::RadioButton("imgseq##pending_bridge_imgseq", m_pendingNode.bridgeMediaType == BridgeMediaType::ImgSeq))
				ApplyBridgeDefaults(m_pendingNode, BridgeMediaType::ImgSeq);
			ImGui::SameLine();
			if (ImGui::RadioButton("image##pending_bridge_image", m_pendingNode.bridgeMediaType == BridgeMediaType::Image))
				ApplyBridgeDefaults(m_pendingNode, BridgeMediaType::Image);

			ImGui::TextUnformatted("fps");
			ImGui::PushItemWidth(-1.0f);
			ImGui::InputInt("##pending_bridge_fps", &m_pendingNode.fps);
			ImGui::PopItemWidth();
			if (m_pendingNode.fps < 1)
				m_pendingNode.fps = 1;

			ImGui::TextUnformatted("faceDetect");
			ImGui::Checkbox("##pending_bridge_facedetect", &m_pendingNode.bridgeFaceDetect);

			if (m_pendingNode.bridgeMediaType == BridgeMediaType::Camera)
			{
				ImGui::TextUnformatted("width");
				ImGui::PushItemWidth(-1.0f);
				ImGui::InputInt("##pending_bridge_width", &m_pendingNode.width);
				ImGui::PopItemWidth();
				if (m_pendingNode.width < 1)
					m_pendingNode.width = 1;

				ImGui::TextUnformatted("height");
				ImGui::PushItemWidth(-1.0f);
				ImGui::InputInt("##pending_bridge_height", &m_pendingNode.height);
				ImGui::PopItemWidth();
				if (m_pendingNode.height < 1)
					m_pendingNode.height = 1;
			}
		}
		else if (m_pendingNode.classStyle == "HJNodeClass_SplitScreenMediaData")
		{
			ImGui::TextUnformatted("fps");
			ImGui::PushItemWidth(-1.0f);
			ImGui::InputInt("##pending_split_fps", &m_pendingNode.fps);
			ImGui::PopItemWidth();
			if (m_pendingNode.fps < 1)
				m_pendingNode.fps = 1;
		}
		if (m_pendingNode.classStyle == "HJNodeClass_SourceBridgeMediaData" || m_pendingNode.classStyle == "HJNodeClass_SplitScreenMediaData")
		{
			ImGui::TextUnformatted("captureUrl");
			ImGui::PushItemWidth(-1.0f);
			InputTextField("##pending_captureUrl", m_pendingNode.captureUrl);
			ImGui::PopItemWidth();
		}
		else
		{
			ImGui::TextUnformatted("resourceUrl");
			ImGui::PushItemWidth(-1.0f);
			InputTextField("##pending_resourceUrl", m_pendingNode.resourceUrl);
			ImGui::PopItemWidth();
		}
		if (m_pendingNode.classStyle == "HJNodeClass_SourceFaceu")
		{
			ImGui::TextUnformatted("dependsOn");
			ImGui::PushItemWidth(-1.0f);
			InputTextField("##pending_dependsOn", m_pendingNode.dependsOn);
			ImGui::PopItemWidth();
		}
	}
    else if (m_pendingNode.role == NodeRole::Filter)
    {
        if (m_pendingNode.classStyle == "HJNodeClass_FilterDenoise")
        {
            ImGui::TextUnformatted("denoiseStrength");
            ImGui::PushItemWidth(-1.0f);
            ImGui::SliderFloat("##pending_denoiseStrength", &m_pendingNode.denoiseStrength, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
        }
        else if (m_pendingNode.classStyle == "HJNodeClass_FilterSR")
        {
            ImGui::TextUnformatted("sharpness");
            ImGui::PushItemWidth(-1.0f);
            ImGui::SliderFloat("##pending_sharpness", &m_pendingNode.sharpness, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::TextUnformatted("match");
            ImGui::PushItemWidth(-1.0f);
            ImGui::SliderFloat("##pending_match", &m_pendingNode.match, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
        }
    }
	if (ImGui::Button("Create Pending Node"))
	{
		AddNode(m_pendingNode);
		m_hasPendingNode = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel Pending"))
	{
		m_hasPendingNode = false;
	}
}

//void HJNodePipelineEditor::DrawDuplicatePopup()
//{
//	if (m_showDuplicatePopup)
//	{
//		ImGui::OpenPopup("Duplicate Node");
//		m_showDuplicatePopup = false;
//	}
//	if (ImGui::BeginPopupModal("Duplicate Node", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
//	{
//		ImGui::TextUnformatted(m_duplicateMessage.empty() ? "Duplicate node." : m_duplicateMessage.c_str());
//		if (ImGui::Button("OK"))
//			ImGui::CloseCurrentPopup();
//		ImGui::EndPopup();
//	}
//}

PinLookup HJNodePipelineEditor::FindPin(ed::PinId id)
{
	for (auto& node : m_Nodes)
	{
		if (node.hasInput && node.inputPin.id == id)
			return { &node, false };
		if (node.hasOutput && node.outputPin.id == id)
			return { &node, true };
	}
	return {};
}

NodeSpec* HJNodePipelineEditor::FindNodeByPin(ed::PinId pin, bool& isOutput)
{
	for (auto& node : m_Nodes)
	{
		if (node.hasInput && node.inputPin.id == pin)
		{
			isOutput = false;
			return &node;
		}
		if (node.hasOutput && node.outputPin.id == pin)
		{
			isOutput = true;
			return &node;
		}
	}
	return nullptr;
}

LinkSpec* HJNodePipelineEditor::FindLink(ed::LinkId id)
{
	for (auto& link : m_Links)
	{
		if (link.id == id)
		{
			return &link;
		}
	}
	return nullptr;
}

bool HJNodePipelineEditor::HasLink(ed::PinId from, ed::PinId to) const
{
	for (const auto& link : m_Links)
	{
		if (link.from == from && link.to == to)
			return true;
	}
	return false;
}

void HJNodePipelineEditor::DrawNodes()
{
	std::vector<ed::NodeId> toRemove;

	for (auto& node : m_Nodes)
	{
		ed::BeginNode(node.nodeId);

		ImGui::PushID(static_cast<int>(node.nodeId.Get()));

		if (node.data.role == NodeRole::Link)
		{
			ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), "Link");
			ImGui::SameLine();
			ImGui::Text("%s", node.link.shaderType.c_str());
		}
		else
		{
			ImGui::Text("%s", node.data.classStyle.c_str());
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("X"))
			toRemove.push_back(node.nodeId);

		if (node.data.role != NodeRole::Link && node.data.enable.has_value())
		{
			bool oldEnable = node.data.enable.value_or(true);
			bool curEnable = oldEnable;
			ImGui::Checkbox("enable", &curEnable);
			if (curEnable != oldEnable)
			{
				node.data.enable = curEnable;
				if (m_onNodeEnableChanged)
					m_onNodeEnableChanged(node.data);
			}
		}

		if (node.data.role == NodeRole::Source)
		{
			ImGui::SameLine();
			bool mainSrc = node.data.isMainSource;
			if (ImGui::Checkbox("Main", &mainSrc))
			{
				node.data.isMainSource = mainSrc;
				if (mainSrc)
				{
					for (auto& other : m_Nodes)
						if (&other != &node && other.data.role == NodeRole::Source)
							other.data.isMainSource = false;
				}
			}
		}

		if (node.hasInput)
		{
			ed::BeginPin(node.inputPin.id, ed::PinKind::Input);
			ImGui::Text("In");
			ed::EndPin();
		}

		ImGui::SameLine();

		if (node.hasOutput)
		{
			ed::BeginPin(node.outputPin.id, ed::PinKind::Output);
			ImGui::Text("Out");
			ed::EndPin();
		}

		ImGui::Separator();

		if (node.data.role != NodeRole::Link)
		{
			const bool isSourceBridgeMediaData = node.data.classStyle == "HJNodeClass_SourceBridgeMediaData";
			if (isSourceBridgeMediaData)
			{
				ImGui::TextUnformatted("Double-click to edit");
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					node.sourceBridgeExpanded = !node.sourceBridgeExpanded;
			}

			if (!isSourceBridgeMediaData || node.sourceBridgeExpanded)
			{
				ImGui::TextUnformatted("insName");
				ImGui::PushItemWidth(240.0f);
				InputTextField("##node_insName", node.data.insName);
				ImGui::PopItemWidth();
				if (node.data.role == NodeRole::Source)
				{
					bool mediaUrlChanged = false;
					if (node.data.classStyle == "HJNodeClass_SourceBridgeMediaData" || node.data.classStyle == "HJNodeClass_SplitScreenMediaData")
					{
						ImGui::TextUnformatted("captureUrl");
						ImGui::PushItemWidth(220.0f);
						mediaUrlChanged = InputTextField("##node_captureUrl", node.data.captureUrl);
						ImGui::PopItemWidth();
					}
					else
					{
						ImGui::TextUnformatted("resourceUrl");
						ImGui::PushItemWidth(220.0f);
						mediaUrlChanged = InputTextField("##node_resourceUrl", node.data.resourceUrl);
						ImGui::PopItemWidth();
					}
					if (node.data.classStyle == "HJNodeClass_SourceFaceu")
					{
						ImGui::TextUnformatted("dependsOn");
						ImGui::PushItemWidth(220.0f);
						InputTextField("##node_dependsOn", node.data.dependsOn);
						ImGui::PopItemWidth();
					}

					if (node.data.classStyle == "HJNodeClass_SourceBridgeMediaData")
					{
						bool bridgeChanged = false;
						ImGui::TextUnformatted("mediaType");
						if (ImGui::RadioButton("video", node.data.bridgeMediaType == BridgeMediaType::Video))
						{
							ApplyBridgeDefaults(node.data, BridgeMediaType::Video);
							bridgeChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::RadioButton("camera", node.data.bridgeMediaType == BridgeMediaType::Camera))
						{
							ApplyBridgeDefaults(node.data, BridgeMediaType::Camera);
							bridgeChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::RadioButton("imgseq", node.data.bridgeMediaType == BridgeMediaType::ImgSeq))
						{
							ApplyBridgeDefaults(node.data, BridgeMediaType::ImgSeq);
							bridgeChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::RadioButton("image", node.data.bridgeMediaType == BridgeMediaType::Image))
						{
							ApplyBridgeDefaults(node.data, BridgeMediaType::Image);
							bridgeChanged = true;
						}

						ImGui::TextUnformatted("fps");
						ImGui::PushItemWidth(220.0f);
						bridgeChanged |= ImGui::InputInt("##node_bridge_fps", &node.data.fps);
						ImGui::PopItemWidth();
						if (node.data.fps < 1)
							node.data.fps = 1;

						ImGui::TextUnformatted("faceDetect");
						bridgeChanged |= ImGui::Checkbox("##node_bridge_facedetect", &node.data.bridgeFaceDetect);

						if (node.data.bridgeMediaType == BridgeMediaType::Camera)
						{
							ImGui::TextUnformatted("width");
							ImGui::PushItemWidth(220.0f);
							bridgeChanged |= ImGui::InputInt("##node_bridge_width", &node.data.width);
							ImGui::PopItemWidth();
							if (node.data.width < 1)
								node.data.width = 1;

							ImGui::TextUnformatted("height");
							ImGui::PushItemWidth(220.0f);
							bridgeChanged |= ImGui::InputInt("##node_bridge_height", &node.data.height);
							ImGui::PopItemWidth();
							if (node.data.height < 1)
								node.data.height = 1;
						}

						if (bridgeChanged || mediaUrlChanged)
							node.bridgeVisualData = CreateBridgeVisualData(node.data);
					}
					else if (node.data.classStyle == "HJNodeClass_SplitScreenMediaData")
					{
						bool bridgeChanged = false;
						ImGui::TextUnformatted("fps");
						ImGui::PushItemWidth(220.0f);
						bridgeChanged |= ImGui::InputInt("##node_split_fps", &node.data.fps);
						ImGui::PopItemWidth();
						if (node.data.fps < 1)
							node.data.fps = 1;

						if (bridgeChanged || mediaUrlChanged)
							node.bridgeVisualData = CreateBridgeVisualData(node.data);
					}
				}
                else if (node.data.role == NodeRole::Filter)
                {
                    bool filterParamChanged = false;
                    if (node.data.classStyle == "HJNodeClass_FilterDenoise")
                    {
                        ImGui::TextUnformatted("denoiseStrength");
                        ImGui::PushItemWidth(220.0f);
                        filterParamChanged |= ImGui::SliderFloat("##node_denoiseStrength", &node.data.denoiseStrength, 0.0f, 1.0f, "%.2f");
                        ImGui::PopItemWidth();
                    }
                    else if (node.data.classStyle == "HJNodeClass_FilterSR")
                    {
                        ImGui::TextUnformatted("sharpness");
                        ImGui::PushItemWidth(220.0f);
                        filterParamChanged |= ImGui::SliderFloat("##node_sharpness", &node.data.sharpness, 0.0f, 1.0f, "%.2f");
                        ImGui::PopItemWidth();
                        ImGui::TextUnformatted("match");
                        ImGui::PushItemWidth(220.0f);
                        filterParamChanged |= ImGui::SliderFloat("##node_match", &node.data.match, 0.0f, 1.0f, "%.2f");
                        ImGui::PopItemWidth();
                    }

                    if (filterParamChanged && m_onNodeParamChanged)
                        m_onNodeParamChanged(node.data);
                }
			}
		}
		else
		{
			ImGui::TextUnformatted("Double-click to edit");
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				node.linkExpanded = !node.linkExpanded;
			if (node.linkExpanded)
				DrawLinkNodeEditor(node);
		}
		ImGui::PopID(); // node id

		ed::EndNode();
	}

	for (auto id : toRemove)
		DeleteNode(id);
}

void HJNodePipelineEditor::DrawLinks()
{
	if (m_Links.empty())
		return;

	const int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	m_linkRenderPulses.erase(
		std::remove_if(m_linkRenderPulses.begin(), m_linkRenderPulses.end(),
			[this, nowMs](const LinkRenderPulse& pulse)
			{
				return (nowMs - pulse.lastRenderMs) > m_linkRenderHoldMs;
			}),
		m_linkRenderPulses.end());

	for (size_t i = 0; i < m_Links.size(); ++i)
	{
		auto& link = m_Links[i];

		bool srcPinIsOutput = false;
		NodeSpec* srcNode = FindNodeByPin(link.from, srcPinIsOutput);
		bool dstPinIsOutput = false;
		NodeSpec* dstNode = FindNodeByPin(link.to, dstPinIsOutput);

		std::string logicalLinkId;
		std::string logicalFrom = srcNode ? srcNode->data.insName : "";
		std::string logicalTo = dstNode ? dstNode->data.insName : "";

		if (srcNode && srcNode->data.role == NodeRole::Link)
		{
			logicalLinkId = srcNode->link.viewport.linkId;
			NodeSpec* logicalSrc = nullptr;
			NodeSpec* logicalDst = nullptr;
			if (GetLinkEndpoints(*srcNode, logicalSrc, logicalDst))
			{
				logicalFrom = logicalSrc ? logicalSrc->data.insName : logicalFrom;
				logicalTo = logicalDst ? logicalDst->data.insName : logicalTo;
			}
		}
		else if (dstNode && dstNode->data.role == NodeRole::Link)
		{
			logicalLinkId = dstNode->link.viewport.linkId;
			NodeSpec* logicalSrc = nullptr;
			NodeSpec* logicalDst = nullptr;
			if (GetLinkEndpoints(*dstNode, logicalSrc, logicalDst))
			{
				logicalFrom = logicalSrc ? logicalSrc->data.insName : logicalFrom;
				logicalTo = logicalDst ? logicalDst->data.insName : logicalTo;
			}
		}

		bool isActive = false;
		bool isCopyOnly = false;
		int64_t latestMatchMs = -1;
		for (const auto& pulse : m_linkRenderPulses)
		{
			const bool matchById = !pulse.linkId.empty() && !logicalLinkId.empty() && pulse.linkId == logicalLinkId;
			const bool matchByPair = logicalLinkId.empty() && pulse.linkId.empty() &&
				!pulse.fromInsName.empty() && !pulse.toInsName.empty() &&
				pulse.fromInsName == logicalFrom && pulse.toInsName == logicalTo;
			if (matchById || matchByPair)
			{
				isActive = true;
				isCopyOnly = pulse.bCopyOnly;
				break;
			}
		}

		const ImVec4 drawColor = !isActive
			? ImVec4(0.82f, 0.82f, 0.82f, 0.85f)
			: (isCopyOnly ? ImVec4(0.10f, 0.55f, 1.00f, 1.00f) : ImVec4(1.00f, 0.35f, 0.10f, 1.00f));
		const float thickness = !isActive ? 1.4f : (isCopyOnly ? 5.2f : 1.5f);
		ed::Link(link.id, link.from, link.to, drawColor, thickness);
		if (isActive && !isCopyOnly)
		{
			ed::Flow(link.id, ed::FlowDirection::Forward);
		}
	}
}

void HJNodePipelineEditor::clearRenderedLinks()
{
	m_linkRenderPulses.clear();
}

void HJNodePipelineEditor::notifyLinkRendered(const std::string& linkId, const std::string& fromInsName, const std::string& toInsName, bool i_bOnlyCopy)
{
	const int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
	for (auto& pulse : m_linkRenderPulses)
	{
		const bool sameId = !linkId.empty() && !pulse.linkId.empty() && pulse.linkId == linkId;
		const bool samePair = linkId.empty() && pulse.linkId.empty() &&
			!fromInsName.empty() && !toInsName.empty() &&
			pulse.fromInsName == fromInsName && pulse.toInsName == toInsName;
		if (sameId || samePair)
		{
			pulse.linkId = linkId;
			pulse.fromInsName = fromInsName;
			pulse.toInsName = toInsName;
			pulse.bCopyOnly = i_bOnlyCopy;
			pulse.lastRenderMs = nowMs;
			return;
		}
	}
	LinkRenderPulse pulse;
	pulse.linkId = linkId;
	pulse.fromInsName = fromInsName;
	pulse.toInsName = toInsName;
	pulse.bCopyOnly = i_bOnlyCopy;
	pulse.lastRenderMs = nowMs;
	m_linkRenderPulses.push_back(std::move(pulse));
}

bool HJNodePipelineEditor::CanCreate(const PinLookup& a, const PinLookup& b)
{
	if (!a.node || !b.node || a.node == b.node)
		return false;
	if (a.isOutput == b.isOutput)
		return false;
	return CanConnectNodes(*a.node, *b.node);
}

void HJNodePipelineEditor::HandleCreate()
{
	if (ed::BeginCreate())
	{
		ed::PinId a, b;
		if (ed::QueryNewLink(&a, &b))
		{
			auto pa = FindPin(a);
			auto pb = FindPin(b);

			const bool validPins = pa.node && pb.node && pa.node != pb.node && pa.isOutput != pb.isOutput;
			const bool nonLinkPair = validPins &&
				pa.node->data.role != NodeRole::Link &&
				pb.node->data.role != NodeRole::Link;

			if (nonLinkPair)
			{
				if (ed::AcceptNewItem())
				{
					ed::PinId outputPin = pa.isOutput ? a : b;
					ed::PinId inputPin = pa.isOutput ? b : a;

					bool dummy = false;
					NodeSpec* srcNode = FindNodeByPin(outputPin, dummy);
					NodeSpec* dstNode = FindNodeByPin(inputPin, dummy);
					if (srcNode && dstNode)
					{
						NodeData linkData;
						linkData.classStyle = "HJLink";
						linkData.insName = "Link_" + std::to_string(NextId());
						linkData.role = NodeRole::Link;
						linkData.enable = std::nullopt;
						LinkData linkInfo;
						if (dstNode->data.role == NodeRole::Target)
						{
							const bool splitScreenToTarget = (srcNode->data.classStyle == "HJNodeClass_SplitScreenMediaData");
							linkInfo.shaderType = splitScreenToTarget ? "HJNodeShaderType_SplitScreenLR_2D" : "HJNodeShaderType_Copy2D";
						}
						else
						{
							linkInfo.shaderType = "";
						}
						linkInfo.viewport.linkId = "linkid_" + std::to_string(NextId());

						NodeSpec linkNode = MakeNode(linkData, true, true, SuggestPosition(NodeRole::Link, CountNodesByRole(NodeRole::Link)));
						linkNode.link = linkInfo;
						m_Nodes.push_back(linkNode);
						NodeSpec& newLinkNode = m_Nodes.back();

						m_Links.push_back({ ed::LinkId(NextId()), outputPin, newLinkNode.inputPin.id });
						ed::Link(m_Links.back().id, m_Links.back().from, m_Links.back().to);
						m_Links.push_back({ ed::LinkId(NextId()), newLinkNode.outputPin.id, inputPin });
						ed::Link(m_Links.back().id, m_Links.back().from, m_Links.back().to);

						NotifyLinkReady(newLinkNode);
					}
				}
			}
			else if (CanCreate(pa, pb))
			{
				if (pa.isOutput)
					std::swap(a, b); // ensure a is input, b is output

				if (ed::AcceptNewItem())
				{
					const int linkNumericId = NextId();
					LinkSpec link;
					link.id = ed::LinkId(linkNumericId);
					link.from = b;
					link.to = a;
					bool dummy = false;
					NodeSpec* src = FindNodeByPin(link.from, dummy);
					NodeSpec* dst = FindNodeByPin(link.to, dummy);
					if (src && src->data.role == NodeRole::Link)
						RemoveLinksForPin(src->outputPin.id);
					if (dst && dst->data.role == NodeRole::Link)
						RemoveLinksForPin(dst->inputPin.id);
					m_Links.push_back(link);
					ed::Link(m_Links.back().id, m_Links.back().from, m_Links.back().to);
					if (src && dst)
					{
						if (src->data.role == NodeRole::Link)
							NotifyLinkReady(*src);
						if (dst->data.role == NodeRole::Link)
							NotifyLinkReady(*dst);
					}
				}
			}
		}
	}
	ed::EndCreate();
}

void HJNodePipelineEditor::HandleDelete()
{
	if (ed::BeginDelete())
	{
		ed::LinkId deleted;
		while (ed::QueryDeletedLink(&deleted))
		{
			if (ed::AcceptDeletedItem())
			{
				DeleteLink(deleted);
			}
		}
	}
	ed::EndDelete();
}

void HJNodePipelineEditor::HandleLinkContextDelete()
{
	auto hovered = ed::GetHoveredLink();
	if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		DeleteLink(hovered);
	}
}

void HJNodePipelineEditor::DeleteLink(ed::LinkId id)
{
	LinkSpec* link = FindLink(id);
	if (link)
	{
		bool dummy = false;
		NodeSpec* src = FindNodeByPin(link->from, dummy);
		NodeSpec* dst = FindNodeByPin(link->to, dummy);
		if (src && src->data.role == NodeRole::Link)
			NotifyLinkBroken(*src);
		if (dst && dst->data.role == NodeRole::Link)
			NotifyLinkBroken(*dst);
	}

	m_Links.erase(
		std::remove_if(m_Links.begin(), m_Links.end(),
			[&](const LinkSpec& linkSpec) { return linkSpec.id == id; }),
		m_Links.end());
}

void HJNodePipelineEditor::RemoveLinksForPin(ed::PinId pin)
{
	std::vector<ed::LinkId> toRemove;
	toRemove.reserve(m_Links.size());
	for (const auto& link : m_Links)
	{
		if (link.from == pin || link.to == pin)
			toRemove.push_back(link.id);
	}
	for (auto id : toRemove)
		DeleteLink(id);
}

void HJNodePipelineEditor::DeleteNode(ed::NodeId id)
{
	auto nodeIt = std::find_if(m_Nodes.begin(), m_Nodes.end(), [&](const NodeSpec& n) { return n.nodeId == id; });
	if (nodeIt == m_Nodes.end())
		return;

	std::vector<ed::NodeId> preNodes;
	std::vector<ed::NodeId> postNodes;
	if (m_reconnectOnDelete)
	{
		for (auto& link : m_Links)
		{
			if (nodeIt->hasInput && link.to == nodeIt->inputPin.id)
			{
				bool dummy = false;
				NodeSpec* src = FindNodeByPin(link.from, dummy);
				if (src)
					preNodes.push_back(src->nodeId);
			}
			if (nodeIt->hasOutput && link.from == nodeIt->outputPin.id)
			{
				bool dummy = false;
				NodeSpec* dst = FindNodeByPin(link.to, dummy);
				if (dst)
				{
					postNodes.push_back(dst->nodeId);
				}
			}
		}
	}

	if (nodeIt->data.role == NodeRole::Link)
		NotifyLinkBroken(*nodeIt);

	// remove links attached to this node
	m_Links.erase(
		std::remove_if(m_Links.begin(), m_Links.end(),
			[&](const LinkSpec& link)
			{
				bool hitInput = nodeIt->hasInput && (link.to == nodeIt->inputPin.id);
				bool hitOutput = nodeIt->hasOutput && (link.from == nodeIt->outputPin.id);
				if (hitInput || hitOutput)
				{
					bool dummy = false;
					NodeSpec* other = nullptr;
					if (hitInput)
						other = FindNodeByPin(link.from, dummy);
					else if (hitOutput)
						other = FindNodeByPin(link.to, dummy);
					if (other && other->data.role == NodeRole::Link)
						NotifyLinkBroken(*other);
				}
				return hitInput || hitOutput;
			}),
		m_Links.end());

	if (m_onNodeDeleted && nodeIt->data.role != NodeRole::Link)
		m_onNodeDeleted(nodeIt->data);

	auto findNodeById = [this](ed::NodeId nodeId) -> NodeSpec*
		{
			for (auto& node : m_Nodes)
			{
				if (node.nodeId == nodeId)
					return &node;
			}
			return nullptr;
		};

	m_Nodes.erase(nodeIt);

	if (m_reconnectOnDelete)
	{
		// link-node-only mode: skip automatic reconnect
	}
}

bool HJNodePipelineEditor::InputTextField(const char* label, std::string& value)
{
	char buffer[256];
	std::strncpy(buffer, value.c_str(), sizeof(buffer));
	buffer[sizeof(buffer) - 1] = '\0';

	const bool changed = ImGui::InputText(label, buffer, sizeof(buffer));
	if (changed)
		value = buffer;

	return changed;
}

void HJNodePipelineEditor::DrawLinkNodeEditor(NodeSpec& node)
{
	bool changed = false;
	ImGui::PushItemWidth(160.0f);
	changed |= InputTextField("renderMode", node.link.viewport.renderMode);
	changed |= InputTextField("linkId", node.link.viewport.linkId);
	ImGui::PopItemWidth();

	ImGui::PushItemWidth(120.0f);
	changed |= ImGui::InputFloat("x", &node.link.viewport.x, 0.1f, 1.0f, "%.3f");
	changed |= ImGui::InputFloat("y", &node.link.viewport.y, 0.1f, 1.0f, "%.3f");
	changed |= ImGui::InputFloat("w", &node.link.viewport.w, 0.1f, 1.0f, "%.3f");
	changed |= ImGui::InputFloat("h", &node.link.viewport.h, 0.1f, 1.0f, "%.3f");
	ImGui::PopItemWidth();
	changed |= ImGui::Checkbox("xMirror", &node.link.viewport.xMirror);
	ImGui::SameLine();
	changed |= ImGui::Checkbox("yMirror", &node.link.viewport.yMirror);

	if (changed)
		NotifyLinkReady(node);
}

void HJNodePipelineEditor::DrawReferenceList()
{
	static const char* shaderList =
		"HJNodeShaderType_Copy2D\n"
		"HJNodeShaderType_CopyOES\n"
		"HJNodeShaderType_PreMul_Copy2D\n"
		"HJNodeShaderType_PreMul_CopyOES\n"
		"HJNodeShaderType_SplitScreenLR_2D\n"
		"HJNodeShaderType_SplitScreenLR_OES";

	static const char* renderList =
		"HJNodeRenderType_Clip\n"
		"HJNodeRenderType_Fit\n"
		"HJNodeRenderType_Origin\n"
		"HJNodeRenderType_Full";

	ImGui::TextUnformatted("Shader Types");
	ImGui::InputTextMultiline("##shaderlist", const_cast<char*>(shaderList), std::strlen(shaderList) + 1, ImVec2(-1, 120), ImGuiInputTextFlags_ReadOnly);

	ImGui::Separator();

	ImGui::TextUnformatted("Render Modes");
	ImGui::InputTextMultiline("##renderlist", const_cast<char*>(renderList), std::strlen(renderList) + 1, ImVec2(-1, 100), ImGuiInputTextFlags_ReadOnly);
}


void HJNodePipelineEditor::DrawPaletteSection(const char* title, NodeRole role, const std::vector<const char*>& classes)
{
	ImGui::TextUnformatted(title);
	ImGui::Separator();
	for (auto* cls : classes)
	{
		if (ImGui::Button(cls, ImVec2(-1, 0)))
			BeginPendingNode(role, cls);
	}
}

void HJNodePipelineEditor::DrawPalette()
{
	static const std::vector<const char*> sourceClasses =
	{
		//"HJNodeClass_SourceBridge",
		"HJNodeClass_SourceBridgeMediaData",
		//"HJNodeClass_SplitScreen",
		"HJNodeClass_SplitScreenMediaData",
		"HJNodeClass_SourcePNGSeq",
		"HJNodeClass_SourceFaceu",
		"HJNodeClass_SourceImage",
		"HJNodeClass_SourceImageSeq"
	};

	static const std::vector<const char*> filterClasses =
	{
		"HJNodeClass_CustomSourceFilter",
		"HJNodeClass_FilterBlur",
		"HJNodeClass_FilterDenoise",
		"HJNodeClass_FilterGray",
		"HJNodeClass_FilterSR",
		"HJNodeClass_FilterCopy2D",
		"HJNodeClass_FilterCopyOES"
	};

	static const std::vector<const char*> targetClasses =
	{
		"HJNodeClass_TargetUI_0",
		//"HJNodeClass_TargetUI_1",
		//"HJNodeClass_TargetUI_2",
		//"HJNodeClass_TargetUI_3",
		//"HJNodeClass_TargetEncoder",
		//"HJNodeClass_TargetImgReceiver",
		//"HJNodeClass_TargetPBODetect",
		"HJNodeClass_TargetPBO_0",
		"HJNodeClass_TargetPBO_1",
		//"HJNodeClass_TargetPBO_2",
		//"HJNodeClass_TargetPBO_3"
	};

	static const std::vector<const char*> linkShaders =
	{
		"",
		"HJNodeShaderType_Copy2D",
		"HJNodeShaderType_CopyOES",
		"HJNodeShaderType_PreMul_Copy2D",
		//"HJNodeShaderType_PreMul_CopyOES",
		"HJNodeShaderType_SplitScreenLR_2D",
		//"HJNodeShaderType_SplitScreenLR_OES"
	};

	DrawPaletteSection("Sources", NodeRole::Source, sourceClasses);
	ImGui::Spacing();
	DrawPaletteSection("Filters", NodeRole::Filter, filterClasses);
	ImGui::Spacing();
	DrawPaletteSection("Targets", NodeRole::Target, targetClasses);
	ImGui::Spacing();
	ImGui::TextUnformatted("Links");
	ImGui::Separator();
	for (auto* shader : linkShaders)
	{
		const char* label = (shader && shader[0] != '\0') ? shader : "<empty>##shader_empty";
		ImGui::Selectable(label, false);
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			NodeData data;
			data.classStyle = "HJLink";
			data.insName = "Link_" + std::to_string(NextId());
			data.role = NodeRole::Link;
			data.enable = std::nullopt;
			NodeSpec node = MakeNode(data, true, true, SuggestPosition(NodeRole::Link, CountNodesByRole(NodeRole::Link)));
			node.link.shaderType = shader;
			node.link.viewport.linkId = "linkid_" + std::to_string(NextId());
			m_Nodes.push_back(node);
			if (m_Context)
				ed::SetNodePosition(node.nodeId, node.position);
			if (m_onNodeCreated)
				m_onNodeCreated(node.data);
		}
	}
	DrawPendingCreate();
}

crude_json::value HJNodePipelineEditor::BuildGraphJson()
{
	crude_json::value root;
	crude_json::array nodes;
	for (auto& node : m_Nodes)
	{
		if (node.data.role == NodeRole::Link)
			continue;
		crude_json::object obj;
		obj["classStyle"] = node.data.classStyle;
		obj["insName"] = node.data.insName;
		obj["role"] = node.data.role == NodeRole::Source ? "HJNodeRole_Source" : node.data.role == NodeRole::Filter ? "HJNodeRole_Filter" : "HJNodeRole_Target";
		obj["enable"] = node.data.enable.value_or(true);
		obj["isMainSource"] = node.data.isMainSource;
		obj["resourceUrl"] = node.data.resourceUrl;
		obj["dependsOn"] = node.data.dependsOn;
        if (node.data.classStyle == "HJNodeClass_FilterDenoise")
            obj["denoiseStrength"] = node.data.denoiseStrength;
        if (node.data.classStyle == "HJNodeClass_FilterSR")
        {
            obj["sharpness"] = node.data.sharpness;
            obj["match"] = node.data.match;
        }
		nodes.push_back(obj);
	}

	crude_json::array links;
	for (auto& node : m_Nodes)
	{
		if (node.data.role != NodeRole::Link)
			continue;
		NodeSpec* src = nullptr;
		NodeSpec* dst = nullptr;
		if (!GetLinkEndpoints(node, src, dst))
			continue;

		crude_json::object linkObj;
		linkObj["from"] = src->data.insName;
		linkObj["to"] = dst->data.insName;
		linkObj["shaderType"] = node.link.shaderType;

		crude_json::object linkInfo;
		linkInfo["renderMode"] = node.link.viewport.renderMode;
		linkInfo["x"] = node.link.viewport.x;
		linkInfo["y"] = node.link.viewport.y;
		linkInfo["w"] = node.link.viewport.w;
		linkInfo["h"] = node.link.viewport.h;
		linkInfo["xMirror"] = node.link.viewport.xMirror;
		linkInfo["yMirror"] = node.link.viewport.yMirror;
		linkInfo["linkId"] = node.link.viewport.linkId;
		linkObj["link"] = linkInfo;

		links.push_back(linkObj);
	}

	root["nodes"] = nodes;
	root["links"] = links;
	return root;
}

bool HJNodePipelineEditor::ExportGraphToString(std::string& json)
{
	auto root = BuildGraphJson();
	json = root.dump(4);
	return true;
}

bool HJNodePipelineEditor::ExportGraph(const char* path)
{
	auto root = BuildGraphJson();

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;
	file << root.dump(4);
	return true;
}

void HJNodePipelineEditor::ClearGraph()
{
	m_Nodes.clear();
	m_Links.clear();
	m_NextId = 1;
	m_FirstFrame = true;
}

NodeRole HJNodePipelineEditor::ParseRole(const std::string& roleStr) const
{
	if (roleStr == "HJNodeRole_Target")
		return NodeRole::Target;
	if (roleStr == "HJNodeRole_Filter")
		return NodeRole::Filter;
	return NodeRole::Source;
}

LinkViewport HJNodePipelineEditor::ParseViewport(const crude_json::value& linkJson) const
{
	LinkViewport viewport;
	if (!linkJson.is_object())
		return viewport;

	if (linkJson["renderMode"].is_string())
		viewport.renderMode = linkJson["renderMode"].get<std::string>();

	if (linkJson["x"].is_number()) viewport.x = static_cast<float>(linkJson["x"].get<double>());
	if (linkJson["y"].is_number()) viewport.y = static_cast<float>(linkJson["y"].get<double>());
	if (linkJson["w"].is_number()) viewport.w = static_cast<float>(linkJson["w"].get<double>());
	if (linkJson["h"].is_number()) viewport.h = static_cast<float>(linkJson["h"].get<double>());

	if (linkJson["xMirror"].is_boolean()) viewport.xMirror = linkJson["xMirror"].get<bool>();
	if (linkJson["yMirror"].is_boolean()) viewport.yMirror = linkJson["yMirror"].get<bool>();
	if (linkJson["linkId"].is_string()) viewport.linkId = linkJson["linkId"].get<std::string>();

	return viewport;
}

void HJNodePipelineEditor::BuildGraphFromJson(const crude_json::value& root)
{
	if (!root.is_object() || !root["nodes"].is_array())
		return;

	auto nodes = root["nodes"].get<crude_json::array>();
	for (const auto& nodeJson : nodes)
	{
		if (!nodeJson.is_object())
			continue;

		NodeData data;
		if (nodeJson["classStyle"].is_string())
			data.classStyle = nodeJson["classStyle"].get<std::string>();
		if (data.classStyle.empty())
			data.classStyle = "UnknownNode";

		if (nodeJson["insName"].is_string())
			data.insName = nodeJson["insName"].get<std::string>();
		if (data.insName.empty())
			data.insName = data.classStyle;

		if (nodeJson["role"].is_string())
			data.role = ParseRole(nodeJson["role"].get<std::string>());
		else
			data.role = NodeRole::Source;

		if (nodeJson["enable"].is_boolean())
			data.enable = nodeJson["enable"].get<bool>();
		else
			data.enable = true;

		if (nodeJson["isMainSource"].is_boolean())
			data.isMainSource = nodeJson["isMainSource"].get<bool>();

		if (nodeJson.contains("resourceUrl"))
		{
			if (nodeJson["resourceUrl"].is_string())
				data.resourceUrl = nodeJson["resourceUrl"].get<std::string>();
		}
		if (nodeJson.contains("dependsOn"))
		{
			if (nodeJson["dependsOn"].is_string())
				data.dependsOn = nodeJson["dependsOn"].get<std::string>();
		}
        if (nodeJson.contains("denoiseStrength") && nodeJson["denoiseStrength"].is_number())
            data.denoiseStrength = static_cast<float>(nodeJson["denoiseStrength"].get<double>());
        if (nodeJson.contains("sharpness") && nodeJson["sharpness"].is_number())
            data.sharpness = static_cast<float>(nodeJson["sharpness"].get<double>());
        if (nodeJson.contains("match") && nodeJson["match"].is_number())
            data.match = static_cast<float>(nodeJson["match"].get<double>());

		if (data.classStyle == "HJNodeClass_SourceBridgeMediaData")
		{
			data.resourceUrl.clear();
			if (data.captureUrl.empty())
				data.captureUrl = BridgeMediaTypeToDefaultUrl(data.bridgeMediaType);
		}
		else if (data.classStyle == "HJNodeClass_SplitScreenMediaData")
		{
			data.resourceUrl.clear();
			if (data.captureUrl.empty())
				data.captureUrl = kSplitScreenMediaDefaultUrl;
			data.bridgeFaceDetect = false;
		}
		else if (data.classStyle == "HJNodeClass_SourceFaceu")
		{
			if (data.resourceUrl.empty())
				data.resourceUrl = kFaceuDefaultUrl;
			if (data.dependsOn.empty())
				data.dependsOn = kFaceuDefaultDependsOn;
		}

		const bool hasInput = data.role != NodeRole::Source;
		const bool hasOutput = data.role != NodeRole::Target;
		const ImVec2 pos = SuggestPosition(data.role, CountNodesByRole(data.role));
		NodeSpec node = MakeNode(data, hasInput, hasOutput, pos);
		if (IsBridgeVisualSourceClass(data.classStyle))
			node.bridgeVisualData = CreateBridgeVisualData(data);
		m_Nodes.push_back(node);
	}

	if (!root["links"].is_array())
		return;

	struct NodePins
	{
		ed::PinId inPin;
		ed::PinId outPin;
		bool hasInput = false;
		bool hasOutput = false;
	};
	std::map<std::string, NodePins> nodeByName;
	for (auto& node : m_Nodes)
	{
		NodePins pins;
		pins.hasInput = node.hasInput;
		pins.hasOutput = node.hasOutput;
		if (node.hasInput)
			pins.inPin = node.inputPin.id;
		if (node.hasOutput)
			pins.outPin = node.outputPin.id;
		nodeByName[node.data.insName] = pins;
	}
	{
		std::string keys;
		for (const auto& it : nodeByName)
		{
			if (!keys.empty())
				keys.append(", ");
			keys.append(it.first);
		}
		HJFLogi("BuildGraphFromJson node keys: {}", keys);
	}

	auto links = root["links"].get<crude_json::array>();
	for (const auto& linkJson : links)
	{
		if (!linkJson.is_object())
			continue;

		std::string from;
		std::string to;
		if (linkJson["from"].is_string())
			from = linkJson["from"].get<std::string>();
		if (linkJson["to"].is_string())
			to = linkJson["to"].get<std::string>();

		auto srcIt = nodeByName.find(from);
		auto dstIt = nodeByName.find(to);
		if (srcIt == nodeByName.end() || dstIt == nodeByName.end())
		{
			HJFLoge("BuildGraphFromJson link skip: from='{}' (found:{}) to='{}' (found:{})",
				from, srcIt != nodeByName.end(), to, dstIt != nodeByName.end());
			continue;
		}
		if (!srcIt->second.hasOutput || !dstIt->second.hasInput)
		{
			HJFLoge("BuildGraphFromJson link skip: from='{}'(out:{}) to='{}'(in:{})",
				from, srcIt->second.hasOutput, to, dstIt->second.hasInput);
			continue;
		}

		NodeData linkNode;
		linkNode.classStyle = "HJLink";
		linkNode.insName = "Link_" + std::to_string(NextId());
		linkNode.role = NodeRole::Link;
		linkNode.enable = std::nullopt;
		NodeSpec linkSpec = MakeNode(linkNode, true, true, SuggestPosition(NodeRole::Link, CountNodesByRole(NodeRole::Link)));
		if (linkJson["shaderType"].is_string())
			linkSpec.link.shaderType = linkJson["shaderType"].get<std::string>();
		if (linkJson["link"].is_object())
			linkSpec.link.viewport = ParseViewport(linkJson["link"]);
		if (linkSpec.link.viewport.linkId.empty())
			linkSpec.link.viewport.linkId = "linkid_" + std::to_string(NextId());

		m_Nodes.push_back(linkSpec);

		m_Links.push_back({ ed::LinkId(NextId()), srcIt->second.outPin, linkSpec.inputPin.id });
		m_Links.push_back({ ed::LinkId(NextId()), linkSpec.outputPin.id, dstIt->second.inPin });
	}
}

bool HJNodePipelineEditor::CanConnectNodes(const NodeSpec& from, const NodeSpec& to) const
{
	if (from.data.role == NodeRole::Link && to.data.role == NodeRole::Link)
		return false;
	return from.data.role == NodeRole::Link || to.data.role == NodeRole::Link;
}

bool HJNodePipelineEditor::GetLinkEndpoints(NodeSpec& linkNode, NodeSpec*& src, NodeSpec*& dst)
{
	if (linkNode.data.role != NodeRole::Link)
		return false;

	src = nullptr;
	dst = nullptr;
	for (auto& link : m_Links)
	{
		if (link.to == linkNode.inputPin.id)
		{
			bool dummy = false;
			src = FindNodeByPin(link.from, dummy);
		}
		if (link.from == linkNode.outputPin.id)
		{
			bool dummy = false;
			dst = FindNodeByPin(link.to, dummy);
		}
	}
	return src && dst;
}

void HJNodePipelineEditor::NotifyLinkReady(NodeSpec& linkNode)
{
	NodeSpec* src = nullptr;
	NodeSpec* dst = nullptr;
	if (!GetLinkEndpoints(linkNode, src, dst))
		return;
	if (m_onLinkCreated)
		m_onLinkCreated(src->data, dst->data, linkNode.link);
	if (m_onLinkInfoChanged)
		m_onLinkInfoChanged(src->data, dst->data, linkNode.link);
}

void HJNodePipelineEditor::NotifyLinkBroken(NodeSpec& linkNode)
{
	NodeSpec* src = nullptr;
	NodeSpec* dst = nullptr;
	if (!GetLinkEndpoints(linkNode, src, dst))
		return;
	if (m_onLinkDeleted)
		m_onLinkDeleted(src->data, dst->data, linkNode.link.viewport.linkId);
}

bool HJNodePipelineEditor::ImportGraph(const char* path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;

	file.seekg(0, std::ios::end);
	const std::streampos size = file.tellg();
	if (size <= 0)
		return false;
	file.seekg(0, std::ios::beg);

	std::string content;
	content.resize(static_cast<size_t>(size));
	file.read(&content[0], content.size());

	crude_json::value root;
	try
	{
		root = crude_json::value::parse(content);
	}
	catch (...)
	{
		return false;
	}

	ClearGraph();
	BuildGraphFromJson(root);

	return true;
}


//int Main(int argc, char** argv)
//{
//    Example example("Pipeline JSON Demo", argc, argv);
//    if (example.Create())
//        return example.Run();
//    return 0;
//}

NS_HJ_END
