#pragma once

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <napi/native_api.h>
#include <string>
#include <unordered_map>

#include "HJNAPISRSession.h"
#include "HJVerifyRender.h"

NS_HJ_USING

namespace NativeXComponentSample {
class HJVerifyManager {
public:
    ~HJVerifyManager();

    static HJVerifyManager* GetInstance() {
        return &HJVerifyManager::m_HJVerifyManager;
    }

    static napi_value GetContext(napi_env env, napi_callback_info info);

    static napi_value srInit(napi_env env, napi_callback_info info);
    static napi_value srStart(napi_env env, napi_callback_info info);
    static napi_value srStop(napi_env env, napi_callback_info info);
    static napi_value srGetInfo(napi_env env, napi_callback_info info);

    void SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent);
    HJVerifyRender* GetRender(const std::string& id);
    void Export(napi_env env, napi_value exports);

private:
    static HJVerifyManager m_HJVerifyManager;

    static HJNAPISRSession::Ptr m_srSession;
    static bool s_srCtxInit;
    static int s_logCnt;

    std::unordered_map<std::string, OH_NativeXComponent*> m_nativeXComponentMap;
    std::unordered_map<std::string, HJVerifyRender*> m_HJVerifyRenderMap;
};
} // namespace NativeXComponentSample
