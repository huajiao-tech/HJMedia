//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJVRenderBase : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJVRenderBase>;
    HJVRenderBase() {};
    virtual ~HJVRenderBase() {};
    
    virtual int init(const HJVideoInfo::Ptr& info);
    virtual int render(const HJMediaFrame::Ptr frame) { return HJ_OK; }
    virtual int setWindow(const std::any& window) { return HJ_OK; }
    virtual int flush() { return HJ_OK; }
    virtual bool isReady() {
        return (HJRun_Ready == m_runState || HJRun_Running == m_runState);
    }
public:
    static HJVRenderBase::Ptr createVideoRender();
protected:
    HJRunState             m_runState = HJRun_NONE;
    HJVideoInfo::Ptr       m_info;
    std::recursive_mutex    m_mutex;
};

NS_HJ_END
