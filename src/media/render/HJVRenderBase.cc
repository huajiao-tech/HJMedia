//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJVRenderBase.h"
#include "HJVRenderFake.h"
// #if defined(HJ_OS_IOS)
// #include "isys/HJVideoRenderIOS.h"
// #endif

NS_HJ_BEGIN
//***********************************************************************************//
int HJVRenderBase::init(const HJVideoInfo::Ptr& info)
{
    if (!info) {
        return HJErrInvalidParams;
    }
    m_info = info;
    
    return HJ_OK;
}

//***********************************************************************************//
HJVRenderBase::Ptr HJVRenderBase::createVideoRender()
{
    HJVRenderBase::Ptr baseRender = nullptr;
#if defined(HJ_OS_IOS)
//    baseRender = std::dynamic_pointer_cast<HJVRenderBase>(std::make_shared<HJVideoRenderIOS>());
#else
    //baseRender = std::dynamic_pointer_cast<HJVRenderBase>(std::make_shared<HJVRenderSDL>());
    baseRender = std::dynamic_pointer_cast<HJVRenderBase>(std::make_shared<HJVRenderFake>());
#endif
    return baseRender;
}

NS_HJ_END
