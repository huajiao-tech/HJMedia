#include "HJVidKeyStrategy.h"
#include "HJFLog.h"
#include "HJMediaFrame.h"
#include "HJTime.h"

NS_HJ_BEGIN

const int HJVidKeyStrategy::s_nTryKeyThres = 40;

HJVidKeyStrategy::HJVidKeyStrategy()
{
    HJ_SetInsName(HJVidKeyStrategy);
}
HJVidKeyStrategy::~HJVidKeyStrategy()
{
}
bool HJVidKeyStrategy::priIsDone()
{
    return (m_state == HJVidKeyStrategyState_done);
}

void HJVidKeyStrategy::process(const std::shared_ptr<HJMediaFrame>& i_frame, std::function<int()> i_func)
{
    do 
    {
        if (priIsDone())
        {
            break;    
        }
        if (m_state == HJVidKeyStrategyState_idle)
        {
            if (!i_frame->isKeyFrame())
            {
                m_state = HJVidKeyStrategyState_done;
                HJFLogi("{} the first frame is not key, not proc", getInsName());
                break;
            }    
            m_state = HJVidKeyStrategyState_run;
            m_statInputIdx = 0;
            m_statStartTime = HJCurrentSteadyMS();
        }    
        
        if (m_state == HJVidKeyStrategyState_run)
        {
            int i = 0;
            for (i = 0; (i < s_nTryKeyThres) && !m_bQuit; i++, m_statInputIdx++)
            {
                HJFLogi("{} keyframe func i:{} idx:{}", getInsName(), i, m_statInputIdx);
                int i_err = i_func();
                if (i_err == HJ_CLOSE)
                {
                    int64_t diff = HJCurrentSteadyMS() - m_statStartTime;
                    HJFLogi("{} keyframe input end, change to filter i:{} idx:{} time:{}", getInsName(), i, m_statInputIdx, diff);
                    m_state = HJVidKeyStrategyState_filter;
                    break;
                }    
                else if (i_err < 0)
                {
                    HJFLoge("{} keyframe i:{} error find i_err:{} idx:{}", getInsName(), i, i_err, m_statInputIdx);
                    m_state = HJVidKeyStrategyState_done;
                    break;
                }        
            } 
            if (i == s_nTryKeyThres)
            {
                m_state = HJVidKeyStrategyState_filter;
                HJFLogi("{} keyframe i:{} idx:{} attain thres, direct change to filter", getInsName(), i, m_statInputIdx);
            }    
        }
    } while (false);
}

bool HJVidKeyStrategy::filter()
{
    bool bFilter = false;
    do 
    {
        if (m_state != HJVidKeyStrategyState_filter)
        {
            break;    
        }
        
        if (m_filterIdx < m_statInputIdx)
        {
            HJFLogi("{} stat filter filterIdx:{} statInputIdx:{}", getInsName(), m_filterIdx, m_statInputIdx);
            bFilter = true;
        }
        else 
        {
            m_state = HJVidKeyStrategyState_done;
            HJFLogi("{} stat filter end-----------------done, filterIdx:{} statInputIdx:{}", getInsName(), m_filterIdx, m_statInputIdx);
        }    
        m_filterIdx++;
    } while (false);
    return bFilter;
}
bool HJVidKeyStrategy::isAvaiable()
{
    return (m_state == HJVidKeyStrategyState_idle);
}

NS_HJ_END