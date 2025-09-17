#include "HJThreadPool.h"
#include <chrono>
#include "HJTime.h"
#include "HJSleepto.h"
#include "HJFLog.h"

#define USE_FIEXED_TIME 1

NS_HJ_BEGIN

int64_t HJThreadPool::s_defaultTimeOut = 100;

HJThreadPool::HJThreadPool()
{
}
HJThreadPool::~HJThreadPool()
{
	priDone();
}
void HJThreadPool::priThreadFun(void *argc, std::shared_ptr<std::promise<void>> i_promise)
{
	if (i_promise)
	{
		i_promise->set_value();
	}

	HJThreadPool *the = (HJThreadPool *)argc;
	the->threadFun();
}

HJTaskWrapper::Ptr HJThreadPool::priGetTaskWrapper()
{
	HJTaskWrapper::Ptr taskWrapper = nullptr;
	do
	{
		m_threadId = std::this_thread::get_id();

		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_task_queue.empty())
		{
			break;
		}
		auto it = m_task_queue.begin();
		while (it != m_task_queue.end())
		{
			if ((*it)->m_targetTime == 0)
			{
				break;
			}
			else if (((*it)->m_targetTime > 0) && (HJCurrentSteadyMS() >= (*it)->m_targetTime))
			{
				break;
			}
			it++;
		}
		if (it != m_task_queue.end())
		{
			taskWrapper = *it;
			m_task_queue.erase(it);
		}

	} while (false);
	return taskWrapper;
}
int HJThreadPool::priProcTask()
{
	int i_err = 0;
	do
	{
		while (true)
		{
			HJTaskWrapper::Ptr taskWrapper = priGetTaskWrapper();
			if (taskWrapper)
			{
				i_err = taskWrapper->m_func();
				if (taskWrapper->m_promise)
				{
					taskWrapper->m_promise->set_value(i_err);
				}
			}
			else
			{
				break;
			}
			if (priIsQuit())
			{
				break;
			}
		}
	} while (false);
	return i_err;
}
void HJThreadPool::setPause(bool i_bPause)
{
    HJFLogi("{} tryreplace set pause :{} ", m_insName, i_bPause);
    if (i_bPause == m_bPaused)
    {
        HJFLogi("{} set pause :{} match not proc return ", m_insName, i_bPause);
        return;
    }

    signal();
    {
        HJ_LOCK(m_pause_mutex);
	    m_bPaused = i_bPause;
    }
    signal();
}
void HJThreadPool::priWaitFor()
{
	std::unique_lock<std::mutex> lk(m_mutex);
	m_cv.wait_for(lk, std::chrono::milliseconds(m_time_out), [this]()
				  {
			bool bWaitUtil = false;
			do
			{
				if (m_bQuit || !m_task_queue.empty() || m_bSignaled)
				{
					bWaitUtil = true;
					break;
				}
			} while (false);
			return bWaitUtil; });
	m_bSignaled = false;
}
int HJThreadPool::priProcess()
{
	int i_err = 0;
	do
	{
		HJ_LOCK(m_pause_mutex);
		if (!m_bPaused)
		{
			i_err = priProcTask();
			if (i_err < 0)
			{
                HJFLoge("{} proc task err:{}", getInsName(), i_err);
				break;
			}

			if (m_run_func)
			{
				i_err = m_run_func();
				if (i_err < 0)
				{
                    HJFLoge("{} run func err:{}", getInsName(), i_err);
					break;
				}
			}
		}
	} while (false);
	return i_err;
}

void HJThreadPool::priResetTimeout()
{
	m_time_out = s_defaultTimeOut;
}
bool HJThreadPool::priIsTimeout()
{
	return (m_time_out > 0);
}
int HJThreadPool::threadFun()
{
	int i_err = 0;
	do
	{
		if (m_begin_func)
		{
			m_begin_func();
		}
		while (true)
		{
			priResetTimeout();

            i_err = priProcess();
            if (i_err < 0)
            {
                HJFLoge("priProcess err:{}", i_err);
                break;
            }

			if (priIsTimeout())
			{
				priWaitFor();
			}

			if (priIsQuit())
			{
				break;
			}
		}
	} while (false);

	if (m_end_func)
	{
		m_end_func();
	}
	return i_err;
}
void HJThreadPool::setTimeout(int64_t i_time_out)
{
	m_time_out = i_time_out;
}
void HJThreadPool::setBeginFunc(HJThreadTaskFunc i_func)
{
	m_begin_func = i_func;
}
void HJThreadPool::setEndFunc(HJThreadTaskFunc i_func)
{
	m_end_func = i_func;
}
void HJThreadPool::setRunFunc(RunFunc i_func)
{
	m_run_func = i_func;
}
int HJThreadPool::priStart()
{
    int i_err = 0;
	do
	{
		auto promise = std::make_shared<std::promise<void>>();
		std::future<void> start_future = promise->get_future();
		m_worker_thread = std::thread(priThreadFun, this, promise);
		if (!m_worker_thread.joinable())
		{
			i_err = -1;
			break;
		}
		start_future.wait();

	} while (false);
	return i_err;
}
int HJThreadPool::start()
{
	int i_err = 0;
    HJ_LOCK(m_mutex_run)
    do
    {
        if (m_bThreadRunning)
        {
            HJFLogi("{} thread is running ,not start, return", m_insName);
            return i_err;
        }
        i_err = priStart();
        if (i_err == 0)
        {
            m_bThreadRunning = true;
        }
    } while(false);
    return i_err;
}
std::mutex &HJThreadPool::getMutex()
{
	return m_mutex;
}
bool HJThreadPool::priIsQuit()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_bQuit;
}
void HJThreadPool::priDone()
{
    HJ_LOCK(m_mutex_run)
    {
        if (!m_bThreadRunning)
        {
            //HJFLogi("{} thread is not running, so not join, return", m_insName);
            return;
        }
	    if (m_worker_thread.joinable())
	    {
		    {
			    std::lock_guard<std::mutex> lock(m_mutex);
			    m_bQuit = true;
		    }
		    m_cv.notify_one();
		    m_worker_thread.join();

            m_bThreadRunning = false;
	    }
    }
}
void HJThreadPool::done()
{
	priDone();
}
void HJThreadPool::priEnterTask(const HJThreadTaskFunc &task, std::shared_ptr<std::promise<int>> i_promise, int64_t i_delayTime, int i_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	HJTaskWrapper::Ptr wrapper = std::make_shared<HJTaskWrapper>();
	wrapper->m_func = task;
	wrapper->m_promise = i_promise;
	wrapper->m_id = i_id;
	if (i_delayTime > 0)
	{
		wrapper->m_targetTime = i_delayTime + HJCurrentSteadyMS();
	}
	m_task_queue.push_back(wrapper);
}

void HJThreadPool::priClearTask(int i_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_task_queue.begin(); it != m_task_queue.end();)
	{
		if ((*it)->m_id == i_id)
		{
			it = m_task_queue.erase(it);
		}
		else
		{
			it++;
		}
	}
}
void HJThreadPool::clearAllMsg()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_task_queue.clear();
}
bool HJThreadPool::isQuit()
{
	return priIsQuit();
}
void HJThreadPool::signal()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_bSignaled = true;
	}
	m_cv.notify_one();
}
int HJThreadPool::asyncClear(HJThreadTaskFunc task, int i_id)
{
    int i_err = 0;
    HJ_LOCK(m_mutex_run)
    do
    {
        if (!m_bThreadRunning)
        {
            HJFLogi("{} asyncClear proc the thread is already joined, not proc in thread, direct proc task", m_insName);
            i_err = -1;
            break;
        }
	    priClearTask(i_id);
	    priEnterTask(task, nullptr, 0, i_id);
	    m_cv.notify_one();
    } while(false);
    return i_err;
}
int HJThreadPool::async(HJThreadTaskFunc task, int64_t i_delayTime)
{
    int i_err = 0;
    HJ_LOCK(m_mutex_run)
    do
    {
        if (!m_bThreadRunning)
        {
            HJFLogi("{} sync proc the thread is already joined, not proc in thread, direct proc task", m_insName);
            i_err = -1;
            break;
        }
        priEnterTask(task, nullptr, i_delayTime, 0);
	    m_cv.notify_one();
    } while(false);
	return i_err;
}
int HJThreadPool::sync(HJThreadTaskFunc task)
{
	int i_err = 0;
    HJ_LOCK(m_mutex_run)
	do
    {
        if (!m_bThreadRunning)
        {
            HJFLogi("{} sync proc the thread is already joined, not proc in thread, direct proc task", m_insName);
            i_err = task();
            return i_err;
        }

		if (std::this_thread::get_id() == m_threadId)
		{
			i_err = task();
		}
		else
		{
			auto promise = std::make_shared<std::promise<int>>();
			std::future<int> future = promise->get_future();
			priEnterTask(task, promise, 0, 0);
			m_cv.notify_one();
			i_err = future.get();
		}
	} while (false);
	return i_err;
}

///////////////////////////////////////////////////////////////
uint64_t HJThreadTimer::s_maxSleepTimeMs = 50;

HJThreadTimer::HJThreadTimer()
{
}
HJThreadTimer::~HJThreadTimer()
{
}

int HJThreadTimer::startSchedule(int64_t i_intervalMs, RunFunc i_func)
{
	int i_err = 0;
	do
	{
#if defined(WIN32_LIB)
		m_curTime = HJSleepto::Gettime100ns();
		HJThreadPool::setRunFunc([this, i_intervalMs, i_func]()
		{
				if (i_func)
				{
					i_func();
				}

				uint64_t remainder = i_intervalMs;
				while (!isQuit() && remainder)
				{
					uint64_t curdiff = 0;
					if (remainder > s_maxSleepTimeMs)
					{
						curdiff = s_maxSleepTimeMs;
					}
					else
					{
						curdiff = remainder;
					}
					remainder -= curdiff;
					curdiff *= 10000; //1000*1000/100     100ns
					HJSleepto::Sleepto100ns(m_curTime += curdiff);
				}
								
				HJThreadPool::setTimeout(0);
				return 0;
        });
#else

#if USE_FIEXED_TIME
        
        m_curTime = HJCurrentSteadyMS();
		HJThreadPool::setRunFunc([this, i_intervalMs, i_func]()
		{
            if (i_func)
            {
                i_func();
            }
            int64_t diff = HJSleepto::sleepTo(m_curTime += i_intervalMs);
            HJThreadPool::setTimeout(diff);

//            int64_t curTime = HJCurrentSteadyMS();
//            if (m_startTime == 0)
//            {
//                m_startTime = curTime;
//            }
//            while (curTime >= m_nextTime) 
//            {
//                m_statIdx++;
//                m_nextTime = m_startTime + m_statIdx * i_intervalMs;
//            }
//            int64_t diff = m_nextTime - curTime;
//            HJThreadPool::setTimeout(diff);
            return 0; 
        });
#else
		HJThreadPool::setTimeout(i_intervalMs);
		HJThreadPool::setRunFunc(i_func);
#endif

#endif

		i_err = HJThreadPool::start();
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}

/// ///////////////////////////////////////////////////////////////////////
int HJTimerThreadPool::s_timerId = 8888;
HJTimerThreadPool::HJTimerThreadPool()
{
}
HJTimerThreadPool::~HJTimerThreadPool()
{
}
int HJTimerThreadPool::startTimer(int64_t i_intervalMs, RunFunc i_func)
{
	int i_err = 0;
	do
	{
		i_err = HJThreadPool::start();
		if (i_err < 0)
		{
			break;
		}
		m_timer = HJThreadTimer::Create();
		m_timer->startSchedule(i_intervalMs, [this, i_func]()
		{
				HJThreadPool::asyncClear([this, i_func] {
					if (i_func)
					{
						i_func();
					}
					return 0;
				}, s_timerId);
				return 0;
        });
	} while (false);
	return i_err;
}

void HJTimerThreadPool::stopTimer()
{
	if (m_timer)
	{
		m_timer->done();
		m_timer = nullptr;
	}
	HJThreadPool::done();
}

NS_HJ_END
