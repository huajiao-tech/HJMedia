//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNetManager
{

public:
    HJ_DECLARE_PUWTR(HJNetManager);
    virtual ~HJNetManager() = default;
    //HJ_INSTANCE_DECL(HJNetManager);

    static void registerAllNetworks();
    static void unregisterAllNetworks();
    static void preloadUrl(const std::string& url);
private:
    
};


NS_HJ_END