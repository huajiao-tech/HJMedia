#pragma once

#include <napi/native_api.h>
#include <string>

#include "HJFLog.h"
#include "HJJson.h"
#include "HJPrerequisites.h"

NS_HJ_BEGIN

class HJNativeExportCommon {
public:
    static HJNativeExportCommon& Instance();
    
    int Export(napi_env env, napi_value exports);

    //static napi_value contextInit(napi_env env, napi_callback_info info);
   
    static napi_value setFaceInfo(napi_env env, napi_callback_info info);
    static napi_value nativeSourceOpen(napi_env env, napi_callback_info info);
    static napi_value nativeSourceClose(napi_env env, napi_callback_info info);
    static napi_value nativeSourceAcquire(napi_env env, napi_callback_info info);
    static napi_value openFaceu(napi_env env, napi_callback_info info);
    static napi_value closeFaceu(napi_env env, napi_callback_info info);
    static napi_value setVideoEncQuantOffset(napi_env env, napi_callback_info info);
    static napi_value setFaceProtected(napi_env env, napi_callback_info info);

    static napi_value nodeEnable(napi_env env, napi_callback_info info);
    static napi_value nodeSetParam(napi_env env, napi_callback_info info);
    static napi_value nodeCreate(napi_env env, napi_callback_info info);
    static napi_value nodeConnect(napi_env env, napi_callback_info info);
    static napi_value nodeDelete(napi_env env, napi_callback_info info);
    static napi_value nodeDisconnect(napi_env env, napi_callback_info info);
    static napi_value nodeLinkInfoChange(napi_env env, napi_callback_info info);
    static napi_value nodeGetPre(napi_env env, napi_callback_info info);
    static napi_value nodeGetNext(napi_env env, napi_callback_info info);


private:
    static std::pair<std::string, std::string> priParseClassAndName(const napi_env &env, napi_value i_classStyle, napi_value i_insName);
};

NS_HJ_END
