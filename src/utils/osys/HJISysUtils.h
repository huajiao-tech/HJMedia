//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <string>
#include "HJMacros.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJISysUtils
{
public:
    static std::string getDocumentsPath();
    static std::string getLibraryPath();
    static std::string getMainBundlePath();
    static int createDirectory(const std::string& dir);
    static std::string logDir();
    static std::string defaultLogsDirectory();
    static std::string defaultCacheDirectory();
    static std::string devicePlatformString();
    //
    static std::tuple<float, float> getMemoryUsage();
    static float getCpuUsage();
    static float getCpuUsageX();
    static float getGpuUsage();
};

NS_HJ_END
