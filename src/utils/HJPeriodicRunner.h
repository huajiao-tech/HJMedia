//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJChore.h"
#include "HJNotify.h"

NS_HJ_BEGIN
//***********************************************************************************//
template<class R, class... Args>
class HJAnyTask;
template<class R, class... Args>
class HJAnyTask<R(Args...)> : public HJTaskCancelable<R(Args...)>, public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJAnyTask<R(Args...)>);

    template<typename FUNC>
    HJAnyTask(FUNC&& task, const std::string& name = "", size_t identify = -1)
        : HJObject(name, identify)
        , HJTaskCancelable<R(Args...)>(std::forward<FUNC>(task))
    {
    }

    virtual ~HJAnyTask() { }
private:

};

class HJPeriodicRunner : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJPeriodicRunner);
	virtual ~HJPeriodicRunner() {};
    HJ_INSTANCE_DECL(HJPeriodicRunner);

	template<typename T>
	auto run(T&& func, size_t key, int64_t delay) -> decltype(func()) 
    {
        auto it = m_tasks.find(key);
        if (it != m_tasks.end()) {
            using TaskType = HJAnyTask<decltype(func())()>;
            auto task = std::dynamic_pointer_cast<TaskType>(it->second);
            if (task) {
                auto now = HJCurrentSteadyUS();
                if (now >= task->m_runtime + task->m_delayTime) {
                    task->m_runtime = getRuntime(task->m_runtime, now, task->m_delayTime);
                    return task->run();
                } else {
                    return HJDefaultValue::get<decltype(func())>();
                }
            }  else {
                return HJDefaultValue::get<decltype(func())>();
            }
        }
        //
        using TaskType = HJAnyTask<decltype(func())()>;
        auto task = std::make_shared<TaskType>(std::forward<T>(func));
        task->setName(HJMakeGlobalName("anytask"));
        task->setID(key);
        task->m_runtime = HJCurrentSteadyUS();
        task->m_delayTime = HJ_MS_TO_US(delay);
        //
        m_tasks[key] = task;
        
        return task->run();
	}

    int64_t getRuntime(int64_t pretime, int64_t nowtime, int64_t delay)
    {
        int64_t n = (nowtime - pretime) / delay;
        return pretime +  n * delay;
    }

    void removeTask(size_t key) {
        auto it = m_tasks.find(key);
        if (it != m_tasks.end()) {
            m_tasks.erase(it);
        }
    }
    void clearAllTasks() {
        m_tasks.clear();
    }
    size_t getTaskCount() const {
        return m_tasks.size();
    }

public:
    static size_t getDefaultKey(const char* file, int line, const char* function) {
        const char* safeFile = (file != nullptr) ? file : "";
        const char* safeFunction = (function != nullptr) ? function : "";

        std::string combined = std::string(safeFile) + ":" +
            std::to_string(line) + ":" +
            std::string(safeFunction);

        std::hash<std::string> hasher;
        return hasher(combined);
    }
private:
	std::map<size_t, HJObject::Ptr> m_tasks;
};

#define HJPERIOD_RUN(func, delay) m_periodicRunner.run(std::move(func), HJPeriodicRunner::getDefaultKey(__FILE__, __LINE__, __FUNCTION__), delay)
#define HJPERIOD_RUN1(func) m_periodicRunner.run(std::move(func), HJPeriodicRunner::getDefaultKey(__FILE__, __LINE__, __FUNCTION__), 1 * 1000)
#define HJPERIOD_RUN2(func) m_periodicRunner.run(std::move(func), HJPeriodicRunner::getDefaultKey(__FILE__, __LINE__, __FUNCTION__), 2 * 1000)
#define HJPERIOD_RUN5(func) m_periodicRunner.run(std::move(func), HJPeriodicRunner::getDefaultKey(__FILE__, __LINE__, __FUNCTION__), 5 * 1000)

NS_HJ_END