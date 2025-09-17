//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJCommons.h"
#include "HJError.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef struct HJInstanceSettings
{
	bool autoDelayEnable = true;
	int cacheTargetDuration = 3000;
	int cacheLimitedRange = 2000;
	float stutterRatioMax = 0.03f;
	float stutterRatioDelta = 0.005f;
	float stutterFactorMax = 3.0f;
	int observeTime = 15000;
	//
	bool pluginInfosEnable = false;
} HJInstanceSettings;

class HJInstanceSettingsManager
{
public:
	using Ptr = std::shared_ptr<HJInstanceSettingsManager>;
	void setSettings(const HJInstanceSettings& settings);
	//
	bool getAutoDelayEnable() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.autoDelayEnable;
	}
	int getCacheTargetDuration() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.cacheTargetDuration;
	}
	int getCacheLimitedRange() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.cacheLimitedRange;
	}
	float getStutterRatioMax() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.stutterRatioMax;
	}
	float getStutterRatioDelta() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.stutterRatioDelta;
	}
	float getStutterFactorMax() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.stutterFactorMax;
	}
	int getObserveTime() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.observeTime;
	}
    int getPluginInfosEnable() const
	{
		HJ_AUTOS_LOCK(m_mutex);
		return m_settings.pluginInfosEnable;
	}
private:
	mutable std::shared_mutex m_mutex;
	HJInstanceSettings m_settings;
};

NS_HJ_END