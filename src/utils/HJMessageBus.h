#pragma once

#include <cstdint>
#include <algorithm>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "HJError.h"
#include "HJMacros.h"

NS_HJ_BEGIN

template <typename... Args>
struct HJTypeList {};

// Simple constexpr FNV-1a hash for message ids.
constexpr uint32_t HJMsgHash(const char* str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<uint8_t>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

// 64-bit version of FNV-1a hash for message IDs.
constexpr uint64_t HJMsgHash64(const char* str) {
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis for 64-bit
    while (*str) {
        hash ^= static_cast<uint8_t>(*str++);
        hash *= 1099511628211ULL; // FNV prime for 64-bit
    }
    return hash;
}

template <typename T>
struct HJResult {
    HJRet code{ HJ_OK };
    T value{};

    template <typename U>
    static HJResult ok(U&& v) {
        return HJResult{ HJ_OK, T(std::forward<U>(v)) };
    }
    static HJResult error(HJRet err) {
        return HJResult{ err, T{} };
    }
    bool isOk() const { return code == HJ_OK; }
};

template <>
struct HJResult<void> {
    HJRet code{ HJ_OK };

    static HJResult ok() { return HJResult{ HJ_OK }; }
    static HJResult error(HJRet err) { return HJResult{ err }; }
    bool isOk() const { return code == HJ_OK; }
};

template <typename Ret, typename Func, typename List>
struct HJIsCallableWith;

template <typename Ret, typename Func, typename... Args>
struct HJIsCallableWith<Ret, Func, HJTypeList<Args...>>
    : std::bool_constant<std::is_invocable_r_v<Ret, Func, Args...>> {};

template <typename List, typename... CallArgs>
struct HJArgsMatchImpl;

template <typename... Expected, typename... CallArgs>
struct HJArgsMatchImpl<HJTypeList<Expected...>, CallArgs...>
    : std::bool_constant<(sizeof...(Expected) == sizeof...(CallArgs)) && (std::is_convertible_v<CallArgs, Expected> && ...)> {};

template <typename Msg, typename... CallArgs>
struct HJArgsMatch : HJArgsMatchImpl<typename Msg::Args, CallArgs...> {};

template <typename Msg>
struct HJMsgTag {
    static int value;
};

template <typename Msg>
int HJMsgTag<Msg>::value = 0;

template <typename Ret, typename List>
struct HJFunctionFromList;

template <typename Ret, typename... Args>
struct HJFunctionFromList<Ret, HJTypeList<Args...>> {
    using type = std::function<Ret(Args...)>;
};

// Declare a message with fixed signature.
// Example:
//   HJ_MSG_DECLARE(bool, MSG_CAN_DELIVER, const std::string&, const HJMediaFrame::Ptr&)
#define HJ_MSG_DECLARE(RET, MSG, ...) \
    struct MSG { \
        using Ret = RET; \
        using Args = HJTypeList<__VA_ARGS__>; \
        static constexpr const char* name = #MSG; \
        static constexpr uint32_t id = HJMsgHash(#MSG); \
    }; \
    inline constexpr MSG MSG##_ID{}

#define HJ_QUERY_DECLARE                    HJ_MSG_DECLARE
#define HJ_EVENT_DECLARE(MSG, ...)          HJ_MSG_DECLARE(void, MSG, __VA_ARGS__)

class HJMessageBus {
public:
    HJMessageBus() = default;
    HJMessageBus(const HJMessageBus&) = delete;
    HJMessageBus& operator=(const HJMessageBus&) = delete;

    template <typename Msg, typename Func>
    HJRet registerHandler(Func&& func) {
        using Ret = typename Msg::Ret;
        using Args = typename Msg::Args;
        static_assert(HJIsCallableWith<Ret, Func, Args>::value, "handler signature mismatch");

        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }
        {
            std::shared_lock<std::shared_mutex> lock(m_dynamicMutex);
            if (m_handlers_dynamic.find(Msg::id) != m_handlers_dynamic.end()) {
                return HJErrAlreadyExist;
            }
        }

        auto handler = std::make_shared<Handler<Msg>>(std::forward<Func>(func));
        m_handlers_static[Msg::id] = std::move(handler);
        return HJ_OK;
    }

    template <typename Msg, typename Func>
    HJRet registerHandler(Msg, Func&& func) {
        return registerHandler<Msg>(std::forward<Func>(func));
    }

    template <typename Msg, typename Func>
    HJRet registerDynamicHandler(Func&& func) {
        using Ret = typename Msg::Ret;
        using Args = typename Msg::Args;
        static_assert(HJIsCallableWith<Ret, Func, Args>::value, "handler signature mismatch");

        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }
        std::unique_lock<std::shared_mutex> lock(m_dynamicMutex);
        if (m_handlers_dynamic.find(Msg::id) != m_handlers_dynamic.end()) {
            return HJErrAlreadyExist;
        }

        auto handler = std::make_shared<Handler<Msg>>(std::forward<Func>(func));
        m_handlers_dynamic[Msg::id] = std::move(handler);
        return HJ_OK;
    }

    template <typename Msg, typename Func>
    HJRet registerDynamicHandler(Msg, Func&& func) {
        return registerDynamicHandler<Msg>(std::forward<Func>(func));
    }

    template <typename Msg>
    HJRet unRegisterDynamicHandler() {
        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }
        std::unique_lock<std::shared_mutex> lock(m_dynamicMutex);
        auto it = m_handlers_dynamic.find(Msg::id);
        if (it == m_handlers_dynamic.end()) {
            return HJErrNotExist;
        }
        m_handlers_dynamic.erase(it);
        return HJ_OK;
    }

    template <typename Msg>
    HJRet unRegisterDynamicHandler(Msg) {
        return unRegisterDynamicHandler<Msg>();
    }

    template <typename Msg, typename... CallArgs>
    HJResult<typename Msg::Ret> call(CallArgs&&... args) const {
        static_assert(HJArgsMatch<Msg, CallArgs...>::value, "call args mismatch");

        auto it_static = m_handlers_static.find(Msg::id);
        if (it_static != m_handlers_static.end()) {
            auto* base = it_static->second.get();
            if (base->tag != &HJMsgTag<Msg>::value) {
                return HJResult<typename Msg::Ret>::error(HJErrInvalidParams);
            }

            auto* handler = static_cast<Handler<Msg>*>(base);
            return handler->invoke(std::forward<CallArgs>(args)...);
        }

        std::shared_lock<std::shared_mutex> lock(m_dynamicMutex);
        auto it_dynamic = m_handlers_dynamic.find(Msg::id);
        if (it_dynamic == m_handlers_dynamic.end()) {
            return HJResult<typename Msg::Ret>::error(HJErrNotFind);
        }

        auto* base = it_dynamic->second.get();
        if (base->tag != &HJMsgTag<Msg>::value) {
            return HJResult<typename Msg::Ret>::error(HJErrInvalidParams);
        }

        auto* handler = static_cast<Handler<Msg>*>(base);
        return handler->invoke(std::forward<CallArgs>(args)...);
    }

    template <typename Msg, typename... CallArgs>
    HJResult<typename Msg::Ret> call(Msg, CallArgs&&... args) const {
        return call<Msg>(std::forward<CallArgs>(args)...);
    }

private:
    struct HandlerBase {
        explicit HandlerBase(const void* i_tag) : tag(i_tag) {}
        virtual ~HandlerBase() = default;
        const void* tag;
    };

    template <typename Msg>
    struct Handler final : HandlerBase {
        using Ret = typename Msg::Ret;
        using Fn = typename HJFunctionFromList<Ret, typename Msg::Args>::type;

        template <typename Func>
        explicit Handler(Func&& func)
            : HandlerBase(&HJMsgTag<Msg>::value), fn(std::forward<Func>(func)) {}

        template <typename... CallArgs>
        HJResult<Ret> invoke(CallArgs&&... args) {
            if constexpr (std::is_void_v<Ret>) {
                fn(std::forward<CallArgs>(args)...);
                return HJResult<void>::ok();
            } else {
                return HJResult<Ret>::ok(fn(std::forward<CallArgs>(args)...));
            }
        }

        Fn fn;
    };

    std::unordered_map<uint32_t, std::shared_ptr<HandlerBase>> m_handlers_static;
    std::unordered_map<uint32_t, std::shared_ptr<HandlerBase>> m_handlers_dynamic;
    mutable std::shared_mutex m_dynamicMutex;
};

#define HJ_BUS_REGISTER(BUS, MSG, HANDLER)      (BUS).registerHandler<MSG>(HANDLER)
#define HJ_BUS_REGISTER_DYNAMIC(BUS, MSG, HANDLER) (BUS).registerDynamicHandler<MSG>(HANDLER)
#define HJ_BUS_UNREGISTER_DYNAMIC(BUS, MSG)     (BUS).unRegisterDynamicHandler<MSG>()
#define HJ_BUS_CALL(BUS, MSG, ...)              (BUS).call<MSG>(__VA_ARGS__)

class HJQueryBus {
public:
    using Ptr = std::shared_ptr<HJQueryBus>;
    using Wtr = std::weak_ptr<HJQueryBus>;

    HJQueryBus() = default;
    HJQueryBus(const HJQueryBus&) = delete;
    HJQueryBus& operator=(const HJQueryBus&) = delete;

    template <typename Msg, typename Func>
    HJRet registerHandler(Func&& func) {
        using Ret = typename Msg::Ret;
        using Args = typename Msg::Args;
        static_assert(!std::is_void_v<Ret>, "QueryBus requires non-void return");
        static_assert(HJIsCallableWith<Ret, Func, Args>::value, "handler signature mismatch");

        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }
        {
            std::shared_lock<std::shared_mutex> lock(m_dynamicMutex);
            if (m_handlers_dynamic.find(Msg::id) != m_handlers_dynamic.end()) {
                return HJErrAlreadyExist;
            }
        }

        auto handler = std::make_shared<StaticHandler<Msg>>(std::forward<Func>(func));
        m_handlers_static[Msg::id] = std::move(handler);
        return HJ_OK;
    }

    template <typename Msg, typename Func>
    HJRet registerHandler(Msg, Func&& func) {
        return registerHandler<Msg>(std::forward<Func>(func));
    }

    template <typename Msg, typename Func>
    HJRet registerDynamicHandler(Func&& func) {
        using Ret = typename Msg::Ret;
        using Args = typename Msg::Args;
        static_assert(!std::is_void_v<Ret>, "QueryBus requires non-void return");
        static_assert(HJIsCallableWith<HJResult<Ret>, Func, Args>::value, "dynamic handler signature mismatch");

        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }
        std::unique_lock<std::shared_mutex> lock(m_dynamicMutex);
        if (m_handlers_dynamic.find(Msg::id) != m_handlers_dynamic.end()) {
            return HJErrAlreadyExist;
        }

        auto handler = std::make_shared<DynamicHandler<Msg>>(std::forward<Func>(func));
        m_handlers_dynamic[Msg::id] = std::move(handler);
        return HJ_OK;
    }

    template <typename Msg, typename Func>
    HJRet registerDynamicHandler(Msg, Func&& func) {
        return registerDynamicHandler<Msg>(std::forward<Func>(func));
    }

    template <typename Msg>
    HJRet unRegisterDynamicHandler() {
        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }
        std::unique_lock<std::shared_mutex> lock(m_dynamicMutex);
        auto it = m_handlers_dynamic.find(Msg::id);
        if (it == m_handlers_dynamic.end()) {
            return HJErrNotExist;
        }
        m_handlers_dynamic.erase(it);
        return HJ_OK;
    }

    template <typename Msg>
    HJRet unRegisterDynamicHandler(Msg) {
        return unRegisterDynamicHandler<Msg>();
    }

    template <typename Msg, typename... CallArgs>
    HJResult<typename Msg::Ret> query(CallArgs&&... args) const {
        static_assert(!std::is_void_v<typename Msg::Ret>, "QueryBus requires non-void return");
        static_assert(HJArgsMatch<Msg, CallArgs...>::value, "call args mismatch");

        auto it_static = m_handlers_static.find(Msg::id);
        if (it_static != m_handlers_static.end()) {
            auto* base = it_static->second.get();
            if (base->tag != &HJMsgTag<Msg>::value) {
                return HJResult<typename Msg::Ret>::error(HJErrInvalidParams);
            }

            auto* handler = static_cast<StaticHandler<Msg>*>(base);
            return handler->invoke(std::forward<CallArgs>(args)...);
        }

        std::shared_lock<std::shared_mutex> lock(m_dynamicMutex);
        auto it_dynamic = m_handlers_dynamic.find(Msg::id);
        if (it_dynamic == m_handlers_dynamic.end()) {
            return HJResult<typename Msg::Ret>::error(HJErrNotFind);
        }

        auto* base = it_dynamic->second.get();
        if (base->tag != &HJMsgTag<Msg>::value) {
            return HJResult<typename Msg::Ret>::error(HJErrInvalidParams);
        }

        auto* handler = static_cast<DynamicHandler<Msg>*>(base);
        return handler->invoke(std::forward<CallArgs>(args)...);
    }

    template <typename Msg, typename... CallArgs>
    HJResult<typename Msg::Ret> query(Msg, CallArgs&&... args) const {
        return query<Msg>(std::forward<CallArgs>(args)...);
    }

private:
    struct HandlerBase {
        explicit HandlerBase(const void* i_tag) : tag(i_tag) {}
        virtual ~HandlerBase() = default;
        const void* tag;
    };

    template <typename Msg>
    struct StaticHandler final : HandlerBase {
        using Ret = typename Msg::Ret;
        using Fn = typename HJFunctionFromList<Ret, typename Msg::Args>::type;

        template <typename Func>
        explicit StaticHandler(Func&& func)
            : HandlerBase(&HJMsgTag<Msg>::value), fn(std::forward<Func>(func)) {}

        template <typename... CallArgs>
        HJResult<Ret> invoke(CallArgs&&... args) {
            return HJResult<Ret>::ok(fn(std::forward<CallArgs>(args)...));
        }

        Fn fn;
    };

    template <typename Msg>
    struct DynamicHandler final : HandlerBase {
        using Ret = typename Msg::Ret;
        using Fn = typename HJFunctionFromList<HJResult<Ret>, typename Msg::Args>::type;

        template <typename Func>
        explicit DynamicHandler(Func&& func)
            : HandlerBase(&HJMsgTag<Msg>::value), fn(std::forward<Func>(func)) {}

        template <typename... CallArgs>
        HJResult<Ret> invoke(CallArgs&&... args) {
            return fn(std::forward<CallArgs>(args)...);
        }

        Fn fn;
    };

    std::unordered_map<uint32_t, std::shared_ptr<HandlerBase>> m_handlers_static;
    std::unordered_map<uint32_t, std::shared_ptr<HandlerBase>> m_handlers_dynamic;
    mutable std::shared_mutex m_dynamicMutex;
};

// Query bus helper macros (optional).
#define HJ_QUERY_REGISTER(BUS, MSG, HANDLER)        (BUS).registerHandler<MSG>(HANDLER)
#define HJ_QUERY_REGISTER_DYNAMIC(BUS, MSG, HANDLER) (BUS).registerDynamicHandler<MSG>(HANDLER)
#define HJ_QUERY_UNREGISTER_DYNAMIC(BUS, MSG)       (BUS).unRegisterDynamicHandler<MSG>()
#define HJ_QUERY_CALL(BUS, MSG, ...)                (BUS).call<MSG>(__VA_ARGS__)

class HJEventBus {
public:
    using Ptr = std::shared_ptr<HJEventBus>;
    using Wtr = std::weak_ptr<HJEventBus>;

    HJEventBus() = default;
    HJEventBus(const HJEventBus&) = delete;
    HJEventBus& operator=(const HJEventBus&) = delete;

    template <typename Msg, typename Func>
    HJRet registerHandler(Func&& func) {
        using Ret = typename Msg::Ret;
        using Args = typename Msg::Args;
        static_assert(std::is_void_v<Ret>, "EventBus requires void return");
        static_assert(HJIsCallableWith<Ret, Func, Args>::value, "handler signature mismatch");

        if (m_handlers_static.find(Msg::id) != m_handlers_static.end()) {
            return HJErrAlreadyExist;
        }

        auto handler = std::make_shared<StaticHandler<Msg>>(std::forward<Func>(func));
        m_handlers_static[Msg::id] = std::move(handler);
        return HJ_OK;
    }

    template <typename Msg, typename Func>
    HJRet registerHandler(Msg, Func&& func) {
        return registerHandler<Msg>(std::forward<Func>(func));
    }

    template <typename Msg, typename Func>
    std::shared_ptr<void> subscribe(Func&& func) {
        using Ret = typename Msg::Ret;
        using Args = typename Msg::Args;
        static_assert(std::is_void_v<Ret>, "EventBus requires void return");
        static_assert(HJIsCallableWith<HJRet, Func, Args>::value, "listener signature mismatch");

        auto listener = std::make_shared<Listener<Msg>>(std::forward<Func>(func));
        {
            std::unique_lock<std::shared_mutex> lock(m_listenerMutex);
            m_listeners[Msg::id].push_back(listener);
        }
        return listener;
    }

    template <typename Msg, typename Func>
    std::shared_ptr<void> subscribe(Msg, Func&& func) {
        return subscribe<Msg>(std::forward<Func>(func));
    }

    template <typename Msg>
    HJRet unSubscribe(std::shared_ptr<void> token) {
        std::unique_lock<std::shared_mutex> lock(m_listenerMutex);
        auto it = m_listeners.find(Msg::id);
        if (it == m_listeners.end()) {
            return HJErrNotExist;
        }
        auto& vec = it->second;
        bool removed = false;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [&](const std::shared_ptr<ListenerBase>& ptr) {
                if (ptr == token) {
                    removed = true;
                    return true;
                }
                return false;
            }), vec.end());
        return removed ? HJ_OK : HJErrNotExist;
    }

    template <typename Msg>
    HJRet unSubscribe(Msg, std::shared_ptr<void> token) {
        return unSubscribe<Msg>(std::move(token));
    }

    template <typename Msg, typename... CallArgs>
    HJRet publish(CallArgs&&... args) {
        static_assert(std::is_void_v<typename Msg::Ret>, "EventBus requires void return");
        static_assert(HJArgsMatch<Msg, CallArgs...>::value, "publish args mismatch");

        auto it_static = m_handlers_static.find(Msg::id);
        if (it_static != m_handlers_static.end()) {
            auto* base = it_static->second.get();
            if (base->tag == &HJMsgTag<Msg>::value) {
                auto* handler = static_cast<StaticHandler<Msg>*>(base);
                handler->invoke(std::forward<CallArgs>(args)...);
            }
        }

        std::vector<std::shared_ptr<ListenerBase>> snapshot;
        {
            std::shared_lock<std::shared_mutex> lock(m_listenerMutex);
            auto it = m_listeners.find(Msg::id);
            if (it != m_listeners.end()) {
                snapshot = it->second;
            }
        }
        std::vector<std::shared_ptr<ListenerBase>> to_remove;
        for (const auto& base : snapshot) {
            if (base && base->tag == &HJMsgTag<Msg>::value) {
                auto* listener = static_cast<Listener<Msg>*>(base.get());
                HJRet ret = listener->invoke(std::forward<CallArgs>(args)...);
                if (ret == HJErrAlreadyDone) {
                    to_remove.push_back(base);
                }
            }
        }
        if (!to_remove.empty()) {
            std::unique_lock<std::shared_mutex> lock(m_listenerMutex);
            auto it = m_listeners.find(Msg::id);
            if (it != m_listeners.end()) {
                auto& vec = it->second;
                vec.erase(std::remove_if(vec.begin(), vec.end(),
                    [&](const std::shared_ptr<ListenerBase>& ptr) {
                        return std::find(to_remove.begin(), to_remove.end(), ptr) != to_remove.end();
                    }), vec.end());
            }
        }

        return HJ_OK;
    }

    template <typename Msg, typename... CallArgs>
    HJRet publish(Msg, CallArgs&&... args) {
        return publish<Msg>(std::forward<CallArgs>(args)...);
    }

    template <typename Msg, typename... CallArgs>
    HJRet report(CallArgs&&... args) const {
        static_assert(std::is_void_v<typename Msg::Ret>, "EventBus requires void return");
        static_assert(HJArgsMatch<Msg, CallArgs...>::value, "info args mismatch");

        auto it_static = m_handlers_static.find(Msg::id);
        if (it_static != m_handlers_static.end()) {
            auto* base = it_static->second.get();
            if (base->tag == &HJMsgTag<Msg>::value) {
                auto* handler = static_cast<StaticHandler<Msg>*>(base);
                handler->invoke(std::forward<CallArgs>(args)...);
            }
        }

        return HJ_OK;
    }

    template <typename Msg, typename... CallArgs>
    HJRet report(Msg, CallArgs&&... args) const {
        return report<Msg>(std::forward<CallArgs>(args)...);
    }

    template <typename Msg, typename... CallArgs>
    HJRet inform(CallArgs&&... args) {
        static_assert(std::is_void_v<typename Msg::Ret>, "EventBus requires void return");
        static_assert(HJArgsMatch<Msg, CallArgs...>::value, "info args mismatch");

        std::vector<std::shared_ptr<ListenerBase>> snapshot;
        {
            std::shared_lock<std::shared_mutex> lock(m_listenerMutex);
            auto it = m_listeners.find(Msg::id);
            if (it != m_listeners.end()) {
                snapshot = it->second;
            }
        }
        std::vector<std::shared_ptr<ListenerBase>> to_remove;
        for (const auto& base : snapshot) {
            if (base && base->tag == &HJMsgTag<Msg>::value) {
                auto* listener = static_cast<Listener<Msg>*>(base.get());
                HJRet ret = listener->invoke(std::forward<CallArgs>(args)...);
                if (ret == HJErrAlreadyDone) {
                    to_remove.push_back(base);
                }
            }
        }
        if (!to_remove.empty()) {
            std::unique_lock<std::shared_mutex> lock(m_listenerMutex);
            auto it = m_listeners.find(Msg::id);
            if (it != m_listeners.end()) {
                auto& vec = it->second;
                vec.erase(std::remove_if(vec.begin(), vec.end(),
                    [&](const std::shared_ptr<ListenerBase>& ptr) {
                        return std::find(to_remove.begin(), to_remove.end(), ptr) != to_remove.end();
                    }), vec.end());
            }
        }

        return HJ_OK;
    }

    template <typename Msg, typename... CallArgs>
    HJRet inform(Msg, CallArgs&&... args) {
        return inform<Msg>(std::forward<CallArgs>(args)...);
    }

private:
    struct HandlerBase {
        explicit HandlerBase(const void* i_tag) : tag(i_tag) {}
        virtual ~HandlerBase() = default;
        const void* tag;
    };

    struct ListenerBase {
        explicit ListenerBase(const void* i_tag) : tag(i_tag) {}
        virtual ~ListenerBase() = default;
        const void* tag;
    };

    template <typename Msg>
    struct StaticHandler final : HandlerBase {
        using Ret = typename Msg::Ret;
        using Fn = typename HJFunctionFromList<Ret, typename Msg::Args>::type;

        template <typename Func>
        explicit StaticHandler(Func&& func)
            : HandlerBase(&HJMsgTag<Msg>::value), fn(std::forward<Func>(func)) {}

        template <typename... CallArgs>
        void invoke(CallArgs&&... args) {
            fn(std::forward<CallArgs>(args)...);
        }

        Fn fn;
    };

    template <typename Msg>
    struct Listener final : ListenerBase {
        using Ret = typename Msg::Ret;
        using Fn = typename HJFunctionFromList<HJRet, typename Msg::Args>::type;

        template <typename Func>
        explicit Listener(Func&& func)
            : ListenerBase(&HJMsgTag<Msg>::value), fn(std::forward<Func>(func)) {}

        template <typename... CallArgs>
        HJRet invoke(CallArgs&&... args) {
            return fn(std::forward<CallArgs>(args)...);
        }

        Fn fn;
    };

    std::unordered_map<uint32_t, std::shared_ptr<HandlerBase>> m_handlers_static;
    std::unordered_map<uint32_t, std::vector<std::shared_ptr<ListenerBase>>> m_listeners;
    mutable std::shared_mutex m_listenerMutex;
};

// Event bus helper macros (optional).
#define HJ_EVENT_REGISTER(BUS, MSG, HANDLER)        (BUS).registerHandler<MSG>(HANDLER)
#define HJ_EVENT_SUBSCRIBE(BUS, MSG, HANDLER)       (BUS).subscribe<MSG>(HANDLER)
#define HJ_EVENT_UNSUBSCRIBE(BUS, MSG, TOKEN)       (BUS).unSubscribe<MSG>(TOKEN)
#define HJ_EVENT_PUBLISH(BUS, MSG, ...)             (BUS).publish<MSG>(__VA_ARGS__)

NS_HJ_END
