#pragma once

#include "HJPrioCom.h"
#include "HJPrioComFBOBase.h"

NS_HJ_BEGIN

class HJPrioComFBOBase;

class HJPrioComSourcePingPangFBO : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJPrioComSourcePingPangFBO);
    HJPrioComSourcePingPangFBO();
    virtual ~HJPrioComSourcePingPangFBO();
        
	//int init(HJBaseParam::Ptr i_param);
    const HJPrioComFBOBase::Ptr& getDetectFBO();
    const HJPrioComFBOBase::Ptr& getRenderFBO();
    
    void submit();
    
protected:
    
private:
    std::vector<HJPrioComFBOBase::Ptr> m_pingPangFBOs;
    int64_t m_index = 0;
};

NS_HJ_END



