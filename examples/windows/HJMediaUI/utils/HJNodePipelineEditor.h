#pragma once

# include <imgui.h>
# include <imgui_node_editor.h>
# include <crude_json.h>

# include <cstring>
# include <string>
# include <map>
# include <fstream>
# include <cstdint>
# include <vector>
# include <algorithm>
# include <functional>
# include <optional>
# include <memory>
#include "HJPrerequisites.h"

NS_HJ_BEGIN

namespace ed = ax::NodeEditor;

class VisualDataBase;

enum class NodeRole
{
    Source,
    Filter,
    Target,
    Link
};

enum class BridgeMediaType
{
    Video,
    Camera,
    ImgSeq,
    Image
};

struct LinkViewport
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
    bool  xMirror = false;
    bool  yMirror = false;
    std::string renderMode = "HJNodeRenderType_Clip";
    std::string linkId = "";
};

struct LinkData
{
    std::string shaderType = "";
    LinkViewport viewport;
};

struct NodeData
{
    std::string classStyle;
    std::string insName;
    NodeRole    role = NodeRole::Source;
    std::optional<bool> enable = true;
    bool        isMainSource = false;
    std::string resourceUrl;
    std::string captureUrl;
    std::string dependsOn;
    BridgeMediaType bridgeMediaType = BridgeMediaType::Video;
    int         fps = 30;
    int         width = 720;
    int         height = 1280;
    bool        bridgeFaceDetect = true;
    float       denoiseStrength = 1.0f;
    float       sharpness = 1.0f;
    float       match = 1.0f;
};

struct PinSpec
{
    ed::PinId     id;
    ed::PinKind   kind;
};
//
struct NodeSpec
{
    ed::NodeId nodeId;
    bool       hasInput = false;
    bool       hasOutput = false;
    PinSpec    inputPin;
    PinSpec    outputPin;
    NodeData   data;
    LinkData   link;
    std::shared_ptr<VisualDataBase> bridgeVisualData = nullptr;
    bool       sourceBridgeExpanded = false;
    bool       linkExpanded = false;
    ImVec2     position = ImVec2(0, 0);
};
//
struct LinkSpec
{
    ed::LinkId id;
    ed::PinId  from; // output
    ed::PinId  to;   // input
};

struct PinIdLess
{
    bool operator()(const ed::PinId& lhs, const ed::PinId& rhs) const
    {
        return lhs.Get() < rhs.Get();
    }
};


struct PinLookup
{
    NodeSpec* node = nullptr;
    bool      isOutput = false;
};


class HJNodePipelineEditor
{
public:
    HJ_DEFINE_CREATE(HJNodePipelineEditor);

    HJNodePipelineEditor() = default;
    virtual ~HJNodePipelineEditor() = default;

    void OnStart();
    void OnStop();
    void OnFrame();
    bool ExportGraphToString(std::string& json);
    bool ExportGraph(const char* path);
    bool ImportGraph(const char* path);
    // Callbacks for link events. Provide src/dst node data when links are created or deleted.
    void setOnLinkCreated(std::function<void(const NodeData&, const NodeData&, const LinkData&)> cb) { m_onLinkCreated = std::move(cb); }
    void setOnLinkDeleted(std::function<void(const NodeData&, const NodeData&, const std::string&)> cb) { m_onLinkDeleted = std::move(cb); }
    void setOnLinkInfoChanged(std::function<void(const NodeData&, const NodeData&, const LinkData&)> cb) { m_onLinkInfoChanged = std::move(cb); }
    void setOnNodeCreated(std::function<void(const NodeData&)> cb) { m_onNodeCreated = std::move(cb); }
    void setOnNodeDeleted(std::function<void(const NodeData&)> cb) { m_onNodeDeleted = std::move(cb); }
    void setOnNodeEnableChanged(std::function<void(const NodeData&)> cb) { m_onNodeEnableChanged = std::move(cb); }
    void setOnNodeParamChanged(std::function<void(const NodeData&)> cb) { m_onNodeParamChanged = std::move(cb); }
    void setRelinkAfterDelete(bool relink) { m_reconnectOnDelete = relink; }
    void forEachBridgeNode(const std::function<void(const NodeData&, const std::shared_ptr<VisualDataBase>&)>& cb) const;
    void forEachNode(const std::function<void(const NodeData&)>& cb) const;
    std::shared_ptr<VisualDataBase> getBridgeVisualData(const std::string& classStyle, const std::string& insName) const;
    void clearRenderedLinks();
    void notifyLinkRendered(const std::string& linkId, const std::string& fromInsName, const std::string& toInsName, bool i_bOnlyCopy);

private:
    void BeginPendingNode(NodeRole role, const char* classStyle);
    void DrawPendingCreate();
    //void DrawDuplicatePopup();
    void AddNode(const NodeData& data);
    void DrawLinkNodeEditor(NodeSpec& node);

    ed::EditorContext* m_Context = nullptr;
    std::vector<NodeSpec> m_Nodes;
    std::vector<LinkSpec> m_Links;
    int m_NextId = 1;
    bool m_FirstFrame = true;
    struct LinkRenderPulse
    {
        std::string linkId;
        std::string fromInsName;
        std::string toInsName;
        bool bCopyOnly = false;
        int64_t lastRenderMs = 0;
    };
    std::vector<LinkRenderPulse> m_linkRenderPulses;
    int64_t m_linkRenderHoldMs = 4000;
    std::string m_Status;
    bool m_reconnectOnDelete = false;
    bool m_hasPendingNode = false;
    NodeData m_pendingNode;
    //bool m_showDuplicatePopup = false;
    //std::string m_duplicateMessage;

    std::function<void(const NodeData&, const NodeData&, const LinkData&)> m_onLinkCreated = nullptr;
    std::function<void(const NodeData&, const NodeData&, const std::string&)> m_onLinkDeleted = nullptr;
    std::function<void(const NodeData&, const NodeData&, const LinkData&)> m_onLinkInfoChanged = nullptr;
    std::function<void(const NodeData&)> m_onNodeCreated = nullptr;
    std::function<void(const NodeData&)> m_onNodeDeleted = nullptr;
    std::function<void(const NodeData&)> m_onNodeEnableChanged = nullptr;
    std::function<void(const NodeData&)> m_onNodeParamChanged = nullptr;

    int NextId();
    int CountNodesByRole(NodeRole role) const;
    bool HasMainSource() const;
    ImVec2 SuggestPosition(NodeRole role, int roleIndex) const;
    void AddNode(NodeRole role, const char* classStyle);
    NodeSpec MakeNode(const NodeData& data, bool hasInput, bool hasOutput, ImVec2 pos);
    void BuildDemoNodes();
    PinLookup FindPin(ed::PinId id);
    NodeSpec* FindNodeByPin(ed::PinId pin, bool& isOutput);
    LinkSpec* FindLink(ed::LinkId id);
    bool HasLink(ed::PinId from, ed::PinId to) const;
    void DrawNodes();
    void DrawLinks();
    bool CanCreate(const PinLookup& a, const PinLookup& b);
    void HandleCreate();
    void HandleDelete();
    void HandleLinkContextDelete();
    void DeleteLink(ed::LinkId id);
    void RemoveLinksForPin(ed::PinId pin);
    void DeleteNode(ed::NodeId id);
    bool InputTextField(const char* label, std::string& value);
    void DrawReferenceList();
    void DrawPaletteSection(const char* title, NodeRole role, const std::vector<const char*>& classes);
    void DrawPalette();
    crude_json::value BuildGraphJson();
    void ClearGraph();
    NodeRole ParseRole(const std::string& roleStr) const;
    LinkViewport ParseViewport(const crude_json::value& linkJson) const;
    void BuildGraphFromJson(const crude_json::value& root);
    bool CanConnectNodes(const NodeSpec& from, const NodeSpec& to) const;
    bool GetLinkEndpoints(NodeSpec& linkNode, NodeSpec*& src, NodeSpec*& dst);
    void NotifyLinkReady(NodeSpec& linkNode);
    void NotifyLinkBroken(NodeSpec& linkNode);

    static std::string s_imgSeq;
    static std::string s_pngSeq;
};

NS_HJ_END
