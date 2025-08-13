//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJObject.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
const size_t HJObject::getGlobalID()
{
    static std::atomic<size_t> g_objectIDCounter(0);
    return ++g_objectIDCounter;
}

const std::string HJObject::getGlobalName(const std::string prefix)
{
    auto value = HJFMT("{}",getGlobalID());
    return prefix.empty() ? value : (prefix + "_" + value);
}


NS_HJ_END