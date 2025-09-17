//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJGlobalSettings.h"

NS_HJ_BEGIN
std::shared_mutex HJGlobalSettingsManager::m_mutex;
HJGlobalSettings HJGlobalSettingsManager::m_settings;
//***********************************************************************************//
int HJGlobalSettingsManager::init(HJGlobalSettings* setting)
{
	if (!setting) {
		return HJErrInvalidParams;
	}
	HJ_AUTOU_LOCK(m_mutex);
    m_settings = *setting;

	return HJ_OK;
}



NS_HJ_END