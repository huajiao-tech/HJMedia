//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJVRenderBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJVRenderFake : public HJVRenderBase
{
public:
    using Ptr = std::shared_ptr<HJVRenderFake>;
    HJVRenderFake();
    virtual ~HJVRenderFake();

    virtual int init(const HJVideoInfo::Ptr& info) override;
    virtual int render(const HJMediaFrame::Ptr frame) override;
    virtual int setWindow(const std::any& window) override;
    virtual int flush() override;
protected:
    HJNipMuster::Ptr       m_nipMuster = nullptr;
};

NS_HJ_END

