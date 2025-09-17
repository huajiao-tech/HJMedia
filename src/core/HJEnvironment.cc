//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJEnvironment.h"
#include "HJTicker.h"
#include "HJContext.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJEnvironment::HJEnvironment()
{
    setName(HJContext::Instance().getGlobalName(HJ_TYPE_NAME(HJEnvironment)));
	//m_noticeHandler = std::make_shared<HJNoticeHandler>();
	m_timeline = std::make_shared<HJTimelineHandler>();
}
            
HJEnvironment::~HJEnvironment()
{

}

HJHWDevice::Ptr HJEnvironment::getHWDevice(HJDeviceType devType, int devID)
{
	std::string key = "hwdev_" + HJ2SSTR(devType) + "_" + HJ2SSTR(devID);
	auto it = m_hwDevices.find(key);
	if (it != m_hwDevices.end()) {
		return it->second;
	}
	HJHWDevice::Ptr hwDev = std::make_shared<HJHWDevice>(devType, devID);
	int res = hwDev->init();
	if (HJ_OK != res){
		return nullptr;
	}
	m_hwDevices[key] = hwDev;

	return hwDev;
}

HJExecutor::Ptr HJEnvironment::getExecutor(const std::string key)
{
    if (key.empty()) {
        return nullptr;
    }
    std::string tname = getName() + "_" + key;
    auto it = m_executors.find(tname);
    if (it != m_executors.end()) {
        return it->second;
    }
    auto executor = HJExecutorManager::Instance().getExecutor();
    m_executors[tname] = executor;
    return executor;
}

NS_HJ_END
