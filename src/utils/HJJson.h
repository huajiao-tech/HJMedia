//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

#if defined(HJ_HAVE_YYJSON)
struct yyjson_doc;
struct yyjson_val;
struct yyjson_mut_val;
struct yyjson_mut_doc;
#endif //
#if defined(HJ_HAVE_SIMDJSON)
namespace simdjson {
    namespace ondemand {
        class parser;
        class document;
    }
}
#endif

#include <any>
#include <tuple>
#include <type_traits>
#include <utility>
#include <map>

NS_HJ_BEGIN

class HJYJsonObject;
class HJInterpreter;

template<typename T> struct is_vector : std::false_type {};
template<typename T, typename A> struct is_vector<std::vector<T, A>> : std::true_type {};

template<typename T> struct is_pair : std::false_type {};
template<typename T1, typename T2> struct is_pair<std::pair<T1, T2>> : std::true_type {};

template<typename T> struct is_map : std::false_type {};
template<typename K, typename V, typename C, typename A> struct is_map<std::map<K, V, C, A>> : std::true_type {};

namespace detail {

template <typename T>
struct is_std_tuple : std::false_type {};
template <typename... Ts>
struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};

template <typename T, typename = void>
struct is_reflectable : std::false_type {};
template <typename T>
struct is_reflectable<T, std::void_t<decltype(T::kJsonFields)>>
    : is_std_tuple<typename std::decay<decltype(T::kJsonFields)>::type> {};

template <typename Class, typename T>
struct HJJsonField {
    const char* name;
    T Class::*member;
};

template <typename Class, typename T>
constexpr HJJsonField<Class, T> makeField(const char* name, T Class::*member) {
    return HJJsonField<Class, T>{name, member};
}

template <typename Tuple, typename Func, size_t... I>
constexpr void tupleForEachImpl(Tuple&& tuple, Func&& func, std::index_sequence<I...>) {
    (func(std::get<I>(tuple)), ...);
}

template <typename Tuple, typename Func>
constexpr void tupleForEach(Tuple&& tuple, Func&& func) {
    constexpr size_t kSize = std::tuple_size<typename std::decay<Tuple>::type>::value;
    tupleForEachImpl(std::forward<Tuple>(tuple), std::forward<Func>(func), std::make_index_sequence<kSize>{});
}

template <typename Class, typename T>
void deserialField(Class* instance, const HJYJsonObject* json, const HJJsonField<Class, T>& field);

template <typename Class, typename T>
void serialField(const Class* instance, HJYJsonObject* json, const HJJsonField<Class, T>& field);

} // namespace detail

struct HJJsonReflector {
    template <typename Class>
    static int deserial(Class* instance, const std::shared_ptr<HJYJsonObject>& obj);

    template <typename Class>
    static int serial(const Class* instance, const std::shared_ptr<HJYJsonObject>& obj);
};

#define HJ_JSON_REFLECT_BEGIN(ClassName) \
public: \
    ClassName() = default; \
    ClassName(const HJ::HJYJsonObject::Ptr& obj) : HJ::HJInterpreter(obj) {} \
    ClassName(const std::string& info) : HJ::HJInterpreter(info) { \
        deserialInfo(); \
    } \
    using HJJsonSelf = ClassName; \
    static constexpr auto kJsonFields = std::tuple{

#define HJ_JSON_REFLECT_MEMBER(MemberName) \
            HJ::detail::makeField<HJJsonSelf>(#MemberName, &HJJsonSelf::MemberName),

#define HJ_JSON_REFLECT_END(ClassName) \
    }; \
    int deserialInfo(const HJ::HJYJsonObject::Ptr& obj = nullptr) override { \
        return HJ::HJJsonReflector::deserial(this, obj ? obj : m_obj); \
    } \
    int serialInfo(const HJ::HJYJsonObject::Ptr& obj = nullptr) override { \
        return HJ::HJJsonReflector::serial(this, obj ? obj : m_obj); \
    }

#define HJ_JSON_REFLECT_IMPLEMENT(ClassName) \
    static_assert(HJ::detail::is_reflectable<ClassName>::value, \
        "HJ_JSON_REFLECT_IMPLEMENT requires kJsonFields to be a std::tuple");

//***********************************************************************************//
template <typename T>
class HJValue : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJValue<T>>;
    HJValue() = default;
    HJValue(const T& val)
        : m_value(val)
    { }
    HJValue(const std::string& key, const T& val)
        : HJObject(key)
        , m_value(val)
    { }
    void set(const T& val) {
        m_value = val;
    }
    T& get() {
        return m_value;
    }
    void operator=(const T& val) {
        m_value = val;
    }
    T& operator()() {
        return m_value;
    }
protected:
    T           m_value;
};
using HJVBool = HJValue<bool>;
using HJVInt = HJValue<int>;
using HJVInt64 = HJValue<int64_t>;
using HJVUInt64 = HJValue<uint64_t>;
using HJVReal = HJValue<double>;
using HJVString = HJValue<std::string>;
using HJVPtr = HJValue<const uint8_t *>;
using HJVCPtr = HJValue<const char *>;
using HJVStrArray = HJValue<std::vector<std::string>>;
using HJVIntArray = HJValue<std::vector<int>>;
using HJVInt64Array = HJValue<std::vector<int64_t>>;
using HJVBoolArray = HJValue<std::vector<bool>>;
using HJVRealArray = HJValue<std::vector<double>>;
//***********************************************************************************//
#if defined(HJ_HAVE_YYJSON)
class HJYJsonObject : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJYJsonObject>;
    HJYJsonObject();
    HJYJsonObject(HJYJsonObject&& other) noexcept;
    HJYJsonObject& operator=(HJYJsonObject&& other) noexcept;
    HJYJsonObject(const std::string& key, yyjson_val* val);
    HJYJsonObject(const std::string& key, yyjson_mut_val* val, yyjson_mut_doc* doc);
    HJYJsonObject(const std::string& key, HJYJsonObject::Ptr obj);
    virtual ~HJYJsonObject();
    
    void setRVal(yyjson_val* val) {
        m_rval = val;
    }
    yyjson_val* getRVal() const {
        return m_rval;
    }
    void setWVal(yyjson_mut_val* val) {
        m_wval = val;
    }
    yyjson_mut_val* getWVal() {
        return m_wval;
    }
    void setKey(const std::string& key) {
        m_key = key;
    }
    const std::string& getKey() const {
        return m_key;
    }
    void setMDoc(yyjson_mut_doc* doc) {
        m_mdoc = doc;
    }
    yyjson_mut_doc* getMDoc() {
        return m_mdoc;
    }
    //self
    bool isNull() const;
    bool isObj() const;
    bool isArr() const;
    bool isStr() const;
    bool isInt64() const;
    bool isUInt64() const;
    bool isReal() const;
    bool isBool() const;
    int getType() const;
    //child
    bool hasObj(const std::string& key) const;
    bool isObj(const std::string& key) const;
    bool isArr(const std::string& key) const;
    HJYJsonObject::Ptr getObj(const std::string& key) const;
    int addObj(HJYJsonObject::Ptr obj);
    //self
    bool getBool() const;
    int getInt() const;
    int64_t getInt64() const;
    uint64_t getUInt64() const;
    double getReal() const;
    std::string getString() const;
    uint8_t* getRaw() const;
    std::vector<std::string> getKeys() const;
    //read child
    bool getBool(const std::string& key) const;
    int getInt(const std::string& key) const;
    int64_t getInt64(const std::string& key) const;
    uint64_t getUInt64(const std::string& key) const;
    double getReal(const std::string& key) const;
    std::string getString(const std::string& key) const;
    uint8_t* getRaw(const std::string& key) const;
    std::deque<HJYJsonObject::Ptr> getArr(const std::string& key) const;
    
    int getMember(const std::string& key, bool& value) const;
    int getMember(const std::string& key, int& value) const;
    int getMember(const std::string& key, int64_t& value) const;
    int getMember(const std::string& key, uint64_t& value) const;
    int getMember(const std::string& key, double& value) const;
    int getMember(const std::string& key, std::string& value) const;
    int getMember(const std::string& key, uint8_t*& value, size_t& len) const;
    int getMember(const std::string& key, std::vector<HJYJsonObject::Ptr>& objs) const;
    int forEach(const std::string& key, const std::function<int(const HJYJsonObject::Ptr &)>& cb) const;
    int forEachAnonymous(const std::function<int(const HJYJsonObject::Ptr&)>& cb) const;
//    template <typename T>
//    int getMember(const std::string& key, T& value);
//    template <typename T>
//    int getMember(const std::string& key, std::deque<T>& value);
    //write
    HJYJsonObject::Ptr setMember(const std::string& key);
    int setMember(const std::string& key, const bool value);
    int setMember(const std::string& key, const int value);
    int setMember(const std::string& key, const int64_t value);
    int setMember(const std::string& key, const uint64_t value);
    int setMember(const std::string& key, const double value);
    int setMember(const std::string& key, const std::string& value);
    int setMember(const std::string& key, const char* value);
    int setMember(const std::string& key, const uint8_t* value, const size_t len);
    int setMember(const std::string& key, const std::vector<HJYJsonObject::Ptr>& value);
    //add vector
    int setMember(const std::string& key, const std::vector<std::string>& value);
    int setMember(const std::string& key, const std::vector<int>& value);
    int setMember(const std::string& key, const std::vector<int64_t>& value);
    int setMember(const std::string& key, const std::vector<bool>& value);
    int setMember(const std::string& key, const std::vector<double>& value);
protected:
    //proxy []
    class ConstJsonProxy {
    public:
        ConstJsonProxy(const HJYJsonObject* obj, const std::string& key) : m_obj(obj), m_key(key) {}
        
        operator bool() const {
            bool value = false;
            m_obj->getMember(m_key, value);
            return value;
        }
        
        operator int() const {
            int value = 0;
            m_obj->getMember(m_key, value);
            return value;
        }
        
        operator int64_t() const {
            int64_t value = 0;
            m_obj->getMember(m_key, value);
            return value;
        }
        
        operator uint64_t() const {
            uint64_t value = 0;
            m_obj->getMember(m_key, value);
            return value;
        }
        
        operator double() const {
            double value = 0.0;
            m_obj->getMember(m_key, value);
            return value;
        }
        
        operator std::string() const {
            std::string value;
            m_obj->getMember(m_key, value);
            return value;
        }
    protected:
        const HJYJsonObject* m_obj = nullptr;
        std::string m_key{""};
    };

    class JsonProxy : public ConstJsonProxy {
    public:
        JsonProxy(HJYJsonObject* obj, const std::string& key) : ConstJsonProxy(obj, key) {}
        

        JsonProxy& operator=(bool value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, value);
            return *this;
        }
        
        JsonProxy& operator=(int value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, value);
            return *this;
        }
        
        JsonProxy& operator=(int64_t value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, value);
            return *this;
        }
        
        JsonProxy& operator=(uint64_t value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, value);
            return *this;
        }
        
        JsonProxy& operator=(double value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, value);
            return *this;
        }
        
        JsonProxy& operator=(const std::string& value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, value);
            return *this;
        }
        
        JsonProxy& operator=(const char* value) {
            const_cast<HJYJsonObject*>(m_obj)->setMember(m_key, std::string(value));
            return *this;
        }
    };
public:
    JsonProxy operator[](const std::string& key) {
        return JsonProxy(this, key);
    }
    
    ConstJsonProxy operator[](const std::string& key) const {
        return ConstJsonProxy(this, key);
    }
protected:
    std::string         m_key  = "";
    yyjson_val*         m_rval = NULL;
    yyjson_mut_val*     m_wval = NULL;
    yyjson_mut_doc*     m_mdoc = NULL;
    HJObjectMap        m_subObjs;
    std::vector<HJYJsonObject::Ptr> m_childList;
};
//using HJYJsonObjectDeque = std::deque<HJYJsonObject::Ptr>;
using HJYJsonObjectVector = std::vector<HJYJsonObject::Ptr>;
using HJVObjsVector = HJValue<std::vector<HJYJsonObject::Ptr>>;

class HJYJsonDocument : public HJYJsonObject
{
public:
    using Ptr = std::shared_ptr<HJYJsonDocument>;
    HJYJsonDocument();
    virtual ~HJYJsonDocument();
    
    /**
     * serial *
     */
    int init();
    /**
     * deserial *
     */
    int init(const std::string& info);
    int initWithUrl(const std::string& url);
    
    yyjson_mut_doc* getWDoc() {
        return m_wdoc;
    }
    std::string getSerialInfo() const;
    int writeFile(const std::string &path);
public:
    static HJYJsonDocument::Ptr create() {
        auto doc = std::make_shared<HJYJsonDocument>();
        if (doc->init() == HJ_OK) {
            return doc;
        }
        return nullptr;
    }

    static HJYJsonDocument::Ptr createWithInfo(const std::string& info) {
        auto doc = std::make_shared<HJYJsonDocument>();
        if (doc->init(info) == HJ_OK) {
            return doc;
        }
        return nullptr;
    }
    
    static HJYJsonDocument::Ptr createWithUrl(const std::string& url) {
        auto doc = std::make_shared<HJYJsonDocument>();
        if (doc->initWithUrl(url) == HJ_OK) {
            return doc;
        }
        return nullptr;
    }
private:
    yyjson_doc*         m_rdoc = NULL;
    yyjson_val*         m_rroot = NULL;
    yyjson_mut_doc*     m_wdoc = NULL;
    yyjson_mut_val*     m_wroot = NULL;
};
#endif
//***********************************************************************************//
/**
 * ondemand::parser
 * ondemand::array
 * ondemand::document
 * ondemand::field
 * ondemand::object
 */
#if defined(HJ_HAVE_SIMDJSON)
class HJSJsonDocument
{
public:
    using Ptr = std::shared_ptr<HJSJsonDocument>;
    
    int init(const std::string& info);
    int initWithUrl(const std::string& url);
private:
    std::shared_ptr<simdjson::ondemand::parser>     m_parser;
    std::shared_ptr<simdjson::ondemand::document>   m_doc;
};
#endif

//***********************************************************************************//
#if defined(HJ_HAVE_YYJSON)
class HJInterpreter
{
public:
    using Ptr = std::shared_ptr<HJInterpreter>;
    HJInterpreter() = default;
    HJInterpreter(const std::string& info);
    HJInterpreter(const HJYJsonObject::Ptr& obj)
        : m_obj(obj) {}
    virtual ~HJInterpreter() {}
    
    virtual int init() { return  HJ_OK; }
    virtual int init(const std::string& info) { return  HJ_OK; }
    virtual int initWithUrl(const std::string& url) { return  HJ_OK; }
    
    virtual int deserialInfo(const HJYJsonObject::Ptr& obj = nullptr);
    virtual int serialInfo(const HJYJsonObject::Ptr& obj = nullptr);

    virtual std::string getSerialInfo();
    
    template <typename T>
    int getSubInfo(const std::string& key, std::shared_ptr<T>& subInfo) {
        auto subObj = m_obj->getObj(key);
        if(!subObj) {
            return HJErrJSONValue;
        }
        subInfo = std::make_shared<T>(subObj);
        subInfo->deserialInfo();
        return HJ_OK;
    }
    template <typename T>
    int setSubInfo(const std::string& key, std::shared_ptr<T>& subInfo) {
        if(!subInfo || subInfo->getJObj()) {
            return HJErrAlreadyExist;
        }
        auto subObj = std::make_shared<HJYJsonObject>(key, m_obj);
        if(!subObj) {
            return HJErrJSONValue;
        }
        subInfo->setJObj(subObj);
        //subInfo->serialInfo();
        m_obj->addObj(subObj);
        
        return HJ_OK;
    }
    
    template <typename T>
    int getSubArrInfo(const std::string& key, std::vector<std::shared_ptr<T>>& subInfos) {
        if(!m_obj->isArr(key)) {
            return HJErrJSONValue; //{}
        }
        m_obj->forEach(key, [&](const HJYJsonObject::Ptr& subObj) -> int {
            auto subInfo = std::make_shared<T>(subObj);
            subInfo->deserialInfo();
            subInfos.push_back(subInfo);
            return HJ_OK;
        });
        return HJ_OK;
    }
    
    template <typename T>
    int setSubArrInfo(const std::string& key, std::vector<std::shared_ptr<T>>& subInfos) {
        std::vector<HJYJsonObject::Ptr> objs;
        for (auto subInfo : subInfos) {
            auto subObj = std::make_shared<HJYJsonObject>(key, m_obj);
            if(!subObj) {
                return HJErrJSONValue;
            }
            subInfo->setJObj(subObj);
            //subInfo->serialInfo();
            
            objs.push_back(subObj);
        }
        if (objs.size() > 0) {
            m_obj->setMember(key, objs);
        }
        return HJ_OK;
    }
    
    void setJObj(const HJYJsonObject::Ptr obj) {
        m_obj = obj;
    }
    const HJYJsonObject::Ptr& getJObj() const {
        return m_obj;
    }
protected:
    mutable HJYJsonObject::Ptr     m_obj = nullptr;
};
#endif //#if defined(HJ_HAVE_YYJSON)

//#define HJJsonGetValue(variable)   m_obj->getMember(HJ_VAR_NAME(variable), variable());
//#define HJJsonSetValue(variable)   m_obj->setMember(HJ_VAR_NAME(variable), variable());
#define HJJsonObjGetValue(obj,variable)   (obj)->getMember(HJ_VAR_NAME(variable), variable)
#define HJJsonObjSetValue(obj,variable)   (obj)->setMember(HJ_VAR_NAME(variable), variable)
#define HJJsonGetValue(variable)   HJJsonObjGetValue(obj?obj:m_obj,variable)
#define HJJsonSetValue(variable)   HJJsonObjSetValue(obj?obj:m_obj,variable)

#define HJJsonGetArray(variable,type)  getSubArrInfo<type>(HJ_VAR_NAME(variable), variable)
#define HJJsonSetArray(variable,type)  setSubArrInfo<type>(HJ_VAR_NAME(variable), variable)

#define HJJsonGetObj(variable,type) getSubInfo<type>(HJ_VAR_NAME(variable), variable)
#define HJJsonSetObj(variable,type) setSubInfo<type>(HJ_VAR_NAME(variable), variable)

#if defined(HJ_HAVE_YYJSON)
// Reflection helpers - must be after HJYJsonObject full declaration
namespace detail {

template <typename Class, typename T>
void deserialField(Class* instance, const HJYJsonObject* json, const HJJsonField<Class, T>& field) {
    if (!instance || !json) {
        return;
    }
    const char* name = field.name;
    auto member = field.member;

    if constexpr (std::is_same_v<T, int>) {
        instance->*member = json->getInt(name);
    } else if constexpr (std::is_same_v<T, short>) {
        instance->*member = static_cast<short>(json->getInt(name));
    } else if constexpr (std::is_same_v<T, int64_t>) {
        instance->*member = json->getInt64(name);
    } else if constexpr (std::is_same_v<T, float>) {
        instance->*member = static_cast<float>(json->getReal(name));
    } else if constexpr (std::is_same_v<T, double>) {
        instance->*member = json->getReal(name);
    } else if constexpr (std::is_same_v<T, bool>) {
        instance->*member = json->getBool(name);
    } else if constexpr (std::is_same_v<T, std::string>) {
        instance->*member = json->getString(name);
    } else if constexpr (is_vector<T>::value) {
        using ValType = typename T::value_type;
        if constexpr (std::is_same_v<ValType, int>) {
            std::deque<HJYJsonObject::Ptr> arr = json->getArr(name);
            (instance->*member).clear();
            for (auto& item : arr) {
                (instance->*member).push_back(item->getInt());
            }
        } else if constexpr (std::is_base_of_v<HJInterpreter, ValType>) {
            std::deque<HJYJsonObject::Ptr> arr = json->getArr(name);
            (instance->*member).clear();
            for (auto& item : arr) {
                ValType elem;
                elem.deserialInfo(item);
                (instance->*member).push_back(elem);
            }
        } else if constexpr (is_pair<ValType>::value) {
            using FirstType = typename ValType::first_type;
            using SecondType = typename ValType::second_type;
            if constexpr (std::is_same_v<FirstType, std::string> && std::is_same_v<SecondType, int>) {
                std::deque<HJYJsonObject::Ptr> arr = json->getArr(name);
                (instance->*member).clear();
                for (auto& item : arr) {
                    (instance->*member).push_back({item->getString("name"), item->getInt("max")});
                }
            }
        }
    } else if constexpr (is_map<T>::value) {
        using KeyType = typename T::key_type;
        using ValType = typename T::mapped_type;
        if constexpr (std::is_same_v<KeyType, std::string>) {
            auto target_obj = json->getObj(name);
            if (target_obj && target_obj->isObj()) {
                (instance->*member).clear();
                std::vector<std::string> keys = target_obj->getKeys();
                for (const auto& k : keys) {
                    if constexpr (std::is_same_v<ValType, bool>) {
                        (instance->*member)[k] = target_obj->getBool(k);
                    } else if constexpr (std::is_same_v<ValType, int>) {
                        (instance->*member)[k] = target_obj->getInt(k);
                    } else if constexpr (std::is_same_v<ValType, std::string>) {
                        (instance->*member)[k] = target_obj->getString(k);
                    }
                }
            }
        }
    } else if constexpr (std::is_base_of_v<HJInterpreter, T>) {
        auto subObj = json->getObj(name);
        if (subObj) {
            (instance->*member).deserialInfo(subObj);
        }
    }
}

template <typename Class, typename T>
void serialField(const Class* instance, HJYJsonObject* json, const HJJsonField<Class, T>& field) {
    if (!instance || !json) {
        return;
    }
    const char* name = field.name;
    auto member = field.member;

    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, short>) {
        json->setMember(name, static_cast<int>(instance->*member));
    } else if constexpr (std::is_same_v<T, int64_t>) {
        json->setMember(name, static_cast<int64_t>(instance->*member));
    } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
        json->setMember(name, static_cast<double>(instance->*member));
    } else if constexpr (std::is_same_v<T, bool>) {
        json->setMember(name, static_cast<bool>(instance->*member));
    } else if constexpr (std::is_same_v<T, std::string>) {
        json->setMember(name, static_cast<std::string>(instance->*member));
    } else if constexpr (is_vector<T>::value) {
        using ValType = typename T::value_type;
        if constexpr (std::is_same_v<ValType, int>) {
            json->setMember(name, (instance->*member));
        } else if constexpr (std::is_base_of_v<HJInterpreter, ValType>) {
            std::vector<HJYJsonObject::Ptr> objs;
            auto parent = const_cast<HJYJsonObject*>(json);
            if (parent->getMDoc()) {
                for (const auto& item : (instance->*member)) {
                    auto subObj = std::make_shared<HJYJsonObject>("", (yyjson_mut_val*)nullptr, parent->getMDoc());
                    const_cast<ValType&>(item).serialInfo(subObj);
                    objs.push_back(subObj);
                }
                if (!objs.empty()) {
                    parent->setMember(name, objs);
                }
            }
        } else if constexpr (is_pair<ValType>::value) {
            using FirstType = typename ValType::first_type;
            using SecondType = typename ValType::second_type;
            if constexpr (std::is_same_v<FirstType, std::string> && std::is_same_v<SecondType, int>) {
                std::vector<HJYJsonObject::Ptr> objs;
                auto parent = const_cast<HJYJsonObject*>(json);
                if (parent->getMDoc()) {
                    for (const auto& item : (instance->*member)) {
                        auto subObj = std::make_shared<HJYJsonObject>("", (yyjson_mut_val*)nullptr, parent->getMDoc());
                        subObj->setMember("name", item.first);
                        subObj->setMember("max", item.second);
                        objs.push_back(subObj);
                    }
                    if (!objs.empty()) {
                        parent->setMember(name, objs);
                    }
                }
            }
        }
    } else if constexpr (is_map<T>::value) {
        using KeyType = typename T::key_type;
        using ValType = typename T::mapped_type;
        if constexpr (std::is_same_v<KeyType, std::string>) {
            auto parent = const_cast<HJYJsonObject*>(json);
            if (parent->getMDoc()) {
                auto subObj = parent->setMember(name);
                if (subObj) {
                    for (const auto& kv : (instance->*member)) {
                        if constexpr (std::is_same_v<ValType, bool>) {
                            subObj->setMember(kv.first, static_cast<bool>(kv.second));
                        } else if constexpr (std::is_same_v<ValType, int>) {
                            subObj->setMember(kv.first, static_cast<int>(kv.second));
                        } else if constexpr (std::is_same_v<ValType, std::string>) {
                            subObj->setMember(kv.first, static_cast<std::string>(kv.second));
                        }
                    }
                }
            }
        }
    } else if constexpr (std::is_base_of_v<HJInterpreter, T>) {
        auto parent = const_cast<HJYJsonObject*>(json);
        if (parent->getMDoc()) {
            auto subObj = std::make_shared<HJYJsonObject>(name, (yyjson_mut_val*)nullptr, parent->getMDoc());
            const_cast<T&>(instance->*member).serialInfo(subObj);
            parent->addObj(subObj);
        }
    }
}

} // namespace detail

template <typename Class>
int HJJsonReflector::deserial(Class* instance, const std::shared_ptr<HJYJsonObject>& obj) {
    if (!instance) {
        return HJErrInvalidParams;
    }
    auto target_obj = obj ? obj : instance->getJObj();
    if (!target_obj) {
        return HJErrInvalidParams;
    }
    detail::tupleForEach(Class::kJsonFields, [&](const auto& field) {
        detail::deserialField(instance, target_obj.get(), field);
    });
    return HJ_OK;
}

template <typename Class>
int HJJsonReflector::serial(const Class* instance, const std::shared_ptr<HJYJsonObject>& obj) {
    if (!instance) {
        return HJErrInvalidParams;
    }
    auto target_obj = obj ? obj : instance->getJObj();
    if (!target_obj) {
        return HJErrInvalidParams;
    }
    detail::tupleForEach(Class::kJsonFields, [&](const auto& field) {
        detail::serialField(instance, target_obj.get(), field);
    });
    return HJ_OK;
}
#endif // HJ_HAVE_YYJSON

NS_HJ_END
