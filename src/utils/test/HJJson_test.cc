//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJJson.h"

NS_HJ_BEGIN
//***********************************************************************************//
auto json = std::make_shared<HJYJsonDocument>();
json->init();
(*json)["kbps"] = kbps;
(*json)["fps"] = fps;
(*json)["delay"] = delay;

HJYJsonObject obj;
obj["key"] = "value";
int value = obj["key"];

NS_HJ_END