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

#include "HJImageNative.h"

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
    
    //
    HJEntryStatInfo statInfo;
    statInfo.bUseStat = false; //fixme after modify
    
    nPusher->openPreview(
        {.videoWidth = config->realWidth, .videoHeight = config->realHeight, .videoFps = config->previewFps}, [nPusher
        ](int i_type, const std::string &i_msgInfo) {
            // HJFLogi("{} HJPusherNapi::openPreview {} {}", TAG, i_type, i_msgInfo);
            auto data = new HJYJsonDocument{};
            data->init();
            data->setMember("type", i_type);
            data->setMember("msgInfo", i_msgInfo);
            nPusher->Call(data);
        }, statInfo, surfaceId);

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
    NAPI_PARSE_ARGS(PARAM_COUNT_4)

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

    std::string jsonStr1;
    NapiUtil::JsValueToString(env, args[INDEX_2], STR_DEFAULT_SIZE, jsonStr1);
    auto jsonDoc1 = std::make_shared<HJYJsonDocument>();
    auto config1 = std::make_shared<MediaStateInfo>(jsonDoc1);
    if (jsonDoc1->init(jsonStr1) != HJ_OK || config1->deserialInfo() != HJ_OK) {
        napi_throw_error(env, nullptr, "Failed to parse JSON configuration");
        return nullptr;
    }

    auto statCall = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_3]);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);

    nPusher->setStateCall(std::move(statCall));

    HJEntryStatInfo statInfo;
    statInfo.bUseStat = true;
    statInfo.uid = config1->uid;
    statInfo.device = config1->device;
    statInfo.sn = config1->sn;
    statInfo.statNotify = [nPusher](const std::string& i_name, int i_nType, const std::string& i_info) {
        auto data = new HJYJsonDocument{};
        data->init();
        data->setMember("name", i_name);
        data->setMember("type", i_nType);
        data->setMember("info", i_info);
        nPusher->stateCall(data);
    };

    i_err = nPusher->openPusher({
                                  .videoCodecId = config->videoConfig->codecID,
                                  .videoWidth = config->videoConfig->width,
                                  .videoHeight = config->videoConfig->height,
                                  .videoBitrateBit = config->videoConfig->bitrate,
                                  .videoFramerate = config->videoConfig->frameRate,
                                  .videoGopSize = config->videoConfig->gopSize,
                                  .videoIsROIEnc = config->videoConfig->videoIsROIEnc
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
                              }, statInfo);
    
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

napi_value HJPusherNapi::setDoubleScreen(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    bool bDoubleScreen;
    napi_get_value_bool(env, args[INDEX_1], &bDoubleScreen);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        nPusher->setDoubleScreen(bDoubleScreen);
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::setGiftPusher(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    bool bGiftPusher;
    napi_get_value_bool(env, args[INDEX_1], &bGiftPusher);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        nPusher->setGiftPusher(bGiftPusher);
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::openSpeechRecognizer(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    auto audioDataCall = std::make_unique<ThreadSafeFunctionWrapper>(env, args[INDEX_1], ThreadSafeFunctionWrapper::audioDataCallback);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);

    nPusher->setAudioCall(std::move(audioDataCall));
    if (nPusher)
    {
        nPusher->openSpeechRecognizer([nPusher](const HJMediaFrame::Ptr &i_frame) {
            uint8_t* data = nullptr;
            int data_size;
            HJMediaFrame::getDataFromAVFrame(i_frame, data, data_size);
            auto data1 = std::make_unique<uint8_t[]>(data_size);
            memcpy(data1.get(), data, data_size);
            auto* data2 = new std::pair<std::unique_ptr<uint8_t[]>, int>(std::move(data1), data_size);
            nPusher->audioCall(data2);
        });
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::closeSpeechRecognizer(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        nPusher->closeSpeechRecognizer();
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::openPixelMapOutput(napi_env env, napi_callback_info info)
{
    auto nImageNativePtr = new HJImageNative;

    napi_value nImageNativeHandle;
    napi_create_bigint_uint64(env, reinterpret_cast<uint64_t>(nImageNativePtr), &nImageNativeHandle);

    return nImageNativeHandle;
}

napi_value HJPusherNapi::getImageReceiver(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    uint64_t nImageNativeHandle;
    bool lossless = true;
    napi_get_value_bigint_uint64(env, args[INDEX_0], &nImageNativeHandle, &lossless);

    auto nImageNativePtr = reinterpret_cast<HJImageNative *>(nImageNativeHandle);
    
    uint64_t surfaceId = nImageNativePtr->getSurfaceId();

    napi_value n_surfaceId;
    napi_create_bigint_uint64(env, surfaceId, &n_surfaceId);
    return n_surfaceId;
}

napi_value HJPusherNapi::closePixelMapOutput(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nImageNativeHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nImageNativeHandle, &lossless);

    auto nImageNativePtr = reinterpret_cast<HJImageNative *>(nImageNativeHandle);

    delete nImageNativePtr;

    NAPI_INT_RET
}

napi_value HJPusherNapi::setFaceInfo(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_4)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);

    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
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
        nPusher->setFaceInfo(faceInfo);
    }

    NAPI_INT_RET    
}
napi_value HJPusherNapi::nativeSourceOpen(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        bool bUsePBO = false;
        napi_get_value_bool(env, args[INDEX_1], &bUsePBO);
        i_err = nPusher->openNativeSource(bUsePBO);
    }

    NAPI_INT_RET
}
napi_value HJPusherNapi::nativeSourceClose(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        nPusher->closeNativeSource();
    }

    NAPI_INT_RET
}
napi_value HJPusherNapi::nativeSourceAcquire(napi_env env, napi_callback_info info)
{
    NAPI_PARSE_ARGS(PARAM_COUNT_1)

    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    std::shared_ptr<HJRGBAMediaData> data = nullptr;
    if (nPusher)
    {
        data = nPusher->acquireNativeSource();
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

napi_value HJPusherNapi::openFaceu(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
    
    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        size_t strLen;
        napi_get_value_string_utf8(env, args[INDEX_1], nullptr, 0, &strLen);

        std::vector<char> buffer(strLen + 1);
        napi_get_value_string_utf8(env, args[INDEX_1], buffer.data(), buffer.size(), nullptr);
        std::string url = std::string(buffer.data());
        i_err = nPusher->openFaceu(url);
    }

    NAPI_INT_RET
}
napi_value HJPusherNapi::closeFaceu(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_1)
       
    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        nPusher->closeFaceu();
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::setVideoEncQuantOffset(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
       
    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        int32_t quantoffset = 0;
        napi_get_value_int32(env, args[INDEX_1], &quantoffset);
        nPusher->setVideoEncQuantOffset(quantoffset);
    }

    NAPI_INT_RET
}

napi_value HJPusherNapi::setFaceProtected(napi_env env, napi_callback_info info)
{
    int i_err = HJ_OK;
    NAPI_PARSE_ARGS(PARAM_COUNT_2)
       
    int64_t nPusherHandle;
    bool lossless = true;
    napi_get_value_bigint_int64(env, args[INDEX_0], &nPusherHandle, &lossless);
    auto nPusher = reinterpret_cast<HJPusherBridge *>(nPusherHandle);
    if (nPusher)
    {
        bool bFaceProtected = false;
        napi_get_value_bool(env, args[INDEX_1], &bFaceProtected);
        nPusher->setFaceProtected(bFaceProtected);
    }

    NAPI_INT_RET
}

NS_HJ_END