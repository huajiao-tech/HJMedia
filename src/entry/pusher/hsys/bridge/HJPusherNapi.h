#pragma once

#include <napi/native_api.h>
#include "HJMacros.h"

NS_HJ_BEGIN

class HJPusherNapi {

public:
    static napi_value contextInit(napi_env env, napi_callback_info info);
    static napi_value createPusher(napi_env env, napi_callback_info info);
    static napi_value destroyPusher(napi_env env, napi_callback_info info);
    static napi_value setWindow(napi_env env, napi_callback_info info);
    static napi_value openPreview(napi_env env, napi_callback_info info);
    static napi_value closePreview(napi_env env, napi_callback_info info);
    static napi_value openPusher(napi_env env, napi_callback_info info);
    static napi_value closePusher(napi_env env, napi_callback_info info);
    static napi_value setMute(napi_env env, napi_callback_info info);
    static napi_value openRecorder(napi_env env, napi_callback_info info);
    static napi_value closeRecorder(napi_env env, napi_callback_info info);
    static napi_value openPngSeq(napi_env env, napi_callback_info info);
    static napi_value setDoubleScreen(napi_env env, napi_callback_info info);
    static napi_value setGiftPusher(napi_env env, napi_callback_info info);
    static napi_value openSpeechRecognizer(napi_env env, napi_callback_info info);
    static napi_value closeSpeechRecognizer(napi_env env, napi_callback_info info);
    static napi_value openPixelMapOutput(napi_env env, napi_callback_info info);
    static napi_value getImageReceiver(napi_env env, napi_callback_info info);
    static napi_value closePixelMapOutput(napi_env env, napi_callback_info info);
};

NS_HJ_END