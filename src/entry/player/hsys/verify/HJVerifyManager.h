#pragma once

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <napi/native_api.h>
#include <string>
#include <unordered_map>
#include "HJVerifyRender.h"
#include "HJNAPITestPlayer.h"
#include "HJNAPIPlayer.h"
#include "HJThreadPool.h"

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
    static void priTestClose();
    static napi_value testEffectGray(napi_env env, napi_callback_info info);
    static napi_value testEffectBlur(napi_env env, napi_callback_info info);
    
    static napi_value tryOpen(napi_env env, napi_callback_info info);
    static napi_value tryMute(napi_env env, napi_callback_info info);
    static napi_value tryOpenImgReceiver(napi_env env, napi_callback_info info);
    static napi_value tryCloseImageReceiver(napi_env env, napi_callback_info info);
    static napi_value tryClose(napi_env env, napi_callback_info info);
    static napi_value tryGetMediaData(napi_env env, napi_callback_info info);
    static napi_value trySetFacePoints(napi_env env, napi_callback_info info);
    
    static int priTryStart(void *window, int width, int height);
    static int priTryStop();
//    static napi_value tryOpenPreview(napi_env env, napi_callback_info info);
//    static napi_value tryOpenPush(napi_env env, napi_callback_info info);    

    
    void SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent);
    HJVerifyRender* GetRender(const std::string& id);
    void Export(napi_env env, napi_value exports);

private:
    static void priTest();
    static void priTryExit(HJNAPIPlayer::Wtr i_Player);
    
    static HJVerifyManager m_HJVerifyManager;
    
    
    static HJTimerThreadPool::Ptr m_testThreadTimer; 
    static HJThreadPool::Ptr m_testThreadPool;
    static HJNAPITestPlayer::Ptr m_testPlayer;
    
    static std::deque<HJNAPIPlayer::Ptr> m_playerActiveQueue;
    static std::deque<HJNAPIPlayer::Ptr> m_playerDisposeQueue;
    static HJTimerThreadPool::Ptr m_playerThread;
    static HJThreadPool::Ptr m_exitThread;
    static std::atomic<bool> m_bQuit;
    static int m_restartTime;
    static int s_logCnt;
    
    static std::string s_path;
    static std::string s_url;
    static int s_fps;
    static int s_videoCodecType;
    static int s_sourceType;
    static std::string s_faceuUrl;
    
    std::unordered_map<std::string, OH_NativeXComponent*> m_nativeXComponentMap;
    std::unordered_map<std::string, HJVerifyRender*> m_HJVerifyRenderMap;
};
} // namespace NativeXComponentSample
