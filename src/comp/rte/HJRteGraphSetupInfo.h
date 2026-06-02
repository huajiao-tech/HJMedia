#pragma once

#include "HJComUtils.h"
#include "HJReflCppJson.h"

NS_HJ_BEGIN

#define HJRteGraphNode_Del(name)  static std::string name

#define HJRteGraphNodeClass_Del(name)  HJRteGraphNode_Del(name)
#define HJRteGraphNodeType_Del(type)  HJRteGraphNode_Del(type)
#define HJRteGraphNodeShaderType_Del(type)  HJRteGraphNode_Del(type)
#define HJRteGraphNodeRenderType_Del(type)  HJRteGraphNode_Del(type)

class HJRteGraphConfig
{
public:
    HJRteGraphNodeClass_Del(HJNodeClass_SourceBridge);
    HJRteGraphNodeClass_Del(HJNodeClass_SourceBridgeMediaData);

    HJRteGraphNodeClass_Del(HJNodeClass_SplitScreen);
    HJRteGraphNodeClass_Del(HJNodeClass_SplitScreenMediaData);

    HJRteGraphNodeClass_Del(HJNodeClass_SourcePNGSeq);
    HJRteGraphNodeClass_Del(HJNodeClass_SourceFaceu);
    HJRteGraphNodeClass_Del(HJNodeClass_SourceImage);
    HJRteGraphNodeClass_Del(HJNodeClass_SourceImageSeq);

    HJRteGraphNodeClass_Del(HJNodeClass_CustomSourceFilter);
    HJRteGraphNodeClass_Del(HJNodeClass_FilterBlur);
    HJRteGraphNodeClass_Del(HJNodeClass_FilterDenoise);
    HJRteGraphNodeClass_Del(HJNodeClass_FilterGray);
    HJRteGraphNodeClass_Del(HJNodeClass_FilterSR);
    HJRteGraphNodeClass_Del(HJNodeClass_FilterCopy2D);
    HJRteGraphNodeClass_Del(HJNodeClass_FilterCopyOES);

    HJRteGraphNodeClass_Del(HJNodeClass_TargetUI_0);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetUI_1);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetUI_2);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetUI_3);

    HJRteGraphNodeClass_Del(HJNodeClass_TargetEncoder);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetImgReceiver);

    HJRteGraphNodeClass_Del(HJNodeClass_TargetPBODetect);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetPBO_0);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetPBO_1);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetPBO_2);
    HJRteGraphNodeClass_Del(HJNodeClass_TargetPBO_3);

    HJRteGraphNodeType_Del(HJNodeRole_Source);
    HJRteGraphNodeType_Del(HJNodeRole_Filter);
    HJRteGraphNodeType_Del(HJNodeRole_Target);

    HJRteGraphNodeShaderType_Del(HJNodeShaderType_Copy2D);
    HJRteGraphNodeShaderType_Del(HJNodeShaderType_CopyOES);
    HJRteGraphNodeShaderType_Del(HJNodeShaderType_PreMul_Copy2D);
    HJRteGraphNodeShaderType_Del(HJNodeShaderType_PreMul_CopyOES);
    HJRteGraphNodeShaderType_Del(HJNodeShaderType_SplitScreenLR_2D);
    HJRteGraphNodeShaderType_Del(HJNodeShaderType_SplitScreenLR_OES);

    HJRteGraphNodeRenderType_Del(HJNodeRenderType_Clip);
    HJRteGraphNodeRenderType_Del(HJNodeRenderType_Fit);
    HJRteGraphNodeRenderType_Del(HJNodeRenderType_Origin);
    HJRteGraphNodeRenderType_Del(HJNodeRenderType_Full);

};

// Destination Node JSON description (replaces HJRteJsonLink)
class HJRteJsonLinkInfo : public HJReflCppJsonInterpreter<HJRteJsonLinkInfo>
{
public:
    HJ_DEFINE_CREATE(HJRteJsonLinkInfo);
    HJRteJsonLinkInfo() = default;
    virtual ~HJRteJsonLinkInfo() = default;

    //virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;
    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;

    void setLink(const std::string& i_renderMode, double i_x, double i_y, double i_w, double i_h, bool i_xMirror, bool i_yMirror);
    const std::string& getRenderMode() const;
    void setRenderMode(const std::string& i_renderMode);
    double getX() const;
    void setX(double i_x);
    double getY() const;
    void setY(double i_y);
    double getW() const;
    void setW(double i_w);
    double getH() const;
    void setH(double i_h);
    bool isXMirror() const;
    void setXMirror(bool i_xMirror);
    bool isYMirror() const;
    void setYMirror(bool i_yMirror);
    std::string getLinkInfo() const;
    void setLinkId(const std::string& i_linkId);
    const std::string& getLinkId() const;

    //if more links, use linkId
    std::string linkId = "";

    // Render Mode
    std::string renderMode = HJRteGraphConfig::HJNodeRenderType_Clip;

    // Rect
    double x = 0.0;
    double y = 0.0;
    double w = 1.0;
    double h = 1.0;

    bool xMirror = false;
    bool yMirror = false;
};

// Single Node JSON description
class HJRteJsonNode : public HJReflCppJsonInterpreter<HJRteJsonNode>
{
public:
    HJ_DEFINE_CREATE(HJRteJsonNode);
    HJRteJsonNode() = default;
    virtual ~HJRteJsonNode() = default;
    HJRteJsonNode(const std::string &i_classStyle, const std::string &i_insName, const std::string &i_role, bool i_bEnable = true, bool i_bMainSource = false, const std::string &i_resourceUrl = "", const std::string &i_dependsOn = "");
    //virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;
    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;
    const std::string& getClassStyle() const;
    void setClassStyle(const std::string& i_classStyle);
    const std::string& getInsName() const;
    void setInsName(const std::string& i_insName);
    const std::string& getRole() const;
    void setRole(const std::string& i_role);
    bool isEnable() const;
    void setEnable(bool i_enable);
    bool isMainSourceNode() const;
    void setMainSource(bool i_isMainSource);
    void setResourceUrl(const std::string& i_nodeInfo);
    const std::string& getResourceUrl() const;
    void setDependsOn(const std::string& i_source);
    const std::string& getDependsOn() const;

    std::string classStyle = "";
    std::string insName = "";
    std::string role = "";

    //int64_t priority = 0;

    bool enable = true;
	
    //only source
    bool isMainSource = false;

    std::string resourceUrl = "";
    std::string dependsOn = "";

};

// Link JSON description
class HJRteJsonLink : public HJReflCppJsonInterpreter<HJRteJsonLink>
{
public:
    HJ_DEFINE_CREATE(HJRteJsonLink);
    HJRteJsonLink() = default;
    virtual ~HJRteJsonLink() = default;

    void setFrom(const std::string& i_from);
    const std::string& getFrom() const;
    void setTo(const std::string& i_to);
    const std::string& getTo() const;
    void setLink(const HJRteJsonLinkInfo& i_link);
    HJRteJsonLinkInfo& getLink();
    const HJRteJsonLinkInfo& getLink() const;
    void setShaderType(const std::string& i_shaderType);
    const std::string& getShaderType() const;

    std::string from = "";
    std::string to = "";
    std::string shaderType = "";
    HJRteJsonLinkInfo link;
};

class HJRteGraphSetupInfo : public HJReflCppJsonInterpreter<HJRteGraphSetupInfo>
{
public:
    HJ_DEFINE_CREATE(HJRteGraphSetupInfo);
    HJRteGraphSetupInfo() = default;
    virtual ~HJRteGraphSetupInfo() = default;

    //virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;
    //virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;

    std::vector<HJRteJsonNode::Ptr> nodes;
    std::vector<HJRteJsonLink::Ptr> links;

    void addSource(HJRteJsonNode::Ptr node)
    {
        nodes.push_back(std::move(node));
    }

    void addLink(HJRteJsonLink::Ptr link)
    {
        links.push_back(std::move(link));
    }
};



class HJRteGraphConfigConstructor : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteGraphConfigConstructor);
    HJRteGraphConfigConstructor() = default;
    virtual ~HJRteGraphConfigConstructor() = default;

    static std::string constructGraph(HJBaseParam::Ptr i_param);
private:
	static std::string priConstructPlaceHolder(HJBaseParam::Ptr i_param);
    static std::string priConstructSplictScreen(HJBaseParam::Ptr i_param);
    static std::string priConstructImage(HJBaseParam::Ptr i_param);
    static std::string priConstructImageSeq(HJBaseParam::Ptr i_param);
    static std::string priConstructTest(HJBaseParam::Ptr i_param);
	static std::shared_ptr<HJRteJsonNode> priCreateJsonNode(const HJRteGraphSetupInfo::Ptr &cfg, const std::string& i_classStyle, const std::string& i_insName, const std::string& i_role, bool i_bEnable = true, bool i_bMainSource = false);
	static void priConnectNode(const HJRteGraphSetupInfo::Ptr &cfg, std::shared_ptr<HJRteJsonNode> i_srcNode, std::shared_ptr<HJRteJsonNode> i_dstNode, const HJRteJsonLinkInfo& i_link, const std::string& i_shaderType = "");
    static std::string priGetSplictScreenSourceName();
    static std::string priGetDefaultSourceName();
};

NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJRteJsonLinkInfo,
	renderMode,
	x, y, w, h,
	xMirror,
	yMirror,
	linkId)

REFL_AUTO_SIMPLE(HJ::HJRteJsonNode,
    classStyle,
	insName,
	role,
	enable,
	isMainSource,
	resourceUrl,
	dependsOn)

REFL_AUTO_SIMPLE(HJ::HJRteJsonLink,
    from,
    to,
    shaderType,
    link)

REFL_AUTO_SIMPLE(HJ::HJRteGraphSetupInfo, nodes, links)
