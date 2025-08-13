#include "HJJsonBase.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

HJJsonBase::HJJsonBase()
{
}

HJJsonBase::~HJJsonBase()
{
}
std::string HJJsonBase::priSerialToStr()
{
    std::string result = "";
    do
    {
        if (m_obj)
        {
            int i_err = serialInfo();
            if (i_err < 0)
            {
                break;
            }
            HJYJsonDocument::Ptr doc = std::dynamic_pointer_cast<HJYJsonDocument>(m_obj);
            if (doc)
            {
                result = doc->getSerialInfo();
            }
        }
    } while (false);
    return result;
}
// int HJJsonBase::serialUrl(const std::string &i_url)
//{
//
// }

std::string HJJsonBase::initSerial()
{
    std::string result = "";
    do
    {
        HJYJsonDocument::Ptr doc = std::make_shared<HJYJsonDocument>();
        int i_err = doc->init();
        if (i_err < 0)
        {
            break;
        }
        HJInterpreter::setJObj(doc);
        result = priSerialToStr();
    } while (false);
    return result;
}
int HJJsonBase::init(const std::string &info)
{
    int i_err = HJ_OK;
    do
    {
        HJYJsonDocument::Ptr doc = std::make_shared<HJYJsonDocument>();
        i_err = doc->init(info);
        if (i_err < 0)
        {
            break;
        }
        HJInterpreter::setJObj(doc);
        
        i_err = deserialInfo();
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
int HJJsonBase::initWithUrl(const std::string &url)
{
    int i_err = HJ_OK;
    do
    {
        HJYJsonDocument::Ptr doc = std::make_shared<HJYJsonDocument>();
        i_err = doc->initWithUrl(url);
        if (i_err < 0)
        {
            break;
        }
        HJInterpreter::setJObj(doc);
        i_err = deserialInfo();
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

std::vector<std::string> HJJsonBase::getAllKeys(const std::string &input)
{
    std::vector<std::string> result;

    size_t start = 0;
    size_t end = input.find(',');

    while (end != std::string::npos)
    {
        std::string token = input.substr(start, end - start);
        // 去除前后空格
        token.erase(0, token.find_first_not_of(' '));
        token.erase(token.find_last_not_of(' ') + 1);
        result.push_back(token);
        start = end + 1;
        end = input.find(',', start);
    }
    // 添加最后一个元素
    std::string token = input.substr(start);
    token.erase(0, token.find_first_not_of(' '));
    token.erase(token.find_last_not_of(' ') + 1);
    result.push_back(token);

    return result;
}

/////////////////////////////////////////////////////////////////////////////////
int HJJsonTestSub::deserialInfo(const HJYJsonObject::Ptr &i_obj)
{
    int i_err = HJ_OK;
    do
    {
        HJ_JSON_BASE_DESERIAL(m_obj, i_obj, subvaluef, subvaluestr);
    } while (false);
    return i_err;
}
int HJJsonTestSub::serialInfo(const HJYJsonObject::Ptr &i_obj)
{
    int i_err = HJ_OK;
    do
    {
        HJ_JSON_BASE_SERIAL(m_obj, i_obj, subvaluef, subvaluestr);
    } while (false);
    return i_err;
}

int HJJsonTest::deserialInfo(const HJYJsonObject::Ptr &i_obj)
{
    int i_err = HJ_OK;
    do
    {
        HJ_JSON_BASE_DESERIAL(m_obj, i_obj, color, cropMode, bAlphaVideoLeftRight, viewOffx, viewOffy, viewWidth, viewHeight);
        HJ_JSON_SUB_DESERIAL(m_obj, i_obj, subObject);
        
//        {
//            HJYJsonObject::Ptr subObj = m_obj->getObj("subObject");
//            if (subObj)
//            {
//                i_err = subObject.deserialInfo(subObj);
//                if (i_err < 0)
//                {
//                    break;    
//                }
//            }    
//        }
    } while (false);
    return i_err;
}




int HJJsonTest::serialInfo(const HJYJsonObject::Ptr &i_obj)
{
    int i_err = HJ_OK;
    do
    {
        HJ_JSON_BASE_SERIAL(m_obj, i_obj, color, cropMode, bAlphaVideoLeftRight, viewOffx, viewOffy, viewWidth, viewHeight);
        HJ_JSON_SUB_SERIAL(m_obj, i_obj, subObject);
        
//        {
//            HJYJsonObject::Ptr subObj = std::make_shared<HJYJsonObject>("subObject", m_obj);
//            m_obj->addObj(subObj);
////            HJYJsonObject::Ptr subObj = m_obj->setMember("subObject");
//            subObject.serialInfo(subObj);
//        }
    } while (false);
    return i_err;
}

NS_HJ_END
