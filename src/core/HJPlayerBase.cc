//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJPlayerBase.h"
#include "HJContext.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJPlayerBase::HJPlayerBase(const HJScheduler::Ptr& scheduler/* = nullptr*/)
{
    setName(HJMakeGlobalName("player"));
    if(scheduler) {
        m_scheduler = scheduler;
    } else {
        m_scheduler = std::make_shared<HJScheduler>();
    }
    m_env = std::make_shared<HJEnvironment>();
}

HJPlayerBase::~HJPlayerBase()
{
    
}

int HJPlayerBase::init(const HJMediaUrl::Ptr& mediaUrl)
{
    m_mediaUrl = mediaUrl->dup();
    
    return HJ_OK;
}

int HJPlayerBase::init(const HJMediaUrlVector& mediaUrls)
{
    for (const auto& url : mediaUrls) {
        m_mediaUrls.emplace_back(url->dup());
    }
    return HJ_OK;
}

int HJPlayerBase::start()
{
    return HJ_OK;
}

int HJPlayerBase::play()
{
    return HJ_OK;
}

int HJPlayerBase::pause()
{
    return HJ_OK;
}
int HJPlayerBase::resume()
{
    return HJ_OK;
}
//
int HJPlayerBase::setWindow(const std::any& window)
{
    return HJ_OK;
}
int HJPlayerBase::seek(int64_t pos)
{
    return HJ_OK;
}
int HJPlayerBase::seek(float progress)
{
    return HJ_OK;
}
int HJPlayerBase::speed(float speed)
{
    return HJ_OK;
}
int64_t HJPlayerBase::getCurrentPos()
{
    return 0;
}
float HJPlayerBase::getProgress()
{
    return 0.0f;
}

void HJPlayerBase::setVolume(const float volume)
{

}

NS_HJ_END
