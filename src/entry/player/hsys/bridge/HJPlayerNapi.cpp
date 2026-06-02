#include "HJPlayerNapi.h"

#include "HJFLog.h"
#include "HJJson.h"
#include "HJNativeCommon.h"
#include "HJNapiUtils.h"
#include "HJPlayerTypes.h"
#include "HJPlayerBridge.h"
#include "HJRteGraphSetupInfo.h"
#include "HJException.h"
#include "HJLocalIOUtils.h"

NS_HJ_BEGIN

class HJPlayerContextInitConfig final : public HJInterpreter {
public:
    bool valid{false};
    std::string logDir{""};
    int logLevel{0};
    int logMode{0};
    int maxSize{0};
    int maxFiles{0};
    //
    std::string medias_dir;
    int medias_cache_max;
    std::vector<std::pair<std::string, int>> other_dirs_options;
    int download_retry_max = 10;

    HJ_JSON_REFLECT_BEGIN(HJPlayerContextInitConfig)
        HJ_JSON_REFLECT_MEMBER(valid)
        HJ_JSON_REFLECT_MEMBER(logDir)
        HJ_JSON_REFLECT_MEMBER(logLevel)
        HJ_JSON_REFLECT_MEMBER(logMode)
        HJ_JSON_REFLECT_MEMBER(maxSize)
        HJ_JSON_REFLECT_MEMBER(maxFiles)
        HJ_JSON_REFLECT_MEMBER(medias_dir)
        HJ_JSON_REFLECT_MEMBER(medias_cache_max)
        HJ_JSON_REFLECT_MEMBER(other_dirs_options)
        HJ_JSON_REFLECT_MEMBER(download_retry_max)
    HJ_JSON_REFLECT_END(HJPlayerContextInitConfig)    
};
HJ_JSON_REFLECT_IMPLEMENT(HJPlayerContextInitConfig)

napi_value HJPlayerNapi::contextInit(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    std::string jsonStr;
    NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, jsonStr);
    auto jsonDoc = HJYJsonDocument::createWithInfo(jsonStr);//std::make_shared<HJYJsonDocument>();
    if(!jsonDoc) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }
    auto config = std::make_shared<HJPlayerContextInitConfig>();
    if (config->deserialInfo(jsonDoc) != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    HJEntryContextPlayerInfo contextConfig;
    contextConfig.logIsValid = config->valid;
    contextConfig.logDir = config->logDir;
    contextConfig.logLevel = config->logLevel;
    contextConfig.logMode = config->logMode;
    contextConfig.logMaxFileSize = config->maxSize;
    contextConfig.logMaxFileNum = config->maxFiles;
    contextConfig.medias_dir = config->medias_dir;
    contextConfig.medias_cache_max = config->medias_cache_max;
    contextConfig.other_dirs_options = config->other_dirs_options;
    contextConfig.download_retry_max = config->download_retry_max;

    i_err = HJPlayerBridge::contextInit(contextConfig);

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
    size_t argc = PARAM_COUNT_6;
    napi_value args[PARAM_COUNT_6] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < PARAM_COUNT_5) {
        napi_throw_error(env, nullptr, "Invalid argument count");
        return nullptr;
    }

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    std::string jsonStr = NapiUtil::parseString(env, args[INDEX_1]);
    auto jsonDoc = std::make_shared<HJYJsonDocument>();
    auto config = std::make_shared<OpenPlayerInfo>(jsonDoc);
    if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto tsfn = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_2]);

    std::string jsonStr1 = NapiUtil::parseString(env, args[INDEX_3]);
    auto jsonDoc1 = std::make_shared<HJYJsonDocument>();
    auto config1 = std::make_shared<MediaStateInfo>(jsonDoc1);
    if (jsonDoc1->init(jsonStr1) != HJ_OK || config1->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto statCall = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_4]);
    std::unique_ptr<ThreadSafeFunctionWrapper> seiCall = nullptr;
    const bool hasSeiCallbackArg = argc >= PARAM_COUNT_6 && args[INDEX_5] != nullptr;
    if (hasSeiCallbackArg) {
        napi_valuetype seiCallbackType = napi_undefined;
        napi_typeof(env, args[INDEX_5], &seiCallbackType);
        if (seiCallbackType == napi_function) {
            seiCall = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_5], ThreadSafeFunctionWrapper::playerSeiCallback);
        }
        else if (seiCallbackType != napi_undefined && seiCallbackType != napi_null) {
            napi_throw_type_error(env, nullptr, "sei callback must be a function");
            return nullptr;
        }
    }

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    nPlayerPtr->setThreadSafeFunction(std::move(tsfn));
    nPlayerPtr->setStateCall(std::move(statCall));
    const bool hasSeiCallback = seiCall != nullptr;
    nPlayerPtr->setSeiCall(std::move(seiCall));
    nPlayerPtr->setSeiNotify(nullptr);
    if (hasSeiCallback) {
        nPlayerPtr->setSeiNotify([nPlayerPtr](const std::vector<HJSEIData>& userSEIDatas, bool keyFrame) {
            auto data = new HJPlayerSeiCallbackData{};
            data->isKeyFrame = keyFrame;
            data->seiDatas = userSEIDatas;
            nPlayerPtr->seiCall(data);
        });
    }

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
                                       .m_url = config->url, .m_localDir = config->localDir, .m_rid = config->rid,
                                       .m_fps = config->fps, .m_repeats = config->repeats,
                                       .m_videoCodecType = config->videoCodecType,
                                       .m_sourceType = static_cast<HJPrioComSourceType>(config->sourceType),
                                       .m_playerType = static_cast<HJPlayerType>(config->playerType),
                                       .m_bSplitScreenMirror = config->bSplitScreenMirror,
                                       .m_disableMFlag = config->disableMFlag,
                                       .m_graphConfig = config->m_graphConfig,
                                       .m_enableSEIUUids = config->m_enableSEIUUids,
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
        i_err = nPlayer->setNativeWindow(config->classStyle, config->insName, surfaceId, config->width, config->height, config->state);
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

class HJPlayerDownloadInfo final : public HJInterpreter {
public:
    std::string url{""};
    std::string dir{""};
    std::string rid{""};
    int preCacheSize{-1};
    int priority{0};

    HJ_JSON_REFLECT_BEGIN(HJPlayerDownloadInfo)
        HJ_JSON_REFLECT_MEMBER(url)
        HJ_JSON_REFLECT_MEMBER(dir)
        HJ_JSON_REFLECT_MEMBER(rid)
        HJ_JSON_REFLECT_MEMBER(preCacheSize)
        HJ_JSON_REFLECT_MEMBER(priority)
    HJ_JSON_REFLECT_END(HJPlayerDownloadInfo)
};
HJ_JSON_REFLECT_IMPLEMENT(HJPlayerDownloadInfo)

typedef struct HJPlayerDownloadNotify
{
    int type{0};
    // std::string url{""};
    std::string rid{""};
    int64_t totalSize{0};
    int64_t validSize{0};
    int status{0};
} HJPlayerDownloadNotify;

napi_value HJPlayerNapi::download(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(2);
    std::string ret_rid = "";
    try
    {
        std::string jsonStr;
        NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, jsonStr);
        HJPlayerDownloadInfo info(jsonStr);
        info.deserialInfo();
        
        auto statCall = std::make_shared<ThreadSafeFunctionWrapper>(env, args[INDEX_1]);

        ret_rid = HJPlayerBridge::download(info.url, info.dir, info.rid, info.preCacheSize, info.priority, [statCall](const HJNotification::Ptr ntf) -> int {
                if (!ntf) {
                    return HJ_OK;
                }
                int ntfID = HJ_RENDER_NOTIFY_CACHE_START;
                HJPlayerDownloadNotify retInfo;
                // retInfo.url = ntf->getString("url");
                retInfo.rid = ntf->getString("rid");
                retInfo.totalSize = ntf->getInt64("total");
                retInfo.validSize = ntf->getInt64("valid");
                //
                switch (ntf->getID()) {
                    case HJ_LOCALIO_NOTIFY_CACHE_START: {
                        ntfID = HJ_RENDER_NOTIFY_CACHE_START;
                        HJFLogi("local io open: {}, size: {} / {}", ntf->getMsg(), retInfo.totalSize, retInfo.validSize);
                        break;
                    }
                    case HJ_LOCALIO_NOTIFY_CACHE_PROGRESS: {
                        ntfID = HJ_RENDER_NOTIFY_CACHE_PROGRESS;
                        HJFLogi("local io progress: {}, size: {} / {}", ntf->getMsg(), retInfo.totalSize, retInfo.validSize);
                        break;
                    }
                    case HJ_LOCALIO_NOTIFY_CACHE_COMPLETED: {
                        ntfID = HJ_RENDER_NOTIFY_CACHE_COMPLETED;
                        HJFLogi("local io completed: {}, size: {} / {}", ntf->getMsg(), retInfo.totalSize, retInfo.validSize);
                        break;
                    }
                    case HJ_LOCALIO_NOTIFY_CACHE_FAILED: {
                        ntfID = HJ_RENDER_NOTIFY_CACHE_FAILED;
                        retInfo.status = ntf->getVal();
                        HJFLogi("local io failed: {}, size: {} / {}, status: {}", ntf->getMsg(), retInfo.totalSize, retInfo.validSize, retInfo.status);
                        break;
                    }
                }
            
                auto data = new HJYJsonDocument{};
                data->init();
                data->setMember("type", ntfID);
                data->setMember("rid", retInfo.rid);
                data->setMember("totalSize", retInfo.totalSize);
                data->setMember("validSize", retInfo.validSize);
                data->setMember("status", retInfo.status);
                statCall->Call(data);
                return HJ_OK;
            });
    }
    catch(const std::exception& e) {
        HJFLoge("error, download error");
    }
    
    NAPI_STR_RET(ret_rid)
}

napi_value HJPlayerNapi::cancelDownload(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    std::string url;
    NapiUtil::JsValueToString(env, args[INDEX_0], STR_DEFAULT_SIZE, url);

    HJPlayerBridge::cancelDownload(url);

    return nullptr;
}

napi_value HJPlayerNapi::setPause(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    bool pause;
    napi_get_value_bool(env, args[INDEX_1], &pause);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    i_err = nPlayerPtr->setPause(pause);

    NAPI_INT_RET
}

napi_value HJPlayerNapi::getDuration(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    auto duration = nPlayerPtr->getDuration();

    napi_value nDuration;
    napi_create_bigint_int64(env, duration, &nDuration);

    return nDuration;
}

napi_value HJPlayerNapi::getCurrentTimestamp(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    auto currentTimestamp = nPlayerPtr->getCurrentTimestamp();

    napi_value nCurrentTimestamp;
    napi_create_bigint_int64(env, currentTimestamp, &nCurrentTimestamp);

    return nCurrentTimestamp;
}

napi_value HJPlayerNapi::setEnableSEIUUids(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_3)

    int64_t nPlayerHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

    const std::string uuid = NapiUtil::parseString(env, args[INDEX_1]);

    bool keyMustCb = false;
    napi_get_value_bool(env, args[INDEX_2], &keyMustCb);

    auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
    i_err = nPlayerPtr->setEnableSEIUUids(uuid, keyMustCb);

    NAPI_INT_RET
}


// napi_value HJPlayerNapi::setFaceInfo(napi_env env, napi_callback_info info)
// {
//     int i_err = HJ_OK;
//     NAPI_PARSE_ARGS(PARAM_COUNT_4)

//     int64_t nPlayerHandle;
//     bool lossless = true;
//     napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

//     auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
//     if (nPlayerPtr)
//     {
//         int32_t w;
//         napi_get_value_int32(env, args[INDEX_1], &w);
//         int32_t h;
//         napi_get_value_int32(env, args[INDEX_2], &h);
        
        
//         size_t strLen;
//         napi_get_value_string_utf8(env, args[INDEX_3], nullptr, 0, &strLen);

//         std::vector<char> buffer(strLen + 1);
//         napi_get_value_string_utf8(env, args[INDEX_3], buffer.data(), buffer.size(), nullptr);
//         std::string pointsInfo = std::string(buffer.data());
        
//         HJFacePointsWrapper::Ptr faceInfo = HJFacePointsWrapper::Create<HJFacePointsWrapper>(w, h, pointsInfo);
//         nPlayerPtr->setFaceInfo(faceInfo);
//     }

//     NAPI_INT_RET    
// }
// napi_value HJPlayerNapi::nativeSourceOpen(napi_env env, napi_callback_info info)
// {
//     int i_err = HJ_OK;
//     NAPI_PARSE_ARGS(PARAM_COUNT_2)

//     int64_t nPlayerHandle;
//     bool lossless = true;
//     napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

//     auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
//     if (nPlayerPtr)
//     {
//         bool bUsePBO = false;
//         napi_get_value_bool(env, args[INDEX_1], &bUsePBO);
//         i_err = nPlayerPtr->openNativeSource(bUsePBO);
//     }

//     NAPI_INT_RET
// }
// napi_value HJPlayerNapi::nativeSourceClose(napi_env env, napi_callback_info info)
// {
//     int i_err = HJ_OK;
//     NAPI_PARSE_ARGS(PARAM_COUNT_1)

//     int64_t nPlayerHandle;
//     bool lossless = true;
//     napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

//     auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
//     if (nPlayerPtr)
//     {
//         nPlayerPtr->closeNativeSource();
//     }

//     NAPI_INT_RET
// }
// napi_value HJPlayerNapi::nativeSourceAcquire(napi_env env, napi_callback_info info)
// {
//     NAPI_PARSE_ARGS(PARAM_COUNT_1)
//     std::shared_ptr<HJRGBAMediaData> data = nullptr;
    
//     int64_t nPlayerHandle;
//     bool lossless = true;
//     napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

//     auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
//     if (nPlayerPtr)
//     {
//         data = nPlayerPtr->acquireNativeSource();
//         if (data)
//         {
//             void* arrayBufferPtr = nullptr;
//             napi_value arrayBuffer = nullptr;
//             size_t bufferSize = data->m_nSize;
//             napi_status status = napi_create_arraybuffer(env, bufferSize, &arrayBufferPtr, &arrayBuffer);
//             if (status != napi_ok || arrayBufferPtr == nullptr) 
//             {
//                 napi_throw_error(env, nullptr, "Failed to create ArrayBuffer");
//                 return nullptr;
//             }
//             uint8_t *pData = static_cast<uint8_t*>(arrayBufferPtr);
        
//             //int64_t t0 = HJCurrentSteadyMS();
//             memcpy(pData, data->m_buffer->getBuf(), bufferSize);
//             //int64_t t1 = HJCurrentSteadyMS();
//             //HJFLogi("memcpy size:{} time is:{} ", bufferSize, (t1 - t0));
                
//             napi_value resultObj;
//             napi_create_object(env, &resultObj);
        
//             napi_value jsWidth, jsHeight;
//             napi_create_uint32(env, data->m_width, &jsWidth);
//             napi_create_uint32(env, data->m_height, &jsHeight);

//             napi_set_named_property(env, resultObj, "data", arrayBuffer);
//             napi_set_named_property(env, resultObj, "width", jsWidth);
//             napi_set_named_property(env, resultObj, "height", jsHeight);
//             return resultObj; 
//         }
//     }
//     return nullptr;
// }

// napi_value HJPlayerNapi::openFaceu(napi_env env, napi_callback_info info)
// {
//     int i_err = HJ_OK;
//     NAPI_PARSE_ARGS(PARAM_COUNT_2)
    
//     int64_t nPlayerHandle;
//     bool lossless = true;
//     napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

//     auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
//     if (nPlayerPtr)
//     {
//         size_t strLen;
//         napi_get_value_string_utf8(env, args[INDEX_1], nullptr, 0, &strLen);

//         std::vector<char> buffer(strLen + 1);
//         napi_get_value_string_utf8(env, args[INDEX_1], buffer.data(), buffer.size(), nullptr);
//         std::string url = std::string(buffer.data());
//         i_err = nPlayerPtr->openFaceu(url);
//     }

//     NAPI_INT_RET
// }
// napi_value HJPlayerNapi::closeFaceu(napi_env env, napi_callback_info info)
// {
//     int i_err = HJ_OK;
//     NAPI_PARSE_ARGS(PARAM_COUNT_1)
       
//     int64_t nPlayerHandle;
//     bool lossless = true;
//     napi_get_value_bigint_int64(env, args[INDEX_0], &nPlayerHandle, &lossless);

//     auto nPlayerPtr = reinterpret_cast<HJPlayerBridge *>(nPlayerHandle);
//     if (nPlayerPtr)
//     {
//         nPlayerPtr->closeFaceu();
//     }

//     NAPI_INT_RET
// }

NS_HJ_END
