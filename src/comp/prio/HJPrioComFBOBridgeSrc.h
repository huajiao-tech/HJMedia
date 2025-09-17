#pragma once

#include "HJPrioComFBOBase.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;

class HJPrioComFBOBridgeSrc : public HJPrioComFBOBase
{
public:
    HJ_DEFINE_CREATE(HJPrioComFBOBridgeSrc);
    HJPrioComFBOBridgeSrc();
    virtual ~HJPrioComFBOBridgeSrc();
     
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
    
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
#endif
    
private:
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
};

NS_HJ_END



