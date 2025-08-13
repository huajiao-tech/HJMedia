//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaUtils.h"

typedef struct mco_coro mco_coro;

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJTaskState
{
    HJTASK_NONE = 0x00,
    HJTASK_Init,
    HJTASK_Ready,
    HJTASK_Running,
    HJTASK_Done,
    HJTASK_RESERVED = 0xFFFF,
    HJTASK_Reject = 0x10000,
} HJTaskState;
#define HJ_TASK_RESERVED(v)     (v & HJTASK_RESERVED)
#define HJ_TASK_REJECT(v)       (v & HJTASK_Reject)
//
HJEnumToStringFuncDecl(HJTaskState);

//***********************************************************************************//
class HJFixedTimer
{
public:
    using Ptr = std::shared_ptr<HJFixedTimer>;
    HJFixedTimer(const int rate);                //rate: 15/s
    HJFixedTimer(const uint64_t delta);          //ms
    HJFixedTimer(const HJRational timeBase);    //ms
    virtual ~HJFixedTimer();
    
    int getTIdx() {
        return m_TIdx++;
    }
    uint64_t getDelta() {
        if (m_deltas.size() <= 0) {
            return 0;
        }
        int i = getTIdx() % m_deltas.size();
        return m_deltas[i];
    }
    void setStartTime(uint64_t time) {
        m_startTime = time;
    }
    uint64_t getStartTime() {
        return m_startTime;
    }
    uint64_t getRunTime(uint64_t curTime) {  //us
        uint64_t elapse = curTime - m_startTime;
        uint64_t denMode = elapse % (uint64_t)m_timeBase.den;
        int idx = (int)(denMode / m_deltas[0]);
        uint64_t delayTime = m_denDeltas[idx] - denMode;
        if (delayTime > m_deltas[0] || delayTime < 0) {
            //HJLogw("check delayTime:" + HJ2STR(delayTime) + ", delta:" + HJ2STR(m_deltas[0]));
        }
        //uint64_t runtimex = getRunTimex(curTime);
        //HJLogi("getRunTime run time:" + HJ2STR((curTime + delayTime)/1000.0) + ", delay time:" + HJ2STR(delayTime / 1000.0) + ", runtimex:" + HJ2STR(runtimex / 1000.0));
        return curTime + delayTime;
    }
    uint64_t getRunTimex(uint64_t curTime) { //us
        uint64_t elapse = curTime - m_startTime;
        uint64_t idx = elapse * m_timeBase.num / m_timeBase.den;
        uint64_t runTime = m_startTime + (idx + 1) * m_timeBase.den / m_timeBase.num;
        //HJLogi("curTime:" + HJ2STR(curTime) + ", elapse:" + HJ2STR(elapse) + ", runTime:" + HJ2STR(runTime));
        return runTime;
    }
private:
    void builDeltas();
public:
    int                     m_TIdx = 0;
    uint64_t                m_startTime = 0;   //us
    HJRational             m_timeBase = {30, HJ_MS_TO_US(1000)};
    std::vector<uint64_t>   m_deltas;
    std::vector<uint64_t>   m_denDeltas;
};

template<class R, class... Args>
class HJTaskCancelable;
template<class R, class... Args>
class HJTaskCancelable<R(Args...)> : public HJNonCopyable //, public std::enable_shared_from_this<HJTaskCancelable<R, Args...>>
{
public:
    using Ptr = std::shared_ptr<HJTaskCancelable>;
    using func_type = std::function<R(Args...)>;
    
    HJTaskCancelable() {}
    template<typename FUNC>
    HJTaskCancelable(FUNC &&task) {
        m_strongTask = std::make_shared<func_type>(std::forward<FUNC>(task));
        m_weakTask = m_strongTask;
        m_tskState = HJTASK_Init;
    }
    
    template<typename FUNC>
    void setRunnable(FUNC &&task) {
        m_strongTask = std::make_shared<func_type>(std::forward<FUNC>(task));
        m_weakTask = m_strongTask;
        m_tskState = HJTASK_Init;
    }

    void cancel() {
        //HJ_AUTOU_LOCK(m_tskMutex)
        m_strongTask = nullptr;
    }
    operator bool() {
        return m_strongTask && *m_strongTask;
    }
    void operator=(std::nullptr_t) {
        m_strongTask = nullptr;
    }

    virtual R operator()(Args ...args) const {
        auto strongTask = m_weakTask.lock();
        if (strongTask && *strongTask) {
            return (*strongTask)(std::forward<Args>(args)...);
        }
        return defaultValue<R>();
    }

    template<typename T>
    static typename std::enable_if<std::is_void<T>::value, void>::type
    defaultValue() {}

    template<typename T>
    static typename std::enable_if<std::is_pointer<T>::value, T>::type
    defaultValue() {
        return nullptr;
    }

    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type
    defaultValue() {
        return 0;
    }
    //
    virtual R run(Args ...args) {
        //HJ_AUTOU_LOCK(m_tskMutex)
        return (*this)(std::forward<Args>(args)...);
    }
    virtual int reAsync() { return HJ_OK; };
    virtual int yield() { return HJ_OK; };
    
    const size_t getTid() const {
        return m_tid;
    }
    void setTid(const size_t tid) {
        m_tid = tid;
    }
    const size_t getClsID() const {
        return m_clsID;
    }
    void setClsID(const size_t clsID) {
        m_clsID = clsID;
    }
    const uint64_t getRunTime() const {
        return m_runtime;
    }
    void setRunTime(const uint64_t time) {
        m_runtime = time;
    }
    const uint64_t getDelayTime() const {
        return m_delayTime;
    }
    void setDelayTime(const uint64_t time) {
        m_delayTime = time;
    }
    //void setExecutor(const std::shared_ptr<HJExecutor>& executor) {
    //    if (!(m_tskState & HJTASK_Reject)) {
    //        m_executor = executor;
    //    } else {
    //        HJLogi("m_executor");
    //    }
    //}
    void setTskState(const HJTaskState state) {
        m_tskState = state;
    }
    const HJTaskState getTskState() {
        return (HJTaskState)HJ_TASK_RESERVED(m_tskState);
    }
    void rejectTskState() {
        m_tskState = (HJTaskState)(m_tskState | HJTASK_Reject);
    }
    const bool isTskReady() {
        return (HJTASK_Ready == HJ_TASK_RESERVED(m_tskState));
    }
    const bool isTskDone() {
        return (HJTASK_Done == HJ_TASK_RESERVED(m_tskState));
    }
protected:
    std::weak_ptr<func_type>    m_weakTask;
    std::shared_ptr<func_type>  m_strongTask;
public:
    size_t                      m_clsID = 0;
    size_t                      m_tid = 0;
    uint64_t                    m_runtime = 0;          //us
    uint64_t                    m_delayTime = 0;        //ms
    //std::weak_ptr<HJExecutor>  m_executor;
    HJTaskState                 m_tskState = HJTASK_NONE;
    HJFixedTimer::Ptr           m_fixed = nullptr;
    //std::mutex                  m_tskMutex;
};
using HJVoidTask = HJTaskCancelable<void(void)>;

class HJTask : public HJVoidTask, public virtual HJObject
{
public:
    using Ptr = std::shared_ptr<HJTask>;
    HJTask() = default;
    HJTask(HJRunnable&& run)
        : HJVoidTask(std::move(run))
    { }
};

//***********************************************************************************//
class HJCoroTask : public HJTask
{
public:
    using Ptr = std::shared_ptr<HJCoroTask>;
    static const int STACK_SIZE_DEFAULT;

    HJCoroTask(const size_t stackSize = STACK_SIZE_DEFAULT);
    HJCoroTask(HJRunnable&& run, const size_t stackSize = STACK_SIZE_DEFAULT);
    virtual ~HJCoroTask();
    
    virtual int setRunnable(HJRunnable&& run, const size_t stackSize = STACK_SIZE_DEFAULT);
    virtual void destroy();
    virtual void run() override;
    virtual int reAsync() override;
    virtual int yield() override;
private:
    virtual int initmco();
protected:
    size_t      m_stackSize = 0;
    mco_coro*   m_CoHandle = NULL;
};

//***********************************************************************************//
template<typename T>
class HJTimeObjectMap {
public:
    using Ptr = std::shared_ptr<HJTimeObjectMap<T>>;
    //
    void push(T &&task, int64_t time) {
        HJ_AUTO_LOCK(m_mutex);
        m_taskMap.emplace(time, std::forward<T>(task));
        HJLogi("end, tsk run time : " + HJ2STR(time) + " us");
        m_sem.notify();
    }
    void pushFirst(T &&task)
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_taskMap.size() > 0) {
            auto it = m_taskMap.begin();
            int64_t time = it->first - 1;
            m_taskMap.emplace(time, std::forward<T>(task));
        }
        else {
            int64_t time = HJTime::getCurrentMicrosecond();
            m_taskMap.emplace(time, std::forward<T>(task));
        }
        m_sem.notify();
    }
    void pushExit(uint32_t n = 1) {
        m_sem.notify(n);
    }
    int64_t getTask(T &tsk, int64_t nowTime)
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_taskMap.size() <= 0) {
            return HJ_UINT_MAX * HJ_US_PER_SEC;
        }
        auto it = m_taskMap.begin();
        if (it->first <= nowTime) {
            tsk = std::move(it->second);
            m_taskMap.erase(it);
            return 0;
        }
        return it->first - nowTime;
    }
    void waitTask(T &tsk, int64_t nowTime)
    {
        int64_t time = getTask(tsk, nowTime);
        HJLogi("entry, now time:" + HJ2STR(nowTime) + " us, tsk wait time:" + HJ2STR(time) + " us");
        if(time > 0) {
            m_sem.waitFor(HJMicroSeconds(time));
        } else {
            m_sem.wait();
        }
        HJLogi("waitTask end, wait time:" + HJ2String(time) + " us");
    }
    int64_t getMinTime() const {
        HJ_AUTO_LOCK(m_mutex);
        if (m_taskMap.size() > 0) {
            auto it = m_taskMap.begin();
            return it->first;
        }
        return HJTime::getCurrentMicrosecond();
    }
    const size_t size() const {
        HJ_AUTO_LOCK(m_mutex);
        return m_taskMap.size();
    }
    void removeAll() {
        {
            HJ_AUTO_LOCK(m_mutex);
            m_taskMap.clear(); 
        }
        m_sem.reset();
    }
    void remove(size_t clsID)
    {
        HJ_AUTO_LOCK(m_mutex);
        for (auto it = m_taskMap.begin(); it != m_taskMap.end(); ) {
            const HJTask::Ptr& tsk = it->second;
            if (clsID == tsk->getClsID() && m_sem.tryWait()) {
                HJLogi("remove task cls id:" + HJ2String(clsID) + ", task id:" + HJ2String(tsk->getID()));
                it = m_taskMap.erase(it);
            } else {
                it++;
            }
        }
    }
    void remove(const std::string& name)
    {
        if(name.empty()) {
            return;
        }
        HJ_AUTO_LOCK(m_mutex);
        for (auto it = m_taskMap.begin(); it != m_taskMap.end(); ) {
            const HJTask::Ptr& tsk = it->second;
            if (name == tsk->getName() && m_sem.tryWait()) {
                HJLogi("202204301245 remove task name:" + name + ", cls id:" + HJ2String(tsk->getClsID()) + ", task id:" + HJ2String(tsk->getID()));
                it = m_taskMap.erase(it);
            } else {
                it++;
            }
        }
    }
private:
    std::multimap<int64_t, T>       m_taskMap;
    std::recursive_mutex            m_mutex;
    HJSemaphore                    m_sem;
};
//using HJTaskMap = HJTimeObjectMap<HJTask::Ptr>;

//***********************************************************************************//
class HJTaskMap : public HJObject 
{
public:
    using Ptr = std::shared_ptr<HJTaskMap>;
    //
    void push(HJTask::Ptr& tsk);
    void pushFirst(HJTask::Ptr& tsk);
    void pushExit(uint32_t n = 1) {
        HJ_AUTO_LOCK(m_mutex);
        notify(n);
    }
    int64_t getTask(HJTask::Ptr& tsk, int64_t nowTime);
    void waitTask(HJTask::Ptr& tsk, int64_t nowTime);
    const int64_t getMinTime();
    const size_t size() {
        HJ_AUTO_LOCK(m_mutex);
        return m_taskMap.size();
    }
    void removeAll() {
        HJ_AUTO_LOCK(m_mutex);
        m_taskMap.clear();
        m_count = 0;
    }
    void remove(size_t clsID);
    void remove(const std::string& name);
    //
    void notify(uint32_t n = 1) {
        m_count += n;
        if (1 == n) {
            m_cv.notify_one();
        }  else {
            m_cv.notify_all();
        }
    }
    bool tryWait() {
        if (m_count > 0) {
            --m_count;
            return true;
        }
        return false;
    }
private:
    std::multimap<int64_t, HJTask::Ptr>     m_taskMap;
    std::mutex                              m_mutex;
    //
    std::condition_variable                 m_cv;
    size_t                                  m_count = 0;
};


NS_HJ_END
