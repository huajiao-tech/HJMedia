//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJObject.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//

#define HJOBJECT_STAT 0

std::atomic<int64_t> HJObject::m_memoryCreateIdx{0};
std::atomic<int64_t> HJObject::m_memoryReleaseIdx{0};    

void HJObject::priCreateStat()
{
#if HJOBJECT_STAT
    m_curStatIdx = m_memoryCreateIdx++;
    HJFLogi("{} HJObjectStatMemory create object {} curIdx:{} statIdx:{}", m_name, size_t(this), m_curStatIdx, m_memoryCreateIdx);    
#endif
}
HJObject::HJObject()
{
    priCreateStat();
}
HJObject::HJObject(const std::string& name, size_t identify)
        : m_name(name)
        , m_id(identify)
{ 
    priCreateStat();    
}

HJObject::~HJObject() 
{
#if HJOBJECT_STAT
    m_memoryReleaseIdx++;
    HJFLogi("{} HJObjectStatMemory release object {} curIdx:{} statIdx:{}", m_name, size_t(this), m_curStatIdx, (m_memoryCreateIdx - m_memoryReleaseIdx));
#endif
}

const size_t HJObject::getGlobalID()
{
    static std::atomic<size_t> g_objectIDCounter(0);
    return g_objectIDCounter.fetch_add(1, std::memory_order_relaxed);
}

const std::string HJObject::getGlobalName(const std::string prefix)
{
    auto value = HJFMT("{}",getGlobalID());
    return prefix.empty() ? value : (prefix + "_" + value);
}


NS_HJ_END