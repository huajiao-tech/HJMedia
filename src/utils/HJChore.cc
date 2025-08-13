//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJChore.h"
#define MINICORO_IMPL
#include "minicoro.h"
#include "HJFLog.h"
#include "HJExecutor.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJEnumToStringFuncImplBegin(HJTaskState)
    HJEnumToStringItem(HJTASK_NONE),
    HJEnumToStringItem(HJTASK_Init),
    HJEnumToStringItem(HJTASK_Ready),
    HJEnumToStringItem(HJTASK_Running),
    HJEnumToStringItem(HJTASK_Done),
    HJEnumToStringItem(HJTASK_RESERVED),
    HJEnumToStringItem(HJTASK_Reject),
HJEnumToStringFuncImplEnd(HJTaskState);

//***********************************************************************************//
HJFixedTimer::HJFixedTimer(int rate)
{
    m_timeBase = {rate, (int)HJ_US_PER_SEC};
    //builDeltas();
    HJFLogi("time base {} : {} us", m_timeBase.num, m_timeBase.den);
}
HJFixedTimer::HJFixedTimer(uint64_t delta)
{
    m_timeBase = {1, (int)HJ_MS_TO_US(delta)};
    //builDeltas();
    HJFLogi("time base {} : {} us", m_timeBase.num, m_timeBase.den);
}
HJFixedTimer::HJFixedTimer(const HJRational timeBase)
{
    m_timeBase = {timeBase.num, (int)HJ_MS_TO_US(timeBase.den)};
    //builDeltas();
    HJFLogi("time base {} : {} us", m_timeBase.num, m_timeBase.den);
}
HJFixedTimer::~HJFixedTimer()
{
    
}
void HJFixedTimer::builDeltas()
{
    int mod = m_timeBase.den % m_timeBase.num;
    int delta = m_timeBase.den / m_timeBase.num;
    for (int i = 0; i < m_timeBase.num; i++) {
        m_deltas.push_back((uint64_t)delta);
    }
    if (mod > 0)
    {
        int cnt = 0;
        int step = m_timeBase.num/mod + 1;
        for (int i = 0; i < m_timeBase.num; i+=step) {
            m_deltas[i] += 1;
            cnt++;
        }
        for (int i = 1, c = 0; c < (mod - cnt); i+=step, c++) {
            m_deltas[i] += 1;
        }
    }
    m_denDeltas.push_back(m_deltas[0]);
    for(int i = 1; i < m_deltas.size(); i++) {
        m_denDeltas.push_back(m_denDeltas[i-1] + m_deltas[i]);
    }
//    for (int i = 0; i < m_deltas.size(); i++) {
//        HJLogi("builDeltas delta:" + HJ2STR(m_deltas[i]));
//    }
//    HJLogi("builDeltas end, last den time:" + HJ2STR(m_denDeltas.back()));
    return;
}

//***********************************************************************************//
const int HJCoroTask::STACK_SIZE_DEFAULT = 256 * 1024;
static void onCoroEntry(mco_coro* coHandle)
{
//    HJLogi("HJMiniCoroTask onCoroEntry entry");
    if (!coHandle) {
        return;
    }
    HJCoroTask* coroTask = reinterpret_cast<HJCoroTask *>(coHandle->user_data);
    if (!coroTask) {
        return;
    }
    (*coroTask)();
    
//    HJLogi("HJMiniCoroTask onCoroEntry end");
}

HJCoroTask::HJCoroTask(const size_t stackSize/* = 0*/)
    :m_stackSize(stackSize)
{
    HJLogi("HJMiniCoroTask constructed end");
}

HJCoroTask::HJCoroTask(HJRunnable&& run, const size_t stackSize/* = 0*/)
    : HJTask(std::move(run))
    , m_stackSize(stackSize)
{
    initmco();
//    HJLogi("HJMiniCoroTask create end, name:" + getName());
}

HJCoroTask::~HJCoroTask()
{
    destroy();
//    HJLogi("HJMiniCoroTask release end, name:" + getName() + ", id:" + HJ2String(getID()));
}

int HJCoroTask::initmco()
{
    setName(HJExecutorGlobalName(""));
    
    mco_coro* coHandle = NULL;
    mco_desc coDesc = mco_desc_init(onCoroEntry, m_stackSize);
    coDesc.user_data = reinterpret_cast<void *>(this);
    //
    mco_result res = mco_create(&coHandle, &coDesc);
    if (MCO_SUCCESS != res) {
        HJFLoge("HJMiniCoroTask create error:{}",res);
    }
    if (MCO_SUSPENDED != mco_status(coHandle)) {
        HJLoge("HJMiniCoroTask status error");
    }
    m_CoHandle = coHandle;
    return HJ_OK;
}

int HJCoroTask::setRunnable(HJRunnable&& run, const size_t stackSize)
{
    HJTask::setRunnable(std::move(run));
    m_stackSize = stackSize;
    return initmco();
}

void HJCoroTask::destroy()
{
    if (m_CoHandle) {
        mco_destroy((mco_coro*)m_CoHandle);
        m_CoHandle = NULL;
    }
}

int HJCoroTask::reAsync()
{
    HJExecutor::Ptr executor;// = m_executor.lock();
    if (!executor) {
        return HJErrNotAlready;
    }
//    HJLogi("HJMiniCoroTask resume entry, name:" + getName());
    return executor->asyncTask(sharedFrom(this), false);
}

int HJCoroTask::yield()
{
    if (!m_CoHandle) {
        return HJErrNotAlready;
    }
//    HJLogi("HJMiniCoroTask yield entry, name:" + getName());
    return mco_yield((mco_coro *)m_CoHandle);
}

void HJCoroTask::run()
{
    if (!m_CoHandle) {
        return;
    }
//    HJLogi("HJMiniCoroTask coro run entry, name:" + getName());
    if (HJTASK_Done == HJ_TASK_RESERVED(m_tskState)) {
        destroy();
        int res = initmco();
        if (HJ_OK != res) {
            return;
        }
    }
    mco_result res = mco_resume((mco_coro *)m_CoHandle);
    if (MCO_SUCCESS != res) {
        HJFLoge("HJMiniCoroTask run resume error:{}", res);
    }
//    HJLogi("HJMiniCoroTask coro run end, name:" + getName());
    return;
}

//***********************************************************************************//
void HJTaskMap::push(HJTask::Ptr& tsk)
{
    HJ_AUTOU_LOCK(m_mutex);
//    HJFLogi("entry, this:{} task:{} tsk tid:{} tsk id:{} run time:{} us ", (size_t)this, (size_t)tsk.get(), tsk->getTid(), tsk->getID(), tsk->m_runtime);
//    for (const auto& it : m_taskMap) {
//        if (tsk.get() == it.second.get()) {
//            HJFLogw("warning, tsk id:{} aready in lists", tsk->getID());
//            return;
//        }
//    }
    m_taskMap.emplace(tsk->m_runtime, tsk);
    for (const auto& it : m_taskMap) {
        if (it.first != it.second->m_runtime) {
            HJFLogw("warning, task key:{}, tsk tid:{} tsk id:{} run time:{} us ", it.first, it.second->m_tid, it.second->getID(), it.second->m_runtime);
        }
    }
    notify();
    return;
}

void HJTaskMap::pushFirst(HJTask::Ptr& tsk)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (m_taskMap.size() > 0) {
        auto it = m_taskMap.begin();
        int64_t time = it->first - 1;
        m_taskMap.emplace(time, tsk);
    }
    else {
        int64_t time = HJTime::getCurrentMicrosecond();
        m_taskMap.emplace(time, tsk);
    }
    notify();
    return;
}

int64_t HJTaskMap::getTask(HJTask::Ptr& tsk, int64_t nowTime)
{
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
void HJTaskMap::waitTask(HJTask::Ptr& tsk, int64_t nowTime)
{
    HJ_AUTOU_LOCK(m_mutex);
    int64_t time = getTask(tsk, nowTime);
    //HJFLogi("entry, tsk id:{} now time:{} us, run time:{} us, wait time:{} us", (int64_t)(tsk ? tsk->getID() : -1), nowTime, tsk ? tsk->m_runtime : 0, time);
//    if (tsk) {
//        HJFLogi("entry, tsk tid:{} tsk id:{} now time:{} us, run time:{} us, wait time:{} us", tsk->m_tid, tsk->getID(), nowTime, tsk->m_runtime, time);
//    }
    if (time > 0) {
        auto finished = m_cv.wait_for(lock, HJMicroSeconds(time));
        if (finished == std::cv_status::timeout) {
//            HJLogi("wait_for timeout");
        }
    }
    else {
        m_cv.wait(lock, [&] { return m_count > 0; });
        --m_count;
    }
    //HJFLogi("end, now time:{} us, tsk run time:{}", HJTime::getCurrentMicrosecond(), (int64_t)(tsk ? tsk->getRunTime() : HJ_NOTS_VALUE));
}
const int64_t HJTaskMap::getMinTime() {
    HJ_AUTOU_LOCK(m_mutex);
    if (m_taskMap.size() > 0) {
        auto it = m_taskMap.begin();
        return it->first;
    }
    return HJTime::getCurrentMicrosecond();
}
void HJTaskMap::remove(size_t clsID)
{
    HJ_AUTOU_LOCK(m_mutex);
    for (auto it = m_taskMap.begin(); it != m_taskMap.end(); ) {
        const HJTask::Ptr& tsk = it->second;
        if (clsID == tsk->getClsID() && tryWait()) {
            HJFLogi("remove task cls id:{}, task id:{}", clsID, tsk->getID());
            it = m_taskMap.erase(it);
        }
        else {
            it++;
        }
    }
}
void HJTaskMap::remove(const std::string& name)
{
    if (name.empty()) {
        return;
    }
    HJ_AUTOU_LOCK(m_mutex);
    for (auto it = m_taskMap.begin(); it != m_taskMap.end(); ) {
        const HJTask::Ptr& tsk = it->second;
        if (name == tsk->getName() && tryWait()) {
            HJFLogi("remove task name:{}, cls id:{}, task id:{}", name, tsk->getClsID(), tsk->getID());
            it = m_taskMap.erase(it);
        }
        else {
            it++;
        }
    }
}


NS_HJ_END