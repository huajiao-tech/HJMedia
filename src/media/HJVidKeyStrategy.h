#pragma once

#include "HJComUtils.h"

NS_HJ_BEGIN

class HJMediaFrame;
typedef enum HJVidKeyStrategyState
{
    HJVidKeyStrategyState_idle,
    HJVidKeyStrategyState_run,
    HJVidKeyStrategyState_filter,
    HJVidKeyStrategyState_error,
    HJVidKeyStrategyState_done,
}HJVidKeyStrategyState;

class HJVidKeyStrategy : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJVidKeyStrategy);
    HJVidKeyStrategy();
    virtual ~HJVidKeyStrategy();
    
    void process(const std::shared_ptr<HJMediaFrame>& i_frame, std::function<int()> i_func);
    //call notifyOutput thread is same as process; the function is not thread safe;
    bool filter();
    bool isAvaiable();
    
    void setQuit()
    {
        m_bQuit = true;
    }
private:
    const static int s_nTryKeyThres;
    int m_statInputIdx = 0;
    int64_t m_statStartTime = 0;
    int m_filterIdx = 0;
    bool priIsDone();    
    std::atomic<bool> m_bQuit{false};
    HJVidKeyStrategyState m_state = HJVidKeyStrategyState_idle;
    
};

NS_HJ_END
