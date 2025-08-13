#include "HJNapiUtils.h"
#include <codecvt>
#include <cstdio>
#include <locale>
#include <string>
#include <string.h>
#include "HJOHosLog.h"

const int32_t MAX_STR_LENGTH = 2048;
void NapiUtil::JsValueToString(const napi_env & env, const napi_value & value, const int32_t bufLen, std::string & target)
{
//    if (bufLen <= 0 || bufLen > MAX_STR_LENGTH) {
//        LOGI("%s string too long malloc failed", __func__);
//        return;
//    }

    std::unique_ptr < char[] > buf = std::make_unique < char[] >(bufLen);
    if (buf.get() == nullptr) {
        LOGI("%s nullptr js object to string malloc failed", __func__);
        return;
    }
    (void)memset(buf.get(), 0, bufLen);
    size_t result = 0;
    napi_get_value_string_utf8(env, value, buf.get(), bufLen, &result);
    target = buf.get();
}

napi_value NapiUtil::SetNapiCallInt32(const napi_env & env,  const int32_t intValue)
{
    napi_value result;
    napi_create_int32(env, intValue, &result);
    return result;
}

napi_value NapiUtil::SetNapiCallBool(napi_env env, bool value)
{
    napi_value temp;
    napi_value result;
    napi_create_int32(env, value == true ? 1 : 0, &temp);
    napi_coerce_to_bool(env, temp, &result);
    return result;
}

int NapiUtil::StringToInt(std::string value){
    return atoi((char*)value.c_str());
}

int NapiUtil::StringToLong(std::string value){
    return atol((char*)value.c_str());
}

float NapiUtil::StringToFloat(std::string value){
    return std::stof(value);
}

bool NapiUtil::StringToBool(const std::string value){
    return value=="true" ? true:false;
}

