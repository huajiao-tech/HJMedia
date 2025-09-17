//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <string>
#include "HJMacros.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJASysUtils
{
public:
    static float getMemoryUsage();
    static float getCpuUsage();
    static float getGpuUsage();
};

NS_HJ_END