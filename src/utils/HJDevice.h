//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJCpuUsageInfo;
class HJCpuUsage : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJCpuUsage>;
	HJCpuUsage();

	double queryUsage();
private:
	int init();
protected:
	std::shared_ptr<HJCpuUsageInfo>	m_cpuInfo = nullptr;
};

//***********************************************************************************//
class HJDevice : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJDevice>;
    
    static float getCpuUsage();
    static float getGpuUsage();
    static std::tuple<float, float> getMemoryUsage();
protected:

};

NS_HJ_END
