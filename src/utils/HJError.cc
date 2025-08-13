//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJError.h"

NS_HJ_BEGIN
//***********************************************************************************//
static const std::unordered_map<int32_t, std::string> g_hjErrorCodeMessages = 
{
    {HJ_OK, "ok"},
    {HJ_EOF, "eof"}
};

const std::string HJErrorCodeToMsg(const int32_t code) {
    auto it = g_hjErrorCodeMessages.find(code);
    if (it != g_hjErrorCodeMessages.end()) {
        return it->second;
    }
    return "unknown";
}


NS_HJ_END