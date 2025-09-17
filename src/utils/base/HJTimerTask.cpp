#include "HJFLog.h"
#include "HJTime.h"
#include "HJTimerTask.h"

NS_HJ_BEGIN

const int HJTimerTask::s_TimerTaskSleepTime = 200;

const std::string HJTimerTask::TIMERTASK_NAME_LOADING = "TIMERTASK_NAME_LOADING";
const std::string HJTimerTask::TIMERTASK_NAME_PLAYERTIME = "TIMERTASK_NAME_PLAYERTIME";

const std::unordered_map <std::string, int> HJTimerTask::s_TimerTaskMap = {
	{TIMERTASK_NAME_LOADING,    100},
	{TIMERTASK_NAME_PLAYERTIME, 100}
};

//#define TUPLE_IDX_INTERVAL 0
//    const std::unordered_map <std::string, std::tuple<int>> HJTimerTask::s_HeartbeatTabel =
//    {
//        {TIMERTASK_NAME_LOADING,    std::make_tuple(100)},
//        {TIMERTASK_NAME_PLAYERTIME, std::make_tuple(1000)}
//    };

HJTaskFunPack::HJTaskFunPack(HJTimerTaskItem::Ptr i_item, const std::string& i_uniqueName, int64_t i_curTime, int64_t i_duration) :
	m_item(i_item)
	, m_uniqueName(i_uniqueName)
	, m_curTime(i_curTime)
	, m_duration(i_duration)
{
}
void HJTimerTask::joinTaskThread()
{
	HJFLogi("{}: TimerTask join enter", m_insName);
	if (worker_thread_.joinable())
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			m_bQuit = true;
		}
		cv_.notify_one();
		HJFLogi("{}: TimerTask thread join enter", m_insName);
		worker_thread_.join();
		HJFLogi("{}: TimerTask thread join end", m_insName);
	}
	HJFLogi("{}: TimerTask join end", m_insName);
}
HJTimerTask::~HJTimerTask()
{
	HJFLogi("{}: TimerTask ~HJTimerTask enter", m_insName);
	joinTaskThread();
	HJFLogi("{}: TimerTask ~HJTimerTask end", m_insName);
}
int HJTimerTask::priSetState(HJTimerTaskState::Ptr& io_ptr)
{
	int i_err = 0;
	do
	{
		if (io_ptr->m_state == HJTimerTaskState::STATE_UNREGIST)
		{
			HJTimerTaskItem::Ptr item = nullptr;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				m_nSignal++;
				if (priTaskContains(io_ptr->m_uniqueName))
				{
					item = m_taskTimerTaskMap.at(io_ptr->m_uniqueName);
					m_taskTimerTaskMap.erase(io_ptr->m_uniqueName);
					HJFLogi("{} TimerTask unregist remove task, m_uniqueName is: {}", m_insName, io_ptr->m_uniqueName);
				}
			}
			if (item && item->m_fun)
			{
				{
					std::lock_guard<std::mutex> itemLock(item->m_item_mutex);
					item->m_bStop = true;
				}
				int64_t curTime = HJCurrentSteadyMS();
				int64_t durtaion = curTime - item->m_preTime;
				if (durtaion > 0)
				{
					item->m_fun(io_ptr->m_uniqueName, TimerTask_Update, curTime, durtaion); //callback not in lock
					HJFLogi("{} TimerTaskFun unregist remove task, m_uniqueName is: {} interval:{} last duration: {} diff:{}", m_insName, io_ptr->m_uniqueName, item->m_interval, durtaion, (item->m_interval - durtaion));
				}
			}
		}
		else if (io_ptr->m_state == HJTimerTaskState::STATE_REGIST)
		{
			if (io_ptr->m_fun)
			{
				HJFLogi("{} TimerTaskFun regist upload heart name:{} duration:0", m_insName, io_ptr->m_uniqueName); //callback not in lock
				io_ptr->m_fun(io_ptr->m_uniqueName, TimerTask_Update, HJCurrentSteadyMS(), 0);
			}

			std::lock_guard<std::mutex> lock(mutex_);
			m_nSignal++;
			if (!priTaskContains(io_ptr->m_uniqueName))
			{
				HJTimerTaskItem::Ptr item = HJTimerTaskItem::Create();
				item->m_startTime = HJCurrentSteadyMS();
				item->m_interval = io_ptr->m_interval;
				item->m_fun = io_ptr->m_fun;
				item->m_idx = 0;
				item->m_idx++;
				item->m_nextTime = item->m_startTime + (int64_t)item->m_idx * (int64_t)item->m_interval;

				item->m_preTime = item->m_startTime;
				m_taskTimerTaskMap[io_ptr->m_uniqueName] = item;
				HJFLogi("{} TimerTask begin name:{}, interval:{} startTime:{}", m_insName, io_ptr->m_uniqueName, item->m_interval, item->m_startTime);
			}
		}
		cv_.notify_one();
	} while (false);
	return i_err;
}

int HJTimerTask::regist(const std::string& i_unqiueName, int i_interval, TimerTaskFun i_cb)
{
	int i_err = 0;
	do
	{
		HJTimerTaskState::Ptr ptr = HJTimerTaskState::Create();
		//ptr->m_beginTime = HJCurrentSteadyMS();
		ptr->m_uniqueName = i_unqiueName;
		ptr->m_fun = i_cb;
		ptr->m_interval = i_interval;
		//ptr->m_endTime   = -1;	
		ptr->m_state = HJTimerTaskState::STATE_REGIST;
		HJFLogi("{} TimerTask regist enter uniquename:{}", m_insName, i_unqiueName);
		i_err = priSetState(ptr);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
int HJTimerTask::unregist(const std::string& i_unqiueName)
{
	int i_err = 0;
	do
	{
		HJTimerTaskState::Ptr ptr = HJTimerTaskState::Create();
		ptr->m_uniqueName = i_unqiueName;
		//ptr->m_endTime = HJCurrentSteadyMS();
		ptr->m_state = HJTimerTaskState::STATE_UNREGIST;
		//ptr->m_beginTime = -1; 
		ptr->m_fun = nullptr;
		HJFLogi("{} TimerTask unregist enter uniquename:{}", m_insName, i_unqiueName);

		i_err = priSetState(ptr);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
bool HJTimerTask::priTaskContains(const std::string& i_uniqueName)
{
	return (m_taskTimerTaskMap.find(i_uniqueName) != m_taskTimerTaskMap.end());
}
void HJTimerTask::priCallFuns(std::deque<HJTaskFunPack>& taskFunDeque)
{
	if (!taskFunDeque.empty())
	{
		for (auto it = taskFunDeque.begin(); it != taskFunDeque.end(); it++)
		{
			HJTaskFunPack& pack = *it;
			std::lock_guard<std::mutex> itemLock(pack.m_item->m_item_mutex);
			if (!pack.m_item->m_bStop)
			{
				pack.m_item->m_fun(pack.m_uniqueName, TimerTask_Update, pack.m_curTime, pack.m_duration);
				//HJFLogi("{} TimerTaskItem callback uniquename:{}", m_insName, pack.m_uniqueName);
			}
			else
			{
				HJFLogi("{} TimerTaskItem:{} has been stopped not callback fun", m_insName, pack.m_uniqueName);
			}
		}
	}
}
int64_t HJTimerTask::priDetectCall(std::deque<HJTaskFunPack>& taskFunDeque)
{
	int64_t sleepTime = std::numeric_limits<int64_t>::max();

	std::lock_guard<std::mutex> lock(mutex_);
	int64_t curTime = HJCurrentSteadyMS();
	//HJFLogi("{} TimerTask priDetectCall time:{}", m_insName, curTime);
	for (const auto& pair : m_taskTimerTaskMap)
	{
		const std::string uniqueName = pair.first;
		const HJTimerTaskItem::Ptr& item = pair.second;
		if (curTime >= item->m_nextTime)
		{
			int64_t duration = (curTime - item->m_preTime);
			taskFunDeque.emplace_back(item, uniqueName, curTime, duration);

			int idx = item->m_idx;
			while (curTime >= item->m_nextTime) //Give hot pursuit, for example the system idle;
			{
				item->m_idx++;
				item->m_nextTime = item->m_startTime + item->m_idx * item->m_interval;
			}
			if ((item->m_idx - idx) > 1)
			{
				HJFLogi("{} TimerTask upload name:{} idxdiff:{} duration:{}", m_insName, uniqueName, (item->m_idx - idx), duration);
			}
			int64_t diff = item->m_nextTime - curTime;
			sleepTime = std::min(sleepTime, diff);
			//HJFLogi("{} TimerTask upload task name:{}, interval:{} idx:{} curTime:{} nextTime:{} Mapsize:{} diffPre:{}", m_insName, uniqueName, item->m_interval, item->m_idx, curTime, item->m_nextTime, (int)m_taskTimerTaskMap.size(), duration);
			item->m_preTime = curTime;
		}
		else
		{
			int64_t diff = item->m_nextTime - curTime;
			sleepTime = std::min(sleepTime, diff);
			//HJFLogi("{} TimerTask getmin name:{}, sleepTime:{} curSleep:{}", m_insName, uniqueName, sleepTime, diff);
		}
	}
	sleepTime = std::min(sleepTime, int64_t(s_TimerTaskSleepTime));
	//HJFLogi("{} TimerTask sleepTime:{} ", m_insName, sleepTime);
	return sleepTime;
}

int64_t HJTimerTask::priForEachUpdate()
{
	std::deque<HJTaskFunPack> taskFunDeque;
	int64_t sleepTime = priDetectCall(taskFunDeque);
	priCallFuns(taskFunDeque);
	return sleepTime;
}
void HJTimerTask::priTrySleep(int64_t i_sleep)
{
	int64_t t0 = HJCurrentSteadyMS();
	std::unique_lock<std::mutex> lock(mutex_);
	bool bNoTimeout = this->cv_.wait_for(lock, std::chrono::milliseconds(i_sleep), [this]
		{

			bool bWaitUtil = false;
			do
			{
				if (this->m_bQuit)
				{
					bWaitUtil = true;
					break;
				}
				bWaitUtil = m_nSignal > 0;
				if (bWaitUtil)
				{
					HJFLogi("{}: TimerTask have wait state signal:{} set to zero", m_insName, m_nSignal);
					m_nSignal = 0;
				}
			} while (false);
			return bWaitUtil;
		});
	int64_t t1 = HJCurrentSteadyMS();
	//HJFLogi("{}: TimerTask have wait state result i_sleep:{} realDiff:{}", m_insName, i_sleep, (t1-t0));
	if (bNoTimeout)
	{
		HJFLogi("{}: TimerTask wait because signal so wake up early, i_sleep:{} realDiff:{}", m_insName, i_sleep, (t1 - t0));
	}
	else
	{
		//HJFLogi("{}: TimerTask wait timeout i_sleep:{} realDiff:{}", m_insName, i_sleep, (t1-t0));
	}
}
int HJTimerTask::init(const std::string& i_insName)
{
	int i_err = 0;
	do
	{
		m_insName = i_insName;
		worker_thread_ = std::thread([this]()
			{
				for (;;)
				{
					if (!m_bInited)
					{
						HJFLogi("{} TimerTask thread first enter", m_insName);
						m_bInited = true;
					}
					int64_t sleepTime = priForEachUpdate();
					if (sleepTime)
					{
						priTrySleep(sleepTime);
					}
					if (m_bQuit)
					{
						HJFLogi("{} TimerTask bQuit is true, break", m_insName);
						break;
					}
				}
			});

	} while (false);
	return i_err;
}

//////////////////////////////////////////////
int HJTimerTaskState::STATE_REGIST = 1;
int HJTimerTaskState::STATE_UNREGIST = 2;

NS_HJ_END
