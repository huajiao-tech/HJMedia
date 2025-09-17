#pragma once

#include "HJComUtils.h"
#include "HJFLog.h"
#include "HJSPBuffer.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
std::atomic<int> HJBaseObject::m_memoryBaseObjStatIdx = 0;

HJBaseObject::HJBaseObject()
{
    m_curIdx = m_memoryBaseObjStatIdx++;
    HJFLogi("{} HJBaseObject create object {} curIdx:{} statIdx:{}", m_insName, size_t(this), m_curIdx, m_memoryBaseObjStatIdx);
}
HJBaseObject::~HJBaseObject()
{
    m_memoryBaseObjStatIdx--;
    HJFLogi("{} HJBaseObject release object {} curIdx:{} statIdx:{}", m_insName, size_t(this), m_curIdx, m_memoryBaseObjStatIdx);
}
void HJBaseObject::setInsName(const std::string& i_insName)
{
    m_insName = i_insName;
}
const std::string& HJBaseObject::getInsName() const
{
    return m_insName;
}

//////////////////////////////////////////////////////////////////////////////////////////
std::string HJBaseParam::s_paramFps = "fps";
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



