#include "HJPlayerNapi.h"

#include "HJFLog.h"
#include "HJJson.h"
#include "HJNativeCommon.h"
#include "HJNapiUtils.h"
#include "HJJNIField.h"
#include "HJPlayerTypes.h"
#include "HJPlayerBridge.h"
#include <native_window/external_window.h>

NS_HJ_BEGIN

napi_value HJPlayerNapi::contextInit(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    std::string jsonStr;
    NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, jsonStr);
    auto jsonDoc = std::make_shared<HJYJsonDocument>();
    auto config = std::make_shared<ContextInitConfig>(jsonDoc);
    if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    i_err = HJPlayerBridge::contextInit({
        .logIsValid = config->valid, .logDir = config->logDir, .logLevel = config->logLevel, .logMode = config->logMode,
        .logMaxFileSize = config->maxSize, .logMaxFileNum = config->maxFiles
    });

    NAPI_INT_RET
}

napi_value HJPlayerNapi::createPlayer(napi_env env, napi_callback_info info)
{
    auto nPlayerPtr = new HJPlayerBridge();

    napi_value nPlayerHandle;
    napi_create_bigint_int64(env, reinterpret_cast<int64_t>(nPlayerPtr), &nPlayerHandle);

    return nPlayerHandle;
}


napi_value HJPlayerNapi::destroyPlayer(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);
    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    delete nPlayerPtr;

    return nullptr;
}

napi_value HJPlayerNapi::openPlayer(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_5)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    std::string jsonStr;
    NapiUtil::JsValueToString(env, args[INDEX_1], STR_DEFAULT_SIZE, jsonStr);
    auto jsonDoc = std::make_shared<HJYJsonDocument>();
    auto config = std::make_shared<OpenPlayerInfo>(jsonDoc);
    if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto tsfn = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_2]);

    std::string jsonStr1;
    NapiUtil::JsValueToString(env, args[INDEX_3], STR_DEFAULT_SIZE, jsonStr1);
    auto jsonDoc1 = std::make_shared<HJYJsonDocument>();
    auto config1 = std::make_shared<MediaStateInfo>(jsonDoc1);
    if (jsonDoc1->init(jsonStr1) != HJ_OK || config1->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto statCall = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_4]);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    nPlayerPtr->setThreadSafeFunction(std::move(tsfn));
    nPlayerPtr->setStateCall(std::move(statCall));

    HJEntryStatInfo statInfo{};
    statInfo.bUseStat = true;
    statInfo.uid = config1->uid;
    statInfo.device = config1->device;
    statInfo.sn = config1->sn;
    statInfo.statNotify = [nPlayerPtr](const std::string& i_name, int i_nType, const std::string& i_info) {
        auto data = new HJYJsonDocument{};
        data->init();
        data->setMember("name", i_name);
        data->setMember("type", i_nType);
        data->setMember("info", i_info);
        nPlayerPtr->stateCall(data);
    };

    i_err = nPlayerPtr->openPlayer({
                                       .m_url = config->url, .m_fps = config->fps,
                                       .m_videoCodecType = config->videoCodecType,
                                       .m_sourceType = static_cast<HJPrioComSourceType>(config->sourceType),
                                       .m_playerType = static_cast<HJPlayerType>(config->playerType),
                                       .m_bSplitScreenMirror = config->bSplitScreenMirror,
                                       .m_disableMFlag = config->disableMFlag
                                   },
                                   [nPlayerPtr](int i_type, const std::string &i_msgInfo) {
                                       auto data = new HJYJsonDocument{};
                                       data->init();
                                       data->setMember("type", i_type);
                                       data->setMember("msgInfo", i_msgInfo);
                                       nPlayerPtr->call(data);
                                   }, statInfo);

    NAPI_INT_RET
}

napi_value HJPlayerNapi::setWindow(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    do
    {
        NAPI_PARSE_ARGS(PARAM_COUNT_2)

        int64_t nPlayerHandle;
        bool lossless = true;
        napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);
        auto nPlayer = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);

        std::string jsonStr;
        NapiUtil::JsValueToString(env, args[INDEX_1], STR_DEFAULT_SIZE, jsonStr);
        auto jsonDoc = std::make_shared<HJYJsonDocument>();
        auto config = std::make_shared<SetWindowConfig>(jsonDoc);
        config->serialInfo();
        if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
            napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
            return nullptr;
        }

        int64_t surfaceId = std::stoll(config->surfaceId);
        OHNativeWindow *nativeWindow;
        i_err = OH_NativeWindow_CreateNativeWindowFromSurfaceId(surfaceId, &nativeWindow);
        if (i_err != HJ_OK) {
            break;
        }
        i_err = nPlayer->setNativeWindow(nativeWindow, config->width, config->height, config->state);
    } while (false);

    NAPI_INT_RET
}

napi_value HJPlayerNapi::closePlayer(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);
    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    nPlayerPtr->closePlayer();

    return nullptr;
}

napi_value HJPlayerNapi::exitPlayer(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);
    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    nPlayerPtr->exitPlayer();

    return nullptr;
}

napi_value HJPlayerNapi::setMute(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    bool mute;
    napi_get_value_bool(env, args[INDEX_1], &mute);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    nPlayerPtr->setMute(mute);

    return nullptr;
}

napi_value HJPlayerNapi::preloadUrl(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    std::string url;
    NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, url);

    HJPlayerBridge::preloadUrl(url);

    return nullptr;
}


napi_value HJPlayerNapi::setFaceInfo(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_4)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    if (nPlayerPtr)
    {
        int32_t w;
        napi_get_value_int32(env, args[INDEX_1], &w);
        int32_t h;
        napi_get_value_int32(env, args[INDEX_2], &h);
        
        
        size_t strLen;
        napi_get_value_string_utf8(env, args[INDEX_3], nullptr, 0, &strLen);

        std::vector<char> buffer(strLen + 1);
        napi_get_value_string_utf8(env, args[INDEX_3], buffer.data(), buffer.size(), nullptr);
        std::string pointsInfo = std::string(buffer.data());
        
        HJFacePointsWrapper::Ptr faceInfo = HJFacePointsWrapper::Create<HJFacePointsWrapper>(w, h, pointsInfo);
        nPlayerPtr->setFaceInfo(faceInfo);
    }

    NAPI_INT_RET    
}
napi_value HJPlayerNapi::nativeSourceOpen(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    if (nPlayerPtr)
    {
        bool bUsePBO = false;
        napi_get_value_bool(env, args[INDEX_1], &bUsePBO);
        i_err = nPlayerPtr->openNativeSource(bUsePBO);
    }

    NAPI_INT_RET
}
napi_value HJPlayerNapi::nativeSourceClose(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    if (nPlayerPtr)
    {
        nPlayerPtr->closeNativeSource();
    }

    NAPI_INT_RET
}
napi_value HJPlayerNapi::nativeSourceAcquire(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)
    std::shared_ptr<HJRGBAMediaData> data = nullptr;
    
    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    if (nPlayerPtr)
    {
        data = nPlayerPtr->acquireNativeSource();
        if (data)
        {
            void* arrayBufferPtr = nullptr;
            napi_value arrayBuffer = nullptr;
            size_t bufferSize = data->m_nSize;
            napi_status status = napi_create_arraybuffer(env, bufferSize, &arrayBufferPtr, &arrayBuffer);
            if (status != napi_ok || arrayBufferPtr == nullptr) 
            {
                napi_throw_error(env, nullptr, "Failed to create ArrayBuffer");
                return nullptr;
            }
            uint8_t *pData = static_cast<uint8_t*>(arrayBufferPtr);
        
            //int64_t t0 = HJCurrentSteadyMS();
            memcpy(pData, data->m_buffer->getBuf(), bufferSize);
            //int64_t t1 = HJCurrentSteadyMS();
            //HJFLogi("memcpy size:{} time is:{} ", bufferSize, (t1 - t0));
                
            napi_value resultObj;
            napi_create_object(env, &resultObj);
        
            napi_value jsWidth, jsHeight;
            napi_create_uint32(env, data->m_width, &jsWidth);
            napi_create_uint32(env, data->m_height, &jsHeight);

            napi_set_named_property(env, resultObj, "data", arrayBuffer);
            napi_set_named_property(env, resultObj, "width", jsWidth);
            napi_set_named_property(env, resultObj, "height", jsHeight);
            return resultObj; 
        }
    }
    return nullptr;
}

napi_value HJPlayerNapi::openFaceu(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
    
    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    if (nPlayerPtr)
    {
        size_t strLen;
        napi_get_value_string_utf8(env, args[INDEX_1], nullptr, 0, &strLen);

        std::vector<char> buffer(strLen + 1);
        napi_get_value_string_utf8(env, args[INDEX_1], buffer.data(), buffer.size(), nullptr);
        std::string url = std::string(buffer.data());
        i_err = nPlayerPtr->openFaceu(url);
    }

    NAPI_INT_RET
}
napi_value HJPlayerNapi::closeFaceu(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)
       
    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    if (nPlayerPtr)
    {
        nPlayerPtr->closeFaceu();
    }

    NAPI_INT_RET
}

NS_HJ_END