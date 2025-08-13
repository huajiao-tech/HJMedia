#pragma once

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <napi/native_api.h>
#include <string>
#include <unordered_map>
#include "HJVerifyRender.h"
#include "HJNAPITestLive.h"

NS_HJ_USING

namespace NativeXComponentSample {
class HJVerifyManager {
public:
    ~HJVerifyManager();

    static HJVerifyManager* GetInstance() {
        return &HJVerifyManager::m_HJVerifyManager;
    }
    static napi_value GetContext(napi_env env, napi_callback_info info);
    
    static napi_value testOpen(napi_env env, napi_callback_info info);
    static napi_value testClose(napi_env env, napi_callback_info info);
    
    static napi_value tryOpenPreview(napi_env env, napi_callback_info info);
    static napi_value tryOpenPush(napi_env env, napi_callback_info info);    
    static napi_value tryClosePreview(napi_env env, napi_callback_info info);
    static napi_value tryOpenRecord(napi_env env, napi_callback_info info);
    static napi_value tryCloseRecord(napi_env env, napi_callback_info info);
    
    void SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent);
    HJVerifyRender* GetRender(const std::string& id);
    void Export(napi_env env, napi_value exports);

private:
    static void priTest();
    static void priTest(HJVerifyRender* render);
    //static void priTimerRestartRecorder(HJVerifyRender* render);
    static void priConstructParam(HJPusherVideoInfo& o_videoInfo, HJPusherAudioInfo& o_audioInfo, HJPusherRTMPInfo& o_rtmpInfo);
    static HJVerifyManager m_HJVerifyManager;
    
    
    static HJTimerThreadPool::Ptr m_testThreadTimer; 
    static HJThreadPool::Ptr m_testThreadPool;
    static HJNAPITestLive::Ptr m_testLiveStream;
    
    static int s_videoCodecId;
    static int s_previewFps;
    static int s_width;
    static int s_height;
    static int s_videobitrate;
    static int s_framerate;
    static int s_gopsize;
    
    static int s_audioCodecId;
    static int s_audiobitrate;
    static int s_fmt;
    static int s_samplerate;
    static int s_channels;
    
    static std::string s_rtmpUrl;
    static std::string s_localUrl;
    
    std::unordered_map<std::string, OH_NativeXComponent*> m_nativeXComponentMap;
    std::unordered_map<std::string, HJVerifyRender*> m_HJVerifyRenderMap;
};
} // namespace NativeXComponentSample
