//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNetManager.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "HJGlobalSettings.h"
#include "HJTLSLinkManager.h"
#include "HJHLinksManager.h"

struct URLProtocol;
extern "C" URLProtocol ff_hjtcp_protocol;
extern "C" URLProtocol ff_hjtls_protocol;
extern "C" URLProtocol ff_hjhttp_protocol;
extern "C" URLProtocol ff_hjhttps_protocol;
//
extern "C" void av_register_protocol(const URLProtocol* p);

NS_HJ_BEGIN
//HJ_INSTANCE(HJNetManager)
//***********************************************************************************//
void HJNetManager::registerAllNetworks()
{
    static HJOnceToken token([&] {
        HJFLogi("registerAllNetworks, tls:{}, http:{}", HJGlobalSettingsManager::getUseTLSPool(), HJGlobalSettingsManager::getUseHTTPPool());
        if (HJGlobalSettingsManager::getUseTLSPool())
        {
            HJTLSLinkManagerInit();
            HJTLSLinkManagerInstance()->setLinkMax(HJGlobalSettingsManager::getUrlLinkMax());
            HJTLSLinkManagerInstance()->setLinkAliveTime(HJGlobalSettingsManager::getUrlLinkTimeout());
            //
            //av_register_protocol(&ff_jptcp_protocol);
            av_register_protocol(&ff_hjtls_protocol);
            if (HJGlobalSettingsManager::getUseHTTPPool()) {
                av_register_protocol(&ff_hjhttp_protocol);
                av_register_protocol(&ff_hjhttps_protocol);
                HJHLinkManagerInit();
            }
        }
        avformat_network_init();
    });
}

void HJNetManager::unregisterAllNetworks()
{

}

void HJNetManager::preloadUrl(const std::string& url)
{
    if (!HJGlobalSettingsManager::getUseTLSPool()) {
        return;
    }
    if (HJGlobalSettingsManager::getUseHTTPPool()) {
        HJHLinkManagerSetDefaultUrl(url);
    }
    HJTLSLinkManagerSetDefaultUrl(url);

    return;
}


//***********************************************************************************//
NS_HJ_END