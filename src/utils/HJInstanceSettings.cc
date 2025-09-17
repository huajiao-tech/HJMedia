//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJInstanceSettings.h"

NS_HJ_BEGIN
//***********************************************************************************//
void HJInstanceSettingsManager::setSettings(const HJInstanceSettings& settings)
{
	HJ_AUTOU_LOCK(m_mutex);
	m_settings = settings;
}


NS_HJ_END