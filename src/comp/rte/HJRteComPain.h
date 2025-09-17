#pragma once

#include "HJPrerequisites.h"
#include "HJRteCom.h"

NS_HJ_BEGIN


class HJRteComPain : public HJRteCom
{
public:
    HJ_DEFINE_CREATE(HJRteComPain);
    HJRteComPain() = default;
    virtual ~HJRteComPain() = default;
    
//    virtual bool isRenderReady();
//    virtual int update(HJBaseParam::Ptr i_param) override;
    
    virtual int attach()
    {
        return 0;
    }
    virtual int detach()
    {
        return 0;
    }
    virtual int render(HJBaseParam::Ptr i_param) override
    {
        return 0;
    }
};

class HJRteComEGL : public HJRteComPain
{
public:
    HJ_DEFINE_CREATE(HJRteComEGL);
    HJRteComEGL() = default;
    virtual ~HJRteComEGL() = default;
    
//    virtual bool isRenderReady();
//    virtual int update(HJBaseParam::Ptr i_param) override;
    
    virtual int attach() override;;
    virtual int detach() override;;
    virtual int render(HJBaseParam::Ptr i_param) override;   
};

NS_HJ_END



