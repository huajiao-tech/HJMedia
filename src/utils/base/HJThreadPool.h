#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include <future>
#include <deque>

NS_HJ_BEGIN

using HJThreadTaskFunc = std::function<int()>;
class HJTaskWrapper
{
public:
	HJ_DEFINE_CREATE(HJTaskWrapper);
    int m_id = 0;
    int64_t m_targetTime = 0;
	HJThreadTaskFunc m_func = nullptr;
	std::shared_ptr<std::promise<int>> m_promise = nullptr;
};

class HJThreadPool : public HJBaseSharedObject
{
public:
	
	using RunFunc = HJThreadTaskFunc;

	using HJTaskWrapperIt = std::deque<HJTaskWrapper::Ptr>::iterator;

	HJ_DEFINE_CREATE(HJThreadPool);

	HJThreadPool();
	virtual ~HJThreadPool();
    
    HJThreadPool::Ptr getThreadPtr() {
        return getSharedFrom(this);
    }
	//before start, or thread kernal call modify
	void setTimeout(int64_t i_time_out);
	//before start
	void setBeginFunc(HJThreadTaskFunc i_func);
	void setEndFunc(HJThreadTaskFunc i_func);
	void setRunFunc(RunFunc i_func);

	int  start();
	void done();
    int asyncClear(HJThreadTaskFunc task, int i_id);
	int async(HJThreadTaskFunc task, int64_t i_delayTime = 0);
	int sync(HJThreadTaskFunc task);
    void clearAllMsg();
	std::mutex& getMutex();
	void signal();

    void setPause(bool i_bPause);
        
protected:
	bool isQuit();

private:
	static void priThreadFun(void* argc, std::shared_ptr<std::promise<void>> i_promise);
	int threadFun();
	void priResetTimeout();
	bool priIsTimeout();
    int priStart();
    int priProcess();
	void priClearTask(int i_id);
	void priEnterTask(const HJThreadTaskFunc& task, std::shared_ptr<std::promise<int>> i_promise = nullptr, int64_t i_delayTime = 0, int i_id = 0);

	void priWaitFor();
	void priDone();
	bool priIsQuit();
	HJTaskWrapper::Ptr priGetTaskWrapper();
	int priProcTask();
	std::deque<HJTaskWrapper::Ptr> m_task_queue;

	static int64_t s_defaultTimeOut;
	 
	int64_t m_time_out = 0;

	bool m_bSignaled = false;
    
	HJThreadTaskFunc m_begin_func = nullptr;
	HJThreadTaskFunc m_end_func = nullptr;
	RunFunc m_run_func = nullptr;
		
	std::thread m_worker_thread;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	bool m_bQuit = false;
    
    std::mutex m_pause_mutex;
    bool m_bPaused = false;

	std::thread::id m_threadId;
        
    std::mutex m_mutex_run;
    bool m_bThreadRunning = false;
};

class HJThreadTimer : public HJThreadPool
{
public:
    HJ_DEFINE_CREATE(HJThreadTimer);
    HJThreadTimer();
	virtual ~HJThreadTimer();
    int startSchedule(int64_t i_intervalMs, RunFunc i_func);
    
private:
    int64_t m_startTime = 0;
    int64_t m_statIdx = 0;
    int64_t m_nextTime = 0;  

	uint64_t m_curTime = 0;

	static uint64_t s_maxSleepTimeMs;
};

class HJTimerThreadPool : public HJThreadPool
{
public:
	HJ_DEFINE_CREATE(HJTimerThreadPool);
	HJTimerThreadPool();
	virtual ~HJTimerThreadPool();
	int startTimer(int64_t i_intervalMs, RunFunc i_func);
	void stopTimer();
private:
	static int s_timerId;
	HJThreadTimer::Ptr m_timer = nullptr;
};


NS_HJ_END
