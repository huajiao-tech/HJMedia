//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include "HJASysUtils.h"
#include "HJError.h"

NS_HJ_BEGIN
//***********************************************************************************//
float HJASysUtils::getMemoryUsage()
{
    float usage = 0.0f;
    std::ifstream memInfoFile("/proc/meminfo");
    std::string line;
    unsigned long totalMemory = 0;
    unsigned long freeMemory = 0;

    while (std::getline(memInfoFile, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lu kB", &totalMemory);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lu kB", &freeMemory);
        }
    }

    if (totalMemory > 0) {
        float usedMemory = totalMemory - freeMemory;
        usage = (usedMemory / totalMemory) * 100.0;
        // std::cout << "Memory Usage: " << usage << "%" << std::endl;
    }
    return usage;
}

float HJASysUtils::getCpuUsage()
{
    float usage = 0.0f;
    std::ifstream statFile("/proc/stat");
    std::string line;
    std::vector<std::string> cpuStats;

    if (std::getline(statFile, line)) {
        std::istringstream ss(line);
        std::string cpu;
        ss >> cpu; // "cpu" 或 "cpu0", "cpu1" 等
        std::string stat;
        while (ss >> stat) {
            cpuStats.push_back(stat);
        }

        if (cpuStats.size() >= 4) {
            unsigned long user = std::stoul(cpuStats[0]);
            unsigned long nice = std::stoul(cpuStats[1]);
            unsigned long system = std::stoul(cpuStats[2]);
            unsigned long idle = std::stoul(cpuStats[3]);
            unsigned long total = user + nice + system + idle;
            unsigned long totalUsage = user + nice + system;
            usage = ((float)totalUsage / total) * 100.0;
            // std::cout << "CPU Usage: " << usage << "%" << std::endl;
        }
    }
    return usage;
}

float HJASysUtils::getGpuUsage()
{
    float usage = 0.0f;

    std::ifstream gpuFile("/sys/class/mali0/devfreq/mali0/display");
    std::string line;
    float gpuUsage = 0.0f;

    if (gpuFile.is_open()) {
        while (std::getline(gpuFile, line)) {
            // 解析GPU的利用率（假设此文件包含GPU使用信息）
            // 需要根据具体设备的文件格式来解析
            // 示例：读取当前GPU频率或者负载等
            std::cout << "GPU info: " << line << std::endl;
        }
    }

    //  std::ifstream gpuFile("/sys/class/kgsl/kgsl-3d0/gpubusy");

    return usage;
}

NS_HJ_END