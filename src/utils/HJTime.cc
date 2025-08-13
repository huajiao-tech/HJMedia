//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJTime.h"

NS_HJ_BEGIN
//***********************************************************************************//
int64_t HJTime::getCurrentSteadyMS()
{
    static const std::atomic<int64_t> s_currentMillisecond_delta(getCurrentEpochMS() - getCurrentMillisecond());
    return getCurrentMillisecond() + s_currentMillisecond_delta.load(std::memory_order_acquire);
}

int64_t HJTime::getCurrentSteadyUS()
{
    static const std::atomic<int64_t> s_currentMicrosecond_delta(getCurrentEpochUS() - getCurrentMicrosecond());
    return getCurrentMicrosecond() + s_currentMicrosecond_delta.load(std::memory_order_acquire);
}

struct tm HJTime::getLocalTime(time_t sec) {
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &sec);
#else
    localtime_r(&sec, &tm);
#endif //_WIN32
    return tm;
}


NS_HJ_END