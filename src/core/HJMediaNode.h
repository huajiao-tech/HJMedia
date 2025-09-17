//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJExecutor.h"
#include "HJMediaFrame.h"
#include "HJNotify.h"
#include "HJNoticeCenter.h"
#include "HJEnvironment.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJMediaInterface
{
public:
    virtual int init() { return HJ_OK; }
    virtual int start() = 0;
    virtual int play() = 0;
    virtual int pause() = 0;
    virtual int resume() { return HJ_OK; }
    virtual int flush() { return HJ_OK; }
    virtual void done() { }
};

class HJNodeStorage
{
public:
    using Ptr = std::shared_ptr<HJNodeStorage>;

    virtual void addInStorage(const int capacity, const std::string& key) {
        int size = capacity ? capacity : m_defaultCapacity;
        m_inStorages[key] = std::make_shared<HJMediaStorage>(size, key);
    }
    virtual void removeInStorage(const std::string& key) {
        const auto& storeIt = m_inStorages.find(key);
        if (storeIt != m_inStorages.end()) {
            m_inStorages.erase(storeIt);
        }
    }
    virtual HJMediaStorage::Ptr getInStorage(const std::string& key = "") {
        const auto& it = key.empty() ? m_inStorages.begin() : m_inStorages.find(key);
        if (it != m_inStorages.end()) {
            return it->second;
        }
        return nullptr;
    }
    virtual HJMediaStorage::Ptr getMinStorage() {
        HJMediaStorage::Ptr validStore = nullptr;
        std::multimap<int64_t, HJMediaStorage::Ptr> validStorages;
        for (const auto& it : m_inStorages) 
        {
            HJMediaStorage::Ptr store = it.second;
            if (store->isFull()) {
                HJLogw("warning, storage is full");
            }
            if (!store->isEOF()) {
                validStore = store;
                if (store->size() > 0) {
                    validStorages.emplace(store->getTopDTS(), store);
                }
            }
        }
        if (validStorages.size() <= 0)  {
            return validStore;
        }
        auto it = validStorages.begin();
        if (it != validStorages.end()) {
            return it->second;
        }
        return nullptr;
    }
    virtual size_t getInStorageSize(const std::string& key = "") {
        HJMediaStorage::Ptr storage = getInStorage(key);
        if (storage) {
            return storage->size();
        }
        return 0;
    }

    virtual void setOutStorageCapacity(const int capacity = HJ_DEFAULT_STORAGE_CAPACITY) {
        m_outStorage = std::make_shared<HJMediaStorage>(capacity);
    }
    virtual void setOutStorageTimeCapacity(const int64_t capacity) {
        m_outStorage = std::make_shared<HJMediaStorage>();
        m_outStorage->setTimeCapacity(capacity);
    }
    virtual size_t getOutStorageSize() {
        if (m_outStorage) {
            return m_outStorage->size();
        }
        return 0;
    }
    virtual void clearInOutStorage() {
        for (const auto& it : m_inStorages) {
            it.second->clear();
        }
        if (m_outStorage) {
            m_outStorage->clear();
        }
    }
    virtual bool isFull(const std::string& key = "") {
        HJMediaStorage::Ptr storage = getInStorage(key);
        if (storage) {
            return storage->isFull();
        }
        return true;
    }
    virtual bool isOutFull() {
        if (m_outStorage) {
            return m_outStorage->isFull();
        }
        return true;
    }
protected:
    int                        m_defaultCapacity = HJ_DEFAULT_STORAGE_CAPACITY;
    HJMediaStorageMap          m_inStorages;
    HJMediaStorage::Ptr        m_outStorage = nullptr;
};

//***********************************************************************************//
class HJMediaVNode : public HJNodeStorage, public HJMediaInterface, public virtual HJObject
{
public:
    using Ptr = std::shared_ptr<HJMediaVNode>;
    using WPtr = std::weak_ptr<HJMediaVNode>;
    using VNodeWMaps = std::unordered_map<std::string, HJMediaVNode::WPtr>;
    //
    HJMediaVNode(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJMediaVNode(const HJEnvironment::Ptr& env = nullptr, const HJExecutor::Ptr& executor = nullptr, const bool isAuto = true);
    virtual ~HJMediaVNode();
    
    virtual int init(const int capacity = HJ_DEFAULT_STORAGE_CAPACITY);
    virtual int start();
    virtual int play();
    virtual int pause();
    virtual int resume();
    virtual int flush(const HJSeekInfo::Ptr info = nullptr);
    virtual void done();
public:
    virtual int deliver(HJMediaFrame::Ptr mediaFrame, const std::string& key = "");
    virtual const HJMediaFrame::Ptr pop(const std::string& key = "");
    virtual int recv(HJMediaFrame::Ptr mediaFrame);
    virtual const HJMediaFrame::Ptr popRecv();
    //
    virtual int notify(const HJNotification::Ptr ntf) {
        if (m_env) {
            return m_env->notify(ntf);
        }
        return HJ_OK;
    }
    virtual int notifyMediaFrame(const HJMediaFrame::Ptr mavf) {
        if (!mavf) {
            return HJ_OK;
        }
        {
            HJProgressInfo::Ptr progInfo = std::make_shared<HJProgressInfo>(mavf->getPTS(), 0);
            HJNotification::Ptr ntf = HJMakeNotification(HJNotify_ProgressStatus, mavf->getPTS());
            (*ntf)[HJProgressInfo::KEY_WORLDS] = progInfo;
            notify(ntf);
        }
        if (m_env) {
            return m_env->notifyMediaFrame(mavf);
        }
        return HJ_OK;
    }
    //virtual int sendMessage(const HJMessage::Ptr msg);
    //virtual int onMessage(const HJMessage::Ptr msg);
public:
    virtual void setRunState(const HJRunState state) {
        m_runState = state;
    }
    const HJRunState getRunState() const {
        return m_runState;
    }
    bool isEndOfRun() {
        if (HJRun_Stop == m_runState ||
            HJRun_Done == m_runState ||
            HJRun_Error == m_runState) {
            return true;
        }
        return false;
    }
    virtual void setMediaType(const HJMediaType& type) {
        m_mediaType = type;
    }
    const HJMediaType& getMediaType() const {
        return m_mediaType;
    }
    
    void setEnviroment(const HJEnvironment::Ptr& env) {
        m_env = env;
    }
    const HJEnvironment::Ptr getEnviroment() const {
        return m_env;
    }
    /**
     * nexts
     */
    virtual int connect(const HJMediaVNode::Ptr& next, const int capacity = 0) {
        addNext(next);
        next->addPre(sharedFrom(this), capacity);
        HJLogi("node name:" + next->getName() + ", storage size:" + HJ2STR(next->getInStorageSize()));
        return HJ_OK;
    }
    virtual int disconnect(const HJMediaVNode::Ptr& next) {
        removeNext(next->getName());
        next->removePre(getName());
        return HJ_OK;
    }
    virtual void addNext(const HJMediaVNode::Ptr& node) {
        HJMediaVNode::WPtr wp = node;
        m_nexts[node->getName()] = wp;          //node->weak_ptr<HJMediaVNode>();
    }
    virtual void removeNext(const std::string& key) {
        const auto& it = m_nexts.find(key);
        if (it != m_nexts.end()) {
            m_nexts.erase(it);
        }
    }
    virtual HJMediaVNode::Ptr getNext(const std::string& key) {
        const auto& it = m_nexts.find(key);
        if (it != m_nexts.end()) {
            return it->second.lock();
        }
        return nullptr;
    }
    virtual HJMediaVNode::Ptr getNext(const HJMediaType& type) {
        for (const auto& it : m_nexts) {
            auto node = it.second.lock();
            if(node && type == node->getMediaType()) {
                return it.second.lock();
            }
        }
        return nullptr;
    }
    virtual HJMediaVNode::Ptr getNext() {
        auto it = m_nexts.begin();
        if (it != m_nexts.end()) {
            return it->second.lock();
        }
        return nullptr;
    }
    template<typename FUNC>
    void forEachNext(FUNC &&func) {
        for (const auto& it : m_nexts) {
            func(it.second.lock());
        }
    }

    virtual bool IsNextsFull();
   
    /**
     * pres
     */
    virtual void addPre(const HJMediaVNode::Ptr& node, const int capacity = 0) {
        std::string key = node->getName();
        HJMediaVNode::WPtr wp = node;
        m_pres[key] = wp;//node->weak_ptr<HJMediaVNode>();
        //
        addInStorage(capacity, key);
    }
    virtual void removePre(const std::string& key) {
        auto it = m_pres.find(key);
        if (it != m_pres.end()) {
            m_pres.erase(it);
        }
        //
        removeInStorage(key);
    }
    virtual HJMediaVNode::Ptr getPre(const std::string& name) {
        auto it = m_pres.find(name);
        if (it != m_pres.end()) {
            return it->second.lock();
        }
        return nullptr;
    }
    virtual HJMediaVNode::Ptr getPre(const HJMediaType& type) {
        for (const auto& it : m_pres) {
            auto node = it.second.lock();
            if(type == node->getMediaType()) {
                return it.second.lock();
            }
        }
        return nullptr;
    }
    virtual HJMediaVNode::Ptr getPre() {
        auto it = m_pres.begin();
        if (it != m_pres.end()) {
            return it->second.lock();
        }
        return nullptr;
    }
    template<typename FUNC>
    void forEachPre(FUNC &&func) {
        for (const auto& it : m_pres) {
            func(it.second.lock());
        }
    }
    //
    void setScheduler(const HJScheduler::Ptr& scheduler) {
        if(scheduler) {
            m_scheduler = scheduler;
        } else {
            m_scheduler = std::make_shared<HJScheduler>();
        }
    }
    const HJScheduler::Ptr& getScheduler() const {
        return m_scheduler;
    }
    virtual void setEOF(const bool eof) {
        m_eof = eof;
    }
    virtual const bool getEOF() const {
        return m_eof;
    }
    virtual bool getNextsEOF() {
        bool eof = true;
        for (const auto& it : m_nexts) {
            HJMediaVNode::Ptr node = it.second.lock();
            eof = node->getEOF();
            if (!eof) {
                break;
            }
        }
        return eof;
    }
    //virtual void setRunState(const HJRunState state) {
    //    m_runState = state;
    //}
    //virtual void setRunStateEx(const HJRunState state) {
    //    m_runState = HJRUNSTATUS(state) | HJRUNSUBSTATUS(m_runState);
    //}
    //virtual void setFlush(const bool flush) {
    //    if (flush) {
    //        m_runState |= HJRun_FLUSH;
    //    } else {
    //        m_runState &= ~HJRun_FLUSH;
    //    }
    //}
    //virtual bool getFlush() {
    //    return m_runState & HJRun_FLUSH;
    //}
protected:
    virtual int asyncSelf(uint64_t delayTimeMS = 0) { return HJ_OK; };
    virtual int tryAsyncSelf(uint64_t delayTimeMS = 0) { return HJ_OK; };
    virtual int tryAsyncPreNode(uint64_t delayTimeMS = 0) { return HJ_OK; };
protected:
//    HJRunState                                 m_cmdState = HJRun_NONE;
    HJRunState             m_runState = { HJRun_NONE };
    HJMediaType             m_mediaType = HJMEDIA_TYPE_NONE;
    HJScheduler::Ptr       m_scheduler = nullptr;
    HJEnvironment::Ptr     m_env = nullptr;
    std::recursive_mutex    m_mutex;
    
    VNodeWMaps              m_nexts;
    VNodeWMaps              m_pres;
    std::atomic<bool>       m_eof = { false };
};
using HJMediaVNodeVector = std::vector<HJMediaVNode::Ptr>;

//***********************************************************************************//
class HJMediaNode : public HJMediaVNode, public HJTask
{
public:
    using Ptr = std::shared_ptr<HJMediaNode>;
    HJMediaNode(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true)
        : HJMediaVNode(env, scheduler, isAuto) {}
    HJMediaNode(const HJEnvironment::Ptr& env = nullptr, const HJExecutor::Ptr& executor = nullptr, const bool isAuto = true)
        : HJMediaVNode(env, executor, isAuto) {}
public:
    static const int64_t MTASK_DELAY_TIME_DEFAULT;

    typedef enum HJNodeDriveType
    {
        HJNODE_DRIVE_NONE = 0x00,
        HJNODE_DRIVE_NEXT = 0x01,
        HJNODE_DRIVE_PRE = 0x02,
        HJNODE_DRIVE_DEFAULT = HJNODE_DRIVE_NEXT | HJNODE_DRIVE_PRE
    } HJNodeDriveType;
public:
    virtual int proRun() { return HJ_OK; }

    virtual int asyncSelf(uint64_t delayTimeMS = 0) override
    {
        HJ_AUTO_LOCK(m_mutex);
        if (!m_scheduler || m_isBusy) {
            //HJLogw("warning, async self node:" + getName() + ", is busy:" + HJ2STR(m_isBusy));
            return HJ_OK;//HJErrNotAlready;
        }
        //HJLogi("entry, async self node:" + getName() + ", delay time:" + HJ2STR(delayTimeMS) + " ms");
        m_isBusy = true;
        int res = m_scheduler->asyncAlwaysTask(sharedFrom(this), delayTimeMS);
        if (HJ_OK != res) {
            m_isBusy = false;
            HJLoge("error, async task failed");
            return res;
        }
        //HJLogi("end, async self node:" + getName());

        return HJ_OK;
    }
    virtual int tryAsyncSelf(uint64_t delayTimeMS = 0) override
    {
        //HJLogi("entry, node:" + getName() + ", tsk state:" + HJTaskStateToString(getTskState()) + ", delay time:" + HJ2STR(delayTimeMS) + " ms");
        int res = HJ_OK;
        if (!IsNextsFull()) {
            res = asyncSelf(delayTimeMS);
        }
        //HJLogi("end, node:" + getName());
        return res;
    }
    virtual int tryAsyncPreNode(uint64_t delayTimeMS = 0) override
    {
        for (const auto& it : m_pres) {
            auto preNode = std::dynamic_pointer_cast<HJMediaNode>(it.second.lock());
            //HJLogi("entry node:" + getName() + " try async pre node:" + preNode->getName());
            if (!isFull() && preNode) {
                const HJMediaNode::Ptr preTask = std::dynamic_pointer_cast<HJMediaNode>(preNode);
                preTask->asyncSelf(delayTimeMS);
            }
            //HJLogi("end, pre node:" + preNode->getName());
        }
        return HJ_OK;
    }
    /**
    * other thread or
    */
    virtual int deliver(HJMediaFrame::Ptr mediaFrame, const std::string& key = "") override
    {
        int res = HJMediaVNode::deliver(std::move(mediaFrame));
        if (HJ_OK != res) {
            return res;
        }
        if (isDriveNext()) {
            //HJLogi("async self node: " + getName());
            asyncSelf();
        }
        return res;
    }
    virtual const HJMediaFrame::Ptr pop(const std::string& key = "") override
    {
        const auto mavf = HJMediaVNode::pop(key);
        if (isDrivePre()) {
            tryAsyncPreNode();
        }
        return mavf;
    }
    virtual void setDriveType(const HJNodeDriveType type) {
        m_driveType = type;
    }
    virtual bool isDriveNext() {
        return m_driveType & HJNODE_DRIVE_NEXT;
    }
    virtual bool isDrivePre() {
        return m_driveType & HJNODE_DRIVE_PRE;
    }
    virtual void setIsBusy(const bool busy) {
        HJ_AUTO_LOCK(m_mutex);
        m_isBusy = busy;
    }
    virtual const bool isBusy() {
        return m_isBusy;
    }
protected:
    HJNodeDriveType    m_driveType = HJNODE_DRIVE_DEFAULT;
    bool                m_isBusy = false;
};
using HJMediaNodeVector = std::vector<HJMediaNode::Ptr>;

//***********************************************************************************//
class HJMediaCNode : public HJMediaVNode, public HJCoroTask
{
public:
    using Ptr = std::shared_ptr<HJMediaCNode>;
    HJMediaCNode(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true)
        : HJMediaVNode(env, scheduler, isAuto)
        , HJCoroTask([=]() { proc(); }) {}
    HJMediaCNode(const HJEnvironment::Ptr& env = nullptr, const HJExecutor::Ptr& executor = nullptr, const bool isAuto = true)
        : HJMediaVNode(env, executor, isAuto)
        , HJCoroTask([=]() { proc(); }) {}

protected:
    virtual int asyncSelf(uint64_t delayTimeMS = 0) override
    {
        if (!m_scheduler) {
            return HJ_OK;//HJErrNotAlready;
        }
        int res = m_scheduler->asyncAlwaysTask(sharedFrom(this), delayTimeMS);
        if (HJ_OK != res) {
            return res;
        }
        return HJ_OK;
    }

    //virtual int asyncSelf(uint64_t delayTimeMS = 0)
//{
//    if (!m_scheduler) {
//        return HJ_OK;//HJErrNotAlready;
//    }
//    auto task = m_scheduler->asyncCoro([=]() {
//            proc();
//        }, delayTimeMS);
//    if (!task) {
//        HJLogi("error, aync coro task");
//        return HJErrCoTaskCreate;
//    }
//    return HJ_OK;
//}
private:
    virtual void proc() = 0;
};
using HJMediaCNodeVector = std::vector<HJMediaCNode::Ptr>;

NS_HJ_END
