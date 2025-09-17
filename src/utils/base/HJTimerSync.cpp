#include "HJFLog.h"
#include "HJTimerSync.h"
#include "HJTime.h"

NS_HJ_BEGIN



HJBaseTimerSync::HJBaseTimerSync()
{

}

HJBaseTimerSync::~HJBaseTimerSync()
{

}

////////////////////////////////////////////////////

const int64_t HJSysTimerSync::s_timeThres = 2000;

HJSysTimerSync::HJSysTimerSync()
{

}
HJSysTimerSync::~HJSysTimerSync()
{

}
void HJSysTimerSync::priReset()
{
    m_cacheSysTime =  (std::numeric_limits<int64_t>::min)();
    m_cachePts = (std::numeric_limits<int64_t>::min)();
}
int HJSysTimerSync::sync(int64_t i_curPts, int64_t& o_delayTime)
{
	int i_err = HJ_OK;
	do
	{
		int64_t curTime = HJCurrentSteadyMS();

		if (m_cacheSysTime == (std::numeric_limits<int64_t>::min)())
		{
			m_cacheSysTime = curTime;
			m_cachePts = i_curPts;
		}

		int64_t elapseSystemTime = curTime - m_cacheSysTime;
		int64_t elapsePts = i_curPts - m_cachePts;
        
        if (abs(elapsePts) >= s_timeThres)
        {
            HJFLogi("elapsePt:{} is large, priReset", elapsePts);
            priReset();
            o_delayTime = 0;
            break;
        }    

		int64_t diff = elapsePts - elapseSystemTime;
		if (diff > 0)
		{
			i_err = HJ_WOULD_BLOCK;
			o_delayTime = diff;
			break;
		}
		else
		{
			o_delayTime = 0;
		}
	} while (false);
	return i_err;
}

NS_HJ_END
