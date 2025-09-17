#pragma once

#include "HJPrerequisites.h"
#include <limits>

NS_HJ_BEGIN

class HJBaseTimerSync
{
public:
	HJ_DEFINE_CREATE(HJBaseTimerSync);
	HJBaseTimerSync();
	virtual ~HJBaseTimerSync();
private:
};


class HJSysTimerSync : public HJBaseTimerSync
{
public:
	HJ_DEFINE_CREATE(HJSysTimerSync);

	HJSysTimerSync();
	virtual ~HJSysTimerSync();

	int sync(int64_t i_curPts, int64_t& o_delayTime);

private:
    void priReset();
    
    const static int64_t s_timeThres;
	int64_t m_cacheSysTime = (std::numeric_limits<int64_t>::min)();
	int64_t m_cachePts = (std::numeric_limits<int64_t>::min)();
};


NS_HJ_END