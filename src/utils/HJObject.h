//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJCommons.h"
#include "HJError.h"

NS_HJ_BEGIN
//***********************************************************************************//
template <typename T>
class enable_shared_from_this : public std::enable_shared_from_this<T> {
public:
    constexpr enable_shared_from_this() noexcept = default;

    std::weak_ptr<T> weak_from_this() noexcept {
        return weak_from_this_<T>(this);
    }

    std::weak_ptr<T const> weak_from_this() const noexcept {
        return weak_from_this_<T>(this);
    }
    
    template <typename WT>
    std::weak_ptr<WT> weak_ptr() {
        return std::static_pointer_cast<WT>(weak_from_this());
    }
private:
    // Uses SFINAE to detect and call
    // std::enable_shared_from_this<T>::weak_from_this() if available. Falls
    // back to std::enable_shared_from_this<T>::shared_from_this() otherwise.
    template <typename U>
    auto weak_from_this_(std::enable_shared_from_this<U>* base_ptr) noexcept
      -> decltype(base_ptr->weak_from_this()) {
          return base_ptr->weak_from_this();
    }

    template <typename U>
    auto weak_from_this_(std::enable_shared_from_this<U> const* base_ptr)
      const noexcept -> decltype(base_ptr->weak_from_this()) {
          return base_ptr->weak_from_this();
    }

    template <typename U>
    std::weak_ptr<U> weak_from_this_(...) noexcept {
        try {
            return this->shared_from_this();
        } catch (std::bad_weak_ptr const&) {
          // C++17 requires that weak_from_this() on an object not owned by a
          // shared_ptr returns an empty weak_ptr. Sadly, in C++14,
          // shared_from_this() on such an object is undefined behavior, and there
          // is nothing we can do to detect and handle the situation in a portable
          // manner. But in case a compiler is nice enough to implement C++17
          // semantics of shared_from_this() and throws a bad_weak_ptr, we catch it
          // and return an empty weak_ptr.
            return std::weak_ptr<U>{};
        }
    }

    template <typename U>
    std::weak_ptr<U const> weak_from_this_(...) const noexcept {
        try {
            return this->shared_from_this();
        } catch (std::bad_weak_ptr const&) {
            return std::weak_ptr<U const>{};
        }
    }
};

//***********************************************************************************//
class HJObject : public enable_shared_from_this<HJObject>
{
public:
    HJ_DECLARE_PUWTR(HJObject)
    HJObject() = default;
    HJObject(const std::string& name, size_t identify = -1)
        : m_name(name)
        , m_id(identify)
    { }
    virtual ~HJObject() { }
    
    //attributes
    virtual const std::string& getName() const {
        return m_name;
    }
    virtual void setName(const std::string& name) {
        m_name = name;
    }
    virtual const size_t getID() const {
        return m_id;
    }
    virtual void setID(const size_t ID) {
        m_id = ID;
    }
    template<typename T>
    std::shared_ptr<T> getSharedPtr() {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }
    template <typename T>
    std::shared_ptr<T> sharedFrom(T* ptr) {
        return std::dynamic_pointer_cast<T>(ptr->shared_from_this());
    }
public:
    template<typename T, typename ...ArgsType>
    static std::shared_ptr<T> creates(ArgsType &&...args) {
        return std::make_shared<T>(std::forward<ArgsType>(args)...);
    }
    template<typename T, typename ...ArgsType>
    static std::shared_ptr<T> createsn(ArgsType &&...args) {
        std::shared_ptr<T> ret(new T(std::forward<ArgsType>(args)...));
        return ret;
    }
    template<typename T, typename ...ArgsType>
    static std::unique_ptr<T> createu(ArgsType &&...args) {
        return std::make_unique<T>(std::forward<ArgsType>(args)...);
    }
    template<typename T, typename ...ArgsType>
    static std::unique_ptr<T> createun(ArgsType &&...args) {
        std::unique_ptr<T> ret(new T(std::forward<ArgsType>(args)...));
        return ret;
    }

    static const size_t getGlobalID();
    static const std::string getGlobalName(const std::string prefix = "");
protected:
    std::string             m_name = "";
    size_t                  m_id = 0;
};
using HJObjectMap = std::unordered_map<std::string, HJObject::Ptr>;
#define HJMakeGlobalID()            HJ::HJObject::getGlobalID()
#define HJMakeGlobalName(prefix)    HJ::HJObject::getGlobalName(prefix)

template<typename T, typename ...ArgsType>
static inline std::shared_ptr<T>HJCreates(ArgsType&& ...args) {
	return HJObject::creates<T>(std::forward<ArgsType>(args)...);
}

template<typename T, typename ...ArgsType>
static inline std::shared_ptr<T>HJCreatesn(ArgsType&& ...args) {
    return HJObject::createsn<T>(std::forward<ArgsType>(args)...);
}

template<typename T, typename ...ArgsType>
static inline std::unique_ptr<T>HJCreateu(ArgsType&& ...args) {
    return HJObject::createu<T>(std::forward<ArgsType>(args)...);
}

template<typename T, typename ...ArgsType>
static inline std::unique_ptr<T>HJCreateun(ArgsType&& ...args) {
    return HJObject::createun<T>(std::forward<ArgsType>(args)...);
}

//***********************************************************************************//
class HJExObject : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJExObject>;
    HJExObject() = default;
    HJExObject(const std::string& name, size_t identify = -1)
        : HJObject(name, identify)
    {
    }
    virtual ~HJExObject() override {}

    //
    virtual int init() {
        return HJ_OK;
    }
    virtual int release() {
        HJ_AUTO_LOCK(m_mutex)
            m_status = HJSTATUS_Released;
        return HJ_OK;
    }
    virtual int done() { return HJ_OK; }
private:
    int                     m_status = HJSTATUS_NONE;
    std::recursive_mutex    m_mutex;
};

//***********************************************************************************//
template<typename T>
class HJObjectHolder
{
public:
    HJObjectHolder() = default;
    HJObjectHolder(const T& v) {
        m_obj = v;
    }
    void operator=(const T& v){
        HJ_AUTO_LOCK(m_mutex)
        m_obj = v;
    }
    const T get() {
        HJ_AUTO_LOCK(m_mutex)
        return m_obj;
    }
private:
    T                       m_obj;
    std::recursive_mutex    m_mutex;
};

NS_HJ_END
