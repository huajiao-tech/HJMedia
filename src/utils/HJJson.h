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

NS_HJ_BEGIN
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
//***********************************************************************************//
#if defined(HJ_HAVE_YYJSON)
class HJYJsonObject : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJYJsonObject>;
    HJYJsonObject();
    HJYJsonObject(const std::string& key, yyjson_val* val);
    HJYJsonObject(const std::string& key, yyjson_mut_val* val, yyjson_mut_doc* doc);
    HJYJsonObject(const std::string& key, HJYJsonObject::Ptr obj);
    virtual ~HJYJsonObject();
    
    void setRVal(yyjson_val* val) {
        m_rval = val;
    }
    yyjson_val* getRVal() {
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
    const std::string& getKey() {
        return m_key;
    }
    void setMDoc(yyjson_mut_doc* doc) {
        m_mdoc = doc;
    }
    yyjson_mut_doc* getMDoc() {
        return m_mdoc;
    }
    //self
    bool isNull();
    bool isObj();
    bool isArr();
    bool isStr();
    bool isInt64();
    bool isUInt64();
    bool isReal();
    bool isBool();
    int getType();
    //child
    bool hasObj(const std::string& key);
    bool isObj(const std::string& key);
    bool isArr(const std::string& key);
    HJYJsonObject::Ptr getObj(const std::string& key);
    int addObj(HJYJsonObject::Ptr obj);
    //self
    bool getBool();
    int getInt();
    int64_t getInt64();
    uint64_t getUInt64();
    double getReal();
    std::string getString();
    uint8_t* getRaw();
    //read child
    bool getBool(const std::string& key);
    int getInt(const std::string& key);
    int64_t getInt64(const std::string& key);
    uint64_t getUInt64(const std::string& key);
    double getReal(const std::string& key);
    std::string getString(const std::string& key);
    uint8_t* getRaw(const std::string& key);
    std::deque<HJYJsonObject::Ptr> getArr(const std::string& key);
    
    int getMember(const std::string& key, bool& value);
    int getMember(const std::string& key, int& value);
    int getMember(const std::string& key, int64_t& value);
    int getMember(const std::string& key, uint64_t& value);
    int getMember(const std::string& key, double& value);
    int getMember(const std::string& key, std::string& value);
    int getMember(const std::string& key, uint8_t*& value, size_t& len);
    int getMember(const std::string& key, std::vector<HJYJsonObject::Ptr>& objs);
    int forEach(const std::string& key, const std::function<int(const HJYJsonObject::Ptr &)>& cb);
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
    int setMember(const std::string& key, const std::string value);
    int setMember(const std::string& key, const char* value);
    int setMember(const std::string& key, const uint8_t* value, const size_t len);
    int setMember(const std::string& key, std::vector<HJYJsonObject::Ptr> value);
protected:
    std::string         m_key  = "";
    yyjson_val*         m_rval = NULL;
    yyjson_mut_val*     m_wval = NULL;
    yyjson_mut_doc*     m_mdoc = NULL;
    HJObjectMap        m_subObjs;
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
    
    int init();
    int init(const std::string& info);
    int initWithUrl(const std::string& url);
    
    yyjson_mut_doc* getWDoc() {
        return m_wdoc;
    }
    std::string getSerialInfo();
    int writeFile(const std::string &path);
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
    HJInterpreter(HJYJsonObject::Ptr obj)
        : m_obj(obj) {}
    virtual ~HJInterpreter() {}
    
    virtual int init() { return  HJ_OK; }
    virtual int init(const std::string& info) { return  HJ_OK; }
    virtual int initWithUrl(const std::string& url) { return  HJ_OK; }
    
    virtual int deserialInfo(const HJYJsonObject::Ptr& obj = nullptr) = 0;
    virtual int serialInfo(const HJYJsonObject::Ptr& obj = nullptr) = 0;
    
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
        subInfo->serialInfo();
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
            if(!subInfo->getJObj()) {
                auto subObj = std::make_shared<HJYJsonObject>(key, m_obj);
                if(!subObj) {
                    return HJErrJSONValue;
                }
                subInfo->setJObj(subObj);
                subInfo->serialInfo();
                
                objs.push_back(subObj);
            }
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
    HJYJsonObject::Ptr     m_obj = nullptr;
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

NS_HJ_END
