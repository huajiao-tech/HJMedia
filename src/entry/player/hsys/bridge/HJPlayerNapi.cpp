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

NS_HJ_END