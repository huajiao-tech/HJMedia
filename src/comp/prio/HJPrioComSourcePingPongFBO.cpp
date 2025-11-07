#include "HJPrioComSourcePingPongFBO.h"

NS_HJ_BEGIN

#define USE_PING_PANG 1

HJPrioComSourcePingPangFBO::HJPrioComSourcePingPangFBO()
{
	HJ_SetInsName(HJPrioComSourcePingPangFBO);
    HJPrioComFBOBase::Ptr fbo = HJPrioComFBOBase::Create();
    m_pingPangFBOs.push_back(std::move(fbo));
    
    fbo = HJPrioComFBOBase::Create();
    m_pingPangFBOs.push_back(std::move(fbo));
}
HJPrioComSourcePingPangFBO::~HJPrioComSourcePingPangFBO()
{
    for (int i = 0; i < m_pingPangFBOs.size(); i++)
    {
        m_pingPangFBOs[i]->done();
    }
    m_pingPangFBOs.clear();
}
const HJPrioComFBOBase::Ptr& HJPrioComSourcePingPangFBO::getDetectFBO()
{
    if (m_index == 0)
    {
        return m_pingPangFBOs[0];    
    }
    else
    {
        int idx = (int)(m_index & 1);
        return m_pingPangFBOs[idx];
    }    
}
const HJPrioComFBOBase::Ptr& HJPrioComSourcePingPangFBO::getRenderFBO()
{
    if (m_index == 0)
    {
        return m_pingPangFBOs[0];    
    }
    else
    {
#if USE_PING_PANG
        int idx = 1 - (int)(m_index & 1);
#else
        int idx = (int)(m_index & 1);
#endif
        return m_pingPangFBOs[idx];
    }
}
    
void HJPrioComSourcePingPangFBO::submit()
{
    m_index++;    
}

NS_HJ_END