#include "HJVerifyManager.h"

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <hilog/log.h>
#include <vector>

#include "HJFLog.h"
#include "HJError.h"
#include "HJTransferInfo.h"
#include "HJInferenceContextExport.h"

NS_HJ_USING

namespace NativeXComponentSample {

HJVerifyManager HJVerifyManager::m_HJVerifyManager;
HJNAPISRSession::Ptr HJVerifyManager::m_srSession = nullptr;
bool HJVerifyManager::s_srCtxInit = false;
int HJVerifyManager::s_logCnt = 5;

napi_value HJVerifyManager::srInit(napi_env env, napi_callback_info info)
{
    auto retNapi = [env](int ret) -> napi_value {
        napi_value v = nullptr;
        napi_create_int32(env, ret, &v);
        return v;
    };

    if ((env == nullptr) || (info == nullptr))
    {
        return retNapi(HJErrInvalidParams);
    }

    size_t argc = 7;
    napi_value args[7] = { nullptr };
    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok || argc < 2)
    {
        return retNapi(HJErrInvalidParams);
    }

    auto parseStr = [env](napi_value v) -> std::string {
        size_t len = 0;
        if (napi_get_value_string_utf8(env, v, nullptr, 0, &len) != napi_ok || len == 0)
        {
            return "";
        }
        std::vector<char> buf(len + 1);
        if (napi_get_value_string_utf8(env, v, buf.data(), buf.size(), nullptr) != napi_ok)
        {
            return "";
        }
        return std::string(buf.data());
    };

    std::string modelDir = parseStr(args[0]);
    std::string seqDir = parseStr(args[1]);
    int32_t fps = 15;
    bool bLoop = true;
    bool bFullSR = false;
    bool bFaceScale = true;
    int32_t faceScaleWidth = 200;
    if (argc > 2)
    {
        napi_get_value_int32(env, args[2], &fps);
    }
    if (argc > 3)
    {
        napi_get_value_bool(env, args[3], &bLoop);
    }
    if (argc > 4)
    {
        napi_get_value_bool(env, args[4], &bFullSR);
    }
    if (argc > 5)
    {
        napi_get_value_bool(env, args[5], &bFaceScale);
    }
    if (argc > 6)
    {
        napi_get_value_int32(env, args[6], &faceScaleWidth);
    }

    if (!s_srCtxInit)
    {
        HJ::HJLog::Instance().init(true, "/data/storage/el2/base/haps/entry/files/log_outer_sr",HJLOGLevel_INFO, HJLogLMode_CONSOLE | HJLLogMode_FILE, 1024 * 1024 * 5, 5);

        
        HJEntryContextInfo contextInfo;
        contextInfo.logIsValid = true;
        contextInfo.logDir = "/data/storage/el2/base/haps/entry/files/log_sr";
        contextInfo.logLevel = HJLOGLevel_INFO;
        contextInfo.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
        contextInfo.logMaxFileSize = 1024 * 1024 * 5;
        contextInfo.logMaxFileNum = s_logCnt;

        int ctxRet = inferenceContextInit(contextInfo);
        if (ctxRet < 0)
        {
            return retNapi(ctxRet);
        }
        s_srCtxInit = true;
    }

    if (!m_srSession)
    {
        m_srSession = HJNAPISRSession::Create();
    }
    if (!m_srSession)
    {
        return retNapi(HJErrNotInited);
    }

    int ret = m_srSession->init(modelDir, seqDir, fps, bLoop);
    if (ret < 0)
    {
        return retNapi(ret);
    }
    ret = m_srSession->setOptions(bFullSR, bFaceScale, faceScaleWidth);
    return retNapi(ret);
}

napi_value HJVerifyManager::srStart(napi_env env, napi_callback_info info)
{
    (void)info;
    auto retNapi = [env](int ret) -> napi_value {
        napi_value v = nullptr;
        napi_create_int32(env, ret, &v);
        return v;
    };

    if (!m_srSession)
    {
        return retNapi(HJErrNotInited);
    }

    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    if (render)
    {
        render->setRenderChangeFunc([](void* window, int width, int height)
        {
            if (m_srSession)
            {
                m_srSession->onWindowChanged(window, width, height, HJTargetState_Change);
            }
        });

        NativeWindow* window = nullptr;
        int width = 0;
        int height = 0;
        if (render->test(window, width, height) == HJ_OK)
        {
            m_srSession->onWindowChanged(window, width, height, HJTargetState_Create);
        }
    }

    return retNapi(m_srSession->start());
}

napi_value HJVerifyManager::srStop(napi_env env, napi_callback_info info)
{
    (void)info;
    if (m_srSession)
    {
        m_srSession->stop();
        m_srSession = nullptr;
    }

    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    if (render)
    {
        render->setRenderChangeFunc(nullptr);
    }

    napi_value v = nullptr;
    napi_create_int32(env, HJ_OK, &v);
    return v;
}

napi_value HJVerifyManager::srGetInfo(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value resultObj = nullptr;
    napi_create_object(env, &resultObj);
    if (!m_srSession)
    {
        return resultObj;
    }

    HJNAPISRStat st = m_srSession->getStat();
    napi_value v = nullptr;

    napi_create_int32(env, st.lastRet, &v);
    napi_set_named_property(env, resultObj, "ret", v);

    napi_create_int32(env, st.result.faceCount, &v);
    napi_set_named_property(env, resultObj, "faceCount", v);

    napi_create_int64(env, st.result.faceDetectElapsedMs, &v);
    napi_set_named_property(env, resultObj, "faceDetectMs", v);

    napi_create_int64(env, st.result.srElapsedMs, &v);
    napi_set_named_property(env, resultObj, "srMs", v);

    napi_create_int32(env, st.result.faceRect.x, &v);
    napi_set_named_property(env, resultObj, "faceX", v);
    napi_create_int32(env, st.result.faceRect.y, &v);
    napi_set_named_property(env, resultObj, "faceY", v);
    napi_create_int32(env, st.result.faceRect.w, &v);
    napi_set_named_property(env, resultObj, "faceW", v);
    napi_create_int32(env, st.result.faceRect.h, &v);
    napi_set_named_property(env, resultObj, "faceH", v);

    napi_create_int32(env, st.result.scaleW, &v);
    napi_set_named_property(env, resultObj, "scaleW", v);
    napi_create_int32(env, st.result.scaleH, &v);
    napi_set_named_property(env, resultObj, "scaleH", v);

    return resultObj;
}

///////////////////////////////////////////////////
HJVerifyManager::~HJVerifyManager()
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "~HJVerifyManager");
    for (auto iter = m_nativeXComponentMap.begin(); iter != m_nativeXComponentMap.end(); ++iter)
    {
        if (iter->second != nullptr)
        {
            delete iter->second;
            iter->second = nullptr;
        }
    }
    m_nativeXComponentMap.clear();

    for (auto iter = m_HJVerifyRenderMap.begin(); iter != m_HJVerifyRenderMap.end(); ++iter)
    {
        if (iter->second != nullptr)
        {
            delete iter->second;
            iter->second = nullptr;
        }
    }
    m_HJVerifyRenderMap.clear();
}

napi_value HJVerifyManager::GetContext(napi_env env, napi_callback_info info)
{
    if ((env == nullptr) || (info == nullptr))
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext env or info is null ");
        return nullptr;
    }

    size_t argCnt = 1;
    napi_value args[1] = {nullptr};
    if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext napi_get_cb_info failed");
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

void HJVerifyManager::Export(napi_env env, napi_value exports)
{
    if ((env == nullptr) || (exports == nullptr))
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: env or exports is null");
        return;
    }

    napi_value exportInstance = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: napi_get_named_property fail");
        return;
    }

    OH_NativeXComponent *nativeXComponent = nullptr;
    if (napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) != napi_ok)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: napi_unwrap fail");
        return;
    }

    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: OH_NativeXComponent_GetXComponentId fail");
        return;
    }

    std::string id(idStr);
    auto context = HJVerifyManager::GetInstance();
    if ((context != nullptr) && (nativeXComponent != nullptr))
    {
        context->SetNativeXComponent(id, nativeXComponent);
        auto render = context->GetRender(id);
        if (render != nullptr)
        {
            render->RegisterCallback(nativeXComponent);
            render->Export(env, exports);
        }
    }
}

void HJVerifyManager::SetNativeXComponent(std::string &id, OH_NativeXComponent *nativeXComponent)
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

HJVerifyRender *HJVerifyManager::GetRender(const std::string &id)
{
    if (m_HJVerifyRenderMap.find(id) == m_HJVerifyRenderMap.end())
    {
        HJVerifyRender *instance = HJVerifyRender::GetInstance(id);
        m_HJVerifyRenderMap[id] = instance;
        return instance;
    }

    return m_HJVerifyRenderMap[id];
}

} // namespace NativeXComponentSample
