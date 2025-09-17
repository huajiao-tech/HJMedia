#include <hilog/log.h>

#include "verify/HJVerifyManager.h"
#include "HJNativeCommon.h"
#include "HJPusherNapi.h"

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

        DECLARE_NAPI_FUNCTION_VAL(tryOpenPreview, HJVerifyManager::tryOpenPreview),
        DECLARE_NAPI_FUNCTION_VAL(tryOpenPush, HJVerifyManager::tryOpenPush),
        DECLARE_NAPI_FUNCTION_VAL(tryClosePreview, HJVerifyManager::tryClosePreview),
        DECLARE_NAPI_FUNCTION_VAL(tryOpenRecord, HJVerifyManager::tryOpenRecord),
        DECLARE_NAPI_FUNCTION_VAL(tryCloseRecord, HJVerifyManager::tryCloseRecord),

        DECLARE_NAPI_FUNCTION_VAL(tryGiftOpen, HJVerifyManager::tryGiftOpen),
        DECLARE_NAPI_FUNCTION_VAL(tryDoubleScreen, HJVerifyManager::tryDoubleScreen),
        DECLARE_NAPI_FUNCTION_VAL(tryGiftPusher, HJVerifyManager::tryGiftPusher),

        DECLARE_NAPI_FUNCTION_VAL(n_contextInit, HJ::HJPusherNapi::contextInit),
        DECLARE_NAPI_FUNCTION_VAL(n_createPusher, HJ::HJPusherNapi::createPusher),
        DECLARE_NAPI_FUNCTION_VAL(n_destroyPusher, HJ::HJPusherNapi::destroyPusher),
        DECLARE_NAPI_FUNCTION_VAL(n_setWindow, HJ::HJPusherNapi::setWindow),
        DECLARE_NAPI_FUNCTION_VAL(n_openPreview, HJ::HJPusherNapi::openPreview),
        DECLARE_NAPI_FUNCTION_VAL(n_closePreview, HJ::HJPusherNapi::closePreview),
        DECLARE_NAPI_FUNCTION_VAL(n_openPusher, HJ::HJPusherNapi::openPusher),
        DECLARE_NAPI_FUNCTION_VAL(n_closePusher, HJ::HJPusherNapi::closePusher),
        DECLARE_NAPI_FUNCTION_VAL(n_setMute, HJ::HJPusherNapi::setMute),
        DECLARE_NAPI_FUNCTION_VAL(n_openRecorder, HJ::HJPusherNapi::openRecorder),
        DECLARE_NAPI_FUNCTION_VAL(n_closeRecorder, HJ::HJPusherNapi::closeRecorder),
        DECLARE_NAPI_FUNCTION_VAL(n_openPngSeq, HJ::HJPusherNapi::openPngSeq),
        DECLARE_NAPI_FUNCTION_VAL(n_setDoubleScreen, HJ::HJPusherNapi::setDoubleScreen),
        DECLARE_NAPI_FUNCTION_VAL(n_setGiftPusher, HJ::HJPusherNapi::setGiftPusher),
        DECLARE_NAPI_FUNCTION_VAL(n_openSpeechRecognizer, HJ::HJPusherNapi::openSpeechRecognizer),
        DECLARE_NAPI_FUNCTION_VAL(n_closeSpeechRecognizer, HJ::HJPusherNapi::closeSpeechRecognizer),
        // DECLARE_NAPI_FUNCTION_VAL(n_openPixelMapOutput, HJ::HJPusherNapi::openPixelMapOutput),
        // DECLARE_NAPI_FUNCTION_VAL(n_getImageReceiver, HJ::HJPusherNapi::getImageReceiver),
        // DECLARE_NAPI_FUNCTION_VAL(n_closePixelMapOutput, HJ::HJPusherNapi::closePixelMapOutput),
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
