#pragma once

#if defined(HarmonyOS)
#include "HJPrioCom.h"
#include "HJPrioComFBOBase.h"
#include "HJOGCopyShaderStrip.h"

NS_HJ_BEGIN

class HJOGCopyShaderStrip;
class HJPrioComBlurHori : public HJPrioComFBOBase
{
public:
    HJ_DEFINE_CREATE(HJPrioComBlurHori);
    HJPrioComBlurHori();
    virtual ~HJPrioComBlurHori();

    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;

private:
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;    
};

class HJPrioComFBOBlur : public HJPrioComFBOBase
{
public:
    HJ_DEFINE_CREATE(HJPrioComFBOBlur);
    HJPrioComFBOBlur();
    virtual ~HJPrioComFBOBlur();

    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;

private:
    HJPrioComBlurHori::Ptr m_blurHori = nullptr;
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
};

NS_HJ_END

#endif // HarmonyOS

