#pragma once

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <napi/native_api.h>
#include <string>
#include <unordered_map>
#include "HJNAPILiveStream.h"
#include "HJPusherInterface.h"
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <native_buffer/native_buffer.h>


const unsigned int LOG_PRINT_DOMAIN = 0xFF00;
const std::string S_XCOM_ID = "opengl_xcomponent";

namespace NativeXComponentSample {

class HJVerifyRender {
public:
    explicit HJVerifyRender(const std::string& id);
    ~HJVerifyRender() {
    }
    
    int test(NativeWindow *&o_window, int &o_width, int &o_height);
     
    static int contextInit(const HJEntryContextInfo& i_contextInfo);
    int openPreview(const HJPusherPreviewInfo &i_previewInfo, HJNAPIEntryNotify i_notify, uint64_t &o_surfaceId);
    int openPusher(const HJPusherVideoInfo& i_videoInfo, const HJPusherAudioInfo& i_audioInfo, const HJPusherRTMPInfo &i_rtmpInfo); 
    void closePusher();
    int openPngSeq(const HJPusherPNGSegInfo &i_pngseqInfo);
    int openRecorder(const HJPusherRecordInfo &i_recorderInfo);
    void closeRecorder();
    void closePreview();
    void tryGiftOpen();
    void tryDoubleScreen();
    void tryGiftPusher();
    HJ::HJNAPILiveStream::Ptr getLiveStream();

    
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

public:
    static std::unordered_map<std::string, HJVerifyRender*> m_instance;
    static int32_t m_hasChangeColor;
    static int32_t m_hasDraw;
    std::string m_id;

    
private:
    static HJ::HJNAPILiveStream::Ptr m_livestream;
    
    OH_NativeXComponent_Callback m_renderCallback;
    OH_NativeXComponent_MouseEvent_Callback m_mouseCallback;
};
} // namespace NativeXComponentSample
