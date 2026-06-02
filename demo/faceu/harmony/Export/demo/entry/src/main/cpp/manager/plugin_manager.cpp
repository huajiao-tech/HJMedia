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

#include "plugin_manager.h"

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <cstdint>
#include <hilog/log.h>
#include <string>
#include <memory>

#include "../common/common.h"
#include "HJRenderContextExport.h"
#include "FrameLoopThread.h"
#include "HJOGEGLCore.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "egl_core.h"
#include "HJOGCommon.h"
#include "HJOGBaseShader.h"
#include "HJRenderFaceuExport.h"
#include "HJFacePointsMadeup.h"

const static std::string xcom_id = "opengl_xcomponent";

namespace NativeXComponentSample
{

    PluginManager PluginManager::m_pluginManager;
    std::shared_ptr<FrameLoopThread> PluginManager::s_thread = nullptr;
    std::string PluginManager::s_resourcePath = "/data/storage/el1/bundle/entry/resources/resfile/resource/";
    std::shared_ptr<HJOGEGLCore> PluginManager::s_eglCore = nullptr;
    GLuint PluginManager::m_textureImgSeqId = 0;
    void *PluginManager::m_faceuHandle = nullptr;
    std::shared_ptr<HJOGCopyShaderStrip> PluginManager::m_copyShader = nullptr;
    int PluginManager::m_faceuWidth = 720;
    int PluginManager::m_faceuHeight = 1280;
    bool PluginManager::m_initialized = false;
    bool PluginManager::m_showImgSeq = false;
    std::shared_ptr<HJOGFBOCtrl> PluginManager::m_fbo = nullptr;

    void PluginManager::priHJFaceuCallback(const char *i_uniqueKey, int i_type)
    {
        // HJFLogi("HJFaceuCallback: {} i_type:{}", i_uniqueKey, i_type);
        if (i_type == HJ_FACEU_NOTIFY_COMPLETE)
        {
            // HJFLogi("the faceu:{} is complete", i_uniqueKey);
        }
    }

    void PluginManager::ensureFaceu()
    {
        if (m_initialized)
        {
            return;
        }
        m_fbo = std::make_shared<HJOGFBOCtrl>();
        m_fbo->init(m_faceuWidth, m_faceuHeight);
        m_faceuHandle = faceuInit(priHJFaceuCallback, false);
        m_textureImgSeqId = HJOGCommon::textureCreate();
        m_copyShader = std::make_shared<HJOGCopyShaderStrip>();
        m_copyShader->init(OGCopyShaderStripFlag_2D, false);
        m_initialized = true;
    }

    void PluginManager::destroyFaceu()
    {
        if (m_faceuHandle != nullptr)
        {
            faceuDone(m_faceuHandle);
            m_faceuHandle = nullptr;
        }
        if (m_copyShader != nullptr)
        {
            m_copyShader = nullptr;
        }
        if (m_fbo != nullptr)
        {
            m_fbo = nullptr;
        }
        m_textureImgSeqId = 0;
        m_initialized = false;
    }
    PluginManager::~PluginManager()
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "~PluginManager");
        destroyFaceu();
        for (auto iter = m_nativeXComponentMap.begin(); iter != m_nativeXComponentMap.end(); ++iter)
        {
            if (iter->second != nullptr)
            {
                delete iter->second;
                iter->second = nullptr;
            }
        }
        m_nativeXComponentMap.clear();

        for (auto iter = m_pluginRenderMap.begin(); iter != m_pluginRenderMap.end(); ++iter)
        {
            if (iter->second != nullptr)
            {
                delete iter->second;
                iter->second = nullptr;
            }
        }
        m_pluginRenderMap.clear();
    }
    void PluginManager::draw(int i_targetW, int i_targetH)
    {
        ensureFaceu();

        static std::shared_ptr<HJFacePointsMadeup> makeup = std::make_shared<HJFacePointsMadeup>();
        //     static HJPoint2D points[9];
        //     int nCount = sizeof(points) / sizeof(HJPoint2D);
        int oIndex = 0;
        std::shared_ptr<HJFacePointsReal> value = makeup->getFacePoints(-1, oIndex);
        //     for (int i = 0; i < nCount; i++)
        //     {
        //         points[i].x = value->getFilterPt()[i].x;
        //         points[i].y = value->getFilterPt()[i].y;
        //     }

        HJFacePointInfo morePoins;
        float offsety = 0.f;
        morePoins.width = m_faceuWidth;
        morePoins.height = m_faceuHeight;
        morePoins.faceCount = 2;
        morePoins.faces = new HJSingleFaceInfo[morePoins.faceCount];
        for (int i = 0; i < morePoins.faceCount; i++)
        {
            if (i == 1)
            {
                offsety = 400.f;
            }

            for (int j = 0; j < 5; j++)
            {
                morePoins.faces[i].points[j].x = value->getFilterPt()[j].x;
                morePoins.faces[i].points[j].y = value->getFilterPt()[j].y + offsety;
            }
            morePoins.faces[i].rect.x = value->getFilterPt()[5].x;
            morePoins.faces[i].rect.y = value->getFilterPt()[5].y + offsety;
            morePoins.faces[i].rect.w = value->getFilterPt()[8].x - value->getFilterPt()[5].x;
            morePoins.faces[i].rect.h = value->getFilterPt()[8].y - value->getFilterPt()[5].y;
        }

        unsigned char *oRgb = nullptr;
        m_fbo->attach();
        faceuRender(m_faceuHandle, &morePoins, oRgb);
        m_fbo->detach();

        if (morePoins.faceCount > 0)
        {
            delete[] morePoins.faces;
        }

        glViewport(0, 0, i_targetW, i_targetH);

        if (m_showImgSeq)
        {
            int width = 0, height = 0, nrComponents = 0;
            std::string str = s_resourcePath + "imgseq/sing/sing_" + std::to_string(oIndex) + ".jpg";
            unsigned char *data = stbi_load(str.c_str(), &width, &height, &nrComponents, 0);
            if (data)
            {
                GLenum internalFormat;
                GLenum dataFormat;
                if (nrComponents == 3)
                {
                    internalFormat = GL_RGB;
                    dataFormat = GL_RGB;
                }
                else if (nrComponents == 4)
                {
                    internalFormat = GL_RGBA;
                    dataFormat = GL_RGBA;
                }
                HJOGCommon::textureUpload(m_textureImgSeqId, internalFormat, width, height, dataFormat, data);
                stbi_image_free(data);

                m_copyShader->draw(m_textureImgSeqId, HJWindowRenderMode_FIT, m_faceuWidth, m_faceuHeight, i_targetW, i_targetH, nullptr, true);
            }
        }

        m_copyShader->draw(m_fbo->getTextureId(), HJWindowRenderMode_FIT, m_faceuWidth, m_faceuHeight, i_targetW, i_targetH, nullptr, false);
    }
    napi_value PluginManager::nFaceOpen(napi_env env, napi_callback_info info)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "PluginManager", "Open enter");
        auto render = PluginManager::GetInstance()->GetRender(xcom_id);

        NativeWindow *window = nullptr;
        int width = 0;
        int height = 0;

        int i_err = render->getWindow(window, width, height);
        if (i_err < 0)
        {
            return nullptr;
        }

        s_thread = std::make_shared<FrameLoopThread>();
        s_thread->setBeginCb([window, width, height]()
                             {
        s_eglCore = std::make_shared<HJOGEGLCore>();
        s_eglCore->init();
        s_eglCore->EGLSurfaceCreate(window); });
        s_thread->setEndCb([]()
                           {
        if (s_eglCore)
        {
            destroyFaceu();
            s_eglCore->done();
            s_eglCore = nullptr;
        } });

        s_thread->Start(30, [width, height]()
                        {
        if (s_eglCore)
        {
            s_eglCore->makeCurrent();
            
            glViewport(0, 0, width, height);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            
            PluginManager::draw(width, height);
            
            s_eglCore->swap();
        }
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "PluginManager", "thread proc"); });

        //    std::string file = s_resourcePath + "image/play.jpg";
        //    FILE *fd = fopen(file.c_str(), "rb");
        //    if (fd)
        //    {
        //        fseek(fd, 0, SEEK_END);
        //        int cnt = (int)ftell(fd);
        //        fseek(fd, 0, SEEK_SET);
        //        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "PluginManager", "size = %{public}d", cnt);
        //    }
        //    else
        //    {
        //        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "not find img");
        //    }

        return nullptr;
    }
    napi_value PluginManager::nFaceClose(napi_env env, napi_callback_info info)
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "PluginManager", "Close enter");
        if (s_thread)
        {
            s_thread->Stop();
            s_thread = nullptr;
        }
        return nullptr;
    }

    napi_value PluginManager::nFaceuAdd(napi_env env, napi_callback_info info)
    {
        size_t argc = 3;
        napi_value argv[3] = {nullptr};
        napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
        if (argc < 3)
        {
            return nullptr;
        }

        size_t keyLen = 0;
        napi_get_value_string_utf8(env, argv[0], nullptr, 0, &keyLen);
        std::string uniqueKey;
        uniqueKey.resize(keyLen + 1);
        napi_get_value_string_utf8(env, argv[0], &uniqueKey[0], keyLen + 1, &keyLen);
        uniqueKey.resize(keyLen);

        size_t pathLen = 0;
        napi_get_value_string_utf8(env, argv[1], nullptr, 0, &pathLen);
        std::string faceuPath;
        faceuPath.resize(pathLen + 1);
        napi_get_value_string_utf8(env, argv[1], &faceuPath[0], pathLen + 1, &pathLen);
        faceuPath.resize(pathLen);

        bool useDebugPoints = false;
        napi_get_value_bool(env, argv[2], &useDebugPoints);

        ensureFaceu();

        if (s_thread)
        {
            s_thread->PostAsync([uniqueKey, faceuPath, useDebugPoints]()
                                {
            if (m_faceuHandle != nullptr) 
            {
                faceuAdd(m_faceuHandle, uniqueKey.c_str(), faceuPath.c_str(), useDebugPoints);
            } });
        }

        return nullptr;
    }

    napi_value PluginManager::nFaceuRemove(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = {nullptr};
        napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
        if (argc < 1)
        {
            return nullptr;
        }
        size_t keyLen = 0;
        napi_get_value_string_utf8(env, argv[0], nullptr, 0, &keyLen);
        std::string uniqueKey;
        uniqueKey.resize(keyLen + 1);
        napi_get_value_string_utf8(env, argv[0], &uniqueKey[0], keyLen + 1, &keyLen);
        uniqueKey.resize(keyLen);

        if (s_thread)
        {
            s_thread->PostAsync([uniqueKey]()
                                {
            if (m_faceuHandle != nullptr) 
            {
                faceuRemove(m_faceuHandle, uniqueKey.c_str());
            } });
        }
        return nullptr;
    }

    napi_value PluginManager::nFaceuRemoveAll(napi_env env, napi_callback_info info)
    {
        if (s_thread)
        {
            s_thread->PostAsync([]()
                                {
            if (m_faceuHandle != nullptr) 
            {
                faceuRemoveAll(m_faceuHandle);
            } });
        }
        return nullptr;
    }

    napi_value PluginManager::nFaceuSetImgSeqVisible(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = {nullptr};
        napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
        if (argc < 1)
        {
            return nullptr;
        }
        bool show = false;
        napi_get_value_bool(env, argv[0], &show);
        if (s_thread)
        {
            s_thread->PostAsync([show]()
                                { m_showImgSeq = show; });
        }

        return nullptr;
    }
    napi_value PluginManager::GetContext(napi_env env, napi_callback_info info)
    {
        if ((env == nullptr) || (info == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "GetContext env or info is null");
            return nullptr;
        }

        size_t argCnt = 1;
        napi_value args[1] = {nullptr};
        if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "GetContext napi_get_cb_info failed");
        }

        if (argCnt != 1)
        {
            napi_throw_type_error(env, NULL, "Wrong number of arguments");
            return nullptr;
        }

        napi_valuetype valuetype;
        if (napi_typeof(env, args[0], &valuetype) != napi_ok)
        {
            napi_throw_type_error(env, NULL, "napi_typeof failed");
            return nullptr;
        }

        if (valuetype != napi_number)
        {
            napi_throw_type_error(env, NULL, "Wrong type of arguments");
            return nullptr;
        }

        int64_t value;
        if (napi_get_value_int64(env, args[0], &value) != napi_ok)
        {
            napi_throw_type_error(env, NULL, "napi_get_value_int64 failed");
            return nullptr;
        }

        napi_value exports;
        if (napi_create_object(env, &exports) != napi_ok)
        {
            napi_throw_type_error(env, NULL, "napi_create_object failed");
            return nullptr;
        }

        return exports;
    }

    void PluginManager::Export(napi_env env, napi_value exports)
    {
        if ((env == nullptr) || (exports == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "Export: env or exports is null");
            return;
        }

        napi_value exportInstance = nullptr;
        if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "Export: napi_get_named_property fail");
            return;
        }

        OH_NativeXComponent *nativeXComponent = nullptr;
        if (napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "Export: napi_unwrap fail");
            return;
        }

        char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
        uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
        if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
        {
            OH_LOG_Print(
                LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "PluginManager", "Export: OH_NativeXComponent_GetXComponentId fail");
            return;
        }

        std::string id(idStr);
        auto context = PluginManager::GetInstance();
        if ((context != nullptr) && (nativeXComponent != nullptr))
        {
            context->SetNativeXComponent(id, nativeXComponent);
            auto render = context->GetRender(id);
            if (render != nullptr)
            {
                render->RegisterCallback(nativeXComponent);
                // render->Export(env, exports);
            }
        }

        // library load callback
        HJEntryContextInfo info;
        info.logIsValid = true;
        info.logDir = "/data/storage/el2/base/haps/entry/files/log_faceu";
        ;
        info.logLevel = HJLOGLevel_INFO;
        info.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
        info.logMaxFileSize = 5 * 1024 * 1024;
        info.logMaxFileNum = 5;
        renderContextInit(info);

        // app exit callback
        // renderContextUnInit();
    }

    void PluginManager::SetNativeXComponent(std::string &id, OH_NativeXComponent *nativeXComponent)
    {
        if (nativeXComponent == nullptr)
        {
            return;
        }

        if (m_nativeXComponentMap.find(id) == m_nativeXComponentMap.end())
        {
            m_nativeXComponentMap[id] = nativeXComponent;
            return;
        }

        if (m_nativeXComponentMap[id] != nativeXComponent)
        {
            OH_NativeXComponent *tmp = m_nativeXComponentMap[id];
            delete tmp;
            tmp = nullptr;
            m_nativeXComponentMap[id] = nativeXComponent;
        }
    }

    PluginRender *PluginManager::GetRender(const std::string &id)
    {
        if (m_pluginRenderMap.find(id) == m_pluginRenderMap.end())
        {
            PluginRender *instance = PluginRender::GetInstance(id);
            m_pluginRenderMap[id] = instance;
            return instance;
        }

        return m_pluginRenderMap[id];
    }
}
