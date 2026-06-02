#pragma once
#include <string>
#include <napi/native_api.h>
#include "HJNativeCommon.h"

class NapiUtil {
public:
    static void JsValueToString(const napi_env &env, const napi_value &value, const int32_t bufLen,
        std::string &target);
    static napi_value SetNapiCallInt32(const napi_env & env,  const int32_t intValue);
    static napi_value SetNapiCallBool(const napi_env env, bool value);
    static napi_value SetNapiCallString(const napi_env & env, const std::string& stringValue);
    static int StringToInt(std::string value);
    static int StringToLong(std::string value);
    static float StringToFloat(std::string value);
    static bool StringToBool(std::string value);


    static std::string parseString(const napi_env &env, const napi_value &value);
    static bool parseBool(const napi_env &env, const napi_value &value);
};
