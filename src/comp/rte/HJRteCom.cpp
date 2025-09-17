#pragma once

#include "HJRteCom.h"
#include "HJFLog.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////

HJRteComLinkInfo::HJRteComLinkInfo()
{

}
HJRteComLinkInfo::HJRteComLinkInfo(float i_x, float i_y, float i_width, float i_height) :
    m_x(i_x)
   , m_y(i_y)
   , m_width(i_width)
   , m_height(i_height)
{
}
HJRteComLinkInfo::~HJRteComLinkInfo()
{

}

std::string HJRteComLinkInfo::getInfo() const
{
    return HJFMT("viewport: <{} {} {} {}>", m_x, m_y, m_width, m_height);
}

/////////////////////////////////////////////////////////////////////////////////////////
std::atomic<int> HJRteComLink::m_memoryRteComLinkStatIdx = 0;

HJRteComLink::HJRteComLink()
{
    priMemoryStat();
}

HJRteComLink::HJRteComLink(const std::shared_ptr<HJRteCom>& i_srcCom, const std::shared_ptr<HJRteCom>& i_dstCom, std::shared_ptr<HJRteComLinkInfo> i_linkInfo) :
    m_srcCom(i_srcCom),
    m_dstCom(i_dstCom)
    ,m_linkInfo(i_linkInfo)
{
    priMemoryStat();
}

HJRteComLink::~HJRteComLink()
{
    m_memoryRteComLinkStatIdx--;
    HJFLogi("{} ~HJRtelink {} memoryStatIdx:{}", m_insName, size_t(this), m_memoryRteComLinkStatIdx);
}

void HJRteComLink::priMemoryStat()
{
    m_memoryRteComLinkStatIdx++;
    HJFLogi("{} HJRtelink {} memoryStatIdx:{}", m_insName, size_t(this), m_memoryRteComLinkStatIdx);
}
void HJRteComLink::setReady(bool i_ready)
{
    m_bReady = i_ready;
}
bool HJRteComLink::isReady() const
{
    return m_bReady;
}

std::shared_ptr<HJRteCom> HJRteComLink::getSrcComPtr() const
{
    return HJRteCom::GetPtrFromWtr(m_srcCom);
}
std::shared_ptr<HJRteCom> HJRteComLink::getDstComPtr() const
{
    return HJRteCom::GetPtrFromWtr(m_dstCom);
}
std::string HJRteComLink::getSrcComName() const
{
    std::shared_ptr<HJRteCom> com = getSrcComPtr();
    if (com)
    {
        return com->getInsName();
    }
    return "";
}
std::string HJRteComLink::getDstComName() const
{
    std::shared_ptr<HJRteCom> com = getDstComPtr();
    if (com)
    {
        return com->getInsName();
    }
    return "";
}
std::string HJRteComLink::getInfo() const
{
    if (m_linkInfo)
    {
        return m_linkInfo->getInfo();
    }
    return "";
}
int HJRteComLink::breakLink()
{
    int i_err = HJ_OK;
    do 
    {
		HJRteCom::Ptr src = getSrcComPtr();
		HJRteCom::Ptr dst = getDstComPtr();

        if (src && dst)
        {
            HJRteComLink::Ptr link = getSharedFrom(this);
            //remove src->next link
            src->removeNext(link);
            //remove dst->pre link
            dst->removePre(link);
        }
    } while (false);
    return i_err;
}

/////////////////////////////////////////////////////////////////////////////////////////
std::atomic<int> HJRteCom::m_memoryRteComStatIdx = 0;

HJRteCom::HJRteCom()
{
    m_curIdx = m_memoryRteComStatIdx++;
    HJFLogi("{} HJRteCom {} memoryStatIdx:{}", m_insName, size_t(this), m_memoryRteComStatIdx);
}
HJRteCom::~HJRteCom()
{
    m_memoryRteComStatIdx--;
    HJFLogi("{} ~HJRteCom {} m_curIdx:{} memoryStatIdx:{}", m_insName, size_t(this), m_curIdx, m_memoryRteComStatIdx);
}
int HJRteCom::init(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}
void HJRteCom::done()
{

}
bool HJRteCom::isAllPreReady()
{
    bool bReady = false;
    int idx = 0;
    for (auto& linkwtr : m_preQueue)
    {
        HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(linkwtr);
        if (link)
        {
            if (link->isReady())
            {
                idx++;
            }
        }
    }
    if ((idx > 0) && (idx == m_preQueue.size()))
    {
        bReady = true;
    }
    return bReady;
}

bool HJRteCom::pricompare(const std::weak_ptr<HJRteComLink>& i_a, const std::weak_ptr<HJRteComLink>& i_b)
{
    HJRteComLink::Ptr alink = HJRteComLink::GetPtrFromWtr(i_a);
    HJRteComLink::Ptr blink = HJRteComLink::GetPtrFromWtr(i_b);
    if (alink && blink)
    {
        HJRteCom::Ptr a = alink->getSrcComPtr();
        HJRteCom::Ptr b = blink->getSrcComPtr();
        if (a && b)
        {
            if (a->getPriority() != b->getPriority())
            {
                return a->getPriority() < b->getPriority();
            }
            else
            {
                return a->getIndex() < b->getIndex();
            }
        }
    }
    return false;
}
HJRteComLink::Ptr HJRteCom::addTarget(HJRteCom::Ptr i_com, std::shared_ptr<HJRteComLinkInfo> i_linkInfo)
{
    HJRteComLink::Ptr link = HJRteComLink::Create<HJRteComLink>(HJBaseSharedObject::getSharedFrom(this), i_com, i_linkInfo);
    link->setInsName(this->getInsName() + "_" + i_com->getInsName() + "_link");
    m_nextQueue.push_back(link);
    i_com->m_preQueue.push_back(link);

    if (this->isUsePriority())
    {
        std::sort(m_nextQueue.begin(), m_nextQueue.end(), pricompare);
    }
    if (i_com->isUsePriority())
    {
        std::sort(i_com->m_preQueue.begin(), i_com->m_preQueue.end(), pricompare);
    }
    return link;
}
int HJRteCom::foreachPreLink(std::function<int(const std::shared_ptr<HJRteComLink>& i_link)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        for (auto& linkwtr : m_preQueue)
        {
            HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(linkwtr);
            if (link)
            {
                i_err = i_func(link);
                if (i_err < 0)
                {
                    break;
                }
            }
        }
    } while (false);
    return i_err;
}
int HJRteCom::foreachNextLink(std::function<int(const std::shared_ptr<HJRteComLink> &i_link)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        for (auto& linkwtr : m_nextQueue)
        {
            HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(linkwtr);
            if (link)
            {
                i_err = i_func(link);
                if (i_err < 0)
                {
                    break;
                }
            }
        }
    } while (false);
    return i_err;
}
//void HJRteCom::releaseFromPre(const std::shared_ptr<HJRteCom>& i_com)
//{
//    for (auto it = m_preQueue.begin(); it != m_preQueue.end(); ++it)
//    {
//        HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(*it);
//        if (link)
//        {
//            if (link->getSrcComPtr() == i_com)
//            {
//                m_preQueue.erase(it);
//                break;
//            }
//        }       
//    }
//}


void HJRteCom::removePre(const std::shared_ptr<HJRteComLink>& i_link)
{
    for (auto it = m_preQueue.begin(); it != m_preQueue.end(); ++it)
    {
        HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(*it);
        if (link)
        {
            if (link == i_link)
            {
                m_preQueue.erase(it);
                break;
            }
        }   
    }
}
void HJRteCom::removeNext(const std::shared_ptr<HJRteComLink>& i_link)
{
    for (auto it = m_nextQueue.begin(); it != m_nextQueue.end(); ++it)
    {
        HJRteComLink::Ptr link = HJRteComLink::GetPtrFromWtr(*it);
        if (link)
        {
            if (link == i_link)
            {
                m_nextQueue.erase(it);
                break;
            }
        }
    }
}

bool HJRteCom::isPreEmpty()
{
    return m_preQueue.empty();
}
bool HJRteCom::isNextEmpty()
{
    return m_nextQueue.empty();
}
int HJRteCom::getPreCount()
{
    return m_preQueue.size();
}
int HJRteCom::getNextCount()
{
    return m_nextQueue.size();
}
HJRteComType HJRteCom::getRteComType() const
{
    HJRteComType type = HJRteComType_Filter;

    if (!m_nextQueue.empty() && !m_preQueue.empty())
    {
        type = HJRteComType_Filter;
    }
    else if (!m_nextQueue.empty())
    {
        type = HJRteComType_Source;
    }
    else if (!m_preQueue.empty())
    {
        type = HJRteComType_Target;
    }
    return type;
}
bool HJRteCom::isSource() const
{
    HJRteComType type = getRteComType();
    return (type == HJRteComType_Source);
}
bool HJRteCom::isFilter() const
{
    HJRteComType type = getRteComType();
    return (type == HJRteComType_Filter);
}
bool HJRteCom::isTarget() const
{
    HJRteComType type = getRteComType();
    return (type == HJRteComType_Target);
}
int HJRteCom::update(HJBaseParam::Ptr i_param)
{
	return HJ_OK;
}
int HJRteCom::render(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}

//bool HJPrioCom::renderModeIsContain(int i_targetType)
//{
//    return (m_renderMap.find(i_targetType) != m_renderMap.end());
//}
//void HJPrioCom::renderModeClear(int i_targetType)
//{
//    if (renderModeIsContain(i_targetType))
//    {
//        m_renderMap.erase(i_targetType);
//    }
//}
//void HJPrioCom::renderModeClearAll()
//{
//    m_renderMap.clear();
//    renderModeAdd(HJOGEGLSurfaceType_Default);
//}
//void HJPrioCom::renderModeAdd(int i_targetType, HJTransferRenderModeInfo::Ptr i_rendermMode)
//{
//    HJTransferRenderModeInfo::Ptr value = i_rendermMode;
//    if (!value)
//    {
//        value = HJTransferRenderModeInfo::Create();
//    }
//    m_renderMap[i_targetType].push_back(std::move(value));
//}
//std::vector<HJTransferRenderModeInfo::Ptr>& HJPrioCom::renderModeGet(int i_targetType)
//{
//    return m_renderMap[i_targetType];
//}


///////////////////////////////////////////////////////////////////////////////////////////////



NS_HJ_END



