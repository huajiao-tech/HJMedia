//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJChore.h"

NS_HJ_BEGIN
//***********************************************************************************//
enum HJPriority {
    HJ_PRIORITY_LOWEST = 0,
    HJ_PRIORITY_LOW,
    HJ_PRIORITY_NORMAL,
    HJ_PRIORITY_HIGH,
    HJ_PRIORITY_HIGHEST
};

#define HJ_TASK_DELAY_TIME_DEFAULT      5          //ms
//***********************************************************************************//
class HJExecutorLoadCounter {
public:
    /**
     * 构造函数
     * @param max_size 统计样本数量
     * @param max_usec 统计时间窗口,亦即最近{max_usec}的cpu负载率
     */
    HJExecutorLoadCounter(uint64_t max_size, uint64_t max_usec);
    ~HJExecutorLoadCounter() = default;
    /**
     * 线程进入休眠
     */
    void startSleep();
    /**
     * 休眠唤醒,结束休眠
     */
    void sleepWakeUp();
    /**
     * 返回当前线程cpu使用率，范围为 0 ~ 100
     * @return 当前线程cpu使用率
     */
    int load();
private:
    struct TimeRecord {
        TimeRecord(uint64_t tm, bool slp) {
            m_time = tm;
            m_sleep = slp;
        }
        bool        m_sleep;
        uint64_t    m_time;
    };
private:
    bool                m_sleeping = true;
    uint64_t            m_lastSleepTime;
    uint64_t            m_lastWakeTime;
    uint64_t            m_maxSize;
    uint64_t            m_maxUsec;
    std::mutex          m_mtx;
    HJList<TimeRecord>  m_timeList;
};
//***********************************************************************************//
class HJExecutor : public HJExecutorLoadCounter, public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJExecutor>;
    HJExecutor(HJPriority priority = HJ_PRIORITY_NORMAL, bool autoRun = false);
    virtual ~HJExecutor();
    
    void start(bool regSelf = true);
    void stop();
    
    int asyncTask(HJTask::Ptr task, bool maySync = true, bool first = false);
    HJTask::Ptr async(HJRunnable run, uint64_t delayTimeMS = 0, bool maySync = true);
    HJTask::Ptr asyncFirst(HJRunnable run, bool maySync = true);

    int asyncFixedTask(HJTask::Ptr task, const HJRational timeBase = {30, 1000});
    HJTask::Ptr asyncFixed(HJRunnable run, const HJRational timeBase = { 30, 1000 }, const std::string& name = "");
    HJTask::Ptr asyncFixed(HJRunnable run, const int rate, const std::string& name = "");
    HJTask::Ptr asyncFixed(HJRunnable run, const uint64_t delta, const std::string& name = "");  //ms
    
    HJTask::Ptr sync(const HJRunnable& run);

    void removeTask(const size_t clsID) {
        m_taskMap->remove(clsID);
    }
    void removeTask(const std::string& name) {
        m_taskMap->remove(name);
    }
    //
    const HJThreadID& getThreadID() const {
        return m_loopThreadID;
    }
    bool isCurrentThread() {
        return (std::this_thread::get_id() == m_loopThreadID);
    }
    static HJExecutor::Ptr getCurrentExecutor();
    //
    const HJTask::Ptr getCurTask() {
        return m_curTask.get();
    }
    const size_t getSchedulerID() {
        return ++m_schedulerIDCounter;
    }
    const size_t getTaskID() {
        return ++m_taskIDCounter;
    }
private:
    bool setPriority(HJPriority priority = HJ_PRIORITY_NORMAL, std::thread::native_handle_type threadId = 0);
    void runLoop(bool regSelf = true);
private:
    bool                            m_isExitFlag = false;
    HJPriority                     m_priority = HJ_PRIORITY_NORMAL;
    HJThreadID                     m_loopThreadID;
    HJThreadPtr                    m_thread = nullptr;
#if HJ_THREAD_LOCK_CPU
    std::vector<int>                m_sortedCPUIDs
#endif
    HJTaskMap::Ptr                 m_taskMap = nullptr;
    HJObjectHolder<HJTask::Ptr>    m_curTask;
    std::atomic<size_t>             m_schedulerIDCounter = {0};
    std::atomic<size_t>             m_taskIDCounter = {0};
};
#define HJGetCurTask() HJExecutor::getCurrentExecutor()->getCurTask()
using HJExecutorMap = std::map<std::string, HJExecutor::Ptr>;

//***********************************************************************************//
class HJScheduler : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJScheduler>;
    
    HJScheduler(const HJExecutor::Ptr executor = nullptr, const bool randExecutor = true);
    virtual ~HJScheduler();
    
    int asyncTask(HJTask::Ptr task, uint64_t delayTimeMS = 0, bool maySync = true, bool first = false);
    int asyncAlwaysTask(HJTask::Ptr task, uint64_t delayTimeMS = 0) {
        return asyncTask(std::move(task), delayTimeMS, false);
    }
    int asyncFixedTask(HJTask::Ptr task, const HJRational timeBase = {30, 1000});
    /**
     * coro
     */
    HJTask::Ptr asyncCoro(HJRunnable run, uint64_t delayTimeMS = 0, bool maySync = false, bool first = false, const std::string& name = "");
//    HJTask::Ptr asyncCoroRemoveBefores(HJRunnable run, uint64_t delayTimeMS = 0, const std::string& name = "");
    HJTask::Ptr syncCoro(HJRunnable run);
    /**
     * async and sync task
     */
    HJTask::Ptr async(HJRunnable run, uint64_t delayTimeMS = 0, bool maySync = true, const std::string& name = "");
    HJTask::Ptr asyncAlwaysTask(HJRunnable run, const std::string& name = "");
    HJTask::Ptr asyncDelayTask(HJRunnable run, uint64_t delayTimeMS = 0, const std::string& name = "");
    HJTask::Ptr asyncFirst(HJRunnable run, bool maySync = true);
    HJTask::Ptr asyncRemoveBefores(HJRunnable run, uint64_t delayTimeMS = 0, const std::string& name = "");
    /**
     * fixed task
     */
    HJTask::Ptr asyncFixed(HJRunnable run, const HJRational timeBase = {30, 1000}, const std::string& name = "");
    HJTask::Ptr asyncFixed(HJRunnable run, const int rate, const std::string& name = "");
    HJTask::Ptr asyncFixed(HJRunnable run, const uint64_t delta, const std::string& name = "");  //ms
    
    HJTask::Ptr sync(const HJRunnable& run);
    HJTask::Ptr syncFirst(const HJRunnable& run);
    
    template<typename F, typename... Args>
    HJTask::Ptr asyncArgs(uint64_t delayTimeMS, F&& f, Args&&... args);
    
    template<typename F, typename... Args>
    HJTask::Ptr asyncArgsFirst(F&& f, Args&&... args);

    /**
     * remove task
     */
    void removeAllTask();
    void removeTask(const std::string& name);
    
    /**
     * other
     */
    const HJExecutor::Ptr getExecutor() const {
        return m_executor.lock();
    }
    const size_t getTid() const{
        return m_tid;
    }
private:
    std::weak_ptr<HJExecutor>      m_executor;
    size_t                          m_tid = 0;
};

//***********************************************************************************//
class HJExecutorManager : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJExecutorManager>;
    HJExecutorManager();
    virtual ~HJExecutorManager() = default;

    static HJExecutorManager &Instance();
    void done();
    /**
     * 根据线程负载情况，添加任务
     *  @return 任务执行器
     */
    HJTask::Ptr async(HJRunnable run, uint64_t delayTimeMS = 0, bool maySync = true, const std::string& name = "");
    HJTask::Ptr sync(const HJRunnable& run);
    /**
     * 根据线程负载情况，获取最空闲的任务执行器
     * @return 任务执行器
     */
    HJExecutor::Ptr getExecutor(int idx = -1);
    HJExecutor::Ptr getExecutorByTid(HJThreadID tid);
    HJExecutor::Ptr getExecutorByName(const std::string& name);
    static HJExecutor::Ptr createExecutor(const std::string &name = HJ_TYPE_NAME(HJExecutor), HJPriority priority = HJ_PRIORITY_NORMAL, bool isRegister = true);
    /**
     * 获取所有线程的负载率
     * @return 所有线程的负载率
     */
    std::vector<int> getExecutorLoad();
    /**
     * 获取所有线程任务执行延时，单位毫秒
     * 通过此函数也可以大概知道线程负载情况
     * @return
     */
    void getExecutorDelay(const std::function<void(const std::vector<int> &)>& callback);
    /**
     * 遍历所有线程
     */
    void for_each(const std::function<void(const HJExecutor::Ptr &)>& cb);
    /**
     * 获取线程数
     */
    size_t getExecutorSize() const;
    /**
     * task name
     */
    std::string getGlobalName(const std::string& prefix = "") {
        return prefix.empty() ? ("hjtsk_" + HJ2String(m_globalIDCounter++)) : (prefix + "_" + HJ2String(m_globalIDCounter++));
    }
public:
    static void setPoolSize(size_t size = 0);
    //
    static const std::string K_EXCEUTOR_DEFAULT;
    static const std::string K_EXECUTOR_MAIN;
    static const std::string K_EXECUTOR_GLOBAL;
protected:
    size_t createExecutors(const std::string &name, size_t size, HJPriority priority = HJ_PRIORITY_NORMAL, bool isRegister = true);
protected:
    size_t                                              m_threadPos = 0;
    std::vector<HJExecutor::Ptr>                        m_threads;
    std::unordered_map<std::string, HJExecutor::Ptr>    m_otherThreads;
    std::recursive_mutex                                m_mutex;
    std::atomic<size_t>                                 m_globalIDCounter = {0};
};
#define HJExecutorGlobalName(prefix)       HJExecutorManager::Instance().getGlobalName(prefix)
#define HJCreateExecutor(name)             HJExecutorManager::createExecutor(name)
#define HJGetExecutor()                    HJExecutorManager::Instance().getExecutor()
#define HJExecutorByName(name)             HJExecutorManager::Instance().getExecutorByName(name)
#define HJDefaultExecutor()                HJExecutorByName(HJExecutorManager::K_EXCEUTOR_DEFAULT)
#define HJMainExecutor()                   HJExecutorByName(HJExecutorManager::K_EXECUTOR_MAIN)
#define HJGlobalExecutor()                 HJExecutorByName(HJExecutorManager::K_EXECUTOR_GLOBAL)
#define HJExecutorAsync(executor, task)    executor->async(task);
#define HJMainExecutorAsync(task)          HJMainExecutor()->async(task);
#define HJMainExecutorSync(task)           HJMainExecutor()->sync(task);

NS_HJ_END