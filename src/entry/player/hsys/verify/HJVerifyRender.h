#pragma once

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <napi/native_api.h>
#include <string>
#include <unordered_map>
#include "HJPlayerInterface.h"
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <native_buffer/native_buffer.h>


const unsigned int LOG_PRINT_DOMAIN = 0xFF00;
const std::string S_XCOM_ID = "PLAYER_XCOMPONENT_ID";

using RenderFuncChange = std::function<void(void *window, int width, int height)>;
namespace NativeXComponentSample {

class HJVerifyRender {
public:
    explicit HJVerifyRender(const std::string& id);
    ~HJVerifyRender() {
    }
    
    int test(NativeWindow *&o_window, int &o_width, int &o_height);
        
    static void onSurfaceChanged();
         
    static HJVerifyRender* GetInstance(const std::string& id);
    static void Release(std::string& id);
    static napi_value NapiDrawPattern(napi_env env, napi_callback_info info);
    static napi_value TestGetXComponentStatus(napi_env env, napi_callback_info info);
    static napi_value NapiLoadYuv(napi_env env, napi_callback_info info);
    void Export(napi_env env, napi_value exports);
    void OnSurfaceChanged(OH_NativeXComponent* component, void* window);
    void OnTouchEvent(OH_NativeXComponent* component, void* window);
    void OnMouseEvent(OH_NativeXComponent* component, void* window);
    void OnHoverEvent(OH_NativeXComponent* component, bool isHover);
    void OnFocusEvent(OH_NativeXComponent* component, void* window);
    void OnBlurEvent(OH_NativeXComponent* component, void* window);
    void OnKeyEvent(OH_NativeXComponent* component, void* window);
    void RegisterCallback(OH_NativeXComponent* nativeXComponent);
    
    void setRenderChangeFunc(RenderFuncChange i_func)
    {
        m_funcChange = i_func;
    }
    
public:
    static std::unordered_map<std::string, HJVerifyRender*> m_instance;
    static int32_t m_hasChangeColor;
    static int32_t m_hasDraw;
    std::string m_id;

    
private:
    static RenderFuncChange m_funcChange;
    OH_NativeXComponent_Callback m_renderCallback;
    OH_NativeXComponent_MouseEvent_Callback m_mouseCallback;
};
} // namespace NativeXComponentSample
