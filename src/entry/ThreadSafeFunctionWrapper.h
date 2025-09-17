#pragma once

#include <napi/native_api.h>
#include <string>

#include "HJFLog.h"
#include "HJJson.h"
#include "HJPrerequisites.h"

NS_HJ_BEGIN

// Node-API线程安全函数封装类
class ThreadSafeFunctionWrapper {
public:
    using CallJsCallbackType = void(*)(napi_env env, napi_value jsCb, void* context, void* data);

    ThreadSafeFunctionWrapper() {}

    ThreadSafeFunctionWrapper(napi_env env, napi_value jsCallback, CallJsCallbackType callbackFunc = CallJsCallback,
                              const std::string &resourceName = "ThreadSafeFunction") : m_env(env) {
        napi_valuetype valuetype;
        napi_typeof(env, jsCallback, &valuetype);
        if (valuetype != napi_valuetype::napi_function) {
            napi_throw_type_error(env, nullptr, "napi_function is expected");
        }

        napi_create_reference(env, jsCallback, 1, &js_callback_ref_);

        napi_value resourceNameValue;
        napi_create_string_utf8(env, resourceName.c_str(), NAPI_AUTO_LENGTH, &resourceNameValue);

        napi_create_threadsafe_function(env, jsCallback, nullptr, resourceNameValue, 0, 1, nullptr, nullptr, nullptr,
                                        callbackFunc, &m_tsfn);
    }
    
    ~ThreadSafeFunctionWrapper() {
        if (m_tsfn != nullptr) {
            napi_release_threadsafe_function(m_tsfn, napi_tsfn_release);
        }
        if (js_callback_ref_ != nullptr) {
            napi_delete_reference(m_env, js_callback_ref_);
        }
    }
    
    // 禁止拷贝
    ThreadSafeFunctionWrapper(const ThreadSafeFunctionWrapper&) = delete;
    ThreadSafeFunctionWrapper& operator=(const ThreadSafeFunctionWrapper&) = delete;
    
    // 允许移动
    ThreadSafeFunctionWrapper(ThreadSafeFunctionWrapper&& other) noexcept
        : m_env(other.m_env), m_tsfn(other.m_tsfn) {
        other.m_tsfn = nullptr;
    }

    ThreadSafeFunctionWrapper &operator=(ThreadSafeFunctionWrapper &&other) noexcept {
        if (this != &other) {
            if (m_tsfn != nullptr) {
                napi_release_threadsafe_function(m_tsfn, napi_tsfn_release);
            }
            m_tsfn = other.m_tsfn;
            m_env = other.m_env;
            other.m_tsfn = nullptr;
        }
        return *this;
    }

    static void CallJsCallback(napi_env env, napi_value jsCb, void* context, void* data) {
        if (env == nullptr) {
            return;
        }

        auto callbackData = reinterpret_cast<HJYJsonDocument*>(data);

        napi_value argv[1];
        auto str = callbackData->getSerialInfo();
        napi_create_string_utf8(env, callbackData->getSerialInfo().c_str(), callbackData->getSerialInfo().length(), &argv[0]);

        delete callbackData;

        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_call_function(env, undefined, jsCb, 1, argv, nullptr);
    }

    static void audioDataCallback(napi_env env, napi_value jsCb, void* context, void* data) {
        if (env == nullptr) {
            return;
        }

        auto callbackData = static_cast<std::pair<std::unique_ptr<uint8_t[]>, int>*>(data);

        napi_value argv[1];

        auto dataPtr = callbackData->first.get();

        void* arraybuffer_data;
        napi_create_arraybuffer(env, callbackData->second, &arraybuffer_data, &argv[0]);
        memcpy(arraybuffer_data, dataPtr, callbackData->second);

        delete callbackData;  // 智能指针自动释放，pair手动释放

        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_call_function(env, undefined, jsCb, 1, argv, nullptr);
    }
    
    // 调用JS回调
    napi_status Call(void* data, napi_threadsafe_function_call_mode mode = napi_tsfn_nonblocking) const {
        if (m_tsfn == nullptr) {
            return napi_invalid_arg;
        }
        return napi_call_threadsafe_function(m_tsfn, data, mode);
    }
    
    // 获取原始线程安全函数
    napi_threadsafe_function Get() const {
        return m_tsfn;
    }
    
    // 检查是否有效
    bool IsValid() const {
        return m_tsfn != nullptr;
    }

private:
    napi_env m_env = nullptr;
    napi_threadsafe_function m_tsfn = nullptr;
    napi_ref js_callback_ref_ = nullptr;
};

NS_HJ_END