#include "HJRteCom.h"
#include "HJFLog.h"
#include "HJOGBaseShader.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
HJRteDriftInfo::~HJRteDriftInfo()
{
    HJFLogd("{} ~HJRteDriftInfo ", m_strComName);
}

/////////////////////////////////////////////////////////////////////////////////////////

HJRteComLinkInfo::HJRteComLinkInfo()
{

}
HJRteComLinkInfo::HJRteComLinkInfo(float i_x, float i_y, float i_width, float i_height,  HJWindowRenderMode i_renderMode, bool i_xMirror, bool i_yMirror, const std::string &i_linkId) :
    m_x(i_x)
   , m_y(i_y)
   , m_width(i_width)
   , m_height(i_height)
   , m_renderMode(i_renderMode)
   , m_xMirror(i_xMirror)
   , m_yMirror(i_yMirror)
   , m_linkId(i_linkId)
{
}
HJRteComLinkInfo::~HJRteComLinkInfo()
{
}

std::string HJRteComLinkInfo::getInfo() const
{
    return HJFMT("{} linkId:{} viewport: <{} {} {} {}> renderMode:{} xMirror:{} yMirror:{}", getInsName(), m_linkId, m_x, m_y, m_width, m_height, (int)m_renderMode, m_xMirror, m_yMirror);
}
    
HJVec4i HJRteComLinkInfo::convert(int i_targetWidth, int i_targetHeight)
{
    int x = i_targetWidth * m_x;
    int y = (1.f - m_height - m_y) * i_targetHeight;
    int w = i_targetWidth * m_width;
    int h = i_targetHeight * m_height;
    return HJVec4i{x, y, w, h};
}
        
/////////////////////////////////////////////////////////////////////////////////////////
std::atomic<int> HJRteComLink::m_memoryRteComLinkStatIdx = 0;

HJRteComLink::HJRteComLink()
{
    priMemoryStat();
}

HJRteComLink::HJRteComLink(const std::shared_ptr<HJRteCom>& i_srcCom, const std::shared_ptr<HJRteCom>& i_dstCom, std::shared_ptr<HJRteComLinkInfo> i_linkInfo, std::shared_ptr<HJOGBaseShader> i_shader) :
    m_srcCom(i_srcCom),
    m_dstCom(i_dstCom)
    ,m_linkInfo(i_linkInfo)
    ,m_shader(i_shader)
{
    priMemoryStat();
}

HJRteComLink::~HJRteComLink()
{
    m_memoryRteComLinkStatIdx--;
    //HJFLogi("{} ~HJRtelink {} memoryStatIdx:{}", m_insName, size_t(this), m_memoryRteComLinkStatIdx);
  
    if (m_shader)
    {
        //not use this; A-B-C, remove B, B-C link shader is smart pt, and A-C link is shared, not m_shader->release, so release is in ~shader function is best
        //m_shader->release();
        m_shader = nullptr;
    } 
}

void HJRteComLink::priMemoryStat()
{
    m_memoryRteComLinkStatIdx++;
    //HJFLogi("{} HJRtelink {} memoryStatIdx:{}", m_insName, size_t(this), m_memoryRteComLinkStatIdx);
    
    if (!m_linkInfo)
    {
        m_linkInfo = HJRteComLinkInfo::Create();
    }
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
    const int memory_stat_idx = m_memoryRteComStatIdx.fetch_add(1, std::memory_order_relaxed) + 1;
    m_curIdx = memory_stat_idx - 1;
    HJFLogi("{} HJRteCom {} memoryStatIdx:{}", getDebugName(), size_t(this), memory_stat_idx);
}
HJRteCom::~HJRteCom()
{
    const int memory_stat_idx = m_memoryRteComStatIdx.fetch_sub(1, std::memory_order_relaxed) - 1;
    HJFLogi("{} ~HJRteCom {} m_curIdx:{} memoryStatIdx:{}", getDebugName(), size_t(this), m_curIdx,
            memory_stat_idx);
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

HJRteComLink::Ptr HJRteCom::addTarget(HJRteCom::Ptr i_com, std::shared_ptr<HJOGBaseShader> i_shader, std::shared_ptr<HJRteComLinkInfo> i_linkInfo)
{
    if (i_com.get() == this)
    {
        HJFLoge("{} addTarget self link error i_com:{}", getDebugName(), i_com->getInsName());
        return nullptr;
    }

    HJRteComLink::Ptr link = HJRteComLink::Create<HJRteComLink>(HJBaseSharedObject::getSharedFrom(this), i_com, i_linkInfo, i_shader);
    std::string linkName = this->getInsName();
    if (i_shader)
    {
        linkName = linkName + "_" + i_shader->getInsName() + "_";
    }   
    if (i_linkInfo)
    {
        linkName = linkName + "_" + i_linkInfo->getInfo() + "_";
    }  
    linkName += i_com->getInsName() + "_link";
    link->setInsName(linkName);
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
void HJRteCom::setRteComType(HJRteComType i_type)
{
    m_rteComType = i_type;
}
HJRteComType HJRteCom::getRteComType() const
{
    return m_rteComType;
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
void HJRteCom::reset()
{
    m_driftInfo = nullptr;
    m_refCntEvery = m_refCntStable;
    //refCntClear();
}
HJRteDriftInfo::Ptr HJRteCom::getDriftInfo()
{
    HJRteDriftInfo::Ptr driftInfo = m_driftInfo;
    if (m_driftInfo)
    {
        m_refCntEvery--;
		if (m_refCntEvery <= 0)
		{
			m_driftInfo = nullptr;
		}
    }
    return driftInfo;
}
void HJRteCom::setDriftInfo(HJRteDriftInfo::Ptr i_driftInfo)
{
    m_driftInfo = std::move(i_driftInfo);
    //// 
    //if (i_isSrcDrift && HJRteCom::isFilter() && m_driftInfo && !m_driftInfo->hasFboLease())
    //{
    //    m_driftInfo->setFunc([this]()
    //    {
    //        HJFLogi("{} ~driftInfo", getInsName());
    //        resouceRelease();
    //    });
    //}
}
void HJRteCom::refCntClear()
{
    m_refCntStable = 0;
}
void HJRteCom::refCntSet(int i_refCnt)
{
    m_refCntStable = i_refCnt;
    m_refCntEvery = m_refCntStable;
}
int HJRteCom::refCntGet() const
{
    return m_refCntStable;
}
//int HJRteCom::update(HJBaseParam::Ptr i_param)
//{
//	return HJ_OK;
//}
//int HJRteCom::render(HJBaseParam::Ptr i_param)
//{
//    return HJ_OK;
//}

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


