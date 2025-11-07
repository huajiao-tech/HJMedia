#pragma once

#include "HJPrerequisites.h"
#include "HJJson.h"
#include "HJUtilitys.h"

NS_HJ_BEGIN

#define HJ_JSON_BASE_DESERIAL(m_obj, i_obj, ...)                     \
    do                                                               \
    {                                                                \
        const char *keys = #__VA_ARGS__;                             \
        std::vector<std::string> vec = HJUtilitys::split(keys, ","); \
        HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
        i_err = HJJsonBase::deserial(vec, obj, __VA_ARGS__);         \
        if (i_err == HJErrNotExist)\
        {\
            i_err = HJ_OK;\
        }\
        if (i_err < 0) \
        { \
            break;\
        } \
    } while (0)

#define HJ_JSON_BASE_SERIAL(m_obj, i_obj, ...)                       \
    do                                                               \
    {                                                                \
        const char *keys = #__VA_ARGS__;                             \
        std::vector<std::string> vec = HJUtilitys::split(keys, ","); \
        HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
        i_err = HJJsonBase::serial(vec, obj, __VA_ARGS__);           \
        if (i_err < 0) \
        { \
            break;\
        } \
    } while (0)

#define HJ_JSON_SUB_DESERIAL(m_obj, i_obj, childClassIns) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
            HJYJsonObject::Ptr subObj = obj->getObj(HJ_CatchName(childClassIns)); \
            if (subObj) \
            {        \
                i_err = childClassIns.deserialInfo(subObj); \
                if (i_err == HJErrNotExist)\
                {\
                    i_err = HJ_OK;\
                }\
                if (i_err < 0) \
                { \
                    break; \
                } \
            } \
        } \

#define HJ_JSON_SUB_SERIAL(m_obj, i_obj, childClassIns) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
            HJYJsonObject::Ptr childObj = std::make_shared<HJYJsonObject>(HJ_CatchName(childClassIns), obj); \
            i_err = obj->addObj(childObj); \
            if (i_err < 0) \
            { \
                break;\
            } \
            i_err = childClassIns.serialInfo(childObj); \
            if (i_err < 0) \
            { \
                break;\
            } \
        } \

#define HJ_JSON_ARRAY_OBJ_DESERIAL(m_obj, i_obj, arrayIns, itemClassType) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
            i_err = obj->forEach(HJ_CatchName(arrayIns), [&](const HJYJsonObject::Ptr& subObj) \
            { \
                int ret = HJ_OK; \
                itemClassType::Ptr info = std::make_shared<itemClassType>(); \
                ret = info->deserialInfo(subObj);\
                if (ret < 0) \
                { \
                    return ret; \
                } \
                arrayIns.push_back(std::move(info)); \
                return ret; \
            }); \
            if (i_err == HJErrNotExist)\
            {\
                i_err = HJ_OK;\
            }\
        } \

#define HJ_JSON_ARRAY_OBJ_ANOMYMOUS_DESERIAL(m_obj, i_obj, arrayIns, itemClassType) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
            i_err = obj->forEachAnonymous([&](const HJYJsonObject::Ptr& subObj) \
            { \
                int ret = HJ_OK; \
                itemClassType::Ptr info = std::make_shared<itemClassType>(); \
                ret = info->deserialInfo(subObj);\
                if (ret < 0) \
                { \
                    return ret; \
                } \
                arrayIns.push_back(std::move(info)); \
                return ret; \
            }); \
            if (i_err == HJErrNotExist)\
            {\
                i_err = HJ_OK;\
            }\
        } \

#define HJ_JSON_ARRAY_PLAIN_DESERIAL(m_obj, i_obj, arrayItemsIns) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj; \
            i_err = obj->forEach(HJ_CatchName(arrayItemsIns), [&](const HJYJsonObject::Ptr& subObj) \
            { \
                int ret = HJ_OK; \
                ret = HJJsonBase::deserialArray(arrayItemsIns, subObj);\
                return ret; \
            }); \
            if (i_err == HJErrNotExist)\
            {\
                i_err = HJ_OK;\
            }\
        } \

#define HJ_JSON_ARRAY_SERIAL(m_obj, i_obj, childClassIns) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;              \
            std::vector<HJYJsonObject::Ptr> jItems; \
            for (auto& info : childClassIns) \
            { \
                auto itemObj = std::make_shared<HJYJsonObject>(HJ_CatchName(childClassIns) + std::string("_item"), obj);\
                i_err = info->serialInfo(itemObj);\
                if (i_err < 0) \
                { \
                    break; \
                }\
                jItems.push_back(itemObj);\
            } \
            obj->setMember(HJ_CatchName(childClassIns), jItems);\
        } \

#define HJ_JSON_ARRAY_PLAIN_SERIAL(m_obj, i_obj, arrayItemsIns) \
       {                                    \
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj; \
            i_err = obj->setMember(HJ_CatchName(arrayItemsIns), arrayItemsIns); \
            if (i_err < 0) \
            { \
                break; \
            }\
        } \

class HJJsonBase : public HJInterpreter
{
public:
    HJ_DEFINE_CREATE(HJJsonBase);
    HJJsonBase();
    virtual ~HJJsonBase();
    
    std::string initSerial();
    //virtual int serialUrl(const std::string &i_url);
    //virtual int init();
    virtual int init(const std::string &info);
    virtual int initWithUrl(const std::string &url);
    virtual int deserialInfo(const HJYJsonObject::Ptr &obj = nullptr)
    {
        return 0;
    }
    virtual int serialInfo(const HJYJsonObject::Ptr &obj = nullptr)
    {
        return 0;
    }

    //"ab, cd, val" -> ["ab", "cd", "val"]
    static std::vector<std::string> getAllKeys(const std::string &input);

    template <typename T>
    static int deserialDetail(std::vector<std::string> &vec, const HJYJsonObject::Ptr &obj, T &value)
    {
        int i_err = HJ_OK;

        if (vec.empty())
        {
            return i_err;
        }

        std::string key = vec.front();
        key = HJUtilitys::trim(key);
        do
        {
            int i_err = obj->getMember(key, value);
            if (i_err < 0)
            {
                break;
            }
            //		if constexpr (std::is_same<T, int>::value)
            //		{
            //			obj->getMember(key, value);
            //		}
            //		else if constexpr (std::is_same<T, int64_t>::value)
            //		{
            //			obj->getMember(key, value);
            //		}
            //		else if constexpr (std::is_same<T, double>::value)
            //		{
            //			obj->getMember(key, value);;
            //		}
            //		else if constexpr (std::is_same<T, std::string>::value)
            //		{
            //			obj->getMember(key, value);
            //		}
            //		else if constexpr (std::is_same<T, bool>::value)
            //		{
            //			obj->getMember(key, value);
            //		}

        } while (false);
        vec.erase(vec.begin());
        return i_err;
    }
    template <typename... Args>
    static int deserial(std::vector<std::string> &vec, const HJYJsonObject::Ptr &obj, Args &&...fields)
    {
        int final_err = HJ_OK;
        ((final_err != HJ_OK ? final_err : (final_err = deserialDetail(vec, obj, fields))), ...);
        return final_err;
    }
    
    template <typename T>
    static int serialDetail(std::vector<std::string> &vec, const HJYJsonObject::Ptr &obj, T &value)
    {
        int i_err = HJ_OK;

        if (vec.empty())
        {
            return i_err;
        }

        std::string key = vec.front();
        key = HJUtilitys::trim(key);
        do
        {
            int i_err = obj->setMember(key, value);
            if (i_err < 0)
            {
                break;
            }
        } while (false);
        vec.erase(vec.begin());
        return i_err;
    }
    template <typename... Args>
    static int serial(std::vector<std::string> &vec, const HJYJsonObject::Ptr &obj, Args &&...fields)
    {
        int final_err = HJ_OK;
        ((final_err != HJ_OK ? final_err : (final_err = serialDetail(vec, obj, fields))), ...);
        return final_err;
    }

    template <typename T>
    static int deserialArray(std::vector<T>& vecotrs, const HJYJsonObject::Ptr& subObj)
    {
        if constexpr (std::is_same<T, int>::value)
        {
            vecotrs.push_back(subObj->getInt());
        }
        else if constexpr (std::is_same<T, int64_t>::value)
        {
            vecotrs.push_back(subObj->getInt64());
        }
        else if constexpr (std::is_same<T, double>::value)
        {
            vecotrs.push_back(subObj->getReal());
        }
        else if constexpr (std::is_same<T, std::string>::value)
        {
            vecotrs.push_back(std::move(subObj->getString()));
        }
        else if constexpr (std::is_same<T, bool>::value)
        {
            vecotrs.push_back(subObj->getBool());
        }
        return HJ_OK;
    }
private:
    std::string priSerialToStr();
};

class HJJsonTestSub : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJJsonTestSub);
    HJJsonTestSub()
    {
        
    }
    virtual ~HJJsonTestSub()
    {
        
    }

    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr);
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr);
        
    double subvaluef = 0.6;   
    std::string subvaluestr = "subvalue";
};

class HJJsonTest : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJJsonTest);
    HJJsonTest()
    {
        
    }
    virtual ~HJJsonTest()
    {
        
    }

    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr);
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr);
        
    
    HJJsonTestSub subObject;
    
    std::string color = "BT601";
    std::string cropMode = "fit";

    bool bAlphaVideoLeftRight = false;

    double viewOffx = 0.0;
    double viewOffy = 0.0;
    double viewWidth = 1.0;
    double viewHeight = 1.0;
};


class HJJsonTestMonitorInfo : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJJsonTestMonitorInfo);
    HJJsonTestMonitorInfo() = default;
    virtual ~HJJsonTestMonitorInfo() = default;

    virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
    {
		int i_err = HJ_OK;
		do
		{
			HJ_JSON_BASE_DESERIAL(m_obj, i_obj, id);
            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;

            if (!obj->hasObj("value"))
            {
                i_err = HJErrFatal;
                break;
            }
            auto valObj = obj->getObj("value");
            if (valObj->isInt64())
            {
                value = valObj->getInt64();
            }
            else if (valObj->isStr())
            {
                value = valObj->getString();
            }
            else if (valObj->isReal())
            {
                value = valObj->getReal();
            }
		} while (false);
		return i_err;
    }
    virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
		{ 
            HJ_JSON_BASE_SERIAL(m_obj, i_obj, id);

            HJYJsonObject::Ptr obj = m_obj ? m_obj : i_obj;

            const std::type_info& typeInfo = value.type();
            if (typeInfo == typeid(int64_t))
            {
                obj->setMember("value", std::any_cast<int64_t>(value));
            }
            else if (typeInfo == typeid(std::string))
            {
                obj->setMember("value", std::any_cast<std::string>(value));
            }    
            else if (typeInfo == typeid(double))
            {
                obj->setMember("value", std::any_cast<double>(value));
            }
		} while (false);
        return i_err;
    }

    int64_t id = 0;
    std::any value;
};

class HJJsonTestScene : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJJsonTestScene);
    HJJsonTestScene() = default;
    virtual ~HJJsonTestScene() = default;

    virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);
    virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);

    std::vector<HJJsonTestMonitorInfo::Ptr> sceneinfo;
    std::vector<double> doublePlainArray;
    int64_t value = 0;
};


class HJJsonTestMonitor : public HJJsonBase
{
public:
    HJ_DEFINE_CREATE(HJJsonTestMonitor);
    HJJsonTestMonitor() = default;
    virtual ~HJJsonTestMonitor() = default;

    virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);
	virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr);

    int64_t uid = 0;
    std::string device = "";
    int64_t timestamp = 0;
    int64_t business = 0;

    HJJsonTestScene scene;
    std::vector<std::string> strPlainArray;
    std::vector<int> intPlainArray;
    std::vector<HJJsonTestMonitorInfo::Ptr> metrics;
};



NS_HJ_END
