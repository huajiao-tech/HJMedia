#pragma once

#if defined(HarmonyOS)

#include "HJPrioComFBOBase.h"

NS_HJ_BEGIN

class HJOGCopyShaderStrip;

class HJPrioComFBOGray : public HJPrioComFBOBase
{
public:
    HJ_DEFINE_CREATE(HJPrioComFBOGray);
    HJPrioComFBOGray();
    virtual ~HJPrioComFBOGray();
     
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
    
private:
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
};

NS_HJ_END

#endif


