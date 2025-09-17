//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJGraphBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJGraphBase::HJGraphBase(const HJEnvironment::Ptr& env, const HJScheduler::Ptr& scheduler)
    : m_env(env)
{
    //setScheduler(scheduler);
}

HJGraphBase::~HJGraphBase()
{
    
}

NS_HJ_END
