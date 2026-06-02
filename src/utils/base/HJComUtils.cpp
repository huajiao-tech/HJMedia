#include "HJComUtils.h"
#include "HJFLog.h"
#include "HJSPBuffer.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
std::atomic<int> HJBaseObject::m_memoryBaseObjStatIdx = 0;

HJBaseObject::HJBaseObject()
{
    m_curIdx = m_memoryBaseObjStatIdx++;
    //HJFLogi("{} HJBaseObject create object {} curIdx:{} statIdx:{}", m_insName, size_t(this), m_curIdx, m_memoryBaseObjStatIdx);
}
HJBaseObject::~HJBaseObject()
{
    const int stat_idx = m_memoryBaseObjStatIdx.fetch_sub(1, std::memory_order_relaxed) - 1;
    if (stat_idx < 3)
    {
        HJFLogi("{} HJBaseObject release object {} curIdx:{} statIdx:{}", m_insName, size_t(this), m_curIdx, stat_idx);
    }
}
void HJBaseObject::setInsName(const std::string& i_insName)
{
    m_insName = i_insName;
    m_debugName = m_insName + "_" + std::to_string(m_debugIdx);
}
const std::string& HJBaseObject::getInsName() const
{
    return m_insName;
}
void HJBaseObject::setDebugIdx(int i_idx)
{
    m_debugIdx = i_idx;
    m_debugName = m_insName + "_" + std::to_string(m_debugIdx);
}
int HJBaseObject::getDebugIdx() const
{
    return m_debugIdx;
}
const std::string& HJBaseObject::getDebugName() const
{
    return m_debugName;
}
//////////////////////////////////////////////////////////////////////////////////////////
std::string HJBaseParam::s_paramFps = "paramFps";
std::string HJBaseParam::s_paramIsManulDrive = "paramIsManulDrive";
HJBaseParam::HJBaseParam()
{
}
HJBaseParam::~HJBaseParam()
{
}

/////////////////////////////////////////////////////////////////////////////////////////
HJComMedaInfo::HJComMedaInfo()
{ 
}
	
HJComMedaInfo::~HJComMedaInfo()
{ 
}

/////////////////////////////////////////////////////////////////////////////////////////
HJComMediaFrame::HJComMediaFrame()
{
}

HJComMediaFrame::~HJComMediaFrame()
{
}

std::shared_ptr<HJSPBuffer> HJComMediaFrame::GetBuffer() const 
{
    return m_buffer;
}
void HJComMediaFrame::SetBuffer(std::shared_ptr<HJSPBuffer> buffer)
{ 
    m_buffer = buffer; 
}

NS_HJ_END


