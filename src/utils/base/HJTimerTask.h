#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

using TimerTaskFun = std::function<void(const std::string& i_uniqueName, int i_heartState, int64_t i_curTime, int64_t i_duration)>;

typedef enum HJTimerTaskType
{
	TimerTask_Start = 0,
	TimerTask_Update,
	TimerTask_End,
}HJTimerTaskType;

class HJTimerTaskState
{
public:
	HJ_DEFINE_CREATE(HJTimerTaskState);

	std::string m_uniqueName = "";
	int64_t m_beginTime = -1;
	int64_t m_endTime = -1;
	int m_state = -1;
	int m_interval = 100;
	TimerTaskFun m_fun = nullptr;
	static int STATE_REGIST;
	static int STATE_UNREGIST;
};

class HJTimerTaskItem
{
public:
	HJ_DEFINE_CREATE(HJTimerTaskItem);
	TimerTaskFun m_fun = nullptr;
	int64_t m_startTime = -1;
	int64_t m_nextTime = -1;
	int64_t m_preTime = -1;
	int m_idx = 0;
	int m_interval = 0;
	bool m_bStop = false;
	std::mutex m_item_mutex;
};
class HJTaskFunPack
{
public:
	HJTaskFunPack(HJTimerTaskItem::Ptr i_item, const std::string& i_uniqueName, int64_t i_curTime, int64_t i_duration);
	int64_t m_curTime = 0;
	int64_t m_duration = 0;
	HJTimerTaskItem::Ptr m_item = nullptr;
	std::string m_uniqueName = "";
};


class HJTimerTask
{
public:
	HJ_DEFINE_CREATE(HJTimerTask);

	virtual ~HJTimerTask();

	int regist(const std::string& i_unqiueName, int i_interval, TimerTaskFun i_cb);
	int unregist(const std::string& i_unqiueName);
	int init(const std::string& i_insName);
	void joinTaskThread();
	static const int TIMERTASK_PLAYER_FINE_INTERVAL = 100;
	static const int TIMERTASK_PLAYER_ROUGH_INTERVAL = 5000;
	static const int TIMERTASK_PLAYER_FINE_TO_ROUGH = 5000;

	static const std::string TIMERTASK_NAME_LOADING;
	static const std::string TIMERTASK_NAME_PLAYERTIME;

	static const std::unordered_map<std::string, int> s_TimerTaskMap;


private:
	int64_t priForEachUpdate();
	int64_t priDetectCall(std::deque<HJTaskFunPack>& taskFunDeque);
	void priCallFuns(std::deque<HJTaskFunPack>& taskFunDeque);
	void priTrySleep(int64_t i_sleep);
	int priSetState(HJTimerTaskState::Ptr& io_ptr);
	bool priTaskContains(const std::string& i_uniqueName);

	int m_nSignal = 0;

	bool m_bInited = false;
	std::thread worker_thread_;
	std::mutex mutex_;
	std::condition_variable cv_;
	bool m_bQuit = false;

	std::unordered_map<std::string, HJTimerTaskItem::Ptr> m_taskTimerTaskMap;

	int64_t m_preCacheTime = -1;

	static const int s_TimerTaskSleepTime;

	std::string m_insName = "";
};

NS_HJ_END