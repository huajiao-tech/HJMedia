/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <hilog/log.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <string>
#include "HJTransferInfo.h"
#include "HJVerifyRender.h"
#include "HJVerifyManager.h"
#include "HJMediaInfo.h"

NS_HJ_USING

namespace NativeXComponentSample
{
    std::unordered_map<std::string, HJVerifyRender *> HJVerifyRender::m_instance;
    int32_t HJVerifyRender::m_hasDraw = 0;
    int32_t HJVerifyRender::m_hasChangeColor = 0;

    static void *m_window = nullptr;
    static int m_windowWidth = 0;
    static int m_windowHeight = 0;

    HJ::HJNAPILiveStream::Ptr HJVerifyRender::m_livestream = nullptr;

    int HJVerifyRender::test(NativeWindow *&o_window, int &o_width, int &o_height)
    {
        o_window = (NativeWindow *)m_window;
        o_width = m_windowWidth;
        o_height = m_windowHeight;
        if (!m_window || m_windowWidth == 0 || m_windowHeight == 0)
        {
            return -1;
        }
        return 0;
    }

///////////////////////////////////////////////////////////////////////////////////

    int HJVerifyRender::contextInit(const HJPusherContextInfo& i_contextInfo)
    {
        return HJ::HJNAPILiveStream::contextInit(i_contextInfo);
    }

    int HJVerifyRender::openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIPusherNotify i_notify, uint64_t &o_surfaceId)
    {
        int i_err = 0;
        do
        {
            if (nullptr == m_livestream)
            {
                m_livestream = HJ::HJNAPILiveStream::Create();
            }
            i_err = m_livestream->openPreview(i_previewInfo, i_notify, o_surfaceId);
            if (i_err < 0)
            {
                break;
            }
            if (m_window)
            {
                i_err = m_livestream->setNativeWindow(m_window, m_windowWidth, m_windowHeight, HJTargetState_Create);
                if (i_err < 0)
                {
                    break;
                }
            }
        } while (false);
        return i_err;
    }
    int HJVerifyRender::openRecorder(const HJPusherRecordInfo &i_recorderInfo)
    {
        int i_err = 0;
        if (m_livestream)
        {
            i_err = m_livestream->openRecorder(i_recorderInfo);
        }
        return i_err;
    }
    void HJVerifyRender::closeRecorder()
    {
        if (m_livestream)
        {
            m_livestream->closeRecorder();
        }
    }
    void HJVerifyRender::closePreview()
    {
        if (m_livestream)
        {
            m_livestream->closePreview();
            m_livestream = nullptr;
        }
    }
    int HJVerifyRender::openPusher(const HJPusherVideoInfo& i_videoInfo, const HJPusherAudioInfo& i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo)
    {
        int i_err = 0;
        do
        {
            if (nullptr == m_livestream)
            {
                i_err = -1;
                break;
            }
            i_err = m_livestream->openPusher(i_videoInfo, i_audioInfo, i_rtmpInfo);
            if (i_err < 0)
            {
                break;
            }
        } while (false);
        return i_err;
    }
    void HJVerifyRender::closePusher()
    {
        if (m_livestream)
        {
            m_livestream->closePusher();
        }
    }
    int HJVerifyRender::openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo)
    {
        int i_err = 0;
        do
        {
            if (nullptr == m_livestream)
            {
                i_err = -1;
                break;
            }
            i_err = m_livestream->openPngSeq(i_pngseqInfo);
            if (i_err < 0)
            {
                break;
            }
        } while (false);
        return i_err;
    }

///////////////////////////////////////////////////////////////////////////////////

    napi_value HJVerifyRender::NapiDrawPattern(napi_env env, napi_callback_info info)
    {

        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiDrawPattern");
        if ((env == nullptr) || (info == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiDrawPattern: env or info is null");
            return nullptr;
        }

        napi_value thisArg;
        if (napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, nullptr) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiDrawPattern: napi_get_cb_info fail");
            return nullptr;
        }

        napi_value exportInstance;
        if (napi_get_named_property(env, thisArg, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender",
                         "NapiDrawPattern: napi_get_named_property fail");
            return nullptr;
        }

        OH_NativeXComponent *nativeXComponent = nullptr;
        if (napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiDrawPattern: napi_unwrap fail");
            return nullptr;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender",
                         "NapiDrawPattern: Unable to get XComponent id");
            return nullptr;
        }

        std::string id(idStr);
        HJVerifyRender *render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "render->eglCore_->Draw() executed");
        }
        return nullptr;
    }

    void OnSurfaceChangedCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "OnSurfaceChangedCB");
        if ((component == nullptr) || (window == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "OnSurfaceChangedCB: component or window is null");
            return;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "OnSurfaceChangedCB: Unable to get XComponent id");
            return;
        }

        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            render->OnSurfaceChanged(component, window);
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "surface changed");
        }
    }

    void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window)
    {

        m_window = nullptr;
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "OnSurfaceDestroyedCB");
        if ((component == nullptr) || (window == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "OnSurfaceDestroyedCB: component or window is null");
            return;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "OnSurfaceDestroyedCB: Unable to get XComponent id");
            return;
        }

        std::string id(idStr);
        HJVerifyRender::Release(id);
    }

    void DispatchTouchEventCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "DispatchTouchEventCB");
        if ((component == nullptr) || (window == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "DispatchTouchEventCB: component or window is null");
            return;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "DispatchTouchEventCB: Unable to get XComponent id");
            return;
        }

        std::string id(idStr);
        HJVerifyRender *render = HJVerifyRender::GetInstance(id);
        if (render != nullptr && id == S_XCOM_ID)
        {
            render->OnTouchEvent(component, window);
        }
    }

    void DispatchMouseEventCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "DispatchMouseEventCB");
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        int32_t ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
        if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            return;
        }

        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            render->OnMouseEvent(component, window);
        }
    }

    void DispatchHoverEventCB(OH_NativeXComponent *component, bool isHover)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "DispatchHoverEventCB");
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        int32_t ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
        if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            return;
        }

        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            render->OnHoverEvent(component, isHover);
        }
    }

    void OnFocusEventCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "OnFocusEventCB");
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        int32_t ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
        if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            return;
        }

        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            render->OnFocusEvent(component, window);
        }
    }

    void OnBlurEventCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "OnBlurEventCB");
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        int32_t ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
        if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            return;
        }

        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            render->OnBlurEvent(component, window);
        }
    }

    void OnKeyEventCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "OnKeyEventCB");
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        int32_t ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
        if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            return;
        }
        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            render->OnKeyEvent(component, window);
        }
    }

    HJVerifyRender::HJVerifyRender(const std::string &id)
    {
        this->m_id = id;
    }

    HJVerifyRender *HJVerifyRender::GetInstance(const std::string &id)
    {
        if (m_instance.find(id) == m_instance.end())
        {
            HJVerifyRender *instance = new HJVerifyRender(id);
            m_instance[id] = instance;
            return instance;
        }
        else
        {
            return m_instance[id];
        }
    }

    void HJVerifyRender::Export(napi_env env, napi_value exports)
    {
        if ((env == nullptr) || (exports == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "Export: env or exports is null");
            return;
        }

        napi_property_descriptor desc[] = {
            {"drawPattern", nullptr, HJVerifyRender::NapiDrawPattern, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"getStatus", nullptr, HJVerifyRender::TestGetXComponentStatus, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"loadYuv", nullptr, HJVerifyRender::NapiLoadYuv, nullptr, nullptr, nullptr, napi_default, nullptr},
        };
        if (napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "Export: napi_define_properties failed");
        }
    }

    void HJVerifyRender::onSurfaceChanged()
    {
    }

    void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "OnSurfaceCreatedCB");
        if ((component == nullptr) || (window == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "OnSurfaceCreatedCB: component or window is null");
            return;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "OnSurfaceCreatedCB: Unable to get XComponent id");
            return;
        }

        std::string id(idStr);
        auto render = HJVerifyRender::GetInstance(id);
        uint64_t width;
        uint64_t height;
        int32_t xSize = OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback", "widthandheight %{public}d %{public}d", width, height);
        if ((xSize == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) && (render != nullptr))
        {

            m_window = window;
            m_windowWidth = width;
            m_windowHeight = height;
        }
    }

    void HJVerifyRender::Release(std::string &id)
    {
        HJVerifyRender *render = HJVerifyRender::GetInstance(id);
        if (render != nullptr)
        {
            delete render;
            render = nullptr;
            m_instance.erase(m_instance.find(id));
        }
    }

    void HJVerifyRender::OnSurfaceChanged(OH_NativeXComponent *component, void *window)
    {
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback", "OnSurfaceChanged: Unable to get XComponent id");
            return;
        }

        std::string id(idStr);
        HJVerifyRender *render = HJVerifyRender::GetInstance(id);
        double offsetX;
        double offsetY;
        OH_NativeXComponent_GetXComponentOffset(component, window, &offsetX, &offsetY);

        uint64_t width;
        uint64_t height;
        OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);
        if (render != nullptr)
        {
#if USE_ORGIN_EGL_RENDER
            render->m_eglCore->UpdateSize(width, height);
#endif
        }

        // fixme the thread is not safe
        m_window = window;
        m_windowWidth = width;
        m_windowHeight = height;
        onSurfaceChanged();

        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "OH_NativeXComponent_GetXComponentOffset",
                     "offsetX = %{public}lf, offsetY = %{public}lf w:%{public}d h:%{public}d", offsetX, offsetY, width, height);
    }

    void HJVerifyRender::OnTouchEvent(OH_NativeXComponent *component, void *window)
    {
        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Callback",
                         "DispatchTouchEventCB: Unable to get XComponent id");
            return;
        }
        OH_NativeXComponent_TouchEvent touchEvent;
        OH_NativeXComponent_GetTouchEvent(component, window, &touchEvent);
        std::string id(idStr);
        HJVerifyRender *render = HJVerifyRender::GetInstance(id);
        if (render != nullptr && touchEvent.type == OH_NativeXComponent_TouchEventType::OH_NATIVEXCOMPONENT_UP)
        {
        }
        float tiltX = 0.0f;
        float tiltY = 0.0f;
        OH_NativeXComponent_TouchPointToolType toolType =
            OH_NativeXComponent_TouchPointToolType::OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN;
        OH_NativeXComponent_GetTouchPointToolType(component, 0, &toolType);
        OH_NativeXComponent_GetTouchPointTiltX(component, 0, &tiltX);
        OH_NativeXComponent_GetTouchPointTiltY(component, 0, &tiltY);
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "OnTouchEvent",
                     "touch info: toolType = %{public}d, tiltX = %{public}lf, tiltY = %{public}lf", toolType, tiltX, tiltY);
    }

    void HJVerifyRender::RegisterCallback(OH_NativeXComponent *nativeXComponent)
    {
        m_renderCallback.OnSurfaceCreated = OnSurfaceCreatedCB;
        m_renderCallback.OnSurfaceChanged = OnSurfaceChangedCB;
        m_renderCallback.OnSurfaceDestroyed = OnSurfaceDestroyedCB;
        m_renderCallback.DispatchTouchEvent = DispatchTouchEventCB;
        OH_NativeXComponent_RegisterCallback(nativeXComponent, &m_renderCallback);

        m_mouseCallback.DispatchMouseEvent = DispatchMouseEventCB;
        m_mouseCallback.DispatchHoverEvent = DispatchHoverEventCB;
        OH_NativeXComponent_RegisterMouseEventCallback(nativeXComponent, &m_mouseCallback);

        OH_NativeXComponent_RegisterFocusEventCallback(nativeXComponent, OnFocusEventCB);
        OH_NativeXComponent_RegisterKeyEventCallback(nativeXComponent, OnKeyEventCB);
        OH_NativeXComponent_RegisterBlurEventCallback(nativeXComponent, OnBlurEventCB);
    }

    void HJVerifyRender::OnMouseEvent(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "OnMouseEvent");
        OH_NativeXComponent_MouseEvent mouseEvent;
        int32_t ret = OH_NativeXComponent_GetMouseEvent(component, window, &mouseEvent);
        if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender",
                         "MouseEvent Info: x = %{public}f, y = %{public}f, action = %{public}d, button = %{public}d",
                         mouseEvent.x, mouseEvent.y, mouseEvent.action, mouseEvent.button);
        }
        else
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "GetMouseEvent error");
        }
    }

    void HJVerifyRender::OnHoverEvent(OH_NativeXComponent *component, bool isHover)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "OnHoverEvent isHover_ = %{public}d", isHover);
    }

    void HJVerifyRender::OnFocusEvent(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "OnFocusEvent");
    }

    void HJVerifyRender::OnBlurEvent(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "OnBlurEvent");
    }

    void HJVerifyRender::OnKeyEvent(OH_NativeXComponent *component, void *window)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "OnKeyEvent");

        OH_NativeXComponent_KeyEvent *keyEvent = nullptr;
        if (OH_NativeXComponent_GetKeyEvent(component, &keyEvent) >= 0)
        {
            OH_NativeXComponent_KeyAction action;
            OH_NativeXComponent_GetKeyEventAction(keyEvent, &action);
            OH_NativeXComponent_KeyCode code;
            OH_NativeXComponent_GetKeyEventCode(keyEvent, &code);
            OH_NativeXComponent_EventSourceType sourceType;
            OH_NativeXComponent_GetKeyEventSourceType(keyEvent, &sourceType);
            int64_t deviceId;
            OH_NativeXComponent_GetKeyEventDeviceId(keyEvent, &deviceId);
            int64_t timeStamp;
            OH_NativeXComponent_GetKeyEventTimestamp(keyEvent, &timeStamp);
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender",
                         "KeyEvent Info: action=%{public}d, code=%{public}d, sourceType=%{public}d, deviceId=%{public}ld, "
                         "timeStamp=%{public}ld",
                         action, code, sourceType, deviceId, timeStamp);
        }
        else
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "GetKeyEvent error");
        }
    }

    napi_value HJVerifyRender::TestGetXComponentStatus(napi_env env, napi_callback_info info)
    {
        napi_value hasDraw;
        napi_value hasChangeColor;

        napi_status ret = napi_create_int32(env, m_hasDraw, &(hasDraw));
        if (ret != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "TestGetXComponentStatus",
                         "napi_create_int32 hasDraw_ error");
            return nullptr;
        }
        ret = napi_create_int32(env, m_hasChangeColor, &(hasChangeColor));
        if (ret != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "TestGetXComponentStatus",
                         "napi_create_int32 hasChangeColor_ error");
            return nullptr;
        }

        napi_value obj;
        ret = napi_create_object(env, &obj);
        if (ret != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "TestGetXComponentStatus", "napi_create_object error");
            return nullptr;
        }
        ret = napi_set_named_property(env, obj, "hasDraw", hasDraw);
        if (ret != napi_ok)
        {
            OH_LOG_Print(
                LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "TestGetXComponentStatus", "napi_set_named_property hasDraw error");
            return nullptr;
        }
        ret = napi_set_named_property(env, obj, "hasChangeColor", hasChangeColor);
        if (ret != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "TestGetXComponentStatus",
                         "napi_set_named_property hasChangeColor error");
            return nullptr;
        }
        return obj;
    }

    // NAPI registration method type napi_callback. If no value is returned, nullptr is returned.
    napi_value HJVerifyRender::NapiLoadYuv(napi_env env, napi_callback_info info)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiLoadYuv");
        if ((nullptr == env) || (nullptr == info))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiLoadYuv: env or info is null");
            return nullptr;
        }

        napi_value thisArg;
        if (napi_ok != napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiLoadYuv: napi_get_cb_info fail");
            return nullptr;
        }

        napi_value exportInstance;
        if (napi_ok != napi_get_named_property(env, thisArg, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiLoadYuv: napi_get_named_property fail");
            return nullptr;
        }

        OH_NativeXComponent *nativeXComponent = nullptr;
        if (napi_ok != napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiLoadYuv: napi_unwrap fail");
            return nullptr;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NATIVEXCOMPONENT_RESULT_SUCCESS != OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyRender", "NapiLoadYuv: Unable to get XComponent id");
            return nullptr;
        }

        std::string id(idStr);
        HJVerifyRender *render = HJVerifyRender::GetInstance(id);
        if (render)
        {
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "HJVerifyRender", "render->m_eglCore->LoadYuv() executed");
        }
        return nullptr;
    }

} // namespace NativeXComponentSample
