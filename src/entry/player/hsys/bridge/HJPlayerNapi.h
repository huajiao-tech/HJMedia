#pragma once

#include <napi/native_api.h>
#include "HJMacros.h"

NS_HJ_BEGIN

class HJPlayerNapi {

public:
    static napi_value contextInit(napi_env env, napi_callback_info info);
    static napi_value createPlayer(napi_env env, napi_callback_info info);
    static napi_value destroyPlayer(napi_env env, napi_callback_info info);
    static napi_value openPlayer(napi_env env, napi_callback_info info);
    static napi_value setWindow(napi_env env, napi_callback_info info);
    static napi_value closePlayer(napi_env env, napi_callback_info info);
    static napi_value exitPlayer(napi_env env, napi_callback_info info);
    static napi_value setMute(napi_env env, napi_callback_info info);
    static napi_value preloadUrl(napi_env env, napi_callback_info info);
};

NS_HJ_END
