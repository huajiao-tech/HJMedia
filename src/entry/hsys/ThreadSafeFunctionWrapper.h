#pragma once

#include <napi/native_api.h>
#include <cstring>
#include <string>

#include "HJFLog.h"
#include "HJJson.h"
#include "HJPrerequisites.h"
#include "HJSEIWrapper.h"

NS_HJ_BEGIN

struct HJPlayerSeiCallbackData final {
    bool isKeyFrame{false};
    std::vector<HJSEIData> seiDatas;
};

// Node-API thread-safe function wrapper.
class ThreadSafeFunctionWrapper {
public:
    using CallJsCallbackType = void(*)(napi_env env, napi_value jsCb, void* context, void* data);

    ThreadSafeFunctionWrapper() = default;

    ThreadSafeFunctionWrapper(napi_env env, napi_value jsCallback, CallJsCallbackType callbackFunc = CallJsCallback,
                              const std::string& resourceName = "ThreadSafeFunction") : m_env(env) {
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

    ThreadSafeFunctionWrapper(const ThreadSafeFunctionWrapper&) = delete;
    ThreadSafeFunctionWrapper& operator=(const ThreadSafeFunctionWrapper&) = delete;

    ThreadSafeFunctionWrapper(ThreadSafeFunctionWrapper&& other) noexcept
        : m_env(other.m_env), m_tsfn(other.m_tsfn), js_callback_ref_(other.js_callback_ref_) {
        other.m_tsfn = nullptr;
        other.js_callback_ref_ = nullptr;
        other.m_env = nullptr;
    }

    ThreadSafeFunctionWrapper& operator=(ThreadSafeFunctionWrapper&& other) noexcept {
        if (this != &other) {
            if (m_tsfn != nullptr) {
                napi_release_threadsafe_function(m_tsfn, napi_tsfn_release);
            }
            if (js_callback_ref_ != nullptr && m_env != nullptr) {
                napi_delete_reference(m_env, js_callback_ref_);
            }
            m_tsfn = other.m_tsfn;
            m_env = other.m_env;
            js_callback_ref_ = other.js_callback_ref_;
            other.m_tsfn = nullptr;
            other.m_env = nullptr;
            other.js_callback_ref_ = nullptr;
        }
        return *this;
    }

    static void CallJsCallback(napi_env env, napi_value jsCb, void* context, void* data) {
        if (env == nullptr) {
            return;
        }

        auto callbackData = reinterpret_cast<HJYJsonDocument*>(data);

        napi_value argv[1];
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

        void* arraybuffer_data = nullptr;
        napi_create_arraybuffer(env, callbackData->second, &arraybuffer_data, &argv[0]);
        if (arraybuffer_data != nullptr && callbackData->second > 0) {
            memcpy(arraybuffer_data, dataPtr, callbackData->second);
        }

        delete callbackData;

        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_call_function(env, undefined, jsCb, 1, argv, nullptr);
    }

    static void playerSeiCallback(napi_env env, napi_value jsCb, void* context, void* data) {
        if (env == nullptr) {
            return;
        }

        auto callbackData = static_cast<HJPlayerSeiCallbackData*>(data);

        napi_value resultObj;
        napi_create_object(env, &resultObj);

        napi_value isKeyFrame;
        napi_get_boolean(env, callbackData->isKeyFrame, &isKeyFrame);
        napi_set_named_property(env, resultObj, "isKeyFrame", isKeyFrame);

        napi_value seiDatas;
        napi_create_array_with_length(env, callbackData->seiDatas.size(), &seiDatas);

        for (size_t i = 0; i < callbackData->seiDatas.size(); ++i) {
            const auto& item = callbackData->seiDatas[i];

            napi_value itemObj;
            napi_create_object(env, &itemObj);

            napi_value uuid;
            napi_create_string_utf8(env, item.uuid.c_str(), item.uuid.length(), &uuid);
            napi_set_named_property(env, itemObj, "uuid", uuid);

            void* arrayBufferData = nullptr;
            napi_value arrayBuffer = nullptr;
            napi_create_arraybuffer(env, item.data.size(), &arrayBufferData, &arrayBuffer);
            if (arrayBufferData != nullptr && !item.data.empty()) {
                memcpy(arrayBufferData, item.data.data(), item.data.size());
            }
            napi_set_named_property(env, itemObj, "data", arrayBuffer);

            napi_set_element(env, seiDatas, static_cast<uint32_t>(i), itemObj);
        }

        napi_set_named_property(env, resultObj, "seiDatas", seiDatas);

        delete callbackData;

        napi_value argv[1] = {resultObj};
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_call_function(env, undefined, jsCb, 1, argv, nullptr);
    }

    napi_status Call(void* data, napi_threadsafe_function_call_mode mode = napi_tsfn_nonblocking) const {
        if (m_tsfn == nullptr) {
            return napi_invalid_arg;
        }
        return napi_call_threadsafe_function(m_tsfn, data, mode);
    }

    napi_threadsafe_function Get() const {
        return m_tsfn;
    }

    bool IsValid() const {
        return m_tsfn != nullptr;
    }

private:
    napi_env m_env = nullptr;
    napi_threadsafe_function m_tsfn = nullptr;
    napi_ref js_callback_ref_ = nullptr;
};

NS_HJ_END
