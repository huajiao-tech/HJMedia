#include "HJStatBaseNotify.h"
#include "HJTime.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJStatBaseNotify::~HJStatBaseNotify()
{
	HJFLogi("{} ~HJStatBaseNotify ", m_insName);
	if (m_heartBeat)
	{
		m_heartBeat.reset();
		m_heartBeat = nullptr;
	}
	HJFLogi("{} ~HJStatBaseNotify end", m_insName);
}
int64_t HJStatBaseNotify::GetClockTimeFromTime(int64_t i_curSysTime)
{
	return i_curSysTime + m_clockOffset;
}
int64_t HJStatBaseNotify::GetCurClockTime()
{
	return GetClockTimeFromTime(HJCurrentSteadyMS());
}
void HJStatBaseNotify::setInsName(const std::string& i_insName)
{
	m_insName = i_insName;
}
HJStatBaseNotify::Ptr HJStatBaseNotify::GetPtr(HJStatBaseNotify::Wtr i_wr)
{
	HJStatBaseNotify::Ptr ptr = nullptr;
	do
	{
		if (!i_wr.expired())
		{
			ptr = i_wr.lock();
			if (!ptr)
			{
				break;
			}
		}
		else
		{
			break;
		}
	} while (false);
	return ptr;
}

int HJStatBaseNotify::heartBeatInit()
{
	int i_err = 0;
	do
	{
		m_heartBeat = HJTimerTask::Create();
		i_err = m_heartBeat->init("timetask_" + m_insName);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
void HJStatBaseNotify::heartBeatJoin()
{
	if (m_heartBeat)
	{
		m_heartBeat->joinTaskThread();
		m_heartBeat.reset();
		m_heartBeat = nullptr;
	}
}
int HJStatBaseNotify::heartbeatRegist(const std::string& i_unqiueName, int i_interval, TimerTaskFun i_cb)
{
	int i_err = 0;
	do
	{
		if (m_heartBeat)
		{
			i_err = m_heartBeat->regist(i_unqiueName, i_interval, i_cb);
		}
	} while (false);
	return i_err;
}
int HJStatBaseNotify::heartbeatUnRegist(const std::string& i_unqiueName)
{
	int i_err = 0;
	do
	{
		if (m_heartBeat)
		{
			i_err = m_heartBeat->unregist(i_unqiueName);
		}
	} while (false);
	return i_err;
}

NS_HJ_END



