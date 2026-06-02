#include <hilog/log.h>

#include "verify/HJVerifyManager.h"
#include "HJNativeCommon.h"

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
                
        DECLARE_NAPI_FUNCTION_VAL(srInit, HJVerifyManager::srInit),
        DECLARE_NAPI_FUNCTION_VAL(srStart, HJVerifyManager::srStart),
        DECLARE_NAPI_FUNCTION_VAL(srStop, HJVerifyManager::srStop),
        DECLARE_NAPI_FUNCTION_VAL(srGetInfo, HJVerifyManager::srGetInfo),
    };
    
    
    if (napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Init", "napi_define_properties failed");
        return nullptr;
    }
    
    HJVerifyManager::GetInstance()->Export(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module nativeHJInferenceRenderModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "publisher",
    .nm_priv = ((void*)0),
    .reserved = { 0 }
};

extern "C" __attribute__((constructor)) void RegisterModule(void) {
    napi_module_register(&nativeHJInferenceRenderModule);
}
} // namespace NativeXComponentSample
