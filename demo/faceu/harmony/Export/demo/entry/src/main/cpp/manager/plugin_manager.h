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
#ifndef NATIVE_XCOMPONENT_PLUGIN_MANAGER_H
#define NATIVE_XCOMPONENT_PLUGIN_MANAGER_H

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <napi/native_api.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "../render/plugin_render.h"
#include "FrameLoopThread.h"
#include "HJOGFBOCtrl.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

class FrameLoopThread;
class HJOGEGLCore;
class HJOGCopyShaderStrip;
class HJOGFBOCtrl;
namespace NativeXComponentSample {
class PluginManager {
public:
    ~PluginManager();

    static PluginManager* GetInstance() {
        return &PluginManager::m_pluginManager;
    }

    static napi_value GetContext(napi_env env, napi_callback_info info);
    static napi_value nFaceOpen(napi_env env, napi_callback_info info);
    static napi_value nFaceClose(napi_env env, napi_callback_info info);
    static napi_value nFaceuAdd(napi_env env, napi_callback_info info);
    static napi_value nFaceuRemove(napi_env env, napi_callback_info info);
    static napi_value nFaceuRemoveAll(napi_env env, napi_callback_info info);
    static napi_value nFaceuSetImgSeqVisible(napi_env env, napi_callback_info info);
    
    void SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent);
    PluginRender* GetRender(const std::string& id);
    void Export(napi_env env, napi_value exports);

private:
    static void priHJFaceuCallback(const char* i_uniqueKey, int i_type);
    static void draw(int i_targetW, int i_targetH);
    static void ensureFaceu();
    static void destroyFaceu();
    static PluginManager m_pluginManager;
    static std::string s_resourcePath;
    static std::shared_ptr<HJOGEGLCore> s_eglCore;
    static std::shared_ptr<FrameLoopThread> s_thread;
    static GLuint m_textureImgSeqId;
    static void *m_faceuHandle;
    static int m_faceuWidth;
    static int m_faceuHeight;
    static bool m_initialized;
    static bool m_showImgSeq;
    static std::shared_ptr<HJOGFBOCtrl> m_fbo;
    static std::shared_ptr<HJOGCopyShaderStrip> m_copyShader;
    std::unordered_map<std::string, OH_NativeXComponent*> m_nativeXComponentMap;
    std::unordered_map<std::string, PluginRender*> m_pluginRenderMap;
};
} // namespace NativeXComponentSample
#endif // NATIVE_XCOMPONENT_PLUGIN_MANAGER_H
