//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJTimelineHandler.h"
#include "HJContext.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJTimelineHandler::HJTimelineHandler()
{
    
}

HJTimelineHandler::~HJTimelineHandler()
{
    
}

void HJTimelineHandler::start()
{
    HJ_AUTO_LOCK(m_mutex);
    m_sysTime = HJCurrentSteadyMS();

    return;
}

void HJTimelineHandler::pause()
{
    HJ_AUTO_LOCK(m_mutex);
//    m_sysTime = HJUtils::getCurrentMillisecond();
    if(HJ_AVSYNC_SINGLE == m_avsyncMode) {
        
    }

    return;
}

void HJTimelineHandler::update(int64_t time)
{
    HJ_AUTO_LOCK(m_mutex);
    if (HJ_NOTS_VALUE != time) {
        m_refTime = time;
    }
    m_sysTime = HJCurrentSteadyMS();

    return;
}

int64_t HJTimelineHandler::getTimestamp()
{
    HJ_AUTO_LOCK(m_mutex);
    const auto now = HJCurrentSteadyMS();
    if (HJ_NOTS_VALUE == m_sysTime) {
        m_sysTime = now;
    }
    int64_t time = m_refTime + (now - m_sysTime) * m_speed;

    return time;
}

void HJTimelineHandler::reset()
{
    HJ_AUTO_LOCK(m_mutex);
    m_sysTime = HJ_NOTS_VALUE;
    m_refTime = 0;
}

NS_HJ_END
