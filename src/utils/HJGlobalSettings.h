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
typedef struct HJGlobalSettings
{
	bool useTLSPool = true;
	bool useHTTPPool = true;
	int urlLinkMax = 5;
	int urlLinkTimeout = 15;			// sec
	int urlConnectTimeout = 3000000;	//us
	bool netBlockEnable = false;
} HJGlobalSettings;

class HJGlobalSettingsManager
{
public:
	static int init(HJGlobalSettings* setting);

	static bool getUseTLSPool() {
		HJ_AUTOS_LOCK(m_mutex);
        return m_settings.useTLSPool;
	}
    static bool getUseHTTPPool() {
		HJ_AUTOS_LOCK(m_mutex);
        return m_settings.useHTTPPool;
	}
    static int getUrlLinkMax() {
		HJ_AUTOS_LOCK(m_mutex);
        return m_settings.urlLinkMax;
	}
    static int getUrlLinkTimeout() {
		HJ_AUTOS_LOCK(m_mutex);
        return m_settings.urlLinkTimeout;
	}
    static int getUrlConnectTimeout() {
		HJ_AUTOS_LOCK(m_mutex);
        return m_settings.urlConnectTimeout;
	}
	//
    static bool getNetBlockEnable() {
		HJ_AUTOS_LOCK(m_mutex);
        return m_settings.netBlockEnable;
	}
    static void setNetBlockEnable(bool enable) {
		HJ_AUTOS_LOCK(m_mutex);
        m_settings.netBlockEnable = enable;
	}
private:
	static std::shared_mutex m_mutex;
	static HJGlobalSettings m_settings;
};


NS_HJ_END