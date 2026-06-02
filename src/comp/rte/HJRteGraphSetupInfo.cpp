#include "HJRteGraphSetupInfo.h"
#include "HJFLog.h"
#include "HJRteUtils.h"

#define USE_FACEU 1

NS_HJ_BEGIN

std::string HJRteJsonLinkInfo::getLinkInfo() const
{
    return HJFMT("<x, y, w, h>:<{} {} {} {}> xMirror:{} yMirror:{}", x, y, w, h, xMirror, yMirror);
}


void HJRteJsonLinkInfo::setLink(const std::string& i_renderMode, double i_x, double i_y, double i_w, double i_h, bool i_xMirror, bool i_yMirror)
{
    renderMode = i_renderMode;

    x = i_x;
    y = i_y;
    w = i_w;
    h = i_h;

    xMirror = i_xMirror;
    yMirror = i_yMirror;
}
const std::string& HJRteJsonLinkInfo::getRenderMode() const
{
    return renderMode;
}
void HJRteJsonLinkInfo::setRenderMode(const std::string& i_renderMode)
{
    renderMode = i_renderMode;
}
double HJRteJsonLinkInfo::getX() const
{
    return x;
}
void HJRteJsonLinkInfo::setX(double i_x)
{
    x = i_x;
}
double HJRteJsonLinkInfo::getY() const
{
    return y;
}
void HJRteJsonLinkInfo::setY(double i_y)
{
    y = i_y;
}
double HJRteJsonLinkInfo::getW() const
{
    return w;
}
void HJRteJsonLinkInfo::setW(double i_w)
{
    w = i_w;
}
double HJRteJsonLinkInfo::getH() const
{
    return h;
}
void HJRteJsonLinkInfo::setH(double i_h)
{
    h = i_h;
}
bool HJRteJsonLinkInfo::isXMirror() const
{
    return xMirror;
}
void HJRteJsonLinkInfo::setXMirror(bool i_xMirror)
{
    xMirror = i_xMirror;
}
bool HJRteJsonLinkInfo::isYMirror() const
{
    return yMirror;
}
void HJRteJsonLinkInfo::setYMirror(bool i_yMirror)
{
    yMirror = i_yMirror;
}
void HJRteJsonLinkInfo::setLinkId(const std::string& i_linkId)
{
    linkId = i_linkId;
}
const std::string& HJRteJsonLinkInfo::getLinkId() const
{
    return linkId;
}
////////////////////////////////////////////////////////////////////////////////
HJRteJsonNode::HJRteJsonNode(const std::string& i_classStyle, const std::string& i_insName, const std::string& i_role,
    bool i_bEnable, bool i_bMainSource, const std::string& i_resourceUrl, const std::string &i_dependsOn)
    : classStyle(i_classStyle),
    insName(i_insName),
    role(i_role),
    enable(i_bEnable),
    isMainSource(i_bMainSource), 
    resourceUrl(i_resourceUrl),
    dependsOn(i_dependsOn)
{
}

const std::string& HJRteJsonNode::getClassStyle() const
{
    return classStyle;
}
void HJRteJsonNode::setClassStyle(const std::string& i_classStyle)
{
    classStyle = i_classStyle;
}
const std::string& HJRteJsonNode::getInsName() const
{
    return insName;
}
void HJRteJsonNode::setInsName(const std::string& i_insName)
{
    insName = i_insName;
}
const std::string& HJRteJsonNode::getRole() const
{
    return role;
}
void HJRteJsonNode::setRole(const std::string& i_role)
{
    role = i_role;
}
bool HJRteJsonNode::isEnable() const
{
    return enable;
}
void HJRteJsonNode::setEnable(bool i_enable)
{
    enable = i_enable;
}
bool HJRteJsonNode::isMainSourceNode() const
{
    return isMainSource;
}
void HJRteJsonNode::setMainSource(bool i_isMainSource)
{
    isMainSource = i_isMainSource;
}
void HJRteJsonNode::setResourceUrl(const std::string& i_nodeInfo)
{
    resourceUrl = i_nodeInfo;
}
const std::string& HJRteJsonNode::getResourceUrl() const
{
    return resourceUrl;
}
void HJRteJsonNode::setDependsOn(const std::string& i_source)
{
    dependsOn = i_source;
}
const std::string& HJRteJsonNode::getDependsOn() const
{
    return dependsOn;
}

////////////////////////////////////////////////////////////////////////////////
void HJRteJsonLink::setFrom(const std::string& i_from)
{
    from = i_from;
}
const std::string& HJRteJsonLink::getFrom() const
{
    return from;
}
void HJRteJsonLink::setTo(const std::string& i_to)
{
    to = i_to;
}
const std::string& HJRteJsonLink::getTo() const
{
    return to;
}
void HJRteJsonLink::setLink(const HJRteJsonLinkInfo& i_link)
{
    link = i_link;
}
HJRteJsonLinkInfo& HJRteJsonLink::getLink()
{
    return link;
}
const HJRteJsonLinkInfo& HJRteJsonLink::getLink() const
{
    return link;
}
void HJRteJsonLink::setShaderType(const std::string& i_shaderType)
{
    shaderType = i_shaderType;
}
const std::string& HJRteJsonLink::getShaderType() const
{
    return shaderType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define HJRteGraphNode_Def(name) std::string HJRteGraphConfig::name = HJ_CatchName(name);
#define HJRteGraphNodeClass_Def(name) HJRteGraphNode_Def(name)
#define HJRteGraphNodeRole_Def(name) HJRteGraphNode_Def(name)
#define HJRteGraphNodeShaderType_Def(name) HJRteGraphNode_Def(name)
#define HJRteGraphNodeRenderType_Def(name) HJRteGraphNode_Def(name)

HJRteGraphNodeClass_Def(HJNodeClass_SourceBridge);
HJRteGraphNodeClass_Def(HJNodeClass_SourceBridgeMediaData);

HJRteGraphNodeClass_Def(HJNodeClass_SplitScreen);
HJRteGraphNodeClass_Def(HJNodeClass_SplitScreenMediaData);

HJRteGraphNodeClass_Def(HJNodeClass_SourcePNGSeq);
HJRteGraphNodeClass_Def(HJNodeClass_SourceFaceu);
HJRteGraphNodeClass_Def(HJNodeClass_SourceImage);
HJRteGraphNodeClass_Def(HJNodeClass_SourceImageSeq);

HJRteGraphNodeClass_Def(HJNodeClass_CustomSourceFilter);
HJRteGraphNodeClass_Def(HJNodeClass_FilterBlur);
HJRteGraphNodeClass_Def(HJNodeClass_FilterDenoise);
HJRteGraphNodeClass_Def(HJNodeClass_FilterGray);
HJRteGraphNodeClass_Def(HJNodeClass_FilterSR);
HJRteGraphNodeClass_Def(HJNodeClass_FilterCopy2D);
HJRteGraphNodeClass_Def(HJNodeClass_FilterCopyOES);

HJRteGraphNodeClass_Def(HJNodeClass_TargetUI_0);
HJRteGraphNodeClass_Def(HJNodeClass_TargetUI_1);
HJRteGraphNodeClass_Def(HJNodeClass_TargetUI_2);
HJRteGraphNodeClass_Def(HJNodeClass_TargetUI_3);

HJRteGraphNodeClass_Def(HJNodeClass_TargetEncoder);
HJRteGraphNodeClass_Def(HJNodeClass_TargetImgReceiver);

HJRteGraphNodeClass_Def(HJNodeClass_TargetPBODetect);
HJRteGraphNodeClass_Def(HJNodeClass_TargetPBO_0);
HJRteGraphNodeClass_Def(HJNodeClass_TargetPBO_1);
HJRteGraphNodeClass_Def(HJNodeClass_TargetPBO_2);
HJRteGraphNodeClass_Def(HJNodeClass_TargetPBO_3);


HJRteGraphNodeRole_Def(HJNodeRole_Source);
HJRteGraphNodeRole_Def(HJNodeRole_Filter);
HJRteGraphNodeRole_Def(HJNodeRole_Target);

HJRteGraphNodeShaderType_Def(HJNodeShaderType_Copy2D);
HJRteGraphNodeShaderType_Def(HJNodeShaderType_CopyOES);
HJRteGraphNodeShaderType_Def(HJNodeShaderType_PreMul_Copy2D);
HJRteGraphNodeShaderType_Def(HJNodeShaderType_PreMul_CopyOES);
HJRteGraphNodeShaderType_Def(HJNodeShaderType_SplitScreenLR_2D);
HJRteGraphNodeShaderType_Def(HJNodeShaderType_SplitScreenLR_OES);

HJRteGraphNodeRenderType_Def(HJNodeRenderType_Clip);
HJRteGraphNodeRenderType_Def(HJNodeRenderType_Fit);
HJRteGraphNodeRenderType_Def(HJNodeRenderType_Origin);
HJRteGraphNodeRenderType_Def(HJNodeRenderType_Full);


///////////////////////////////////////////////////////////////////////////////////
std::string HJRteGraphConfigConstructor::constructGraph(HJBaseParam::Ptr i_param)
{
    HJRteGraphConstructorType type = HJRteGraphConstructorType_None;
    HJ_CatchMapGetVal(i_param, HJRteGraphConstructorType, type);
    switch (type)
    {
    case HJRteGraphConstructorType_PlaceHolder:
        return priConstructPlaceHolder(i_param);
    case HJRteGraphConstructorType_SplictScreen:
        return priConstructSplictScreen(i_param);
    case HJRteGraphConstructorType_Test:
        return priConstructTest(i_param);
    case HJRteGraphConstructorType_Image:
        return priConstructImage(i_param);
    case HJRteGraphConstructorType_ImageSeq:
        return priConstructImageSeq(i_param);
    default:
        break;
    }
    return "";
}
std::string HJRteGraphConfigConstructor::priConstructPlaceHolder(HJBaseParam::Ptr i_param)
{
    HJRteGraphSetupInfo::Ptr cfg = HJRteGraphSetupInfo::Create();

    bool bUseCustomFilter = false;
    HJ_CatchMapPlainGetVal(i_param, bool, "useCustomFilter", bUseCustomFilter);

	HJRteJsonNode::Ptr videoSourceNode = priCreateJsonNode(cfg, priGetDefaultSourceName(), priGetDefaultSourceName(), HJRteGraphConfig::HJNodeRole_Source, true, true);

	std::string video2DName = "";
#if defined(HarmonyOS) 
	video2DName = HJRteGraphConfig::HJNodeClass_FilterCopyOES;
#else 
	video2DName = HJRteGraphConfig::HJNodeClass_FilterCopy2D;
#endif

	HJRteJsonNode::Ptr video2D = priCreateJsonNode(cfg, video2DName, video2DName, HJRteGraphConfig::HJNodeRole_Filter);

	priConnectNode(cfg, videoSourceNode, video2D, HJRteJsonLinkInfo());

	HJRteJsonNode::Ptr detectPBO = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetPBODetect, HJRteGraphConfig::HJNodeClass_TargetPBODetect, HJRteGraphConfig::HJNodeRole_Target, false);
	priConnectNode(cfg, video2D, detectPBO, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);

	HJRteJsonNode::Ptr customFilter = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_CustomSourceFilter, HJRteGraphConfig::HJNodeClass_CustomSourceFilter, HJRteGraphConfig::HJNodeRole_Filter, bUseCustomFilter);
	priConnectNode(cfg, video2D, customFilter, HJRteJsonLinkInfo());

	HJRteJsonNode::Ptr blur = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_FilterBlur, HJRteGraphConfig::HJNodeClass_FilterBlur, HJRteGraphConfig::HJNodeRole_Filter, false);
	priConnectNode(cfg, customFilter, blur, HJRteJsonLinkInfo());

	HJRteJsonNode::Ptr ui2D_0 = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
	priConnectNode(cfg, blur, ui2D_0, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);

	HJRteJsonNode::Ptr ui2D_1 = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_1, HJRteGraphConfig::HJNodeClass_TargetUI_1, HJRteGraphConfig::HJNodeRole_Target, false);
	priConnectNode(cfg, blur, ui2D_1, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
	//ui2D, uiOES instances, or different layout use for different ui instance, use in different linker shader
	//HJRteJsonNode::Ptr uiOES = priCreateJsonNode(HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false, HJRteGraphConfig::HJNodeShaderType_CopyOES);
	//HJRteJsonNode::Ptr uiTopLayout = priCreateJsonNode(HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false, HJRteGraphConfig::HJNodeShaderType_CopyOES);
#if USE_FACEU
	HJRteJsonNode::Ptr faceu = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeRole_Source, false);
    faceu->setDependsOn(videoSourceNode->getInsName());
	priConnectNode(cfg, faceu, ui2D_0, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
    priConnectNode(cfg, faceu, ui2D_1, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
#endif
	if (priGetDefaultSourceName() == HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData)
	{
		HJRteJsonNode::Ptr targetPBO_0 = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetPBO_0, HJRteGraphConfig::HJNodeClass_TargetPBO_0, HJRteGraphConfig::HJNodeRole_Target, true);
		priConnectNode(cfg, blur, targetPBO_0, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
#if USE_FACEU		
        priConnectNode(cfg, faceu, targetPBO_0, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
#endif
	}

	HJRteJsonNode::Ptr encoderTarget = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetEncoder, HJRteGraphConfig::HJNodeClass_TargetEncoder, HJRteGraphConfig::HJNodeRole_Target, false);
    priConnectNode(cfg, blur, encoderTarget, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);

#if USE_FACEU
    priConnectNode(cfg, faceu, encoderTarget, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
#endif

	std::string config = cfg->serial();
	HJFLogi("config:{}", config);

    return config;
}
std::string HJRteGraphConfigConstructor::priConstructSplictScreen(HJBaseParam::Ptr i_param)
{
    HJRteGraphSetupInfo::Ptr cfg = HJRteGraphSetupInfo::Create();

    bool bSplitScreenXMirror = false;
    HJ_CatchMapPlainGetVal(i_param, bool, "IsMirror", bSplitScreenXMirror);

	HJRteJsonNode::Ptr videoSourceNode = priCreateJsonNode(cfg, priGetSplictScreenSourceName(), priGetSplictScreenSourceName(), HJRteGraphConfig::HJNodeRole_Source, true, true);
    std::string shaderType = "";
#if defined(HarmonyOS)
    shaderType = HJRteGraphConfig::HJNodeShaderType_SplitScreenLR_OES;
#else
    shaderType = HJRteGraphConfig::HJNodeShaderType_SplitScreenLR_2D;
#endif

    HJRteJsonNode::Ptr ui = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
    HJRteJsonLinkInfo uiLink;
    uiLink.setXMirror(bSplitScreenXMirror);
    priConnectNode(cfg, videoSourceNode, ui, uiLink, shaderType);

    if (priGetSplictScreenSourceName() == HJRteGraphConfig::HJNodeClass_SplitScreenMediaData)
    {
        HJRteJsonNode::Ptr targetPBO_0 = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetPBO_0, HJRteGraphConfig::HJNodeClass_TargetPBO_0, HJRteGraphConfig::HJNodeRole_Target, true);
        HJRteJsonLinkInfo targetLink;
        targetLink.setXMirror(bSplitScreenXMirror);
        priConnectNode(cfg, videoSourceNode, targetPBO_0, targetLink, shaderType);
    }


	std::string config = cfg->serial();
    HJFLogi("config:{}", config);
    return config;
}
std::string HJRteGraphConfigConstructor::priConstructImageSeq(HJBaseParam::Ptr i_param)
{
    HJRteGraphSetupInfo::Ptr cfg = HJRteGraphSetupInfo::Create();

    HJRteJsonNode::Ptr imageSeqNode = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_SourceImageSeq, HJRteGraphConfig::HJNodeClass_SourceImageSeq, HJRteGraphConfig::HJNodeRole_Source, true, true);

    HJRteJsonNode::Ptr ui = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
    HJRteJsonNode::Ptr faceu = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeRole_Source, true);
    faceu->setDependsOn(imageSeqNode->getInsName());
    priConnectNode(cfg, imageSeqNode, ui, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_PreMul_Copy2D); 
    priConnectNode(cfg, faceu, ui, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);

    std::string config = cfg->serial();
    HJFLogi("config:{}", config);
    return config;
}
std::string HJRteGraphConfigConstructor::priConstructImage(HJBaseParam::Ptr i_param)
{
    HJRteGraphSetupInfo::Ptr cfg = HJRteGraphSetupInfo::Create();

    HJRteJsonNode::Ptr imageNode = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_SourceImage, HJRteGraphConfig::HJNodeClass_SourceImage, HJRteGraphConfig::HJNodeRole_Source, true, true);

    HJRteJsonNode::Ptr ui = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
    HJRteJsonNode::Ptr faceu = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeRole_Source, false);
    faceu->setDependsOn(imageNode->getInsName());
    priConnectNode(cfg, imageNode, ui, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_PreMul_Copy2D); 
    priConnectNode(cfg, faceu, ui, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
    
    std::string config = cfg->serial();
    HJFLogi("config:{}", config);
    return config;
}

std::string HJRteGraphConfigConstructor::priGetDefaultSourceName()
{
	std::string videoSourceName = "";
#if defined(HarmonyOS) 
	videoSourceName = HJRteGraphConfig::HJNodeClass_SourceBridge;
#else 
	videoSourceName = HJRteGraphConfig::HJNodeClass_SourceBridgeMediaData;
#endif
    return videoSourceName;
}

std::string HJRteGraphConfigConstructor::priGetSplictScreenSourceName()
{
    std::string souceName = "";
#if defined(HarmonyOS) 
    souceName = HJRteGraphConfig::HJNodeClass_SplitScreen;
#else 
    souceName = HJRteGraphConfig::HJNodeClass_SplitScreenMediaData;
#endif
    return souceName;
}
std::shared_ptr<HJRteJsonNode> HJRteGraphConfigConstructor::priCreateJsonNode(const HJRteGraphSetupInfo::Ptr& cfg, const std::string& i_classStyle, const std::string& i_insName, const std::string& i_role, bool i_bEnable, bool i_bMainSource)
{
    HJRteJsonNode::Ptr node = HJRteJsonNode::Create<HJRteJsonNode>(i_classStyle, i_insName, i_role, i_bEnable, i_bMainSource);
    cfg->nodes.push_back(node);
	return node;
}
void HJRteGraphConfigConstructor::priConnectNode(const HJRteGraphSetupInfo::Ptr& cfg, std::shared_ptr<HJRteJsonNode> i_srcNode, std::shared_ptr<HJRteJsonNode> i_dstNode, const HJRteJsonLinkInfo& i_link, const std::string& i_shaderType)
{
    HJRteJsonLink::Ptr link = HJRteJsonLink::Create();
    link->setFrom(i_srcNode->getInsName());
    link->setTo(i_dstNode->getInsName());
    link->setShaderType(i_shaderType);
    link->setLink(i_link);
    cfg->links.push_back(std::move(link));
}


std::string HJRteGraphConfigConstructor::priConstructTest(HJBaseParam::Ptr i_param)
{
    HJRteGraphSetupInfo::Ptr cfg = HJRteGraphSetupInfo::Create();

    bool bUseCustomFilter = false;
    HJ_CatchMapPlainGetVal(i_param, bool, "useCustomFilter", bUseCustomFilter);

    HJRteJsonNode::Ptr videoSourceNode = priCreateJsonNode(cfg, priGetDefaultSourceName(), priGetDefaultSourceName(), HJRteGraphConfig::HJNodeRole_Source, true, true);

    std::string video2DName = "";
#if defined(HarmonyOS) 
    video2DName = HJRteGraphConfig::HJNodeClass_FilterCopyOES;
#else 
    video2DName = HJRteGraphConfig::HJNodeClass_FilterCopy2D;
#endif

    HJRteJsonNode::Ptr video2D = priCreateJsonNode(cfg, video2DName, video2DName, HJRteGraphConfig::HJNodeRole_Filter);

    priConnectNode(cfg, videoSourceNode, video2D, HJRteJsonLinkInfo());

#if 0
    HJRteJsonNode::Ptr ui2D = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
    HJRteJsonLinkInfo ui2DLink;
    ui2DLink.y = 0.0;
    ui2DLink.h = 0.5;
    priConnectNode(cfg, video2D, ui2D, ui2DLink, HJRteGraphConfig::HJNodeShaderType_Copy2D);

    HJRteJsonNode::Ptr ui2 = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
    HJRteJsonLinkInfo ui2Link;
    ui2Link.y = 0.5;
    ui2Link.h = 0.5;
    ui2Link.xMirror = true;
    ui2Link.renderMode = HJRteGraphConfig::HJNodeRenderType_Fit;
    priConnectNode(cfg, video2D, ui2, ui2Link, HJRteGraphConfig::HJNodeShaderType_Copy2D);
#else
    HJRteJsonNode::Ptr ui2D = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeClass_TargetUI_0, HJRteGraphConfig::HJNodeRole_Target, false);
    priConnectNode(cfg, video2D, ui2D, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);

    HJRteJsonNode::Ptr denoise = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_FilterDenoise, HJRteGraphConfig::HJNodeClass_FilterDenoise, HJRteGraphConfig::HJNodeRole_Filter, false);
    priConnectNode(cfg, video2D, denoise, HJRteJsonLinkInfo());

    HJRteJsonNode::Ptr sr = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_FilterSR, HJRteGraphConfig::HJNodeClass_FilterSR, HJRteGraphConfig::HJNodeRole_Filter, false);
    priConnectNode(cfg, denoise, sr, HJRteJsonLinkInfo());

    priConnectNode(cfg, sr, ui2D, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);

    HJRteJsonNode::Ptr faceu = priCreateJsonNode(cfg, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeRole_Source, false);
    faceu->setDependsOn(videoSourceNode->getInsName());
    priConnectNode(cfg, faceu, ui2D, HJRteJsonLinkInfo(), HJRteGraphConfig::HJNodeShaderType_Copy2D);
#endif


    std::string config = cfg->serial();
    HJFLogi("config:{}", config);
    return config;
}

NS_HJ_END
