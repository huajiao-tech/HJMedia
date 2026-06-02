#pragma once

#include "HJPrerequisites.h"
#include "HJRteGraph.h"
#include "HJRteGraphBaseEGL.h"
#include "HJRteUtils.h"
#include "HJRteGraphSetupInfo.h"
#include <unordered_map>

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJRteComSourceBridge;
class HJRteComLinkInfo;
class HJRteCom;
class HJRteComSource;
class HJFacePointMgr;
class HJFaceuInfo;
class HJMorePointSmooth;

class HJRteGraphProc : public HJRteGraphBaseEGL
{
public:
    HJ_DEFINE_CREATE(HJRteGraphProc);

    HJRteGraphProc();
    virtual ~HJRteGraphProc();

    virtual int init(std::shared_ptr<HJBaseParam> i_param) override;
    virtual void done() override;
    virtual int insertFilterAfter(std::shared_ptr<HJRteCom> i_com, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_dstShader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_dstLinkInfo = nullptr) override;
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
#endif

    virtual int constructGraph(HJBaseParam::Ptr i_param)
    {
        return 0;
    }
    virtual int procWindow(const std::string &i_classStyle, const std::string &i_insName, const std::shared_ptr<HJOGEGLSurface>& i_eglSurface, int i_state)
    {
        return 0;
    }

    // Dynamic node APIs (unified nodexx naming)
    virtual int nodeCreate(const std::string& i_nodeInfo)
    {
        return 0;
    }
    virtual int nodeCreate(HJRteJsonNode& i_nodeCfg)
    {
        return 0;
    }
    virtual int nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource, const std::string& i_resourceUrl, const std::string &i_dependsOn)
    {
        return 0;
    }
    virtual int nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, 
                            const std::string& i_dstClassStyle, const std::string& i_dstInsName, 
                            const std::string &i_shaderStyle, const std::string& i_linkInfo)
    {
        return 0;
    }
    virtual int nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink = false)
    {
        return 0;
    }
    virtual int nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
                            const std::string& i_dstClassStyle, const std::string& i_dstInsName,
                            const std::string& i_shaderStyle, HJRteJsonLinkInfo& i_linkInfo)
    {
        return 0;
    }
    virtual int nodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
                               const std::string& i_dstClassStyle, const std::string& i_dstInsName,
                               const std::string& i_linkId = "")
    {
        return 0;
    }   
    virtual int nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
        const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        const std::string& i_linkInfo)
    {
        return 0;
    }
    virtual int nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
        const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        HJRteJsonLinkInfo& i_linkInfo)
    {
        return 0;
    }
    virtual std::string nodeGetPre(const std::string& i_curClassStyle, const std::string& i_curInsName)
    {
        return "";
    }
    virtual std::string nodeGetNext(const std::string& i_curClassStyle, const std::string& i_curInsName)
    {
        return "";
    }
    virtual int nodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info)
    {
        return 0;
    }
    virtual int nodeSetParam(const std::string& i_classStyle, const std::string& i_insName, std::shared_ptr<HJBaseParam> i_param)
    {
        return 0;
    }

    static HJRteGraphProc::Ptr createRteGraphProc(HJRteGraphProcType i_type);


    int openPNGSeq(HJBaseParam::Ptr i_param);
    void setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape);
    void setGiftPusher(bool i_bGiftPusher);

    void openEffect(HJBaseParam::Ptr i_param);
    void closeEffect(HJBaseParam::Ptr i_param);

    void graphProcAsync(std::function<int()> i_func);

    int connectCom(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_shader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);
    void addSource(std::shared_ptr<HJRteCom> i_com);
    void addTarget(std::shared_ptr<HJRteCom> i_com);
    void addFilter(std::shared_ptr<HJRteCom> i_com);

    //bool isForceAdjustResolution() const { return m_bForceAdjustResolution; }
    //void setForceAdjustResolution(bool i_bForceAdjustResolution) { m_bForceAdjustResolution = i_bForceAdjustResolution; }

    int openFaceEffect();
    void closeFaceEffect();
    void openPBO(HJMediaDataReaderCb i_cb);
    void closePBO();
    //int openFaceu(HJBaseParam::Ptr i_param);
    std::shared_ptr<HJFaceuInfo> getFaceuInfo();
    //void closeFaceu();
    //void registSubscriber(HJFaceSubscribeFuncPtr i_subscriberPtr);
    void setFaceProtected(bool i_bProtected);
    void setFaceInfo(const std::string& i_sourceInsName, int i_width, int i_height, const std::string& i_faceInfo, bool i_bDebugPoint = false);

    int procEGLSurface(const std::shared_ptr<HJOGEGLSurface>& i_eglSurface, int i_type, int i_state);

    void setFaceInfo(const std::string& i_sourceInsName, const std::string& i_faceInfo);
    std::string getFaceInfo(const std::string& i_sourceInsName);
    void clearFaceInfo(const std::string& i_sourceInsName);
protected:
    virtual int runRender() override;
    virtual void priDoneOnGraphThread() override;
    void clearSourceResolutionCache(const std::string& i_sourceInsName);
    std::shared_ptr<HJRteComSourceBridge> getVideoSourceBridge();
    //std::shared_ptr<HJRteComSourceBridge> m_videoSource = nullptr;
    static int s_blurCascadeNum;
	//std::shared_ptr<HJFacePointMgr> m_faceMgr = nullptr;
private:
    int priPreCheckGraph();
    void priStatRefCnt();
    int priConnect(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_shader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);
    int priOpenEffect(HJBaseParam::Ptr i_param);
    int priCloseEffect(HJBaseParam::Ptr i_param);
    std::shared_ptr<HJRteComSource> priGetMainFilterSource();
    //bool m_bForceAdjustResolution = false;

    std::shared_ptr<HJMorePointSmooth> m_facePointSmooth = nullptr;
    bool m_bFaceProtected = false;
    HJFaceStatus m_faceStatus = HJFaceStatus_Unknown;

    std::mutex m_faceMutex;
    std::unordered_map<std::string, std::string> m_faceInfoBySource;
    std::unordered_map<std::string, std::pair<int, int>> m_lastAdjustedResolutionBySource;

    bool m_bMainSourceRenderForce = true;

    bool m_bFaceEffectEnable = false;
};

class HJRteGraphProcSplictScreen : public HJRteGraphProc
{
public:
    HJ_DEFINE_CREATE(HJRteGraphProcSplictScreen);
    HJRteGraphProcSplictScreen() = default;
    virtual ~HJRteGraphProcSplictScreen() = default;
    virtual int constructGraph(HJBaseParam::Ptr i_param) override;
private:
    
};

class HJRteGraphProcPlaceHolderDefault : public HJRteGraphProc
{
public:
    HJ_DEFINE_CREATE(HJRteGraphProcPlaceHolderDefault);
    HJRteGraphProcPlaceHolderDefault() = default;
    virtual ~HJRteGraphProcPlaceHolderDefault() = default;
    virtual int constructGraph(HJBaseParam::Ptr i_param) override;

private:

};

class HJRteGraphProcTest : public HJRteGraphProc
{
public:
	HJ_DEFINE_CREATE(HJRteGraphProcTest);
    HJRteGraphProcTest() = default;
	virtual ~HJRteGraphProcTest() = default;
	virtual int constructGraph(HJBaseParam::Ptr i_param) override;

private:

};

class HJRteJsonNode;
struct HJRteNodeAdjacentInfo
{
    std::string classStyle;
    std::string insName;
};

class HJRteNodeAdjacentInfoJson : public HJReflCppJsonInterpreter<HJRteNodeAdjacentInfoJson>
{
public:
    HJ_DEFINE_CREATE(HJRteNodeAdjacentInfoJson);
    HJRteNodeAdjacentInfoJson() = default;
    HJRteNodeAdjacentInfoJson(const HJRteNodeAdjacentInfo& i_info)
        : classStyle(i_info.classStyle)
        , insName(i_info.insName)
    {
    }
    virtual ~HJRteNodeAdjacentInfoJson() = default;

    std::string classStyle = "";
    std::string insName = "";
};

class HJRteNodeAdjacentNodesJson : public HJReflCppJsonInterpreter<HJRteNodeAdjacentNodesJson>
{
public:
    HJ_DEFINE_CREATE(HJRteNodeAdjacentNodesJson);
    HJRteNodeAdjacentNodesJson() = default;
    virtual ~HJRteNodeAdjacentNodesJson() = default;

    std::vector<HJRteNodeAdjacentInfoJson> nodes;
};

class HJRteGraphProcConfigSetup : public HJRteGraphProc
{
public:
	HJ_DEFINE_CREATE(HJRteGraphProcConfigSetup);
    HJRteGraphProcConfigSetup() = default;
	virtual ~HJRteGraphProcConfigSetup() = default;
	virtual int constructGraph(HJBaseParam::Ptr i_param) override;
    virtual int procWindow(const std::string &i_classStyle, const std::string &i_insName, const std::shared_ptr<HJOGEGLSurface>& i_eglSurface, int i_state) override;
    
    virtual int nodeCreate(const std::string& i_nodeInfo) override;
    virtual int nodeCreate(HJRteJsonNode& i_nodeCfg) override;
    virtual int nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource, const std::string& i_resourceUrl, const std::string& i_dependsOn) override;
    virtual int nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName, 
        const std::string& i_shaderStyle, const std::string& i_linkInfo) override;
    virtual int nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        const std::string& i_shaderStyle, HJRteJsonLinkInfo& i_linkInfo) override;

    virtual int nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink = false) override;
    virtual int nodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
                               const std::string& i_dstClassStyle, const std::string& i_dstInsName,
                               const std::string& i_linkId = "") override;
    virtual int nodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info) override;
    virtual int nodeSetParam(const std::string& i_classStyle, const std::string& i_insName, std::shared_ptr<HJBaseParam> i_param) override;

    virtual int nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
        const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        const std::string& i_linkInfo) override;
    virtual int nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
        const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        HJRteJsonLinkInfo& i_linkInfo) override;
    virtual std::string nodeGetPre(const std::string& i_curClassStyle, const std::string& i_curInsName) override;
    virtual std::string nodeGetNext(const std::string& i_curClassStyle, const std::string& i_curInsName) override;
private:
	std::shared_ptr<HJOGBaseShader> priCreateShaderFromString(const std::string& i_type);
	HJWindowRenderMode priParseRenderMode(const std::string& i_modeStr);

    std::shared_ptr<HJRteCom> priCreateComFromJsonNode(const std::shared_ptr<HJRteJsonNode>& i_nodeCfg, const HJBaseParam::Ptr& i_param);
    int priCreateGraphFromJson(const std::string& i_jsonStr, const HJBaseParam::Ptr& i_param);
    void priSetResource(const HJRteJsonNode::Ptr& i_nodeCfg, const HJBaseParam::Ptr& i_param, const std::string &i_paramKey);
    std::pair<std::shared_ptr<HJOGBaseShader>, std::shared_ptr<HJRteComLinkInfo>> priGetShaderAndLinkInfo(const std::string& i_shaderStyle, const HJRteJsonLinkInfo& i_linkInfo);

    int priSetEnableFaceu(std::shared_ptr<HJRteCom> i_com, bool i_enable, const std::string& i_info);
    std::string priGetNodeKey(const std::shared_ptr<HJRteJsonNode>& i_nodeCfg);
    std::string priGetNodeKey(const std::string &i_classStyle, const std::string &i_insName);
    std::shared_ptr<HJRteCom> priGetMapCom(const std::string& i_classStyle, const std::string& i_insName);
    bool priGetNodeIdentity(const std::shared_ptr<HJRteCom>& i_com, HJRteNodeAdjacentInfo& o_info) const;
    std::string priSerializeAdjacentNodes(const std::vector<HJRteNodeAdjacentInfo>& i_nodes) const;
    void priRemoveFromMap(const std::string& i_classStyle, const std::string& i_insName);
    std::unordered_map<std::string, std::weak_ptr<HJRteCom>> m_nodeMap;
    std::unordered_map<std::string, HJRteNodeAdjacentInfo> m_nodeIdentityByKey;

    int priNodeCreate(const std::string& i_nodeInfo);
    int priNodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        const std::string& i_shaderStyle, const std::string& i_linkInfo);
    int priNodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink);
    int priNodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
        const std::string& i_dstClassStyle, const std::string& i_dstInsName, const std::string& i_linkId);
    int priNodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info);
    int priNodeSetParam(const std::string& i_classStyle, const std::string& i_insName, const HJBaseParam::Ptr& i_param);
    int priNodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
        const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        const std::string& i_linkInfo);
};

NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJRteNodeAdjacentInfoJson, classStyle, insName)
REFL_AUTO_SIMPLE(HJ::HJRteNodeAdjacentNodesJson, nodes)
