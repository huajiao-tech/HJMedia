#include "HJPusherNapi.h"

#include "HJFLog.h"
#include "HJJson.h"
#include "HJNativeCommon.h"
#include "HJNapiUtils.h"
#include "HJPusherTypes.h"
#include "HJMediaInfo.h"
#include "ThreadSafeFunctionWrapper.h"
#include "HJJNIField.h"
#include "HJPusherBridge.h"
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <native_buffer/native_buffer.h>

NS_HJ_BEGIN

static std::string TAG = "wkshhh";

napi_value HJPusherNapi::contextInit(napi_env env, napi_callback_info info)
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

    i_err = HJPusherBridge::contextInit({
        .logIsValid = config->valid, .logDir = config->logDir, .logLevel = config->logLevel, .logMode = config->logMode,
        .logMaxFileSize = config->maxSize, .logMaxFileNum = config->maxFiles
    });

    NAPI_INT_RET
}

napi_value HJPusherNapi::createPusher(napi_env env, napi_callback_info info)
{
    auto nPusherPtr = new HJPusherBridge();

    napi_value nPusherHandle;
    napi_create_bigint_int64(env, reinterpret_cast<int64_t>(nPusherPtr), &nPusherHandle);

    return nPusherHandle;
}


napi_value HJPusherNapi::destroyPusher(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge*>(nPusherHandle);
    delete nPusher;

    return nullptr;
}

napi_value HJPusherNapi::setWindow(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    do
    {
        NAPI_PARSE_ARGS(PARAM_COUNT_2)

        int64_t nPusherHandle;
        bool lossless = true;
        napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
        auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);

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
        i_err = nPusher->setNativeWindow(nativeWindow, config->width, config->height, config->state);
    } while (false);

    NAPI_INT_RET
}

napi_value HJPusherNapi::openPreview(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_3)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    std::string jsonStr;
    NapiUtil::JsValueToString(env, args[INDEX_1], STR_DEFAULT_SIZE, jsonStr);
    auto jsonDoc = std::make_shared<HJYJsonDocument>();
    auto config = std::make_shared<PreviewConfig>(jsonDoc);
    if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    auto tsfn = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_2]);
    nPusher->setThreadSafeFunction(std::move(tsfn));

    uint64_t surfaceId;
    nPusher->openPreview(
        {.videoWidth = config->realWidth, .videoHeight = config->realHeight, .videoFps = config->previewFps}, [nPusher
        ](int i_type, const std::string &i_msgInfo) {
            // HJFLogi("{} HJPusherNapi::openPreview {} {}", TAG, i_type, i_msgInfo);
            if (nPusher->isThreadSafeFunctionVaild()) {
                auto data = new HJYJsonDocument{};
                data->init();
                data->setMember("type", i_type);
                data->setMember("msgInfo", i_msgInfo);
                nPusher->Call(data);
            }
        }, surfaceId);

    napi_value nSurfaceId;
    napi_create_int64(env, static_cast<int64_t>(surfaceId), &nSurfaceId);

    return nSurfaceId;
}

napi_value HJPusherNapi::closePreview(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    nPusher->closePreview();

    return nullptr;
}

napi_value HJPusherNapi::openPusher(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    std::string jsonStr;
    NapiUtil::JsValueToString(env, args[INDEX_1], STR_DEFAULT_SIZE, jsonStr);
    auto jsonDoc = std::make_shared<HJYJsonDocument>();
    auto config = std::make_shared<PusherConfig>(jsonDoc);
    if (jsonDoc->init(jsonStr) != HJ_OK || config->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);

    i_err = nPusher->openPusher({
                                  .videoCodecId = config->videoConfig->codecID,
                                  .videoWidth = config->videoConfig->width,
                                  .videoHeight = config->videoConfig->height,
                                  .videoBitrateBit = config->videoConfig->bitrate,
                                  .videoFramerate = config->videoConfig->frameRate,
                                  .videoGopSize = config->videoConfig->gopSize,
                              }, {
                                  .audioCodecId = config->audioConfig->codecID,
                                  .audioBitrateBit = config->audioConfig->bitrate,
                                  .audioCaptureSampleRate = config->audioConfig->samplesRate,
                                  .audioCaptureChannels = config->audioConfig->channels,
                                  .audioEncoderSampleRate = config->audioConfig->samplesRate,
                                  .audioEncoderChannels = config->audioConfig->channels,
                                  .audioSampleFormat = config->audioConfig->sampleFmt
                              }, {
                                  .rtmpUrl = config->url
                              });
    
    NAPI_INT_RET
}

napi_value HJPusherNapi::closePusher(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    nPusher->closePusher();

    return nullptr;
}

napi_value HJPusherNapi::setMute(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    bool mute;
    napi_get_value_bool(env, args[INDEX_1], &mute);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    nPusher->setMute(mute);

    return nullptr;
}

napi_value HJPusherNapi::openRecorder(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        HJPusherRecordInfo recorderInfo;
        {
            napi_value thiz = args[1];
            JNI_OBJ_FIELD_MAP(recorderInfo.recordUrl);
        }
        i_err = nPusher->openRecorder(recorderInfo);
    }
    NAPI_INT_RET
}

napi_value HJPusherNapi::closeRecorder(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        nPusher->closeRecorder();
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::openPngSeq(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    std::string url;
    NapiUtil::JsValueToString(env, args[INDEX_1], STR_DEFAULT_SIZE, url);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        i_err = nPusher->openPngSeq({.pngSeqUrl = url});
    }

    NAPI_INT_RET
}

NS_HJ_END