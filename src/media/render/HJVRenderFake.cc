//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJVRenderFake.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJVRenderFake::HJVRenderFake()
{

}

HJVRenderFake::~HJVRenderFake()
{

}

int HJVRenderFake::init(const HJVideoInfo::Ptr& info)
{
    HJLogi("entry");
    m_nipMuster = std::make_shared<HJNipMuster>();

    return HJ_OK;
}

int HJVRenderFake::render(const HJMediaFrame::Ptr frame)
{
    HJNipInterval::Ptr nip = m_nipMuster->getInNip();
    if (nip && nip->valid()) {
        HJLogi("entry, frame info:" + frame->formatInfo());
    }

    return HJ_OK;
}

int HJVRenderFake::setWindow(const std::any& window)
{
    HJLogi("entry");
    m_runState = HJRun_Ready;

    return HJ_OK;
}

int HJVRenderFake::flush()
{
    return HJ_OK;
}

NS_HJ_END

