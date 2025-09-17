//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJMediaUtils.h"
#include "HJLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJAVSyncMode
{
    HJ_AVSYNC_DEFAULT = 0x00,
    HJ_AVSYNC_SINGLE,
    HJ_AVSYNC_LOWDELAY,
} HJAVSyncMode;
/*
 * 1:VOD,live --  video sync with audio clock
 * 2:low delay -- sync with audio clock range
 * 3:single audio or video -- sync with system clock
 * 4:pause, VOD loading, live loading
 */
class HJTimelineHandler : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJTimelineHandler>;
    HJTimelineHandler();
    virtual ~HJTimelineHandler();
    
    void start();
    void pause();
    void update(int64_t time = HJ_NOTS_VALUE);
    int64_t getTimestamp();
    //
    void reset();
    void setSpeed(const float speed) {
        m_speed = speed;
    }
    const float getSpeed() const {
        return m_speed;
    }
private:
    std::recursive_mutex    m_mutex;
    int64_t                 m_sysTime = HJ_NOTS_VALUE;      //ms
    int64_t                 m_refTime = 0;
    HJRange64i             m_timeRange = {-10, 10};
    HJAVSyncMode           m_avsyncMode = HJ_AVSYNC_DEFAULT;
    float                   m_speed = 2.0f;
};

NS_HJ_END
