#pragma once

#include "HJPrioCom.h"
#include "HJFLog.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////

std::atomic<int> HJPrioCom::m_index = 0;
std::atomic<int> HJPrioCom::m_memoryPrioComStatIdx = 0;

HJPrioCom::HJPrioCom()
{
    m_curIdx = m_index++;
    m_memoryPrioComStatIdx++;
    HJFLogi("{} HJPrioComMemory create {} curIdx:{} memoryStatIdx:{}", m_insName, size_t(this), m_curIdx, m_memoryPrioComStatIdx);
}
HJPrioCom::~HJPrioCom()
{
    m_memoryPrioComStatIdx--;
    HJFLogi("{} HJPrioComMemory release {} curIdx:{} memoryStatIdx:{}", m_insName, size_t(this), m_curIdx, m_memoryPrioComStatIdx);
}
int HJPrioCom::init(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}
void HJPrioCom::done()
{

}
int HJPrioCom::update(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}
int HJPrioCom::render(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}
int HJPrioCom::sendMessage(HJBaseMessage::Ptr i_msg)
{
    return HJ_OK;
}
bool HJPrioCom::renderModeIsContain(int i_targetType)
{
    return (m_renderMap.find(i_targetType) != m_renderMap.end());
}
void HJPrioCom::renderModeClear(int i_targetType)
{
    if (renderModeIsContain(i_targetType))
    {
        m_renderMap.erase(i_targetType);
    }
}
void HJPrioCom::renderModeClearAll()
{
    m_renderMap.clear();
    renderModeAdd(HJOGEGLSurfaceType_Default);
}
void HJPrioCom::renderModeAdd(int i_targetType, HJTransferRenderModeInfo::Ptr i_rendermMode)
{
    HJTransferRenderModeInfo::Ptr value = i_rendermMode;
    if (!value)
    {
        value = HJTransferRenderModeInfo::Create();
    }
    m_renderMap[i_targetType].push_back(std::move(value));
}
std::vector<HJTransferRenderModeInfo::Ptr>& HJPrioCom::renderModeGet(int i_targetType)
{
    return m_renderMap[i_targetType];
}


///////////////////////////////////////////////////////////////////////////////////////////////



NS_HJ_END



