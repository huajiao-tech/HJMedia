//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMacros.h"
#include "HJError.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJStatus
{
    HJSTATUS_NONE = 0x00,
    HJSTATUS_Initializing,
    HJSTATUS_Inited,
    //--------------------
    HJSTATUS_Ready,
    HJSTATUS_Running,
    HJSTATUS_EOF,
    HJSTATUS_Stoped,
    HJSTATUS_Exception,
    //--------------------
    HJSTATUS_Releasing,
    HJSTATUS_Released,
    HJSTATUS_Done
} HJStatus;

using HJNanoSeconds = std::chrono::nanoseconds;
using HJMicroSeconds = std::chrono::microseconds;
using HJMilliSeconds = std::chrono::milliseconds;
using HJSeconds = std::chrono::seconds;
using HJMinutes = std::chrono::minutes;
using HJHours = std::chrono::hours;

#define HJNanaTimePoint(t)     std::chrono::time_point<std::chrono::steady_clock, HJNanoSeconds>(HJNanoSeconds(t))
#define HJMicroTimePoint(t)    std::chrono::time_point<std::chrono::steady_clock, HJMicroSeconds>(HJMicroSeconds(t))
#define HJMiniTimePoint(t)     std::chrono::time_point<std::chrono::steady_clock, HJMilliSeconds>(HJMilliSeconds(t))
#define HJSecondTimePoint(t)   std::chrono::time_point<std::chrono::steady_clock, HJSeconds>(HJSeconds(t))
#define HJMinuteTimePoint(t)   std::chrono::time_point<std::chrono::steady_clock, HJMinutes>(HJMinutes(t))
#define HJHourTimePoint(t)     std::chrono::time_point<std::chrono::steady_clock, HJHours>(HJHours(t))

using HJRunnable = std::function<void(void)>;

using HJThreadID     = std::thread::id;
using HJThreadPtr    = std::shared_ptr<std::thread>;
#define HJThreadID2ID(tid)     (*(uint32_t *)&tid)

typedef void* HJHND;
using HJHNDPtr = std::shared_ptr<void>;
inline HJHNDPtr HJAutoHNDPtr(HJHND handle) {
    return std::shared_ptr<void>(handle, [](HJHND ptr) {
        if (ptr) {
            free(ptr);
        }
        });
}
template<typename FUNC>
inline HJHNDPtr HJAutoHNDPtr(HJHND handle, FUNC&& func) {
    return std::shared_ptr<void>(handle, std::move(func));
}

template<class T, class U>
std::weak_ptr<T> static_pointer_cast(std::weak_ptr<U> const& r) {
    return std::static_pointer_cast<T>(std::shared_ptr<U>(r));
}

template<typename T>
static inline std::shared_ptr<T>HJLockWtr(std::weak_ptr<T> const& wp) {
    if (wp.expired()) {
        return nullptr;
    }
    return wp.lock();
}

using HJRawBuffer = std::vector<uint8_t>;
//***********************************************************************************//


NS_HJ_END

