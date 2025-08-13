//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJARenderBase.h"
#include "HJARenderMini.h"
// #if defined(HJ_OS_IOS)
// #include "HJAudioRenderIOS.h"
// #endif

NS_HJ_BEGIN
//***********************************************************************************//
int HJARenderBase::init(const HJAudioInfo::Ptr& info, ARCallback cb/* = nullptr*/)
{
    if (!info) {
        return HJErrInvalidParams;
    }
    m_info = info;
    m_callback = cb;
    
    return HJ_OK;
}

int HJARenderBase::start()
{
    return HJ_OK;
}

int HJARenderBase::pause()
{
    return HJ_OK;
}

int HJARenderBase::resume()
{
    return HJ_OK;
}

int HJARenderBase::reset()
{
    return HJ_OK;
}

int HJARenderBase::stop()
{
    return HJ_OK;
}

int HJARenderBase::write(const HJMediaFrame::Ptr rawFrame)
{
    return HJ_OK;
}

//***********************************************************************************//
HJARenderBase::Ptr HJARenderBase::createAudioRender()
{
    HJARenderBase::Ptr baseRender = nullptr;
#if defined(HJ_OS_IOS)
//    baseRender = std::dynamic_pointer_cast<HJARenderBase>(std::make_shared<HJAudioQueueRender>());
#else
    //baseRender = std::dynamic_pointer_cast<HJARenderBase>(std::make_shared<HJARenderSDL>());
    baseRender = std::dynamic_pointer_cast<HJARenderBase>(std::make_shared<HJARenderMini>());
#endif
    return baseRender;
}

NS_HJ_END

