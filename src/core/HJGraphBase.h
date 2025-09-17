//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaInfo.h"
#include "HJMediaNode.h"
#include "HJExecutor.h"
#include "HJNoticeCenter.h"
#include "HJEnvironment.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJGraphBase : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJGraphBase>;
    HJGraphBase(const HJEnvironment::Ptr& env, const HJScheduler::Ptr& scheduler = nullptr);
    virtual ~HJGraphBase();
    
    int notify(const HJNotification::Ptr ntf) {
        if (m_env) {
            return m_env->notify(ntf);
        }
        return 0;
    }
    void setEnviroment(const HJEnvironment::Ptr& env) {
        m_env = env;
    }
    const HJEnvironment::Ptr getEnviroment() const {
        return m_env;
    }
    void setScheduler(const HJScheduler::Ptr& scheduler) {
        m_scheduler = (nullptr != scheduler) ? scheduler : std::make_shared<HJScheduler>();
    }
    const HJScheduler::Ptr& getScheduler() const {
        return m_scheduler;
    }
protected:
    HJRunState                 m_runState = HJRun_NONE;
    HJEnvironment::Ptr         m_env = nullptr;
    HJScheduler::Ptr           m_scheduler = nullptr;
};

NS_HJ_END


