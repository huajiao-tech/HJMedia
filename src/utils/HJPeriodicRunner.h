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

	template<typename T>
	auto run(T&& func, size_t key, int64_t delay) -> decltype(func()) 
    {
        auto it = m_tasks.find(key);
        if (it != m_tasks.end()) {
            using TaskType = HJAnyTask<decltype(func())>;
            auto task = std::dynamic_pointer_cast<TaskType>(it->second);
            if (task && *task) {
                auto now = HJCurrentSteadyUS();
                if (now > task->m_runtime + task->m_delay) {
                    return task->run();
                } else {
                    return defaultValue<decltype(func())>();
                }
            }  else {
                return defaultValue<decltype(func())>();
            }
        }
        //
        using TaskType = HJAnyTask<decltype(func())>;
        auto task = std::make_shared<TaskType>(std::forward<T>(func));
        task->setName(HJMakeGlobalName("anytask"));
        task->setID(key);
        task->m_runtime = HJCurrentSteadyUS();
        task->m_delay = HJ_MS_TO_US(delay);   //ms to us
        //
        m_tasks[key] = task;
        
        return task->run();
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


NS_HJ_END