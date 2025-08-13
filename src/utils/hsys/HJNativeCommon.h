#pragma once

#include "stdint.h"
#define NAPI_RETVAL_NOTHING

#define GET_AND_THROW_LAST_ERROR(env)                                                                   \
    do {                                                                                                \
        const napi_extended_error_info* errorInfo = nullptr;                                            \
        napi_get_last_error_info((env), &errorInfo);                                                    \
        bool isPending = false;                                                                         \
        napi_is_exception_pending((env), &isPending);                                                   \
        if (!isPending && errorInfo != nullptr) {                                                       \
            const char* errorMessage =                                                                  \
                errorInfo->error_message != nullptr ? errorInfo->error_message : "empty error message"; \
            napi_throw_error((env), nullptr, errorMessage);                                             \
        }                                                                                               \
    } while (0)

#define NAPI_ASSERT_BASE(env, assertion, message, retVal)                                    \
    do {                                                                                     \
        if (!(assertion)) {                                                                  \
            napi_throw_error((env), nullptr, "assertion (" #assertion ") failed: " message); \
            return retVal;                                                                   \
        }                                                                                    \
    } while (0)

#define NAPI_ASSERT(env, assertion, message) NAPI_ASSERT_BASE(env, assertion, message, nullptr)

#define NAPI_ASSERT_RETURN_VOID(env, assertion, message) NAPI_ASSERT_BASE(env, assertion, message, NAPI_RETVAL_NOTHING)

#define NAPI_CALL_BASE(env, theCall, retVal) \
    do {                                     \
        if ((theCall) != napi_ok) {          \
            GET_AND_THROW_LAST_ERROR((env)); \
            return retVal;                   \
        }                                    \
    } while (0)

#define NAPI_CALL(env, theCall) NAPI_CALL_BASE(env, theCall, nullptr)

#define NAPI_CALL_RETURN_VOID(env, theCall) NAPI_CALL_BASE(env, theCall, NAPI_RETVAL_NOTHING)

#define DECLARE_NAPI_PROPERTY(name, val)                                       \
    {                                                                          \
        (name), nullptr, nullptr, nullptr, nullptr, val, napi_default, nullptr \
    }

#define DECLARE_NAPI_STATIC_PROPERTY(name, val)                               \
    {                                                                         \
        (name), nullptr, nullptr, nullptr, nullptr, val, napi_static, nullptr \
    }

#define DECLARE_NAPI_FUNCTION(name, func)                                         \
    {                                                                             \
        (name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr \
    }

#define DECLARE_NAPI_FUNCTION_VAL(name, func)                                         \
    {                                                                             \
        #name, nullptr, func, nullptr, nullptr, nullptr, napi_default, nullptr \
    }

#define DECLARE_NAPI_FUNCTION_WITH_DATA(name, func, data)                         \
    {                                                                             \
        (name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, data    \
    }

#define DECLARE_NAPI_STATIC_FUNCTION(name, func)                                 \
    {                                                                            \
        (name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr \
    }

#define DECLARE_NAPI_GETTER(name, getter)                                           \
    {                                                                               \
        (name), nullptr, nullptr, (getter), nullptr, nullptr, napi_default, nullptr \
    }

#define DECLARE_NAPI_SETTER(name, setter)                                           \
    {                                                                               \
        (name), nullptr, nullptr, nullptr, (setter), nullptr, napi_default, nullptr \
    }

#define DECLARE_NAPI_GETTER_SETTER(name, getter, setter)                             \
    {                                                                                \
        (name), nullptr, nullptr, (getter), (setter), nullptr, napi_default, nullptr \
    }

#define NAPI_FUNCTION_DEFINE(name) \
    static napi_value name(napi_env env, napi_callback_info info);

#define NAPI_FUNCTION_IMPL(classfuncName) \   
    napi_value classfuncName(napi_env env, napi_callback_info info)


#define NAPI_INT_RET \    
    napi_value napiRet; \
    napi_create_int32(env, (int)i_err, &napiRet); \
    return napiRet;    

#define NAPI_PARSE_ARGS(argCount) \   
    size_t argc = argCount; \
    napi_value args[argCount]; \
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr); 


constexpr int32_t STR_DEFAULT_SIZE = 2048;
constexpr int32_t INDEX_0 = 0;
constexpr int32_t INDEX_1 = 1;
constexpr int32_t INDEX_2 = 2;
constexpr int32_t INDEX_3 = 3;
constexpr int32_t INDEX_4 = 4;
constexpr int32_t INDEX_5 = 5;
constexpr int32_t INDEX_6 = 6;
constexpr int32_t PARAM_COUNT_1 = 1;
constexpr int32_t PARAM_COUNT_2 = 2;
constexpr int32_t PARAM_COUNT_3 = 3;
constexpr int32_t PARAM_COUNT_4 = 4;
constexpr int32_t PARAM_COUNT_5 = 5;
constexpr int32_t PARAM_COUNT_6 = 6;
constexpr int32_t PARAM_COUNT_7 = 7;