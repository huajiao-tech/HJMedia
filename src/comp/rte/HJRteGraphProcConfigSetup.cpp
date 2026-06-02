#include "HJFLog.h"

#include "HJRteGraphProc.h"
#include "HJRteComSource.h"
#include "HJRteComDraw.h"
#include "HJOGBaseShader.h"
#include "HJRteGraphSetupInfo.h"
#include <unordered_map>
#include "HJRteGraphNodeInfo.h"
#include "HJComEvent.h"
#include "HJFacePointsReal.h"

//if detect more than 200, no face-face, flicker, so 200 change to 1000
#define FACE_VALID_TIME_THRESHOLD 1000  //200

NS_HJ_BEGIN

#define HJRteGraphNodeClassStyleIs_Match(src, dst) src == HJRteGraphConfig::dst

int HJRteGraphProcConfigSetup::priSetEnableFaceu(std::shared_ptr<HJRteCom> i_com, bool i_enable, const std::string& i_info)
{
    int i_err = HJ_OK;
    do 
    {
        HJRteComSourceFaceu::Ptr faceu = std::dynamic_pointer_cast<HJRteComSourceFaceu>(i_com);
        if (!faceu)
        {
            break;
        }

        if (i_enable)
        {
			HJBaseParam::Ptr param = HJBaseParam::Create();
            std::string faceInfoSource = faceu->getFaceInfoSource();
			MoreFacePointAcquireFunc faceFunc = [this, faceInfoSource]()
				{
					std::string faceInfo = faceInfoSource.empty() ? "" : getFaceInfo(faceInfoSource);
					std::shared_ptr<HJMoreFacePointsReal> pts = nullptr;
					if (!faceInfo.empty())
					{
						pts = HJMoreFacePointsReal::Create();
						if (pts->deserial(faceInfo) == HJ_OK)
						{
							int64_t systeTime = pts->getSystemTime();
							int64_t timeElapse = abs(HJCurrentSteadyMS() - pts->getSystemTime());
							if (timeElapse > FACE_VALID_TIME_THRESHOLD)
							{
								HJFLogi("{} faceoutput the elapse is:{}, systemTime:{} erase the cacheFaceInfo", getDebugName(), timeElapse, pts->getSystemTime());
								if (!faceInfoSource.empty())
									clearFaceInfo(faceInfoSource);
								pts = nullptr;
							}
						}
					}
                    return pts;
				};
			HJ_CatchMapSetVal(param, MoreFacePointAcquireFunc, faceFunc);

            HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, i_info);
            if (!faceInfoSource.empty())
            {
                HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamFaceInfoSource, faceInfoSource);
            }
			HJ_CatchMapSetVal(param, HJListener, m_renderListener);
			int ret = faceu->resetFaceu(param);
			if (ret < 0)
			{
				if (m_renderListener)
				{
					m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_FACEU_ERROR)));
				}
			}
        }
        else
        {
			faceu->resetFaceu(nullptr);
        }
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeCreate(const std::string& i_nodeInfo)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_nodeInfo]()
            {
                HJFLogi("{} nodeCreate nodeInfo:{}", getDebugName(), i_nodeInfo);
                int ret = HJ_OK;
                do 
                {
					HJRteJsonNode::Ptr nodeInfo = HJRteJsonNode::Create();
					ret = nodeInfo->deserial(i_nodeInfo);
					if (ret < 0)
					{
						HJFLoge("{} nodeCreate nodeInfo init, error", getDebugName());
                        break;
					}
					HJBaseParam::Ptr param = HJBaseParam::Create();
					HJRteCom::Ptr com = priCreateComFromJsonNode(nodeInfo, param);
					if (!com)
					{
						HJFLoge("{} nodeCreate priCreateComFromJsonNode, error", getDebugName());
						ret = HJErrInvalid;
                        break;
					}
                } while (false);
                priCallListener(ret < 0 ? HJVIDEORENDERGRAPH_EVENT_NodeCreateFailed : HJVIDEORENDERGRAPH_EVENT_NodeCreateSuccess);
                return ret;
            });
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName, const std::string& i_shaderStyle, const std::string& i_linkInfo)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_shaderStyle, i_linkInfo]()
            {
                HJFLogi("{} nodeConnect srcClassStyle:{} srcInsName:{} dstClassStyle:{} dstInsName:{} linkInfo:{}", getDebugName(), i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo);
                int ret = HJ_OK;
                do 
                {
					HJRteCom::Ptr srcCom = priGetMapCom(i_srcClassStyle, i_srcInsName);
					if (!srcCom)
					{
						HJFLoge("{} priGetMapCom com is null, classStyle:{} insName:{}", getDebugName(), i_srcClassStyle, i_srcInsName);
						ret = HJErrInvalid;
                        break;
					}
					HJRteCom::Ptr dstCom = priGetMapCom(i_dstClassStyle, i_dstInsName);
					if (!dstCom)
					{
						HJFLoge("{} priGetMapCom com is null, classStyle:{} insName:{}", getDebugName(), i_dstClassStyle, i_dstInsName);
						ret = HJErrInvalid;
                        break;
					}

					if (srcCom == dstCom)
					{
						HJFLoge("{} nodeConnect srcCom == dstCom, error", getDebugName());
						ret = HJErrInvalid;
                        break;
					}

					HJRteJsonLinkInfo linkInfo;
					if (!i_linkInfo.empty())
					{
						ret = linkInfo.deserial(i_linkInfo);
						if (ret < 0)
						{
							HJFLoge("{} nodeConnect linkInfo init, error", getDebugName());
                            break;
						}
					}
					std::pair<std::shared_ptr<HJOGBaseShader>, std::shared_ptr<HJRteComLinkInfo>> pair = priGetShaderAndLinkInfo(i_shaderStyle, linkInfo);
					if (!pair.first && !pair.second)
					{
						ret = HJErrInvalid;
						HJFLoge("{} priGetShaderAndLinkInfo error", getDebugName());
                        break;
					}
					ret = connectCom(srcCom, dstCom, pair.first, pair.second);
					if (ret < 0)
					{
						HJFLoge("{} connectCom error:{}", getDebugName(), ret);
                        break;
					}
                } while (false);
                priCallListener(ret < 0 ? HJVIDEORENDERGRAPH_EVENT_NodeConnectFailed : HJVIDEORENDERGRAPH_EVENT_NodeConnectSuccess);
                return ret;
            });
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info)
{
    int i_err = HJ_OK;
    do 
    {
		i_err = HJRteGraphBaseEGL::asyncOverride([this, i_classStyle, i_insName, i_enable, i_info]()
			{
                HJFLogi("{} nodeEnable classStyle:{} insName:{} enable:{} info:{}", getDebugName(), i_classStyle, i_insName, i_enable, i_info);
                int ret = HJ_OK;
                do 
                {
					HJRteCom::Ptr com = priGetMapCom(i_classStyle, i_insName);
					if (!com)
					{
						HJFLoge("{} setEnable com is null, classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
						ret = HJErrInvalid;
                        break;
					}
					com->setEnable(i_enable);

					if (HJRteGraphNodeClassStyleIs_Match(i_classStyle, HJNodeClass_SourceFaceu))
					{
						ret = priSetEnableFaceu(com, i_enable, i_info);
                        if (ret < 0)
                        {
                            break;
                        }
					}
                } while (false);
                priCallListener(ret < 0 ? HJVIDEORENDERGRAPH_EVENT_NodeEnableFailed : HJVIDEORENDERGRAPH_EVENT_NodeEnableSuccess);
				return ret;
			});
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeSetParam(const std::string& i_classStyle, const std::string& i_insName, const HJBaseParam::Ptr& i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_classStyle, i_insName, i_param]()
            {
                HJFLogi("{} nodeSetParam classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
                int ret = HJ_OK;
                do
                {
                    HJRteCom::Ptr com = priGetMapCom(i_classStyle, i_insName);
                    if (!com)
                    {
                        HJFLoge("{} nodeSetParam com is null, classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
                        ret = HJErrInvalid;
                        break;
                    }
                    ret = com->setParam(i_param);
                } while (false);
                return ret;
            });
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_classStyle, i_insName, i_relink]()
            {
                HJFLogi("{} nodeDelete classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
                int ret = HJ_OK;
                do 
                {
					HJRteCom::Ptr com = priGetMapCom(i_classStyle, i_insName);
					if (!com)
					{
						HJFLoge("{} priGetMapCom com is null, classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
						ret = HJErrInvalid;
                        break;
					}
					ret = removeSingleCom(com, i_relink);
					if (ret < 0)
					{
						HJFLoge("{} nodeDelete removeCom error", getDebugName());
                        break;
					}
                    clearSourceResolutionCache(i_insName);
					priRemoveFromMap(i_classStyle, i_insName);
                } while (false);
                priCallListener(ret < 0 ? HJVIDEORENDERGRAPH_EVENT_NodeDeleteFailed : HJVIDEORENDERGRAPH_EVENT_NodeDeleteSuccess);
                return ret;
            });
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
    const std::string& i_dstClassStyle, const std::string& i_dstInsName, const std::string& i_linkId)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkId]()
            {
                HJFLogi("{} nodeDisconnect srcClassStyle:{} srcInsName:{} dstClassStyle:{} dstInsName:{} linkId:{}", getDebugName(), i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkId);
                int ret = HJ_OK;
                do 
                { 
					HJRteCom::Ptr srcCom = priGetMapCom(i_srcClassStyle, i_srcInsName);
					if (!srcCom)
					{
						HJFLoge("{} nodeDisconnect com is null, src:{}:{}", getDebugName(), i_srcClassStyle, i_srcInsName);
						ret = HJErrInvalid;
                        break;
					}
					HJRteCom::Ptr dstCom = priGetMapCom(i_dstClassStyle, i_dstInsName);
					if (!dstCom)
					{
						HJFLoge("{} nodeDisconnect com is null, dst:{}:{}", getDebugName(), i_dstClassStyle, i_dstInsName);
						ret = HJErrInvalid;
						break;
					}
					ret = breakLinks(srcCom, dstCom, i_linkId);
                    if (ret < 0)
                    {
                        break;
                    }
                } while (false);
                priCallListener(ret < 0 ? HJVIDEORENDERGRAPH_EVENT_NodeDisconnectFailed : HJVIDEORENDERGRAPH_EVENT_NodeDisconnectSuccess);
                return ret;
                
            });
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::priNodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
    const std::string& i_dstClassStyle, const std::string& i_dstInsName,
    const std::string& i_linkInfo)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo]()
            {
                HJFLogi("{} nodeLinkInfoChange srcClassStyle:{} srcInsName:{} dstClassStyle:{} dstInsName:{} i_linkInfo:{}", getDebugName(), i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo);
                int ret = HJ_OK;
                do 
                {
					HJRteCom::Ptr srcCom = priGetMapCom(i_srcClassStyle, i_srcInsName);
					if (!srcCom)
					{
						HJFLoge("{} nodeDisconnect com is null, src:{}:{}", getDebugName(), i_srcClassStyle, i_srcInsName);
						ret = HJErrInvalid;
                        break;
					}
					HJRteCom::Ptr dstCom = priGetMapCom(i_dstClassStyle, i_dstInsName);
					if (!dstCom)
					{
						HJFLoge("{} nodeDisconnect com is null, dst:{}:{}", getDebugName(), i_dstClassStyle, i_dstInsName);
						ret = HJErrInvalid;
                        break;
					}

					HJRteJsonLinkInfo jsonLinkInfo;
                    if (!i_linkInfo.empty())
                    {
                        ret = jsonLinkInfo.deserial(i_linkInfo);
                        if (ret < 0)
                        {
                            HJFLoge("{} nodeLinkInfoChange jsonLinkInfo.init error", getDebugName());
                            break;
                        }
                    }
					HJWindowRenderMode mode = priParseRenderMode(jsonLinkInfo.getRenderMode());
					HJRteComLinkInfo::Ptr linkInfo = HJRteComLinkInfo::Create<HJRteComLinkInfo>(
						(float)jsonLinkInfo.getX(),
						(float)jsonLinkInfo.getY(),
						(float)jsonLinkInfo.getW(),
						(float)jsonLinkInfo.getH(),
						mode,
						jsonLinkInfo.isXMirror(),
						jsonLinkInfo.isYMirror(),
						jsonLinkInfo.getLinkId());
					ret = changeLinkInfo(srcCom, dstCom, linkInfo);
                    if (ret < 0)
                    {
                        HJFLoge("{} changeLinkInfo i_error", getDebugName());
                        break;
                    }
                } while (false);
                priCallListener(ret < 0 ? HJVIDEORENDERGRAPH_EVENT_NodeLinkInfoChangeFailed : HJVIDEORENDERGRAPH_EVENT_NodeLinkInfoChangeSuccess);
                return ret;
                
            });
    } while (false);
    return i_err;
}
int HJRteGraphProcConfigSetup::nodeCreate(const std::string& i_nodeInfo)
{
    return priNodeCreate(i_nodeInfo);
}
int HJRteGraphProcConfigSetup::nodeCreate(HJRteJsonNode& i_nodeCfg)
{
    return priNodeCreate(i_nodeCfg.serial());
}
int HJRteGraphProcConfigSetup::nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource, const std::string& i_resourceUrl, const std::string& i_dependsOn)
{
    HJRteJsonNode node(classStyle, insName, role, enable, isMainSource, i_resourceUrl, i_dependsOn);
	return priNodeCreate(node.serial());
}
int HJRteGraphProcConfigSetup::nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName,
    const std::string& i_shaderStyle, const std::string& i_linkInfo)
{
    return priNodeConnect(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_shaderStyle, i_linkInfo);
}
int HJRteGraphProcConfigSetup::nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName,
    const std::string& i_shaderStyle, HJRteJsonLinkInfo& i_linkInfo)
{
    return priNodeConnect(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_shaderStyle, i_linkInfo.serial());
}

int HJRteGraphProcConfigSetup::nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink) 
{
    return priNodeDelete(i_classStyle, i_insName, i_relink);
}
int HJRteGraphProcConfigSetup::nodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
    const std::string& i_dstClassStyle, const std::string& i_dstInsName, const std::string& i_linkId)
{
    return priNodeDisconnect(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkId);
}
int HJRteGraphProcConfigSetup::nodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info)
{
    return priNodeEnable(i_classStyle, i_insName, i_enable, i_info);
}
int HJRteGraphProcConfigSetup::nodeSetParam(const std::string& i_classStyle, const std::string& i_insName, std::shared_ptr<HJBaseParam> i_param)
{
    return priNodeSetParam(i_classStyle, i_insName, i_param);
}
int HJRteGraphProcConfigSetup::nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
    const std::string& i_dstClassStyle, const std::string& i_dstInsName,
    const std::string& i_linkInfo)
{
    return priNodeLinkInfoChange(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo);
}
int HJRteGraphProcConfigSetup::nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
    const std::string& i_dstClassStyle, const std::string& i_dstInsName,
    HJRteJsonLinkInfo& i_linkInfo)
{
    return priNodeLinkInfoChange(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo.serial());
}
std::string HJRteGraphProcConfigSetup::nodeGetPre(const std::string& i_curClassStyle, const std::string& i_curInsName)
{
    std::string result = "";
    const int ret = HJRteGraphBaseEGL::syncOverride([this, i_curClassStyle, i_curInsName, &result]()
    {
        HJRteCom::Ptr curCom = priGetMapCom(i_curClassStyle, i_curInsName);
        if (!curCom)
        {
            HJFLoge("{} nodeGetPre com is null, classStyle:{} insName:{}", getDebugName(), i_curClassStyle, i_curInsName);
            return HJErrInvalid;
        }

        std::vector<HJRteNodeAdjacentInfo> nodes;
        for (const auto& linkWtr : curCom->getPreQueue())
        {
            HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(linkWtr);
            if (!link)
            {
                continue;
            }
            HJRteCom::Ptr srcCom = link->getSrcComPtr();
            if (!srcCom)
            {
                continue;
            }

            HJRteNodeAdjacentInfo nodeInfo;
            if (!priGetNodeIdentity(srcCom, nodeInfo))
            {
                HJFLoge("{} nodeGetPre identity not found, cur:{}:{} preIns:{}", getDebugName(), i_curClassStyle, i_curInsName, srcCom->getInsName());
                return HJErrInvalid;
            }
            nodes.push_back(nodeInfo);
        }

        result = priSerializeAdjacentNodes(nodes);
        if (result.empty())
        {
            HJFLoge("{} nodeGetPre serialize failed, classStyle:{} insName:{}", getDebugName(), i_curClassStyle, i_curInsName);
            return HJErrFatal;
        }
        return HJ_OK;
    });
    if (ret < 0)
    {
        return "";
    }
    return result;
}
std::string HJRteGraphProcConfigSetup::nodeGetNext(const std::string& i_curClassStyle, const std::string& i_curInsName)
{
    std::string result = "";
    const int ret = HJRteGraphBaseEGL::syncOverride([this, i_curClassStyle, i_curInsName, &result]()
    {
        HJRteCom::Ptr curCom = priGetMapCom(i_curClassStyle, i_curInsName);
        if (!curCom)
        {
            HJFLoge("{} nodeGetNext com is null, classStyle:{} insName:{}", getDebugName(), i_curClassStyle, i_curInsName);
            return HJErrInvalid;
        }

        std::vector<HJRteNodeAdjacentInfo> nodes;
        for (const auto& linkWtr : curCom->getNextQueue())
        {
            HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(linkWtr);
            if (!link)
            {
                continue;
            }
            HJRteCom::Ptr dstCom = link->getDstComPtr();
            if (!dstCom)
            {
                continue;
            }

            HJRteNodeAdjacentInfo nodeInfo;
            if (!priGetNodeIdentity(dstCom, nodeInfo))
            {
                HJFLoge("{} nodeGetNext identity not found, cur:{}:{} nextIns:{}", getDebugName(), i_curClassStyle, i_curInsName, dstCom->getInsName());
                return HJErrInvalid;
            }
            nodes.push_back(nodeInfo);
        }

        result = priSerializeAdjacentNodes(nodes);
        if (result.empty())
        {
            HJFLoge("{} nodeGetNext serialize failed, classStyle:{} insName:{}", getDebugName(), i_curClassStyle, i_curInsName);
            return HJErrFatal;
        }
        return HJ_OK;
    });
    if (ret < 0)
    {
        return "";
    }
    return result;
}

int HJRteGraphProcConfigSetup::procWindow(const std::string &i_classStyle, const std::string &i_insName, const std::shared_ptr<HJOGEGLSurface>& i_eglSurface, int i_state)
{
    int i_err = HJ_OK;
    do
    {
        if ((i_state != HJTargetState_Create) && (i_state != HJTargetState_Destroy))
        {
            break;
        }

        i_err = HJRteGraphBaseEGL::syncOverride([this, i_classStyle, i_insName, i_eglSurface, i_state]()
        {
            bool bEnable = (i_state == HJTargetState_Create) ? true : false;
            int fps = i_eglSurface ? i_eglSurface->getFps() : 30;

            HJRteCom::Ptr com = priGetMapCom(i_classStyle, i_insName);
            if (!com)
            {
                HJFLoge("{} procWindow com is null, classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
                return HJErrInvalid;
            }
            HJRteComDrawEGL::Ptr draw = std::dynamic_pointer_cast<HJRteComDrawEGL>(com);
            if (draw)
            {
                HJFLogi("{} procWindow setSurface, classStyle:{} insName:{} bEnable:{} fps:{}", getDebugName(), i_classStyle, i_insName, bEnable, fps);
                draw->setSurface(i_eglSurface);
                draw->setEnable(bEnable);
                draw->setFps(fps);
            }

            return HJ_OK;
        });
    } while (false);
    return i_err;
}

int HJRteGraphProcConfigSetup::constructGraph(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        std::string config = "";
        HJ_CatchMapPlainGetVal(i_param, std::string, "graphConfigInfo", config);
        if (config.empty())
        { 
            i_err = HJErrInvalidParams;
            HJFLoge("{} graphConfigInfo is empty, error", getDebugName());
            break;
        }   

        i_err = priCreateGraphFromJson(config, i_param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
void HJRteGraphProcConfigSetup::priSetResource(const HJRteJsonNode::Ptr& i_nodeCfg, const HJBaseParam::Ptr& i_param, const std::string& i_paramKey)
{
    if (!i_nodeCfg->getResourceUrl().empty())
    {
        HJ_CatchMapPlainSetVal(i_param, std::string, i_paramKey, i_nodeCfg->getResourceUrl());
    }
}

std::shared_ptr<HJRteCom> HJRteGraphProcConfigSetup::priCreateComFromJsonNode(const HJRteJsonNode::Ptr& i_nodeCfg, const HJBaseParam::Ptr& i_param)
{
    if (i_nodeCfg->getClassStyle().empty())
    {
        HJFLoge("{} priCreateComFromJsonNode node classStyle is empty, error", getDebugName());
        return nullptr;
    }

    if (i_nodeCfg->getInsName().empty())
    {
        i_nodeCfg->setInsName(i_nodeCfg->getClassStyle());
        HJFLogi("{} node insName is empty, set to classStyle name:{}", getDebugName(), i_nodeCfg->getClassStyle());
    }

    std::string key = priGetNodeKey(i_nodeCfg);
    if (m_nodeMap.find(key) != m_nodeMap.end())
    {
        HJFLogi("{} priCreateComFromJsonNode node insName {} already exist, not create again", getDebugName(), i_nodeCfg->getInsName());
        return m_nodeMap[key].lock();
    }

#define HJRteGraphNodeName_Match(namevar) i_nodeCfg->getClassStyle() == HJRteGraphConfig::namevar

    std::shared_ptr<HJRteCom> com = nullptr;

    if (HJRteGraphNodeName_Match(HJNodeClass_SourceBridge))
    {
        com = HJRteComSourceBridge::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SourceBridgeMediaData))
    {
        com = HJRteComSourceBridgeMediaData::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SplitScreen))
    {
        com = HJRteComSourceSplitScreen::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SplitScreenMediaData))
    {
        com = HJRteComSourceSplitScreenMediaData::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SourcePNGSeq))
    {
        com = HJRteComSourceImgSeq::Create();
        priSetResource(i_nodeCfg, i_param, HJRteUtils::ParamUrlImgSeq);
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SourceFaceu))
    {
        com = HJRteComSourceFaceu::Create();
        priSetResource(i_nodeCfg, i_param, HJRteUtils::ParamUrlFaceu);
        if (!i_nodeCfg->getDependsOn().empty())
        {
            HJ_CatchMapPlainSetVal(i_param, std::string, HJRteUtils::ParamFaceInfoSource, i_nodeCfg->getDependsOn());
        }
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SourceImage))
    {
        com = HJRteComSourceImage::Create();    
        priSetResource(i_nodeCfg, i_param, HJRteUtils::ParamUrlImg);
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_SourceImageSeq))
    {
        com = HJRteComSourceImgSeq::Create();
        priSetResource(i_nodeCfg, i_param, HJRteUtils::ParamUrlImgSeq);
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_FilterBlur))
    {
        com = HJRteComDrawBlurCascadeFBO::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_CustomSourceFilter))
    {
        com = HJRteComCustomSourceFilter::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_FilterGray))
    {
        com = HJRteComDrawGrayFBO::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_FilterSR))
    {
        com = HJRteComDrawSRFilter::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_FilterDenoise))
    {
        com = HJRteComDrawDenoiseFilter::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_FilterCopy2D))
    {
        com = HJRteComDrawCopy2DFBO::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_FilterCopyOES))
    {
        com = HJRteComDrawCopyOESFBO::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetUI_0))
    {
        com = HJRteComDrawEGLUI_0::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetUI_1))
    {
        com = HJRteComDrawEGLUI_1::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetUI_2))
    {
        com = HJRteComDrawEGLUI_2::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetUI_3))
    {
        com = HJRteComDrawEGLUI_3::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetEncoder))
    {
        com = HJRteComDrawEGLEncoder::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetImgReceiver))
    {
        com = HJRteComDrawEGLImgReceiver::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetPBODetect))
    {
        com = HJRteComDrawPBOFBODetect::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetPBO_0))
    {
        com = HJRteComDrawPBOFBOTarget_0::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetPBO_1))
    {
        com = HJRteComDrawPBOFBOTarget_1::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetPBO_2))
    {
        com = HJRteComDrawPBOFBOTarget_2::Create();
    }
    else if (HJRteGraphNodeName_Match(HJNodeClass_TargetPBO_3))
    {
        com = HJRteComDrawPBOFBOTarget_3::Create();
    }

    com->setDebugIdx(getDebugIdx());

    HJRteComSource::Ptr source = std::dynamic_pointer_cast<HJRteComSource>(com);
    if (source)
    {
        source->setMainSource(i_nodeCfg->isMainSourceNode());
        source->setDependsOn(i_nodeCfg->getDependsOn());
    }

    if (!i_nodeCfg->getResourceUrl().empty())
    {
        HJ_CatchMapPlainSetVal(i_param, std::string, "resourceUrl", i_nodeCfg->getResourceUrl());
    }

    int i_err = com->init(i_param);
    if (i_err < 0)
    {
        com = nullptr;
    }
    else
    {
        com->setInsName(i_nodeCfg->getInsName());
        com->setEnable(i_nodeCfg->isEnable());
        //fixme lfs, image and imageseq, not use priority, use in background
        com->setUsePriority(false);
        //com->setPriority((HJRteComPriority)i_nodeCfg->priority);

        if (i_nodeCfg->getRole() == HJRteGraphConfig::HJNodeRole_Source)
        {
            com->setRteComType(HJRteComType_Source);
            addSource(com);
        }
        else if (i_nodeCfg->getRole() == HJRteGraphConfig::HJNodeRole_Filter)
        {
            com->setRteComType(HJRteComType_Filter);
            addFilter(com);
        }
        else if (i_nodeCfg->getRole() == HJRteGraphConfig::HJNodeRole_Target)
        {
            com->setRteComType(HJRteComType_Target);
            addTarget(com);
        }
        const std::string key = priGetNodeKey(i_nodeCfg);
        m_nodeMap[key] = com;
        m_nodeIdentityByKey[key] = HJRteNodeAdjacentInfo{i_nodeCfg->getClassStyle(), i_nodeCfg->getInsName()};
    }
    return com;
}
int HJRteGraphProcConfigSetup::priCreateGraphFromJson(const std::string& i_jsonStr, const HJBaseParam::Ptr& i_param)
{
    int i_err = HJ_OK;
    do
    {
        HJRteGraphSetupInfo::Ptr cfg = HJRteGraphSetupInfo::Create();
        i_err = cfg->deserial(i_jsonStr);
        if (i_err < 0)
        {
            HJFLoge("{} priConstructGraphFromJson parse error:{}", getDebugName(), i_err);
            break;
        }

        std::unordered_map<std::string, HJRteJsonNode::Ptr> nodeInsNameMap;
        nodeInsNameMap.reserve(cfg->nodes.size());

        for (auto& nodeCfg : cfg->nodes)
        {
            HJRteCom::Ptr com = priCreateComFromJsonNode(nodeCfg, i_param);
            if (!com)
            {
                HJFLoge("{} priCreateComFromJsonNode error:{}", getDebugName(), i_err);
                i_err = HJErrInvalid;
                break;
            }
            nodeInsNameMap[nodeCfg->getInsName()] = nodeCfg;
        }
        if (i_err < 0)
        {
            break;
        }

        for (auto& linkCfg : cfg->links)
        {
            auto srcIt = nodeInsNameMap.find(linkCfg->getFrom());
            auto dstIt = nodeInsNameMap.find(linkCfg->getTo());
            if (srcIt == nodeInsNameMap.end() || dstIt == nodeInsNameMap.end())
            {
                HJFLoge("{} link node not found, from:{} to:{}", getDebugName(), linkCfg->getFrom(), linkCfg->getTo());
                i_err = HJErrInvalid;
                break;
            }

            HJRteJsonNode::Ptr srcNode = srcIt->second;
            HJRteJsonNode::Ptr dstNode = dstIt->second;
            HJRteCom::Ptr srcCom = priGetMapCom(srcNode->getClassStyle(), srcNode->getInsName());
            HJRteCom::Ptr dstCom = priGetMapCom(dstNode->getClassStyle(), dstNode->getInsName());
            if (!srcCom || !dstCom)
            {
                HJFLoge("{} link com not found, from:{} to:{}", getDebugName(), linkCfg->getFrom(), linkCfg->getTo());
                i_err = HJErrInvalid;
                break;
            }

            std::pair<std::shared_ptr<HJOGBaseShader>, std::shared_ptr<HJRteComLinkInfo>> pair = priGetShaderAndLinkInfo(linkCfg->getShaderType(), linkCfg->getLink());
            if (!pair.first && !pair.second)
            {
                HJFLoge("{} priGetShaderAndLinkInfo error, to:{}", getDebugName(), linkCfg->getTo());
                i_err = HJErrInvalid;
                break;
            }

            i_err = connectCom(srcCom, dstCom, pair.first, pair.second);
            if (i_err < 0)
            {
                HJFLoge("{} connectCom error:{}", getDebugName(), i_err);
                break;
            }
        }
    } while (false);
    return i_err;
}
std::string HJRteGraphProcConfigSetup::priGetNodeKey(const std::string &i_classStyle, const std::string &i_insName)
{
    return HJFMT("{}:{}", i_classStyle, i_insName);
}
std::string HJRteGraphProcConfigSetup::priGetNodeKey(const HJRteJsonNode::Ptr& i_nodeCfg)
{
    return priGetNodeKey(i_nodeCfg->getClassStyle(), i_nodeCfg->getInsName());
}
std::shared_ptr<HJRteCom> HJRteGraphProcConfigSetup::priGetMapCom(const std::string& i_classStyle, const std::string& i_insName)
{
    std::string key = priGetNodeKey(i_classStyle, i_insName);
    if (m_nodeMap.find(key) != m_nodeMap.end())
    {
        HJFLogd("{} find success get from classStyle:{} insName:{}", getInsName(), i_classStyle, i_insName);
        std::shared_ptr<HJRteCom> val = m_nodeMap[key].lock();
        if (!val)
        {
            HJFLoge("{} find success get from classStyle:{} insName:{}, but the com is destroyed, why", getDebugName(), i_classStyle, i_insName);
        }
        return val;
    }
    return nullptr;
}
void HJRteGraphProcConfigSetup::priRemoveFromMap(const std::string& i_classStyle, const std::string& i_insName)
{
    std::string key = priGetNodeKey(i_classStyle, i_insName);
    if (m_nodeMap.find(key) != m_nodeMap.end())
    {
        HJFLogi("{} remove from classStyle:{} insName:{}", getDebugName(), i_classStyle, i_insName);
        m_nodeMap.erase(key);
    }
    m_nodeIdentityByKey.erase(key);
}
bool HJRteGraphProcConfigSetup::priGetNodeIdentity(const std::shared_ptr<HJRteCom>& i_com, HJRteNodeAdjacentInfo& o_info) const
{
    if (!i_com)
    {
        return false;
    }

    const std::string insName = i_com->getInsName();
    for (const auto& item : m_nodeMap)
    {
        std::shared_ptr<HJRteCom> mapCom = item.second.lock();
        if (!mapCom || (mapCom->getInsName() != insName))
        {
            continue;
        }
        if (mapCom.get() != i_com.get())
        {
            continue;
        }

        auto infoIt = m_nodeIdentityByKey.find(item.first);
        if (infoIt == m_nodeIdentityByKey.end())
        {
            return false;
        }
        o_info = infoIt->second;
        return true;
    }

    HJRteNodeAdjacentInfo matchedInfo;
    bool matched = false;
    for (const auto& item : m_nodeIdentityByKey)
    {
        if (item.second.insName != insName)
        {
            continue;
        }
        if (matched)
        {
            return false;
        }
        matchedInfo = item.second;
        matched = true;
    }
    if (!matched)
    {
        return false;
    }
    o_info = matchedInfo;
    return true;
}
std::string HJRteGraphProcConfigSetup::priSerializeAdjacentNodes(const std::vector<HJRteNodeAdjacentInfo>& i_nodes) const
{
    HJRteNodeAdjacentNodesJson result;
    result.nodes.reserve(i_nodes.size());
    for (const auto& nodeInfo : i_nodes)
    {
        result.nodes.emplace_back(nodeInfo);
    }
    return result.serial();
}
std::pair<std::shared_ptr<HJOGBaseShader>, std::shared_ptr<HJRteComLinkInfo>> HJRteGraphProcConfigSetup::priGetShaderAndLinkInfo(const std::string& i_shaderStyle, const HJRteJsonLinkInfo& i_linkInfo)
{
    HJOGBaseShader::Ptr shader = nullptr;
    if (!i_shaderStyle.empty())
    {
        shader = priCreateShaderFromString(i_shaderStyle);//nullptr or shader
        if (!shader)
        {
            HJFLoge("{} create shader error shaderType:{}", getDebugName(), i_shaderStyle);
            return std::make_pair(nullptr, nullptr);
        }
    }
    HJWindowRenderMode mode = priParseRenderMode(i_linkInfo.getRenderMode());
    HJRteComLinkInfo::Ptr linkInfo = HJRteComLinkInfo::Create<HJRteComLinkInfo>(
        (float)i_linkInfo.getX(),
        (float)i_linkInfo.getY(),
        (float)i_linkInfo.getW(),
        (float)i_linkInfo.getH(),
        mode,
        i_linkInfo.isXMirror(),
        i_linkInfo.isYMirror(),
        i_linkInfo.getLinkId());
    return std::make_pair(shader, linkInfo);
}
std::shared_ptr<HJOGBaseShader> HJRteGraphProcConfigSetup::priCreateShaderFromString(const std::string& i_type)
{
#define HJRteGraphNodeShaderType_Match(namevar) i_type == HJRteGraphConfig::namevar

    if (HJRteGraphNodeShaderType_Match(HJNodeShaderType_Copy2D))
    {
        return HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
    }
    if (HJRteGraphNodeShaderType_Match(HJNodeShaderType_CopyOES))
    {
        return HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_OES);
    }
    if (HJRteGraphNodeShaderType_Match(HJNodeShaderType_PreMul_Copy2D))
    {
        return HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
    }
    if (HJRteGraphNodeShaderType_Match(HJNodeShaderType_PreMul_CopyOES))
    {
        return HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_OES);
    }
    if (HJRteGraphNodeShaderType_Match(HJNodeShaderType_SplitScreenLR_2D))
    {
        return HJOGBaseShader::createShader(HJOGBaseShaderType_SplitScreenLR_2D);
    }
    if (HJRteGraphNodeShaderType_Match(HJNodeShaderType_SplitScreenLR_OES))
    {
        return HJOGBaseShader::createShader(HJOGBaseShaderType_SplitScreenLR_OES);
    }
    return nullptr;
}

HJWindowRenderMode HJRteGraphProcConfigSetup::priParseRenderMode(const std::string& i_modeStr)
{
#define HJRteGraphNodeRenderType_Match(namevar) i_modeStr == HJRteGraphConfig::namevar

    if (HJRteGraphNodeRenderType_Match(HJNodeRenderType_Clip))
    {
        return HJWindowRenderMode_CLIP;
    }
    if (HJRteGraphNodeRenderType_Match(HJNodeRenderType_Fit))
    {
        return HJWindowRenderMode_FIT;
    }
    if (HJRteGraphNodeRenderType_Match(HJNodeRenderType_Origin))
    {
        return HJWindowRenderMode_ORIGIN;
    }
    if (HJRteGraphNodeRenderType_Match(HJNodeRenderType_Full))
    {
        return HJWindowRenderMode_FULL;
    }
    return HJWindowRenderMode_CLIP;
}

NS_HJ_END
