//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"

NS_HJ_BEGIN
//***********************************************************************************//
namespace HJBroadcast{
    extern const std::string EVENT_PLAYER_NOTIFY;
    extern const std::string EVENT_PLAYER_SEEKINFO;
    extern const std::string EVENT_PLAYER_MEDIAFRAME;
    extern const std::string EVENT_PLAYER_SOURCEFRAME;
};

//***********************************************************************************//
class HJEventDispatcher {
public:
    friend class HJNoticeCenter;
    using Ptr = std::shared_ptr<HJEventDispatcher>;
    ~HJEventDispatcher() = default;
private:
    using MapType = std::unordered_multimap<void *, std::shared_ptr<void>>;

    HJEventDispatcher() = default;

    class InterruptException : public std::runtime_error {
    public:
        InterruptException() : std::runtime_error("InterruptException") {}
        ~InterruptException() {}
    };

    template<typename ...ArgsType>
    int emitEvent(ArgsType &&...args) {
        using funType = std::function<void(decltype(std::forward<ArgsType>(args))...)>;
        decltype(m_mapListener) copy;
        {
            HJ_AUTO_LOCK(m_mutex);
            copy = m_mapListener;
        }

        int ret = 0;
        for (auto &pr : copy) {
            funType *obj = (funType *) (pr.second.get());
            try {
                (*obj)(std::forward<ArgsType>(args)...);
                ++ret;
            } catch (InterruptException &) {
                ++ret;
                break;
            }
        }
        return ret;
    }

    template<typename FUNC>
    void addListener(void *tag, FUNC &&func) {
        using funType = typename HJFuncTraits<typename std::remove_reference<FUNC>::type>::STLFuncType;
        std::shared_ptr<void> pListener(new funType(std::forward<FUNC>(func)), [](void *ptr) {
            funType *obj = (funType *) ptr;
            delete obj;
        });
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        m_mapListener.emplace(tag, pListener);
    }

    void delListener(void *tag, bool &empty) {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        m_mapListener.erase(tag);
        empty = m_mapListener.empty();
    }

private:
    std::recursive_mutex    m_mutex;
    MapType                 m_mapListener;
};

class HJNoticeCenter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJNoticeCenter>;
    //static HJNoticeCenter &Instance();
    HJ_INSTANCE_DECL(HJNoticeCenter)

    template<typename ...ArgsType>
    int emitEvent(const std::string &event, ArgsType &&...args) {
        auto dispatcher = getDispatcher(event);
        if (!dispatcher) {
            return HJErrNotAlready;
        }
        return dispatcher->emitEvent(std::forward<ArgsType>(args)...);
    }

    template<typename FUNC>
    void addListener(void *tag, const std::string &event, FUNC &&func) {
        getDispatcher(event, true)->addListener(tag, std::forward<FUNC>(func));
    }

    void delListener(void *tag, const std::string &event) {
        auto dispatcher = getDispatcher(event);
        if (!dispatcher) {
            return;
        }
        bool empty;
        dispatcher->delListener(tag, empty);
        if (empty) {
            delDispatcher(event, dispatcher);
        }
    }

    void delListener(void *tag) {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        bool empty;
        for (auto it = m_mapDispatcher.begin(); it != m_mapDispatcher.end();) {
            it->second->delListener(tag, empty);
            if (empty) {
                it = m_mapDispatcher.erase(it);
                continue;
            }
            ++it;
        }
    }
    bool hasListener(const std::string& event) {
        auto dispatcher = getDispatcher(event);
        return (nullptr != dispatcher) ? true : false;
    }

    void clearAll() {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        m_mapDispatcher.clear();
    }
private:
    HJEventDispatcher::Ptr getDispatcher(const std::string &event, bool create = false) {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        auto it = m_mapDispatcher.find(event);
        if (it != m_mapDispatcher.end()) {
            return it->second;
        }
        if (create) {
            HJEventDispatcher::Ptr dispatcher(new HJEventDispatcher());
            m_mapDispatcher.emplace(event, dispatcher);
            return dispatcher;
        }
        return nullptr;
    }

    void delDispatcher(const std::string &event, const HJEventDispatcher::Ptr &dispatcher) {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        auto it = m_mapDispatcher.find(event);
        if (it != m_mapDispatcher.end() && dispatcher == it->second) {
            m_mapDispatcher.erase(it);
        }
    }

private:
    std::recursive_mutex                                        m_mutex;
    std::unordered_map<std::string, HJEventDispatcher::Ptr>    m_mapDispatcher;
};

//***********************************************************************************//
class HJNoticeHandler : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJNoticeHandler>;
    HJNoticeHandler();
    virtual ~HJNoticeHandler();
    
    template<typename ...ArgsType>
    int emitEvent(const std::string &event, ArgsType &&...args) {
        return m_noticeCenter->emitEvent(event, std::forward<ArgsType>(args)...);
    }

    template<typename FUNC>
    void addListener(const std::string &event, FUNC &&func) {
        m_noticeCenter->addListener(this, event, std::forward<FUNC>(func));
    }
    void delListener(const std::string& event) {
        m_noticeCenter->delListener(this, event);
    }
    bool hasListener(const std::string& event) {
        return m_noticeCenter->hasListener(event);
    }
    
    //
    template<typename FUNC>
    void addNotify(FUNC &&func) {
        addListener(HJBroadcast::EVENT_PLAYER_NOTIFY, std::forward<FUNC>(func));
    }
    int notify(const HJNotification::Ptr ntf) {
        if (!m_noticeCenter) {
            return HJErrNotAlready;
        }
        return m_noticeCenter->emitEvent(HJBroadcast::EVENT_PLAYER_NOTIFY, ntf);
    }
    bool hasNotify() {
        return m_noticeCenter->hasListener(HJBroadcast::EVENT_PLAYER_NOTIFY);
    }
protected:
    HJNoticeCenter::Ptr    m_noticeCenter = nullptr;
};

//***********************************************************************************//
NS_HJ_END
