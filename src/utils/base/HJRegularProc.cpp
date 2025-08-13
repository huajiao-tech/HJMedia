#include "HJRegularProc.h"
#include "HJTime.h"

NS_HJ_BEGIN

HJBaseRegularProc::HJBaseRegularProc()
{
}
HJBaseRegularProc::~HJBaseRegularProc()
{
}
bool HJBaseRegularProc::tryUpdate()
{
	return false;
}

void HJBaseRegularProc::proc(RegularFun i_fun, bool i_bConditional)
{
	do
	{	
		if (tryUpdate() || i_bConditional)
		{	
			if (i_fun)
			{
				i_fun();
			}		
		}
	} while (false);
}

//////////////////////////////////////////////////////
HJRegularProcTimer::HJRegularProcTimer()
{
}
HJRegularProcTimer::~HJRegularProcTimer()
{
}
void HJRegularProcTimer::setInervalTime(int64_t i_intervalTime)
{
	m_interval = i_intervalTime;
}

bool HJRegularProcTimer::tryUpdate()
{
	bool bUpdate = false;
	do 
	{
		int64_t curTime = HJCurrentSteadyMS();
		if (m_currentTime < 0)
		{
			bUpdate = true;		
		}
		else if ((curTime - m_currentTime) >= m_interval)
		{
			bUpdate = true;			
		}

		if (bUpdate)
		{
			m_currentTime = curTime;
		}
	} while (false);
	return bUpdate;
}

////////////////////////////////////////////////////////////
HJRegularProcFreq::HJRegularProcFreq()
{
}
HJRegularProcFreq::~HJRegularProcFreq()
{
}
void HJRegularProcFreq::setInervalFreq(int64_t i_intervalFreq)
{
	m_freqInterval = i_intervalFreq;
}

bool HJRegularProcFreq::tryUpdate()
{
	bool bUpdate = false;
	do
	{
		m_currentFreqIdx++;
		bUpdate = (m_currentFreqIdx % m_freqInterval) == 0;
	} while (false);
	return bUpdate;
}

NS_HJ_END
