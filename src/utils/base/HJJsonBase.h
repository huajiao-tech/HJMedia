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

NS_HJ_END
