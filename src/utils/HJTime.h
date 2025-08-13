//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <chrono>
#include "HJCommons.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJTime
{
public:
    //system start
    static int64_t getCurrentEpochMS() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    static int64_t getCurrentEpochUS() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    static int64_t getCurrentEpochNS() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    static int64_t getCurrentEpochHighNS() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    //1970-01-01 00:00:00 UTC
    static int64_t getCurrentMillisecond() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    static int64_t getCurrentMicrosecond() {
        return  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    static int64_t getCurrentNanosecond() {
        return  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    static int64_t getCurrentSteadyMS();
    static int64_t getCurrentSteadyUS();

    static struct tm getLocalTime(time_t sec);
};

#define HJCurrentMillisecond()     HJ::HJTime::getCurrentMillisecond()
#define HJCurrentMicrosecond()     HJ::HJTime::getCurrentMicrosecond()
#define HJCurrentNanoseconds()     HJ::HJTime::getCurrentNanoseconds()
//
#define HJCurrentEpochMS()         HJ::HJTime::getCurrentEpochMS()
#define HJCurrentEpochUS()         HJ::HJTime::getCurrentEpochUS()
#define HJCurrentEpochNS()         HJ::HJTime::getCurrentEpochNS()
#define HJCurrentEpochHNS()        HJ::HJTime::getCurrentEpochHighNS()

#define HJCurrentSteadyMS()        HJ::HJTime::getCurrentSteadyMS()
#define HJCurrentSteadyUS()        HJ::HJTime::getCurrentSteadyUS()

NS_HJ_END
