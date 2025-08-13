//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBaseCapture.h"
#if defined(HJ_OS_HARMONY)
#include "HJACaptureOH.h"
#endif

NS_HJ_BEGIN

//***********************************************************************************//
HJBaseCapture::~HJBaseCapture()
{
   
}

int HJBaseCapture::init(const HJStreamInfo::Ptr& info)
{
    if (!info) {
        return HJErrInvalidParams;
    }
    m_info = info->dup();
    return HJ_OK;
}
HJBaseCapture::Ptr HJBaseCapture::createACapture(HJCaptureType type)
{
    HJBaseCapture::Ptr capture = nullptr;
    switch (type)
    {
#if defined(HJ_OS_HARMONY)
    case HJCAPTURE_TYPE_OH:
        capture = std::make_shared<HJACaptureOH>();
        break;
#endif
    default:
        break;
    }
    return capture;
}
HJBaseCapture::Ptr HJBaseCapture::createVCapture(HJCaptureType type)
{
    HJBaseCapture::Ptr capture = nullptr;
    switch (type)
    {
    case HJCAPTURE_TYPE_OH:
        break;
        
    default:
        break;
    }
    return capture;    
}

HJBaseCapture::Ptr HJBaseCapture::createCapture(const HJMediaType mediaType, HJCaptureType type)
{
    if(HJMEDIA_TYPE_AUDIO == mediaType) {
        return createACapture(type);
    } else if (HJMEDIA_TYPE_VIDEO == mediaType) {
        return createVCapture(type);
    }
    return nullptr;
}

NS_HJ_END
