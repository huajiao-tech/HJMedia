#pragma once

#include "HJRteCom.h"
#include "HJRteGraph.h"
#include "HJFLog.h"
#include "HJThreadPool.h"
#include "HJBaseUtils.h"    

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
HJRteGraph::HJRteGraph()
{

}
HJRteGraph::~HJRteGraph()
{

}

int HJRteGraph::test()
{
    int i_err = HJ_OK;
    do
    {
        HJRteCom::Ptr vidBridgeSource = HJRteCom::Create();
        vidBridgeSource->setInsName("com_vidBridgeSource");
        vidBridgeSource->setPriority(HJRteComPriority_VideoSource);
        m_coms.push_back(vidBridgeSource);

        HJRteCom::Ptr vidBridgeFBO = HJRteCom::Create();
        vidBridgeFBO->setInsName("com_vidBridgeFBO");
        vidBridgeFBO->setPriority(HJRteComPriority_VideoSource);
        m_coms.push_back(vidBridgeFBO);

        HJRteCom::Ptr vidMirror = HJRteCom::Create();
        vidMirror->setInsName("com_vidMirror");
        vidMirror->setPriority(HJRteComPriority_Mirror);
        m_coms.push_back(vidMirror);

        HJRteCom::Ptr vidBlur = HJRteCom::Create();
        vidBlur->setInsName("com_vidBlur");
        vidBlur->setPriority(HJRteComPriority_VideoBlur);
        m_coms.push_back(vidBlur);

        //HJRteCom::Ptr vidCombine = HJRteCom::Create();
        //vidCombine->setInsName("com_vidCombine");
        //m_coms.push_back(vidCombine);

        HJRteCom::Ptr giftSource = HJRteCom::Create();
        giftSource->setInsName("com_giftSource");
        giftSource->setPriority(HJRteComPriority_Gift);
        m_coms.push_back(giftSource);

        HJRteCom::Ptr targetUI = HJRteCom::Create();
        targetUI->setInsName("target_UI");
        targetUI->setPriority(HJRteComPriority_Target);
        m_coms.push_back(targetUI);

        HJRteCom::Ptr targetPusher = HJRteCom::Create();
        targetPusher->setInsName("targetPusher");
        targetPusher->setPriority(HJRteComPriority_Target);
        m_coms.push_back(targetPusher);

#if 0
        m_links.push_back(vidBridgeSource->addTarget(vidBridgeFBO));
        m_links.push_back(vidBridgeFBO->addTarget(vidBlur));

        m_links.push_back(vidBlur->addTarget(targetUI, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 0.5, 1.0)));
        m_links.push_back(vidBlur->addTarget(targetUI, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.5, 1.0, 1.0)));

        m_links.push_back(vidBlur->addTarget(vidMirror));
        m_links.push_back(vidMirror->addTarget(targetPusher));

        m_links.push_back(giftSource->addTarget(targetUI));
        m_links.push_back(giftSource->addTarget(targetPusher));
#else 
        m_links.push_back(vidBridgeSource->addTarget(vidMirror));
        m_links.push_back(giftSource->addTarget(targetPusher));
        m_links.push_back(vidMirror->addTarget(targetUI));
        m_links.push_back(vidMirror->addTarget(targetPusher));

        //m_links.push_back(vidBridgeSource->addTarget(vidBlur, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 0.5, 1.0)));
        //m_links.push_back(vidBlur->addTarget(targetUI));

        //m_links.push_back(giftSource->addTarget(vidMirror, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.5, 1.0, 1.0)));
        //m_links.push_back(vidMirror->addTarget(vidBlur));
        //m_links.push_back(vidBlur->addTarget(targetPusher));
#endif

        m_sources.push_back(vidBridgeSource);
        m_sources.push_back(giftSource);

        for (auto source : m_sources)
        {
            //source is ready, textureId, (OES,2D), etc.

            priNotifyAndTryRender(source);
        }

        priRemoveCom(targetPusher);
        //priRemoveCom(targetPusher);
        //priRemoveCom(vidBlur);

        priReset();

        HJFLogi("");
        HJFLogi("{} next draw begin", getInsName());
        HJFLogi("");

        for (auto source : m_sources)
        {
            //source is ready, textureId, (OES,2D), etc.

            priNotifyAndTryRender(source);
        }

    } while (false);
    return i_err;
}

int HJRteGraph::init(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}
void HJRteGraph::graphReset()
{
    priReset();    
}
void HJRteGraph::priReset()
{
    for (auto link : m_links)
    {
        link->setReady(false);
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
            //priNotifyAndTryRender(source);
        }
    } while (false);
    return i_err;
}
int HJRteGraph::priRender(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do 
    {
        i_err = i_com->foreachPreLink([this](const std::shared_ptr<HJRteComLink>& i_link)
            {
                int ret = HJ_OK;
                do
                {
                    std::string linkName = i_link->getInsName();
                    HJFLogi("{} com:<{} - {}> info:{} render sub #########", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                } while(false);
                return ret;
            });
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
int HJRteGraph::priNotifyAndTryRender(std::shared_ptr<HJRteCom> i_header)
{
    int i_err = HJ_OK;
    do 
    {
        std::string comName = i_header->getInsName();
        HJFLogi("{} com:{} enter", getInsName(), comName);
        //notify all next links ready
        i_err = i_header->foreachNextLink([this](const std::shared_ptr<HJRteComLink>& i_link)
            {
                int ret = HJ_OK;
                do
                {
                    std::string linkName = i_link->getInsName();
                    HJFLogi("{} notify com:<{} - {}> ready", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                    //notify next ready
                    i_link->setReady(true);

                    //com may be have more input, so must just all ready, then render
                    HJRteCom::Ptr com = i_link->getDstComPtr();
                    if (com && com->isAllPreReady())
                    {
                        std::string comName = com->getInsName();
                        HJFLogi("{} com:<{} - {}> info:{} render =======================", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                        //pri render to target
                        ret = priRender(com);
                        if (ret < 0)
                        {
                            HJFLoge("{} com:<{} - {}>  render error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                            break;
                        }

                        ret = priNotifyAndTryRender(com);
                        if (ret < 0)
                        {
                            HJFLoge("{} com:<{} - {}> priNotifyAndTryRender error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                            break;
                        }
                    }
                    else
                    {
                        HJFLogi("{} com:<{} - {}> not ready, so not render---->", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                    }
                } while (false);           
                return ret;
            });

        HJFLogi("{} com:{} end i_err:{}", getInsName(), comName, i_err);

        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
void HJRteGraph::removeFromLinks(std::shared_ptr<HJRteComLink> i_link)
{
	for (auto it = m_links.begin(); it != m_links.end(); ++it)
	{
		if ((*it) == i_link)
		{
			m_links.erase(it);
            HJFLogi("");
            HJFLogi("removeFromLinks:{}", i_link->getInsName());
            HJFLogi("");
			break;
		}
	}
}
void HJRteGraph::removeFromComs(std::shared_ptr<HJRteCom> i_com)
{
    for (auto it = m_coms.begin(); it != m_coms.end(); ++it)
    {
        if ((*it) == i_com)
        {
            HJFLogi("");
            HJFLogi("removeFromComs:{}", i_com->getInsName());
            HJFLogi("");
            m_coms.erase(it);
            break;
        }
    }
}
void HJRteGraph::removeFromSources(std::shared_ptr<HJRteCom> i_com)
{
	for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		if ((*it) == i_com)
		{
			m_sources.erase(it);
			break;
		}
	}
}
int HJRteGraph::priRemoveSource(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do
    {
        std::deque<HJRteComLink::Wtr> tmpQueue = HJBaseUtils::copyDeque<HJRteComLink::Wtr>(i_com->getNextQueue());
        for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
        {
            HJRteComLink::Ptr link = it->lock();
            if (link)
            {
                link->breakLink();
                removeFromLinks(link);

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
        tmpQueue.clear();
        i_com->clearNext();
        if (i_com->isNextEmpty())
        {
            removeFromComs(i_com);
        }
    } while (false);
    return i_err;
}
int HJRteGraph::priRemoveTarget(std::shared_ptr<HJRteCom> i_com)
{
	int i_err = HJ_OK;
	do
	{
		std::deque<HJRteComLink::Wtr> tmpQueue = HJBaseUtils::copyDeque<HJRteComLink::Wtr>(i_com->getPreQueue());
		for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
		{
			HJRteComLink::Ptr link = it->lock();
			if (link)
			{
				link->breakLink();
				removeFromLinks(link);

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
        tmpQueue.clear();
		i_com->clearPre();
		if (i_com->isPreEmpty())
		{
			removeFromComs(i_com);
		}
	} while (false);
	return i_err;
}
int HJRteGraph::priRemoveFilter(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do
    {
        const std::deque<HJRteComLink::Wtr>& preQueue = i_com->getPreQueue();
        HJRteComLink::Ptr preLink = HJRteComLink::GetPtrFromWtr(*preQueue.begin());
        if (preLink)
        {
            HJRteCom::Ptr src = preLink->getSrcComPtr();
            i_err = preLink->breakLink();
            if (i_err < 0)
            {
                break;
            }
            removeFromLinks(preLink);

            std::deque<HJRteComLink::Wtr> tmpQueue = HJBaseUtils::copyDeque<HJRteComLink::Wtr>(i_com->getNextQueue());
            for (auto it = tmpQueue.begin(); it != tmpQueue.end(); ++it)
            {
                HJRteComLink::Ptr oldLink = HJRteComLink::GetPtrFromWtr(*it);
                if (oldLink)
                {
                    HJRteCom::Ptr dst = oldLink->getDstComPtr();
                    oldLink->breakLink();

                    m_links.push_back(src->addTarget(dst, oldLink->getLinkInfo()));
                    removeFromLinks(oldLink);
                }
            }
            tmpQueue.clear();
        }

        removeFromComs(i_com);
    } while (false);    
    return i_err;
}
int HJRteGraph::priRemoveCom(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do
    {
        HJRteComType type = i_com->getRteComType();
        if (type == HJRteComType_Source)
        {
            i_err = priRemoveSource(i_com);
            if (i_err < 0)
            {
                break;
            }
            removeFromSources(i_com);
        }
        else if (type == HJRteComType_Filter)
        {
            if (i_com->getPreCount() != 1)
            {
                i_err = -1;
                HJFLoge("{} this filter is not support, more input, remove cannot relink");
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
            //fixme after modify, UI UI viewport is different, so for loop remove;
			i_err = priRemoveTarget(i_com);
			if (i_err < 0)
			{
				break;
			}
        }
    } while (false);
    return i_err;
}

void HJRteGraph::done()
{

}
//////////////////////////////////////////////////////////////////////

HJRteGraphTimer::HJRteGraphTimer()
{
    m_threadTimer = HJTimerThreadPool::Create();
}
HJRteGraphTimer::~HJRteGraphTimer()
{

}

int HJRteGraphTimer::init(std::shared_ptr<HJBaseParam> i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJRteGraph::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        if (i_param && i_param->contains(HJBaseParam::s_paramFps))
        {
            m_fps = i_param->getValue<int>(HJBaseParam::s_paramFps);
            if (m_fps <= 0)
            {
                i_err = -1;
                break;
            }
        }

        i_err = m_threadTimer->startTimer(1000 / m_fps, [this]()
            {
                return run();
            });

        if (i_err < 0)
        {
            break;
        }

    } while (false);
    return i_err;
}
void HJRteGraphTimer::done()
{
    m_threadTimer->stopTimer();
    HJRteGraph::done();
}

int HJRteGraphTimer::getFps() const
{
    return m_fps;
}
int HJRteGraphTimer::run()
{
    return HJ_OK;
}


NS_HJ_END



