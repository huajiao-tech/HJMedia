//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBaseCodec.h"
#include "HJADecFFMpeg.h"
#include "HJVDecFFMpeg.h"
#include "HJAEncFFMpeg.h"
#include "HJVEncFFMpeg.h"

#if defined(HJ_OS_HARMONY)
    #include "hsys/HJVEncOHCodec.h"
    #include "hsys/HJVDecOHCodec.h"
#endif

//#if defined(HJ_OS_DARWIN)
//#   include "isys/HJVEncVTB.h"
//#endif

NS_HJ_BEGIN
//***********************************************************************************//
HJBaseCodec::~HJBaseCodec()
{
    m_trackers.clear();
}

int HJBaseCodec::init(const HJStreamInfo::Ptr& info)
{
    if (!info) {
        return HJErrInvalidParams;
    }
    m_info = info->dup();
    m_nipMuster = std::make_shared<HJNipMuster>();
    if (info->getBool("pop_front_frame")) {
        m_popFrontFrame = info->getBool("pop_front_frame");
    }
    
    return HJ_OK;
}

HJBaseCodec::Ptr HJBaseCodec::createADecoder(int type/* = 0*/)
{
    HJBaseCodec::Ptr decoder = nullptr;
    
    decoder = std::make_shared<HJADecFFMpeg>();
    
    return decoder;
}

HJBaseCodec::Ptr HJBaseCodec::createVDecoder(HJDeviceType type/* = HJDEVICE_TYPE_NONE*/)
{
    HJBaseCodec::Ptr decoder = nullptr;
    switch (type) {
        case HJDEVICE_TYPE_NONE:
        case HJDEVICE_TYPE_VDPAU:
        case HJDEVICE_TYPE_CUDA:
        case HJDEVICE_TYPE_VAAPI:
        case HJDEVICE_TYPE_QSV:
        case HJDEVICE_TYPE_VIDEOTOOLBOX:
        case HJDEVICE_TYPE_MEDIACODEC:{
            decoder = std::make_shared<HJVDecFFMpegPlus>();
            break;
        }
    case HJDEVICE_TYPE_OHCODEC:
#if defined (HarmonyOS)
            decoder = std::make_shared<HJVDecOHCodec>();
#endif
            break;
        default: {
            HJLoge("create decoder not support, type:" + HJ2STR(type));
            break;
        }
    }
    return decoder;
}

HJBaseCodec::Ptr HJBaseCodec::createDecoder(const HJMediaType mediaType, HJDeviceType type/* = 0*/)
{
    if(HJMEDIA_TYPE_AUDIO == mediaType) {
        return createADecoder(type);
    } else if (HJMEDIA_TYPE_VIDEO == mediaType) {
        return createVDecoder(type);
    }
    return nullptr;
}

HJBaseCodec::Ptr HJBaseCodec::createAEncoder(int type/* = 0*/)
{
    HJBaseCodec::Ptr encoder = nullptr;
    
    encoder = std::make_shared<HJAEncFFMpeg>();
    
    return encoder;
}

HJBaseCodec::Ptr HJBaseCodec::createVEncoder(HJDeviceType type/* = 0*/)
{
    HJBaseCodec::Ptr encoder = nullptr;
    switch (type)
    {
    case HJDEVICE_TYPE_NONE:
    case HJDEVICE_TYPE_VDPAU:
    case HJDEVICE_TYPE_CUDA:
    case HJDEVICE_TYPE_VAAPI:
    case HJDEVICE_TYPE_QSV:
    case HJDEVICE_TYPE_MEDIACODEC: {
        encoder = std::make_shared<HJVEncFFMpeg>();
        break;
    }
#if defined(HJ_OS_HARMONY)
    case HJDEVICE_TYPE_OHCODEC: {
        encoder = std::make_shared<HJVEncOHCodec>();
        break;    
    }
#endif
//#if defined(HJ_OS_IOS)
//    case HJDEVICE_TYPE_VIDEOTOOLBOX: {
//        encoder = std::make_shared<HJVEncVTB>();
//        break;
//    }
//#endif
#if defined(HJ_HAVE_TSCSDK)
    case HJDEVICE_TYPE_TSCSDK: {
        encoder = std::make_shared<HJVEncTXY>();
        break;
    }
#endif
    default:
        break;
    }
    return encoder;
}

HJBaseCodec::Ptr HJBaseCodec::createEncoder(const HJMediaType mediaType, HJDeviceType type/* = 0*/)
{
    if(HJMEDIA_TYPE_AUDIO == mediaType) {
        return createAEncoder(type);
    } else if (HJMEDIA_TYPE_VIDEO == mediaType) {
        return createVEncoder(type);
    }
    return nullptr;
}

NS_HJ_END
