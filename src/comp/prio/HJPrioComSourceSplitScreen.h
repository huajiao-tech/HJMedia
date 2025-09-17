#pragma once

#include "HJPrioCom.h"
#include "HJPrioComSourceBridge.h"

NS_HJ_BEGIN

class HJOGCopyShaderStrip;

class HJPrioComSourceSplitScreen : public HJPrioComSourceBridge
{
public:
    HJ_DEFINE_CREATE(HJPrioComSourceSplitScreen);
    HJPrioComSourceSplitScreen();
    virtual ~HJPrioComSourceSplitScreen();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
    virtual int render(HJBaseParam::Ptr i_param) override;
    
private:
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
};

NS_HJ_END



