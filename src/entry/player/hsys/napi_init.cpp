#include <hilog/log.h>

#include "verify/HJVerifyManager.h"
#include "HJNativeCommon.h"
#include "HJPlayerNapi.h"

namespace NativeXComponentSample {
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    

    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Version", "06171447");
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Init", "Init begins");
    
    if ((env == nullptr) || (exports == nullptr)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Init", "env or exports is null");
        return nullptr;
    }

    napi_property_descriptor desc[] = { 

        DECLARE_NAPI_FUNCTION_VAL(getContext, HJVerifyManager::GetContext),
        
        DECLARE_NAPI_FUNCTION_VAL(testOpen, HJVerifyManager::testOpen),
        DECLARE_NAPI_FUNCTION_VAL(testClose, HJVerifyManager::testClose),  
        DECLARE_NAPI_FUNCTION_VAL(testEffectGray, HJVerifyManager::testEffectGray),
        DECLARE_NAPI_FUNCTION_VAL(testEffectBlur, HJVerifyManager::testEffectBlur),  
        
        DECLARE_NAPI_FUNCTION_VAL(tryOpen, HJVerifyManager::tryOpen),
        DECLARE_NAPI_FUNCTION_VAL(tryMute, HJVerifyManager::tryMute),
        DECLARE_NAPI_FUNCTION_VAL(tryClose, HJVerifyManager::tryClose),

        DECLARE_NAPI_FUNCTION_VAL(n_contextInit, HJPlayerNapi::contextInit),
        DECLARE_NAPI_FUNCTION_VAL(n_createPlayer, HJPlayerNapi::createPlayer),
        DECLARE_NAPI_FUNCTION_VAL(n_destroyPlayer, HJPlayerNapi::destroyPlayer),
        DECLARE_NAPI_FUNCTION_VAL(n_openPlayer, HJPlayerNapi::openPlayer),
        DECLARE_NAPI_FUNCTION_VAL(n_setWindow, HJPlayerNapi::setWindow),
        DECLARE_NAPI_FUNCTION_VAL(n_closePlayer, HJPlayerNapi::closePlayer),
        DECLARE_NAPI_FUNCTION_VAL(n_exitPlayer, HJPlayerNapi::exitPlayer),
        DECLARE_NAPI_FUNCTION_VAL(n_setMute, HJPlayerNapi::setMute),
        DECLARE_NAPI_FUNCTION_VAL(n_preloadUrl, HJPlayerNapi::preloadUrl),
    };
    
    
    if (napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Init", "napi_define_properties failed");
        return nullptr;
    }
    
    HJVerifyManager::GetInstance()->Export(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module nativeHJPlayerModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "publisher",
    .nm_priv = ((void*)0),
    .reserved = { 0 }
};

extern "C" __attribute__((constructor)) void RegisterModule(void) {
    napi_module_register(&nativeHJPlayerModule);
}
} // namespace NativeXComponentSample
