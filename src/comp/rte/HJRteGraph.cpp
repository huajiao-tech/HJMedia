#include "HJRteCom.h"
#include "HJRteGraph.h"
#include "HJFLog.h"
#include "HJRteComDraw.h"
#include "HJRteUtils.h"
#include "HJThreadPool.h"
#include "HJBaseUtils.h"    
#include "HJRteComSource.h"
#include "HJOGBaseShader.h"
#include "HJComEvent.h"
#include "HJFboLease.h"
#include "HJRteGraphBaseEGL.h"

NS_HJ_BEGIN

#define HJFLogiNeed(msg,...)           \
        if (m_bNeedStat)  {        \
            HJFLogi(msg,##__VA_ARGS__);\
        }\

int HJRteGraph::m_regularStatTime = 5000;
/////////////////////////////////////////////////////////////////////////////////////////
HJRteGraph::HJRteGraph()
{
    m_regularStatTimer.setInervalTime(m_regularStatTime);
}
HJRteGraph::~HJRteGraph()
{

}
// void HJRteGraph::tesetBottomToTop()
// {
//     HJRteCom::Ptr vidBridgeSource = HJRteCom::Create();
//     vidBridgeSource->setInsName("com_vidBridgeSource");
//     vidBridgeSource->setPriority(HJRteComPriority_VideoSource);
//     m_sources.push_back(vidBridgeSource);

//     HJRteCom::Ptr vidFilterGray = HJRteCom::Create();
//     vidFilterGray->setInsName("com_vidFilter");
//     vidFilterGray->setPriority(HJRteComPriority_VideoGray);
//     m_filters.push_back(vidFilterGray);


//     HJRteCom::Ptr targetUI = HJRteCom::Create();
//     targetUI->setInsName("target_UI");
//     targetUI->setPriority(HJRteComPriority_Target);
//     m_targets.push_back(targetUI);

//     m_links.push_back(vidBridgeSource->addTarget(vidFilterGray, HJOGBaseShader::Create()));
//     m_links.push_back(vidFilterGray->addTarget(targetUI, HJOGBaseShader::Create()));

//     for (auto target : m_targets)
//     {
//         HJRteDriftInfo::Ptr driftInfo = nullptr;
//         priRenderFromBottomToTop(target, driftInfo);
//     }
// }
// int HJRteGraph::testTopToBottom()
// {
//     int i_err = HJ_OK;
//     do
//     {
//         HJRteCom::Ptr vidBridgeSource = HJRteCom::Create();
//         vidBridgeSource->setInsName("com_vidBridgeSource");
//         vidBridgeSource->setPriority(HJRteComPriority_VideoSource);

//         HJRteCom::Ptr vidBridgeFBO = HJRteCom::Create();
//         vidBridgeFBO->setInsName("com_vidBridgeFBO");
//         vidBridgeFBO->setPriority(HJRteComPriority_VideoSource);
//         m_filters.push_back(vidBridgeFBO);

//         HJRteCom::Ptr vidMirror = HJRteCom::Create();
//         vidMirror->setInsName("com_vidMirror");
//         vidMirror->setPriority(HJRteComPriority_Mirror);
//         m_filters.push_back(vidMirror);

//         HJRteCom::Ptr vidBlur = HJRteCom::Create();
//         vidBlur->setInsName("com_vidBlur");
//         vidBlur->setPriority(HJRteComPriority_VideoBlur);
//         m_filters.push_back(vidBlur);

//         //HJRteCom::Ptr vidCombine = HJRteCom::Create();
//         //vidCombine->setInsName("com_vidCombine");
//         //m_coms.push_back(vidCombine);

//         HJRteCom::Ptr giftSource = HJRteCom::Create();
//         giftSource->setInsName("com_giftSource");
//         giftSource->setPriority(HJRteComPriority_Gift);
//         m_filters.push_back(giftSource);

//         HJRteCom::Ptr targetUI = HJRteCom::Create();
//         targetUI->setInsName("target_UI");
//         targetUI->setPriority(HJRteComPriority_Target);
//         m_targets.push_back(targetUI);

//         HJRteCom::Ptr targetPusher = HJRteCom::Create();
//         targetPusher->setInsName("targetPusher");
//         targetPusher->setPriority(HJRteComPriority_Target);
//         m_targets.push_back(targetPusher);


// #if 0
//         m_links.push_back(vidBridgeSource->addTarget(vidBridgeFBO));
//         m_links.push_back(vidBridgeFBO->addTarget(vidBlur));

//         m_links.push_back(vidBlur->addTarget(targetUI, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 0.5, 1.0)));
//         m_links.push_back(vidBlur->addTarget(targetUI, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.5, 1.0, 1.0)));

//         m_links.push_back(vidBlur->addTarget(vidMirror));
//         m_links.push_back(vidMirror->addTarget(targetPusher));

//         m_links.push_back(giftSource->addTarget(targetUI));
//         m_links.push_back(giftSource->addTarget(targetPusher));
// #else 
//         m_links.push_back(vidBridgeSource->addTarget(vidMirror));
//         m_links.push_back(giftSource->addTarget(targetPusher));
//         m_links.push_back(vidMirror->addTarget(targetUI));
//         m_links.push_back(vidMirror->addTarget(targetPusher));

//         //m_links.push_back(vidBridgeSource->addTarget(vidBlur, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 0.5, 1.0)));
//         //m_links.push_back(vidBlur->addTarget(targetUI));

//         //m_links.push_back(giftSource->addTarget(vidMirror, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.5, 1.0, 1.0)));
//         //m_links.push_back(vidMirror->addTarget(vidBlur));
//         //m_links.push_back(vidBlur->addTarget(targetPusher));
// #endif

//         m_sources.push_back(vidBridgeSource);
//         m_sources.push_back(giftSource);

//         for (auto source : m_sources)
//         {
//             //source is ready, textureId, (OES,2D), etc.

//             //priRenderFromTopToBottom(source, nullptr);
//         }

//         priRemoveCom(targetPusher);
//         //priRemoveCom(targetPusher);
//         //priRemoveCom(vidBlur);

//         priReset();

//         HJFLogi("");
//         HJFLogi("{} next draw begin", getInsName());
//         HJFLogi("");

//         for (auto source : m_sources)
//         {
//             //source is ready, textureId, (OES,2D), etc.

//             //priRenderFromTopToBottom(source, nullptr);
//         }

//     } while (false);
//     return i_err;
// }

int HJRteGraph::init(HJBaseParam::Ptr i_param)
{
    if (i_param)
    {
        HJ_CatchMapGetVal(i_param, HJRteGraphLinkRenderCb, m_linkRenderCb);
        HJ_CatchMapGetVal(i_param, HJRteGraphFrameStatCb, m_frameStatCb);
    }
    return HJ_OK;
}
void HJRteGraph::graphReset()
{
    priReset();    
}
// int HJRteGraph::renderFromTopToBottom(std::shared_ptr<HJRteCom> i_sourceCom, std::shared_ptr<HJRteDriftInfo> i_driftInfo)
// {
//     return priRenderFromTopToBottom(i_sourceCom, i_driftInfo);
// }
int HJRteGraph::renderFromBottomToTop(std::shared_ptr<HJRteCom> i_com, std::shared_ptr<HJRteDriftInfo>& o_driftInfo)
{
    //HJFLogi("{} renderFromBottomToTop enter com: {}", getInsName(), i_com->getInsName());
    return priRenderFromBottomToTop(i_com);
}
std::shared_ptr<HJRteComSource> HJRteGraph::findSourceByInsName(const std::string& i_insName) const
{
    if (i_insName.empty())
        return nullptr;
    for (const auto& com : m_sources)
    {
        auto source = std::dynamic_pointer_cast<HJRteComSource>(com);
        if (source && source->getInsName() == i_insName)
            return source;
    }
    return nullptr;
}
void HJRteGraph::priAdjustResolutionFromComRecursive(const std::shared_ptr<HJRteCom>& i_com, int i_width, int i_height, std::unordered_set<HJRteCom*>& io_visited)
{
    if (!i_com)
        return;
    if (!io_visited.insert(i_com.get()).second)
        return;

    i_com->foreachNextLink([this, i_width, i_height, &io_visited](const std::shared_ptr<HJRteComLink>& i_link)
        {
            if (!i_link)
                return HJ_OK;
            auto dst = i_link->getDstComPtr();
            if (!dst)
                return HJ_OK;
            HJFLogi("{} comname:{} update resolution: {}x{}", getDebugName(), dst->getInsName(), i_width, i_height);
            dst->adjustResolution(i_width, i_height);
            priAdjustResolutionFromComRecursive(dst, i_width, i_height, io_visited);
            return HJ_OK;
        });
}
void HJRteGraph::adjustResolutionFromSourceChain(const std::shared_ptr<HJRteComSource>& i_source, int i_width, int i_height)
{
    if (!i_source || i_width <= 0 || i_height <= 0)
        return;
    HJFLogi("{} source:{} update resolution: {}x{}", getDebugName(), i_source->getInsName(), i_width, i_height);
    i_source->adjustResolution(i_width, i_height);
    std::unordered_set<HJRteCom*> visited;
    priAdjustResolutionFromComRecursive(i_source, i_width, i_height, visited);
}
void HJRteGraph::statRefCntFromTopToBottom(std::shared_ptr<HJRteCom> i_com)
{
    priStatRefCntFromTopToBottom(i_com);
}
int HJRteGraph::changeLinkInfo(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, const std::shared_ptr<HJRteComLinkInfo>& i_linkInfo)
{
    int i_err = HJ_OK;
    do
    {
        //HJFLogi("{} changeLinkInfo links enter com: <{} -> {}>", getInsName(), i_srcCom->getInsName(), i_dstCom->getInsName());
        //for (auto link : m_links)
        //{
        //    if (link->getLinkInfo() && (link->getLinkInfo()->m_linkId == i_linkInfo->m_linkId))
        //    {
        //        link->setLinkInfo(i_linkInfo);
        //        HJFLogi("{} changeLinkInfo setLinkInfo links change linkInfo: <{} -> {}>", getInsName(), i_srcCom->getInsName(), i_dstCom->getInsName());
        //        //break;
        //    }
        //}
		auto tmpQueue = i_srcCom->getNextQueue();
		for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
		{
			HJRteComLink::Ptr link = it->lock();
			if (link)
			{
				HJRteCom::Ptr nextCom = link->getDstComPtr();
				if (nextCom && (nextCom == i_dstCom))
				{
					if (link->getLinkInfo() && (link->getLinkInfo()->m_linkId == i_linkInfo->m_linkId))
					{
						HJFLogi("{} changeLinkInfo setLinkInfo links change linkInfo: <{} -> {}>", getDebugName(), i_srcCom->getInsName(), i_dstCom->getInsName());
						link->setLinkInfo(i_linkInfo);
					}
				}
			}
		}
    } while (false);
    return i_err;
}
int HJRteGraph::breakLinks(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, const std::string& i_linkId)
{
    int i_err = HJ_OK;
    do
    {
        HJFLogi("{} break links enter com: <{} -> {}>", getDebugName(), i_srcCom->getInsName(), i_dstCom->getInsName());
        auto tmpQueue = i_srcCom->getNextQueue();
        for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
        {
            HJRteComLink::Ptr link = it->lock();
            if (link)
            {
                HJRteCom::Ptr nextCom = link->getDstComPtr();
                if (nextCom && (nextCom == i_dstCom))
                {
                    if (!link->getLinkInfo() || i_linkId.empty() || link->getLinkInfo()->m_linkId == i_linkId)
                    {
                        HJFLogi("{} break link: {} -> {} linkId:{}", getDebugName(), link->getSrcComName(), link->getDstComName(), link->getLinkInfo() ? link->getLinkInfo()->m_linkId : "");
                        link->breakLink();
                        removeFromLinks(link);
                    }
                }            
            }
        }
    } while (false);
    return i_err;
}

void HJRteGraph::priReset()
{
    for (auto link : m_links)
    {
        link->setReady(false);
    }
    for (auto source : m_sources)
    {
        source->reset();
    }    
    for (auto filter : m_filters)
    {
        filter->reset();    
    }
    for (auto target : m_targets)
    {
        target->reset();
    }    
}
int HJRteGraph::foreachSource(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        for (auto source : m_sources)
        {
            i_err = i_func(source);
            if (i_err < 0)
            {
                break;
            }
            ////source is ready, textureId, (OES,2D), etc.
            //priRenderFromTopToBottom(source);
        }
    } while (false);
    return i_err;
}

int HJRteGraph::foreachFilter(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        for (auto filter : m_filters)
        {
            i_err = i_func(filter);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
}

int HJRteGraph::foreachTarget(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        for (auto target : m_targets)
        {
            i_err = i_func(target);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
}
int HJRteGraph::foreachComFromName(const std::string& i_name, std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func)
{
    int i_err = HJ_OK;
    std::deque<HJRteCom::Ptr> values;
    for (const auto& obj : m_targets)
    {
        if (obj->getInsName() == i_name)
        {
            values.push_back(obj);
        }
    }
    for (const auto& obj : m_filters)
    {
        if (obj->getInsName() == i_name)
        {
            values.push_back(obj);
        }
    }
    for (const auto& obj : m_sources)
    {
        if (obj->getInsName() == i_name)
        {
            values.push_back(obj);
        }
    }
    for (auto val : values)
    {
        i_err = i_func(val);
        if (i_err < 0)
        {
            break;
        }
    }
    return i_err;
}
int HJRteGraph::foreachAllCom(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        i_err = foreachSource(i_func);
        if (i_err < 0)
        {
            break;
        }
        i_err = foreachFilter(i_func);
        if (i_err < 0)
        {
            break;
        }
        i_err = foreachTarget(i_func);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

int HJRteGraph::priRender(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do 
    {
        //attach
        i_err = i_com->foreachPreLink([this](const std::shared_ptr<HJRteComLink>& i_link)
            {
                int ret = HJ_OK;
                do
                {
                    std::string linkName = i_link->getInsName();
                    HJFLogi("{} com:<{} - {}> info:{} render sub #########", getDebugName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                } while(false);
                return ret;
            });
        if (i_err < 0)
        {
            break;
        }
        //detach
    } while (false);
    return i_err;
}


bool HJRteGraph::priStatRefCntFromTopToBottom(std::shared_ptr<HJRteCom> i_com)
{
    if (i_com->isTarget())
    {
        return i_com->isEnable();
    }
    int nRef = 0;
    int i_err = i_com->foreachNextLink([this, &nRef](const std::shared_ptr<HJRteComLink>& i_link)
        {
            int ret = HJ_OK;
            do
            {
                // only really render path;
                if (!i_link->isDrawEnable())
                {
                    break;
                }
                HJRteCom::Ptr dstCom = i_link->getDstComPtr();
                if (dstCom)
                {
                    bool bEnable = priStatRefCntFromTopToBottom(dstCom);
                    if (bEnable)
                    {
                        nRef++;
                    }
                }
            } while (false);
            return ret;
        });

    i_com->refCntSet(nRef);
    return (nRef > 0);
}

bool HJRteGraph::priTargetRender(const std::shared_ptr<HJRteCom>& i_com)
{
    bool bRender = true;
    do
    {
        if (i_com->isTarget() && !i_com->isEnable())
        {
            HJFLogiNeed("{} this target: {} not enable, not render", getDebugName(), i_com->getInsName());
            bRender = false;
            break;
        }
    } while (false);
    return bRender;
}
int HJRteGraph::priBind(const std::shared_ptr<HJRteCom>& i_com)
{
    int i_err = HJ_OK;
    do
    {
        if (i_com->isEnable()) // (filter && isEnable) || target
        {
            i_err = i_com->bind();
            if (i_err < 0)
            {
                break;
            }
            HJFLogiNeed("{} this :{} bind", getDebugName(), i_com->getInsName());
        }
    } while (false);
    return i_err;
}
int HJRteGraph::priUnBind(const std::shared_ptr<HJRteCom>& i_com, bool bDraw)
{
    int i_err = HJ_OK;
    do
    {
        if (i_com->isEnable()) // (filter && isEnable) || target
        {
            HJFLogiNeed("{} this :{} unbind set avaiable:{} ", getDebugName(), i_com->getInsName(), bDraw);
            i_err = i_com->unbind(bDraw);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
}
int HJRteGraph::priTrySourceProc(const std::shared_ptr<HJRteCom>& i_com)
{
    int i_err = HJ_OK;
    do
    { 
        HJRteComSource::Ptr vidSource = std::dynamic_pointer_cast<HJRteComSource>(i_com);

        if (!vidSource->isEnable())
        {
            break;
        }

        if (vidSource->IsStateReady())
        {
            HJRteDriftInfo::Ptr driftInfo = HJRteDriftInfo::Create<HJRteDriftInfo>(vidSource->getInsName(), vidSource->getTextureId(), vidSource->getTextureType(), vidSource->getWidth(), vidSource->getHeight(), vidSource->getTexMatrix());
			auto fbo = vidSource->takeFbo();
			if (fbo)
			{
				auto lease = HJFboLease::Create<HJFboLease>(fbo->getInsName(), HJRteGraphBaseEGL::getFBOCtrlPool(), std::move(fbo), m_bNeedStat);
                driftInfo->attachFboLease(lease);
			}
            vidSource->setDriftInfo(driftInfo);
            HJFLogiNeed("{} this source: {} set ready", getDebugName(), i_com->getInsName());
        }
    } while (false);
    return i_err;
}
void HJRteGraph::priSetComDrift(const std::shared_ptr<HJRteCom>& i_com, std::shared_ptr<HJRteDriftInfo>& i_driftInfo)
{
    if (i_com->isFilter())
    {
        HJRteDriftInfo::Ptr drift = nullptr;
        if (i_driftInfo)
        {
            drift = std::move(i_driftInfo);
        }
        else
        {
            //Create a drift and transfer the FBO ownership to the drift lease. After destruction, the pool will be automatically returned
            drift = HJRteDriftInfo::Create<HJRteDriftInfo>(i_com->getInsName(), i_com->getTextureId(), HJRteTextureType_2D, i_com->getWidth(), i_com->getHeight());
            HJRteComDrawFBO::Ptr fboCom = std::dynamic_pointer_cast<HJRteComDrawFBO>(i_com);
            if (fboCom)
            {
                auto fbo = fboCom->takeFbo();
                if (fbo)
                {
                    auto lease = HJFboLease::Create<HJFboLease>(fboCom->getInsName(), HJRteGraphBaseEGL::getFBOCtrlPool(), std::move(fbo), m_bNeedStat);
                    drift->attachFboLease(lease);
                }
            }          
        }
        i_com->setDriftInfo(std::move(drift));
    }
}
void HJRteGraph::priDebugLinkFlow(const std::shared_ptr<HJRteComLink>& i_link, bool i_bOnlyCopy)
{
    if (m_bNeedStat && m_linkRenderCb)
    {
        const auto linkInfo = i_link->getLinkInfo();
        const std::string linkId = linkInfo ? linkInfo->m_linkId : "";
        m_linkRenderCb(linkId, i_link->getSrcComName(), i_link->getDstComName(), i_bOnlyCopy);
    }
}
int HJRteGraph::priRenderFromBottomToTop(std::shared_ptr<HJRteCom> i_end)
{
    int i_err = HJ_OK;
    do 
    {
		if (!priTargetRender(i_end))
        {
			break;
		}

        if (i_end->isTarget() || i_end->isFilter())
        {    		
            i_err = priBind(i_end);
            if (i_err < 0)
            {
                break;
            }
     
            bool bDraw = false;
            HJRteDriftInfo::Ptr o_driftInfo = nullptr;
            i_err = i_end->foreachPreLink([this, &i_end, &bDraw, &o_driftInfo](const std::shared_ptr<HJRteComLink>& i_link)
            {
                int ret = HJ_OK;
				do
				{
                    if (!i_link->isDrawEnable()) //for example gift->ui  gift !-> encoder;
                    {
                        HJFLogiNeed("{} com:<{} - {}> info:{} This is link is not draw enable, not draw=====================", getDebugName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                        break;
                    }
					HJRteCom::Ptr srcCom = i_link->getSrcComPtr();
                    HJRteDriftInfo::Ptr driftInfo = srcCom->getDriftInfo();
                    if (driftInfo)
                    {
                        HJFLogiNeed("{} pre is ready direct copy dirft, srcCom:{} w:{} h:{}", getDebugName(), srcCom->getInsName(), srcCom->getWidth(), srcCom->getHeight());
                    }
                    else
                    {
                        ret = priRenderFromBottomToTop(srcCom);
                        if (ret < 0)
                        {
                            HJFLoge("{} com:<{} - {}> priRenderFromBottomToTop error", getDebugName(), i_link->getSrcComName(), i_link->getDstComName());
                            break;
                        }
                        driftInfo = srcCom->getDriftInfo();
                        if (!driftInfo)
                        {
                            HJFLogiNeed("{} com:<{} - {}> not ready no draw", getDebugName(), i_link->getSrcComName(), i_link->getDstComName());
                            ret = HJ_WOULD_BLOCK;
                            break;
                        }
                    }

                    bDraw = true;
                    if (!i_end->isEnable()) //filter is not enable, must only one input, priCheckGraph will check it;
                    {
                        o_driftInfo = driftInfo;
                        priDebugLinkFlow(i_link, true);
                        break;
                    }

                    i_end->render(i_link, driftInfo);// HJRteCom::Ptr target = i_link->getDstComPtr(); ret = target->render(i_link, o_driftInfo); //target=i_end 
					if (ret < 0)
					{
						HJFLoge("{} com:<{} - {}>  render error", getDebugName(), i_link->getSrcComName(), i_link->getDstComName());
						break;
					}
                    HJFLogiNeed("{} real render, curCom:{} draw:<{} - {}> info:{} ", getDebugName(), i_link->getSrcComName(), driftInfo->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                    priDebugLinkFlow(i_link, false);                    					
				} while (false);
				return ret;
            });

            i_err = priUnBind(i_end, bDraw);
            if (i_err < 0)
			{
				break;
			}
            
            if (bDraw)
            {
                priSetComDrift(i_end, o_driftInfo);                  
            }           
        }
        else if (i_end->isSource())
        {
            priTrySourceProc(i_end);
        }
    } while (false);
    return i_err;
}

// int HJRteGraph::priRenderFromTopToBottom(std::shared_ptr<HJRteCom> i_header, std::shared_ptr<HJRteDriftInfo> i_driftInfo)
// {
//     int i_err = HJ_OK;
//     do 
//     {
//         std::string comName = i_header->getInsName();
//         HJFLogi("{} com:{} enter", getInsName(), comName);
//         //notify all next links ready
//         i_err = i_header->foreachNextLink([this, i_driftInfo](const std::shared_ptr<HJRteComLink>& i_link)
//             {
//                 int ret = HJ_OK;
//                 do
//                 {
//                     std::string linkName = i_link->getInsName();
//                     HJFLogi("{} notify com:<{} - {}> ready", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
//                     //notify next ready
//                     i_link->setReady(true);
//                     i_link->setDriftInfo(i_driftInfo);
//                     //com may be have more input, so must just all ready, then render
//                     HJRteCom::Ptr com = i_link->getDstComPtr();
//                     if (com && com->isAllPreReady())
//                     {
//                         std::string comName = com->getInsName();
//                         HJFLogi("{} com:<{} - {}> info:{} render =======================", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
//                         //pri render to target
// //                        ret = priRender(com);
// //                        if (ret < 0)
// //                        {
// //                            HJFLoge("{} com:<{} - {}>  render error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
// //                            break;
// //                        }
//                         HJRteComDraw::Ptr target = HJ_CvtDynamic(HJRteComDraw, com);
//                         if (!target)
//                         {
//                             ret = HJErrFatal;
//                             break;
//                         }
                    
//                         HJRteDriftInfo::Ptr driftInfo = nullptr;
//                         ret = target->render(driftInfo);
//                         if (ret < 0)
//                         {
//                             HJFLoge("{} com:<{} - {}>  render error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
//                             break;
//                         }
                    
//                         ret = priRenderFromTopToBottom(com, driftInfo); //fixme after
//                         if (ret < 0)
//                         {
//                             HJFLoge("{} com:<{} - {}> priRenderFromTopToBottom error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
//                             break;
//                         }
//                     }
//                     else
//                     {
//                         HJFLogi("{} com:<{} - {}> not ready, so not render---->", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
//                     }
//                 } while (false);           
//                 return ret;
//             });

//         HJFLogi("{} com:{} end i_err:{}", getInsName(), comName, i_err);

//         if (i_err < 0)
//         {
//             break;
//         }
//     } while (false);
//     return i_err;
// }
void HJRteGraph::removeFromLinks(std::shared_ptr<HJRteComLink> i_link)
{
	for (auto it = m_links.begin(); it != m_links.end(); ++it)
	{
		if ((*it) == i_link)
		{
			m_links.erase(it);
            HJFLogi("");
            HJFLogi("{} removeFromLinks:{}", getDebugName(), i_link->getInsName());
            HJFLogi("");
			break;
		}
	}
}
void HJRteGraph::removeFromComs(std::shared_ptr<HJRteCom> i_com)
{
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        if ((*it) == i_com)
        {
            HJFLogi("");
            HJFLogi("{} remove from sources:{}", getDebugName(), i_com->getInsName());
            HJFLogi("");
            m_sources.erase(it);
            break;
        }
    }
    
    for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
    {
        if ((*it) == i_com)
        {
            HJFLogi("");
            HJFLogi("{} remove from filters:{}", getDebugName(), i_com->getInsName());
            HJFLogi("");
            m_filters.erase(it);
            break;
        }
    }
    
    for (auto it = m_targets.begin(); it != m_targets.end(); ++it)
    {
        if ((*it) == i_com)
        {
            HJFLogi("");
            HJFLogi("{} remove from targets:{}", getDebugName(), i_com->getInsName());
            HJFLogi("");
            m_targets.erase(it);
            break;
        }
    }    
}
//void HJRteGraph::removeFromSources(std::shared_ptr<HJRteCom> i_com)
//{
//	for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
//	{
//		if ((*it) == i_com)
//		{
//			m_sources.erase(it);
//			break;
//		}
//	}
//}
int HJRteGraph::priRemoveSource(std::shared_ptr<HJRteCom> i_com, bool i_bRecursive)
{
    int i_err = HJ_OK;
    do
    {
        //std::deque<HJRteComLink::Wtr> tmpQueue = HJBaseUtils::copyDeque<HJRteComLink::Wtr>(i_com->getNextQueue());
        auto tmpQueue = i_com->getNextQueue();
        for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
        {
            HJRteComLink::Ptr link = it->lock();
            if (link)
            {
                link->breakLink();
                removeFromLinks(link);

                if (i_bRecursive)
                {
                    HJRteCom::Ptr dst = link->getDstComPtr();
                    if (!dst)
                    {
                        break;
                    }
                    //if (dst->isPreEmpty()) //if is source, recursive remove again;
                    if (dst->isSource())
                    {
                        i_err = priRemoveSource(dst);
                        if (i_err < 0)
                        {
                            break;
                        }
                    }
                }
            }
        }
        tmpQueue.clear();
        i_com->clearNext();
        if (i_com->isNextEmpty())
        {
            removeFromComs(i_com);
        }
    } while (false);
    return i_err;
}
int HJRteGraph::priRemoveTarget(std::shared_ptr<HJRteCom> i_com, bool i_bRecursive)
{
	int i_err = HJ_OK;
	do
	{
		//std::deque<HJRteComLink::Wtr> tmpQueue = HJBaseUtils::copyDeque<HJRteComLink::Wtr>(i_com->getPreQueue());
        auto tmpQueue = i_com->getPreQueue();
		for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
		{
			HJRteComLink::Ptr link = it->lock();
			if (link)
			{
				link->breakLink();
				removeFromLinks(link);

                if (i_bRecursive)
                {
                    HJRteCom::Ptr src = link->getSrcComPtr();
                    if (!src)
                    {
                        break;
                    }

                    //if (src->isNextEmpty()) //if is target, recursive remove again;
                    if (src->isTarget())
                    {
                        i_err = priRemoveTarget(src);
                        if (i_err < 0)
                        {
                            break;
                        }
                    }
                }				
			}
		}
        tmpQueue.clear();
		i_com->clearPre();
		if (i_com->isPreEmpty())
		{
			removeFromComs(i_com);
		}
	} while (false);
	return i_err;
}
int HJRteGraph::priRemoveFilter(std::shared_ptr<HJRteCom> i_com, bool i_bReLink)
{
    int i_err = HJ_OK;
    do
    {
        const std::deque<HJRteComLink::Wtr>& preQueue = i_com->getPreQueue();
        HJRteComLink::Ptr preLink = HJRteComLink::GetPtrFromWtr(*preQueue.begin());
        if (preLink)
        {
            HJRteCom::Ptr src = preLink->getSrcComPtr();
            HJFLogi("{} remove prelink:{}", getDebugName(), preLink->getInsName());
            i_err = preLink->breakLink();
            if (i_err < 0)
            {
                break;
            }
            removeFromLinks(preLink);

            //std::deque<HJRteComLink::Wtr> tmpQueue = HJBaseUtils::copyDeque<HJRteComLink::Wtr>(i_com->getNextQueue());
            auto tmpQueue = i_com->getNextQueue();
            for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
            {
                HJRteComLink::Ptr oldLink = HJRteComLink::GetPtrFromWtr(*it);
                if (oldLink)
                {
                    HJRteCom::Ptr dst = oldLink->getDstComPtr();
                    oldLink->breakLink();
                    HJFLogi("{} old post link:{} shader:{} linkInfo:{}", getDebugName(), oldLink->getInsName(), oldLink->getShader() ? " yes " : " no ", oldLink->getLinkInfo() ? " yes " : " no ");

                    if (i_bReLink)
                    {
                        m_links.push_back(src->addTarget(dst, oldLink->getShader(), oldLink->getLinkInfo()));
                    }
                    
                    removeFromLinks(oldLink);
                }
            }
            tmpQueue.clear();
        }

        removeFromComs(i_com);
    } while (false);    
    return i_err;
}

int HJRteGraph::insertFilterAfter(std::shared_ptr<HJRteCom> i_com, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_dstShader, std::shared_ptr<HJRteComLinkInfo> i_dstLinkInfo)
{
    int i_err = HJ_OK;
    do
    {
         HJRteComType type = i_com->getRteComType();
         if (type == HJRteComType_Filter)
         {
             if ((i_com->getPreCount() != 1) && (i_com->getNextCount() != 1))
             {
                 i_err = -1;
                 HJFLoge("{} this filter after insert filter is not support", getDebugName());
                 break;
             }
            
            HJRteComLink::Ptr oldLink = HJRteComLink::GetPtrFromWtr(i_com->getNextQueue().front());
            if (!oldLink)
            {
                i_err = -1;
                HJFLoge("{} this filter after insert filter is not support", getDebugName());
                break;
            }


            HJRteCom::Ptr A = oldLink->getSrcComPtr();
            HJRteCom::Ptr C = oldLink->getDstComPtr();

            i_err = oldLink->breakLink();
            if (i_err < 0)
            {
                break;
            }

            m_links.push_back(A->addTarget(i_dstCom, i_dstShader, i_dstLinkInfo));

            HJFLogi("{} insert A->B {} -> {} shader:{} linkInfo:{}", getDebugName(), A->getInsName(), i_dstCom->getInsName(), i_dstShader ? " yes " : " no ", i_dstLinkInfo ? " yes " : " no ");

            m_links.push_back(i_dstCom->addTarget(C, oldLink->getShader(), oldLink->getLinkInfo()));

            HJFLogi("{} insert B->C {} -> {} shader:{} linkInfo:{}", getDebugName(), i_dstCom->getInsName(), C->getInsName(), oldLink->getShader() ? " yes " : " no ", oldLink->getLinkInfo() ? " yes " : " no ");

            removeFromLinks(oldLink);
         }
    } while (false);    
    return i_err;   
}

int HJRteGraph::priRecursiveRemoveCom(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do
    {
        HJFLogi("{} priRemoveCom com:{}", getDebugName(), i_com->getInsName());

        HJRteComType type = i_com->getRteComType();
        if (type == HJRteComType_Source)
        {
            i_err = priRemoveSource(i_com);
            if (i_err < 0)
            {
                break;
            }
        }
        else if (type == HJRteComType_Filter)
        {
            if (i_com->getPreCount() != 1)
            {
                i_err = -1;
                HJFLoge("{} this filter is not support, more input, remove cannot relink", getDebugName());
                break;
            }
            i_err = priRemoveFilter(i_com);
            if (i_err < 0)
            {
                break;
            }
        }
        else if (type == HJRteComType_Target)
        {
			i_err = priRemoveTarget(i_com);
			if (i_err < 0)
			{
				break;
			}
        }
    } while (false);
    return i_err;
}
// int HJRteGraph::removeTarget(std::shared_ptr<HJRteCom> i_com)
// {
//     return priRemoveTarget(i_com);
// }
// int HJRteGraph::removeFilter(std::shared_ptr<HJRteCom> i_com)
// {
//     return priRemoveFilter(i_com);
// }
// int HJRteGraph::removeSource(std::shared_ptr<HJRteCom> i_com)
// {
//     return priRemoveSource(i_com);
// }
int HJRteGraph::removeRecursiveCom(std::shared_ptr<HJRteCom> i_com)
{
    return priRecursiveRemoveCom(i_com);
}
int HJRteGraph::removeSingleCom(std::shared_ptr<HJRteCom> i_com, bool i_bReLink)
{
    HJFLogi("{} removeSingleCom com:{}", getDebugName(), i_com->getInsName());
    int i_err = HJ_OK;
    do
    {
        HJRteComType type = i_com->getRteComType();
        if (type == HJRteComType_Source)
        {
            i_err = priRemoveSource(i_com, false);
            if (i_err < 0)
            {
                break;
            }
        }
        else if (type == HJRteComType_Filter)
        {
            if (i_com->getPreCount() != 1)
            {
                i_err = -1;
                HJFLoge("{} this filter is not support, more input, remove cannot relink", getDebugName());
                break;
            }
            i_err = priRemoveFilter(i_com, i_bReLink);
            if (i_err < 0)
            {
                break;
            }
        }
        else if (type == HJRteComType_Target)
        {
            i_err = priRemoveTarget(i_com, false);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
    
}
void HJRteGraph::comDone()
{
    foreachAllCom([this](const std::shared_ptr<HJRteCom>& i_com)
        {
            HJFLogi("{} com name:{} done", getDebugName(), i_com->getDebugName());
            i_com->done();
            return HJ_OK;
        });
    m_targets.clear();
    m_filters.clear();
    m_sources.clear();
    m_links.clear();
    HJFLogi("{} rte graph all deque clear", getDebugName());
}
void HJRteGraph::done()
{
    
}
//////////////////////////////////////////////////////////////////////
int HJRteGraphDrive::s_driveId = 999;

HJRteGraphDrive::HJRteGraphDrive()
{
    m_threadTimer = HJTimerThreadPool::Create();
}
HJRteGraphDrive::~HJRteGraphDrive()
{

}
int HJRteGraphDrive::manualDrive()
{
    int i_err = HJ_OK;
    do
    {
        if (!m_threadTimer)
        {
            i_err = -1;
            break;
        }
        m_threadTimer->asyncClear([this]()
            {
                return priDetailRun();
            }, s_driveId);

    } while (false);
    return i_err;
}
int HJRteGraphDrive::priDetailRun()
{
    int ret = 0;
    m_regularStatTimer.proc([this]()
        {
            m_bNeedStat = true;
        });
    ret = run();
    m_bForceUpdateOnce = false;
    m_bNeedStat = false;
    m_renderStatIdx++;
    return ret;
}
void HJRteGraphDrive::priCallListener(const size_t identify, const int64_t val, const std::string& msg)
{
	if (m_renderListener)
	{
		m_renderListener(std::move(HJMakeNotification(identify, val, msg)));
	}
}
int HJRteGraphDrive::init(std::shared_ptr<HJBaseParam> i_param)
{
    int i_err = 0;
    do
	{
        i_err = HJRteGraph::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        HJ_CatchMapPlainGetVal(i_param, int, HJBaseParam::s_paramFps, m_fps);
        HJ_CatchMapPlainGetVal(i_param, bool, HJBaseParam::s_paramIsManulDrive, m_bManulDrive);
        HJFLogi("{} graph init fps:{} manuldrive:{}", getDebugName(), m_fps, m_bManulDrive);
        if (!m_bManulDrive && (m_fps <= 0))
        {
            i_err = -1;
            HJFLoge("{} manuldrive is false, but fps: {} is invalid", getDebugName(), m_fps);
            break;
        }
        if (m_fps <= 0)
        {
            m_fps = 30;
        }
        m_threadTimer->setErrorFunc([this]
            {
                //every function proc owner error, not callback is this;
                //priCallListener(HJVIDEORENDERGRAPH_EVENT_ERROR_DEFAULT);
                HJFLoge("{} thread error", getDebugName());
                return 0;
            });
		i_err = m_threadTimer->startTimer(1000 / m_fps, [this]()
			{
				return priDetailRun();
			}, m_bManulDrive);
        
        if (i_err < 0)
        {
            break;
        }

    } while (false);
    HJFLogi("{} init end:{}", getDebugName(), i_err);
    return i_err;
}


void HJRteGraphDrive::done()
{
    HJFLogi("{} HJRteGraphDrive done enter", getDebugName());
    if (m_threadTimer)
    {
        m_threadTimer->stopTimer();
        m_threadTimer = nullptr;
    }
    HJFLogi("{} HJRteGraphDrive done end", getDebugName());
    HJRteGraph::done();
}

int HJRteGraphDrive::getFps() const
{
    return m_fps;
}
bool HJRteGraphDrive::isManulDrive() const
{
    return m_bManulDrive;
}

int HJRteGraphDrive::run()
{
    return HJ_OK;
}


NS_HJ_END



