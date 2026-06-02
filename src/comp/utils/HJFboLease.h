#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJFBOCtrlPool.h"
#include "HJOGFBOCtrl.h"

NS_HJ_BEGIN

class HJFboLease : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJFboLease);
    HJFboLease() = default;
    virtual ~HJFboLease();
    HJFboLease(const std::string& i_srcName, const std::shared_ptr<HJFBOCtrlPool>& i_pool, std::shared_ptr<HJOGFBOCtrl> i_fbo, bool i_bNeedStat = false);
    std::shared_ptr<HJOGFBOCtrl> get() const;
    void release();

private:
    std::weak_ptr<HJFBOCtrlPool> m_poolWtr;
    std::shared_ptr<HJOGFBOCtrl> m_fbo = nullptr;
    std::string m_srcName = "";
    bool m_bNeedStat = false;
};

NS_HJ_END

