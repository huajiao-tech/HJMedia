//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJExecutor.h"
#include "HJFLog.h"
#include "HJTicker.h"
#if defined(_WIN32)
#include <windows.h>
#endif

NS_HJ_BEGIN
//***********************************************************************************//
HJExecutorLoadCounter::HJExecutorLoadCounter(uint64_t max_size, uint64_t max_usec) {
    m_lastSleepTime = m_lastWakeTime = HJTime::getCurrentMicrosecond();
    m_maxSize = max_size;
    m_maxUsec = max_usec;
}

void HJExecutorLoadCounter::startSleep() {
    HJ_AUTO_LOCK(m_mtx);
    m_sleeping = true;
    auto current_time = HJTime::getCurrentMicrosecond();
    auto run_time = current_time - m_lastWakeTime;
    m_lastSleepTime = current_time;
    m_timeList.emplace_back(run_time, false);
    if (m_timeList.size() > m_maxSize) {
        m_timeList.pop_front();
    }
}

void HJExecutorLoadCounter::sleepWakeUp() {
    HJ_AUTO_LOCK(m_mtx);
    m_sleeping = false;
    auto current_time = HJTime::getCurrentMicrosecond();
    auto sleep_time = current_time - m_lastSleepTime;
    m_lastWakeTime = current_time;
    m_timeList.emplace_back(sleep_time, true);
    if (m_timeList.size() > m_maxSize) {
        m_timeList.pop_front();
    }
}

int HJExecutorLoadCounter::load() {
    HJ_AUTO_LOCK(m_mtx);
    uint64_t totalSleepTime = 0;
    uint64_t totalRunTime = 0;
    m_timeList.for_each([&](const TimeRecord& rcd) {
        if (rcd.m_sleep) {
            totalSleepTime += rcd.m_time;
        }
        else {
            totalRunTime += rcd.m_time;
        }
        });

    if (m_sleeping) {
        totalSleepTime += (HJTime::getCurrentMicrosecond() - m_lastSleepTime);
    }
    else {
        totalRunTime += (HJTime::getCurrentMicrosecond() - m_lastWakeTime);
    }

    uint64_t totalTime = totalRunTime + totalSleepTime;
    while ((m_timeList.size() != 0) && (totalTime > m_maxUsec || m_timeList.size() > m_maxSize)) {
        TimeRecord& rcd = m_timeList.front();
        if (rcd.m_sleep) {
            totalSleepTime -= rcd.m_time;
        }
        else {
            totalRunTime -= rcd.m_time;
        }
        totalTime -= rcd.m_time;
        m_timeList.pop_front();
    }
    if (totalTime == 0) {
        return 0;
    }
    return (int)(totalRunTime * 100 / totalTime);
}
//***********************************************************************************//
HJExecutor::HJExecutor(HJPriority priority/* = HJ_PRIORITY_HIGHEST*/, bool autoRun/* = true*/)
    : HJExecutorLoadCounter(32, 2 * 1000 * 1000)
    , m_isExitFlag(false)
    , m_priority(priority)
{
    //std::atomic_init(&m_schedulerIDCounter, 0);
    //std::atomic_init(&m_taskIDCounter, 0);
    m_taskMap = std::make_shared<HJTaskMap>();
    if(autoRun) {
        start();
    }
}
HJExecutor::~HJExecutor() {
    stop();
}

void HJExecutor::start(bool regSelf/* = true*/)
{
    if(m_thread) {
        HJLogw("have started");
        return;
    }
//    HJLogi("start entry");
    m_loopThreadID = std::this_thread::get_id();
    m_thread = std::make_shared<std::thread>(&HJExecutor::runLoop, this, regSelf);
//    HJLogi("start end");
}

void HJExecutor::stop()
{
    if (!m_thread) {
        return;
    }
//    HJFLogi("entry, name:{}", getName());
    //ExitException
    m_isExitFlag = true;
    if (m_thread) 
    {
        m_taskMap->pushExit(); //eof
        if (m_thread->joinable()) {
            m_thread->join();
        }
        m_thread = nullptr;
//        m_taskMap = nullptr;
//        HJLogi("thread exit ok, name: " + getName());
    }
//    HJFLogi("end, name:{}", getName());
    return;
}

int HJExecutor::asyncFixedTask(HJTask::Ptr task, const HJRational timeBase)
{
    if (!task) {
        return HJErrInvalid;
    }
    task->setTskState(HJTASK_Ready);
    if (!task->m_fixed) {
        task->setTid(getID());
        task->setID(getTaskID());
        //task->setExecutor(sharedFrom(this));
        task->m_runtime = HJTime::getCurrentMicrosecond() + HJ_MS_TO_US(task->m_delayTime);
        //
        task->m_fixed = std::make_shared<HJFixedTimer>(timeBase);
        task->m_fixed->setStartTime(task->m_runtime);
    } else {
        task->m_runtime = task->m_fixed->getRunTimex(HJTime::getCurrentMicrosecond());
    }
    m_taskMap->push(task);
    
    return HJ_OK;
}

HJTask::Ptr HJExecutor::asyncFixed(HJRunnable run, const HJRational timeBase/* = (HJRational){30, 1000}*/, const std::string& name/* = ""*/)
{
    //HJFLogi("entry num:{}, den:{}, name:{}", timeBase.num, timeBase.den, name);
    auto task = std::make_shared<HJTask>(std::move(run));
    task->setClsID(getID());
    task->setName(name);
    //
    int res = asyncFixedTask(task, timeBase);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJExecutor::asyncFixed(HJRunnable run, const int rate, const std::string& name/* = ""*/)
{
    HJRational timeBase = { rate, HJ_MS_PER_SEC };
    return asyncFixed(std::move(run), timeBase, name);
}

HJTask::Ptr HJExecutor::asyncFixed(HJRunnable run, const uint64_t delta, const std::string& name/* = ""*/)
{
    HJRational timeBase;
    timeBase.num = 1;
    timeBase.den = (int)delta;
    return asyncFixed(std::move(run), timeBase, name);
}

int HJExecutor::asyncTask(HJTask::Ptr task, bool maySync/* = true*/, bool first/* = false*/)
{
    if (!task) {
        return HJErrInvalid;
    }
    if (maySync && isCurrentThread() && !task->m_delayTime) {
        HJLogi("asyncTask run task in same thread");
        task->run();//(*task)();
        return HJ_OK;
    }
    task->setTskState(HJTASK_Ready);
    task->setTid(getID());
    task->setID(getTaskID());
    //task->setExecutor(sharedFrom(this));
   
//    HJLogi("asyncTask entry, tid:" + HJ2String(getID()) + ", schedule id:" + HJ2String(task->getClsID()) + ", task id:" + HJ2String(task->getID()) + ", name:" + task->getName());
    if (first) {
        m_taskMap->pushFirst(task);
    } else {
        task->m_runtime = HJTime::getCurrentMicrosecond() + HJ_MS_TO_US(task->m_delayTime);
        //HJFLogi("task run time:{}", task->m_runtime);
        m_taskMap->push(task);
    }
    return HJ_OK;
}

HJTask::Ptr HJExecutor::async(HJRunnable run, uint64_t delayTimeMS/* = 0*/, bool maySync/* = true*/)
{
    auto task = std::make_shared<HJTask>(std::move(run));
    task->setDelayTime(delayTimeMS);
//    auto executor = std::dynamic_pointer_cast<HJExecutor>(m_executor.lock());
    int res = asyncTask(task, maySync);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJExecutor::asyncFirst(HJRunnable run, bool maySync/* = true*/)
{
    auto task = std::make_shared<HJTask>(std::move(run));
    int res = asyncTask(task, maySync, true);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJExecutor::sync(const HJRunnable& run)
{
    //HJFLogi("HJExecutor:{} sync entry", m_name);
    HJSemaphore sem;
    auto task = std::make_shared<HJTask>([&] {
        HJOnceToken token(nullptr, [&] {
            sem.notify();
        });
        run();
    });
    task->setClsID(getID());
    //
    int res = asyncTask(task);
    if (HJ_OK == res) {
        sem.wait();
    }
    //HJFLogi("HJExecutor:{} sync end", m_name);
    return task;
}

static thread_local std::weak_ptr<HJExecutor> g_CurrentExecutor;
HJExecutor::Ptr HJExecutor::getCurrentExecutor() {
    return g_CurrentExecutor.lock();
}

void HJExecutor::runLoop(bool regSelf)
{
//    HJLogi("runLoop entry, id:" + HJ2String(m_id));
    setPriority(m_priority);

    m_loopThreadID = std::this_thread::get_id();
    m_id = HJThreadID2ID(m_loopThreadID);
    if (regSelf) {
        g_CurrentExecutor = sharedFrom(this);
    }
    //HJLogi("notify start");
    //
    while (!m_isExitFlag)
    {
        HJTask::Ptr task = nullptr;
        startSleep();
        m_taskMap->waitTask(task, HJTime::getCurrentMicrosecond());
        sleepWakeUp();
        //
        if (task) {
            //m_curTask = task;
            task->setTskState(HJTASK_Running);
            try {
                task->run();
            } catch (std::exception &ex) {
                HJLoge("runLoop error:" + std::string(ex.what()));
            }
            //task->setExecutor(nullptr);
            if ((*task) && task->m_fixed) {
                asyncFixedTask(task);
            } else {
                task->setTskState(HJTASK_Done);
            }
        }
        //m_curTask = nullptr;
    } //while
//    HJLogi("runLoop end");
    return;
}

bool HJExecutor::setPriority(HJPriority priority/* = HJ_PRIORITY_NORMAL*/, std::thread::native_handle_type threadId/* = 0*/)
{
#if defined(_WIN32)
    static int Priorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST };
    if (priority != THREAD_PRIORITY_NORMAL && SetThreadPriority(GetCurrentThread(), Priorities[priority]) == 0) {
        return false;
    }
    return true;
#else
    static int Min = sched_get_priority_min(SCHED_OTHER);
    if (Min == -1) {
        return false;
    }
    static int Max = sched_get_priority_max(SCHED_OTHER);
    if (Max == -1) {
        return false;
    }
    static int Priorities[] = { Min, Min + (Max - Min) / 4, Min
        + (Max - Min) / 2, Min + (Max - Min) * 3 / 4, Max };

    if (threadId == 0) {
        threadId = pthread_self();
    }
    struct sched_param params;
    params.sched_priority = Priorities[priority];
    return pthread_setschedparam(threadId, SCHED_OTHER, &params) == 0;
#endif
}

//***********************************************************************************//
HJScheduler::HJScheduler(const HJExecutor::Ptr executor/* = nullptr*/, const bool randExecutor/* = false*/)
{
    HJFLogi("entry, randExecutor:{}", randExecutor);
    if(executor) {
        m_executor = executor;
    } else if(randExecutor){
        m_executor = HJExecutorManager::Instance().getExecutor(std::rand());
    } else {
        m_executor = HJExecutorManager::Instance().getExecutor();
    }
    auto thread = m_executor.lock();
    if(thread) {
        setID(thread->getSchedulerID());
        m_tid = thread->getID();
    }
    HJLogi("end, id:" + HJ2String(getID()) + ", tid:" + HJ2String(m_tid));
}

HJScheduler::~HJScheduler()
{
//    HJFLogi("entry, id:{}", getID());
    auto executor = m_executor.lock();
    if (executor) {
        executor->removeTask(getID());
    }
    m_executor.reset();
//    HJLogi("end, id:" + HJ2String(getID()) + ", tid:" + HJ2String(m_tid));
}

int HJScheduler::asyncFixedTask(HJTask::Ptr task, const HJRational timeBase/* = (HJRational){30, 1000}*/)
{
    auto executor = m_executor.lock();
    if (!task || !executor) {
        return HJErrInvalid;
    }
    task->setClsID(getID());
    
    return executor->asyncFixedTask(std::move(task), timeBase);
}

int HJScheduler::asyncTask(HJTask::Ptr task, uint64_t delayTimeMS/* = 0*/, bool maySync/* = true*/, bool first/* = false*/)
{
    auto executor = m_executor.lock();
    if (!task || !executor) {
        return HJErrInvalid;
    }
    task->setClsID(getID());
    task->setDelayTime(delayTimeMS);
    
    return executor->asyncTask(std::move(task), maySync, false);
}


HJTask::Ptr HJScheduler::asyncCoro(HJRunnable run, uint64_t delayTimeMS/* = 0*/, bool maySync/* = true*/, bool first/* = false*/, const std::string& name/* = ""*/)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    auto task = std::make_shared<HJCoroTask>(std::move(run));
    task->setClsID(getID());
    task->setDelayTime(delayTimeMS);
//    task->setName(name);
    int res = executor->asyncTask(task, maySync, first);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

//HJTask::Ptr HJScheduler::asyncCoroRemoveBefores(HJRunnable run, uint64_t delayTimeMS/* = 0*/, const std::string& name/* = ""*/)
//{
//    if(name.empty()) {
//        return nullptr;
//    }
//    removeTask(name);
//    return asyncCoro(std::move(run), delayTimeMS, false, false, name);
//}

HJTask::Ptr HJScheduler::syncCoro(HJRunnable run)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    //    HJLogi("syncCoro entry");
    const HJTask::Ptr curTask = HJGetCurTask();
    if (HJValidPtr(curTask)) {
        curTask->rejectTskState();
    }
    HJSemaphore sem;
    auto task = std::make_shared<HJCoroTask>([&] {
        HJOnceToken token(nullptr, [&] {
            if (HJValidPtr(curTask)) {
                curTask->reAsync();
            } else {
                sem.notify();
            }
        });
        run();
    });
    task->setClsID(getID());
    int res = executor->asyncTask(task, false);
    if (HJ_OK != res) {
        HJLoge("syncCoro error:" + HJ2String(res));
        return nullptr;
    }
    if (HJValidPtr(curTask)) {
        curTask->yield();
    } else {
        sem.wait();
    }
//    HJLogi("syncCoro end");
    return task;
}

HJTask::Ptr HJScheduler::async(HJRunnable run, uint64_t delayTimeMS/* = 0*/, bool maySync/* = true*/, const std::string& name/* = ""*/)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    auto task = std::make_shared<HJTask>(std::move(run));
    task->setClsID(getID());
    task->setDelayTime(delayTimeMS);
    task->setName(name);
    //
    int res = executor->asyncTask(task, maySync);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJScheduler::asyncAlwaysTask(HJRunnable run, const std::string& name/* = ""*/) {
    return async(std::move(run), 0, false, name);
}

HJTask::Ptr HJScheduler::asyncDelayTask(HJRunnable run, uint64_t delayTimeMS/* = 0*/, const std::string& name/* = ""*/) {
    return async(std::move(run), delayTimeMS, false, name);
}

HJTask::Ptr HJScheduler::asyncFirst(HJRunnable run, bool maySync/* = true*/)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    auto task = std::make_shared<HJTask>(std::move(run));
    task->setClsID(getID());
    //
    int res = executor->asyncTask(task, maySync, true);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJScheduler::asyncRemoveBefores(HJRunnable run, uint64_t delayTimeMS/* = 0*/, const std::string& name/* = ""*/)
{
    if(name.empty()) {
        return nullptr;
    }
    removeTask(name);
    return async(std::move(run), delayTimeMS, false, name);
}

HJTask::Ptr HJScheduler::asyncFixed(HJRunnable run, const HJRational timeBase/* = (HJRational){30, 1000}*/, const std::string& name/* = ""*/)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    auto task = std::make_shared<HJTask>(std::move(run));
    task->setClsID(getID());
    task->setName(name);
    //
    int res = executor->asyncFixedTask(task, timeBase);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJScheduler::asyncFixed(HJRunnable run, const int rate, const std::string& name/* = ""*/)
{
    HJRational timeBase = {rate, HJ_MS_PER_SEC};
    return asyncFixed(std::move(run), timeBase, name);
}

HJTask::Ptr HJScheduler::asyncFixed(HJRunnable run, const uint64_t delta, const std::string& name/* = ""*/)
{
    HJRational timeBase;
    timeBase.num = 1;
    timeBase.den = (int)delta;
    return asyncFixed(std::move(run), timeBase, name);
}

HJTask::Ptr HJScheduler::sync(const HJRunnable& run)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    //HJLogi("HJScheduler sync entry");
    HJSemaphore sem;
    auto task = std::make_shared<HJTask>([&] {
        HJOnceToken token(nullptr, [&] {
            sem.notify();
        });
        run();
    });
    task->setClsID(getID());
    //
    int res = executor->asyncTask(task);
    if (HJ_OK == res) {
        sem.wait();
    }
    //HJLogi("HJScheduler sync end");
    return task;
}

HJTask::Ptr HJScheduler::syncFirst(const HJRunnable& run)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    //HJLogi("HJScheduler syncFirst entry");
    HJSemaphore sem;
    auto task = std::make_shared<HJTask>([&] {
        HJOnceToken token(nullptr, [&] {
            sem.notify();
        });
        run();
    });
    task->setClsID(getID());
    //
    int res = executor->asyncTask(task, true, true);
    if (HJ_OK == res) {
        sem.wait();
    }
    //HJLogi("HJScheduler syncFirst end");
    return task;
}

template<typename F, typename... Args>
HJTask::Ptr HJScheduler::asyncArgs(uint64_t delayTimeMS, F&& f, Args&&... args)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    auto runable = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task = std::make_shared<HJTask>([runable]() {
        runable();
    });
    task->setClsID(getID());
    task->setDelayTime(delayTimeMS);
    //
    int res = executor->asyncTask(task);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    
    return task;
}

template<typename F, typename... Args>
HJTask::Ptr HJScheduler::asyncArgsFirst(F&& f, Args&&... args)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return nullptr;
    }
    auto runable = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task = std::make_shared<HJTask>([runable]() {
        runable();
    });
    task->setClsID(getID());
    //
    int res = executor->asyncTask(task, true, true);
    if (HJ_OK != res) {
        HJLoge("asyncTask error:" + HJ2String(res));
        return nullptr;
    }
    
    return task;
}

void HJScheduler::removeAllTask()
{
    auto executor = m_executor.lock();
    if (!executor) {
        return;
    }
    executor->removeTask(getID());
    return;
}

void HJScheduler::removeTask(const std::string& name)
{
    auto executor = m_executor.lock();
    if (!executor) {
        return;
    }
    executor->removeTask(name);
    return;
}

//***********************************************************************************//
static size_t   g_executorPoolSize = 0;
const std::string HJExecutorManager::K_EXCEUTOR_DEFAULT = "default";
const std::string HJExecutorManager::K_EXECUTOR_MAIN = "main";
const std::string HJExecutorManager::K_EXECUTOR_GLOBAL = "global";
HJ_INSTANCE_IMP(HJExecutorManager);
//
void HJExecutorManager::done() {
    for (const auto& t : m_threads){
        t->stop();
    }
    m_threads.clear();
    for (const auto& it : m_otherThreads) {
        it.second->stop();
    }
    m_otherThreads.clear();
}

HJTask::Ptr HJExecutorManager::async(HJRunnable run, uint64_t delayTimeMS/* = 0*/, bool maySync/* = true*/, const std::string& name/* = ""*/)
{
    auto executor = getExecutor();
    if (!executor) {
        return nullptr;
    }
    auto task = std::make_shared<HJTask>(std::move(run));
//    task->setClsID(getID());
    task->setDelayTime(delayTimeMS);
    task->setName(name);
    //
    int res = executor->asyncTask(task, maySync);
    if (HJ_OK != res) {
        HJLoge("async task error:" + HJ2String(res));
        return nullptr;
    }
    return task;
}

HJTask::Ptr HJExecutorManager::sync(const HJRunnable& run)
{
    auto executor = getExecutor();
    if (!executor) {
        return nullptr;
    }
    HJLogi("entry");
    HJSemaphore sem;
    auto task = std::make_shared<HJTask>([&] {
        HJOnceToken token(nullptr, [&] {
            sem.notify();
        });
        run();
    });
    task->setClsID(getID());
    //
    int res = executor->asyncTask(task);
    if (HJ_OK == res) {
        sem.wait();
    }
    HJLogi("end");
    return task;
}

HJExecutor::Ptr HJExecutorManager::getExecutor(int idx/* = -1*/)
{
    if(idx >= 0) {
        return m_threads[idx % m_threads.size()];
    } else {
        auto thread_pos = m_threadPos;
        if (thread_pos >= m_threads.size()) {
            thread_pos = 0;
        }

        HJExecutor::Ptr executor_min_load = m_threads[thread_pos];
        auto min_load = executor_min_load->load();

        for (size_t i = 0; i < m_threads.size(); ++i, ++thread_pos) {
            if (thread_pos >= m_threads.size()) {
                thread_pos = 0;
            }

            auto th = m_threads[thread_pos];
            auto load = th->load();

            if (load < min_load) {
                min_load = load;
                executor_min_load = th;
            }
            if (min_load == 0) {
                break;
            }
        }
        m_threadPos = thread_pos;
        return executor_min_load;
    }
}

HJExecutor::Ptr HJExecutorManager::getExecutorByTid(HJThreadID tid)
{
    for (auto it = m_threads.begin(); it != m_threads.end(); it++) {
        HJExecutor::Ptr executor = std::dynamic_pointer_cast<HJExecutor>(*it);
        if (executor->getThreadID() == tid) {
            return executor;
        }
    }
    return nullptr;
}

HJExecutor::Ptr HJExecutorManager::getExecutorByName(const std::string& name)
{
    HJ_AUTO_LOCK(m_mutex);
    HJExecutor::Ptr thread = nullptr;
    auto it = m_otherThreads.find(name);
    if (m_otherThreads.end() != it) {
        thread = it->second;
    }
    if (thread) {
        return thread;
    }
    thread = createExecutor(name, HJ_PRIORITY_NORMAL, m_otherThreads.size());
    if (!thread) {
        return nullptr;
    }
    m_otherThreads[name] = thread;

    return thread;
}

std::vector<int> HJExecutorManager::getExecutorLoad() {
    std::vector<int> vec(m_threads.size());
    int i = 0;
    for (auto &executor : m_threads) {
        vec[i++] = executor->load();
    }
    return vec;
}

void HJExecutorManager::getExecutorDelay(const std::function<void(const std::vector<int> &)>& callback) {
    std::shared_ptr<std::vector<int> > delay_vec = std::make_shared<std::vector<int>>(m_threads.size());
    std::shared_ptr<void> finished(nullptr, [callback, delay_vec](void *) {
        callback((*delay_vec));
    });
    int index = 0;
    for (auto &th : m_threads)
    {
        std::shared_ptr<HJTicker> delay_ticker = std::make_shared<HJTicker>();
        th->async([finished, delay_vec, index, delay_ticker]() {
            (*delay_vec)[index] = (int) delay_ticker->elapsedTime();
        }, 0, false);
        ++index;
    }
}

void HJExecutorManager::for_each(const std::function<void(const HJExecutor::Ptr &)>& cb) {
    for (auto &th : m_threads) {
        cb(th);
    }
}

size_t HJExecutorManager::getExecutorSize() const {
    return m_threads.size();
}

void HJExecutorManager::setPoolSize(size_t size/* = 0*/) {
    g_executorPoolSize = size;
}

size_t HJExecutorManager::createExecutors(const std::string &name, size_t size, HJPriority priority, bool isRegister)
{
    auto cpus = std::thread::hardware_concurrency();
    size = size > 0 ? size : cpus;
    for (size_t i = 0; i < size; i++) {
        auto exec = createExecutor(name, priority, isRegister);
        m_threads.emplace_back(std::move(exec));
    }
    return size;
}

HJExecutorManager::HJExecutorManager() {
    auto size = createExecutors("executor", g_executorPoolSize, HJ_PRIORITY_NORMAL/*HJ_PRIORITY_HIGHEST*/, true);
//    HJLogi("init ok, size:" + HJ2String(size));
}

HJExecutor::Ptr HJExecutorManager::createExecutor(const std::string &name/* = HJ_TYPE_NAME(HJExecutor) */, HJPriority priority/* = HJ_PRIORITY_NORMAL*/, bool isRegister/* = true*/)
{
    auto cpus = std::thread::hardware_concurrency();
    HJExecutor::Ptr thread = std::make_shared<HJExecutor>((HJPriority)priority);
    thread->start(isRegister);
    size_t idx = thread->getID();
    auto fullName = name + "_" + HJ2STR(idx);
    thread->setName(fullName);
    thread->async([idx, cpus, fullName]() {
        HJUtilitys::setThreadName(fullName.data());
//        HJLogi("thread setting ok, tid:" + HJ2String(idx) + ", name:" + fullName);
    });
    return thread;
}


NS_HJ_END