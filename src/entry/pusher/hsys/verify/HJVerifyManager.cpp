#include "HJVerifyManager.h"

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <cstdint>
#include <hilog/log.h>
#include <string>
#include <vector>
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJFFHeaders.h"
#include "HJTransferInfo.h"
#include "HJMediaData.h"
#include "HJJson.h"
#include "HJRteGraphSetupInfo.h"

NS_HJ_USING

namespace NativeXComponentSample {

#define RESTART_PUSH    0
#define RESTART_RECORD  0

int HJVerifyManager::s_previewFps = 30;

int HJVerifyManager::s_videoCodecId = (int)AV_CODEC_ID_H265;
int HJVerifyManager::s_width = 720;
int HJVerifyManager::s_height = 1280;
int HJVerifyManager::s_videobitrate = 500 * 1000;//2.2 * 1000 * 1000;
int HJVerifyManager::s_framerate = 15; 
int HJVerifyManager::s_gopsize = 2 * s_framerate;
bool HJVerifyManager::s_roiIsenc = true;
    
int HJVerifyManager::s_audioCodecId = (int)AV_CODEC_ID_AAC;
int HJVerifyManager::s_audiobitrate = 64 * 1000;
int HJVerifyManager::s_fmt = (int)AV_SAMPLE_FMT_S16;
int HJVerifyManager::s_samplerate = 48000;
int HJVerifyManager::s_channels = 2;
bool HJVerifyManager::s_debugPoint = true;
bool HJVerifyManager::s_bGPUToRAMUsePBO = true;
bool HJVerifyManager::s_bFaceProtected = true;

std::string HJVerifyManager::s_rtmpUrl = "rtmp://live-push-1.huajiao.com/main/lfs?auth_key=1769683326-0-0-ac789b2dd63f6e1f8099799d01d7b5f0";
//"flv  https://live-pull-1.huajiao.com/main/lfs.flv?auth_key=1756107592-0-0-18b9d1330cbf136b5187f7909dec501c"
std::string HJVerifyManager::s_localUrl = "/data/storage/el2/base/haps/entry/files/rec.mp4";
std::string HJVerifyManager::s_faceuUrl = "/data/storage/el1/bundle/entry/resources/resfile/800624_1_1504602801";    //"/data/storage/el2/base/haps/entry/files/60031_10";//19154_3";
HJVerifyManager HJVerifyManager::m_HJVerifyManager;

#if defined(HJ_ENABLE_RENDER_PRIO)
    HJNAPITestLive::Ptr HJVerifyManager::m_testLiveStream = nullptr;
#endif

HJTimerThreadPool::Ptr HJVerifyManager::m_testThreadTimer = nullptr;
HJThreadPool::Ptr HJVerifyManager::m_testThreadPool = nullptr;

void HJVerifyManager::priTest()
{
#if defined(HJ_ENABLE_RENDER_PRIO)    
    if (!m_testThreadPool)
    {
        m_testThreadPool = HJThreadPool::Create();
        m_testThreadPool->start();
    }    
    
    if (m_testThreadPool)
    {
#if 1      
        m_testThreadPool->async([](){
            HJPusherVideoInfo o_videoInfo;
            HJPusherAudioInfo o_audioInfo;
            HJPusherRTMPInfo o_rtmpInfo;
            priConstructParam(o_videoInfo, o_audioInfo, o_rtmpInfo);
            return m_testLiveStream->openPusher(o_videoInfo, o_audioInfo, o_rtmpInfo);
        }, 2000);
#endif
        
#if 0
        m_testThreadPool->async([](){
            HJPusherPNGSegInfo pngInfo;
            pngInfo.pngSeqUrl = std::string("/data/storage/el2/base/haps/entry/files/pngseq/ShuangDanCaiShen");
            return m_testLiveStream->openPngSeq(pngInfo);
        }, 1000);
#endif
        
#if 0
        m_testThreadPool->async([](){
            HJPusherRecordInfo recordInfo;
            recordInfo.recordUrl = std::string(s_localUrl);
            return m_testLiveStream->openRecorder(recordInfo);
        }, 3000);
                
        m_testThreadPool->async([](){
            m_testLiveStream->closeRecorder();
            return 0;
        }, 13000);
#endif
    }    
    if (!m_testThreadTimer)
    {
        m_testThreadTimer = HJTimerThreadPool::Create();
        m_testThreadTimer->startTimer(5000, []()
        {
            int ret = 0;
#if RESTART_RECORD
            m_testLiveStream->closeRecorder();
            HJPusherRecordInfo recordInfo;
            recordInfo.recordUrl = std::string(s_localUrl);
            ret = m_testLiveStream->openRecorder(recordInfo);
#endif
            
#if RESTART_PUSH
            HJFLogi("restartpush close push enter");
            m_testLiveStream->closePusher();
            HJFLogi("restartpush close push end, push create");
            HJPusherVideoInfo o_videoInfo;
            HJPusherAudioInfo o_audioInfo;
            HJPusherRTMPInfo o_rtmpInfo;
            priConstructParam(o_videoInfo, o_audioInfo, o_rtmpInfo);
            ret = m_testLiveStream->openPusher(o_videoInfo, o_audioInfo, o_rtmpInfo);
#endif
            return ret;
        });
    }    
#endif
}
napi_value HJVerifyManager::testOpen(napi_env env, napi_callback_info info)
{
#if defined(HJ_ENABLE_RENDER_PRIO)    
    static bool s_btestInit = false;
    if (!s_btestInit)
    {
        HJEntryContextInfo contextInfo;
        contextInfo.logIsValid = true;
        contextInfo.logDir = "/data/storage/el2/base/haps/entry/files/log_pusher";
        contextInfo.logLevel = HJLOGLevel_INFO;
        contextInfo.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
        contextInfo.logMaxFileSize = 1024 * 1024 * 5;
        contextInfo.logMaxFileNum = 5;
        HJNAPITestLive::contextInit(contextInfo);
        s_btestInit = true;
    }
    
    NativeWindow *window = nullptr;
    int width = 0;
    int height = 0;
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    int i_err = render->test(window, width, height);
    if (i_err < 0)
    {
        return nullptr;
    }  
    
    //if (width > height)
    //{
        s_width = 1280;
        s_height = 720;  
    //}
    //else
    //{
    //    s_width = 720;
    //    s_height = 1280; 
    //}    
    m_testLiveStream = HJNAPITestLive::Create();
  
    
    HJPusherPreviewInfo previewInfo;
    previewInfo.videoWidth = s_width;
    previewInfo.videoHeight = s_height;
    previewInfo.videoFps = s_previewFps;
    uint64_t surfaceId = 0;
    i_err = m_testLiveStream->openPreview(previewInfo, [](int i_type, const std::string& i_msgInfo)
    {
    
    }, surfaceId);
    if (i_err < 0)
    {
        HJLoge("open preview error", i_err);
        return nullptr;
    }    
    i_err = m_testLiveStream->setNativeWindow(window, width, height, HJ::HJTargetState_Create);
    if (i_err < 0)
    {
        HJLoge("setNativeWindow error", i_err);
        return nullptr;
    } 
    
    priTest();

    napi_value nSurfaceId;
    napi_create_int64(env, (int64_t)surfaceId, &nSurfaceId);
    return nSurfaceId;
#else
    return nullptr;
#endif
}
napi_value HJVerifyManager::testClose(napi_env env, napi_callback_info info)
{
#if defined(HJ_ENABLE_RENDER_PRIO)    
    HJFLogi("test close enter");
    if (m_testThreadTimer)
    {
        HJFLogi("test close m_testThreadTimer enter");
        m_testThreadTimer->stopTimer();
        m_testThreadTimer = nullptr;
    }     
    if (m_testThreadPool)
    {
        HJFLogi("test close m_testThreadPool sync enter");
        m_testThreadPool->sync([]()
        {
           if (m_testLiveStream)
           {
                HJFLogi("test close closePush enter");
                m_testLiveStream->closePusher();
                HJFLogi("test close closePreview enter"); 
                m_testLiveStream->closePreview();
                m_testLiveStream = nullptr;
           }
           return 0;
        });
        HJFLogi("test close m_testThreadPool done enter"); 
        m_testThreadPool->done();
        HJFLogi("test close m_testThreadPool done end"); 
        m_testThreadPool = nullptr;
    }    
#endif
    return nullptr;
}

napi_value HJVerifyManager::tryOpenPreview(napi_env env, napi_callback_info info)
{
    static bool s_bInit = false;
    if (!s_bInit)
    {
        HJEntryContextInfo contextInfo;
        contextInfo.logIsValid = true;
        contextInfo.logDir = "/data/storage/el2/base/haps/entry/files/log_pusher";
        contextInfo.logLevel = HJLOGLevel_INFO;
        contextInfo.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
        contextInfo.logMaxFileSize = 1024 * 1024 * 5;
        contextInfo.logMaxFileNum = 5;
        HJVerifyRender::contextInit(contextInfo);
    }
    
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    uint64_t surfaceId = 0;
    std::string graphConfig;
    if ((env != nullptr) && (info != nullptr))
    {
        size_t argCnt = 1;
        napi_value args[1] = { nullptr };
        if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) == napi_ok && argCnt >= 1 && args[0] != nullptr)
        {
            napi_valuetype argType = napi_undefined;
            if (napi_typeof(env, args[0], &argType) == napi_ok && argType == napi_string)
            {
                size_t strLen = 0;
                if (napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen) == napi_ok)
                {
                    std::vector<char> buffer(strLen + 1);
                    if (napi_get_value_string_utf8(env, args[0], buffer.data(), buffer.size(), nullptr) == napi_ok)
                    {
                        graphConfig = std::string(buffer.data());
                    }
                }
            }
        }
    }
    if (render)
    {
        NativeWindow *window = nullptr;
        int width = 0;
        int height = 0;
        int i_err = render->test(window, width, height);
//        if (width > height)
//        {
            s_width = 1280;
            s_height = 720;  
//        }
//        else
//        {
//            s_width = 720;
//            s_height = 1280; 
//        }    
                            
        HJPusherPreviewInfo previewInfo;
        previewInfo.videoWidth = s_width;
        previewInfo.videoHeight = s_height;
        previewInfo.videoFps = 30;
        previewInfo.m_graphConfig = graphConfig;
        i_err = render->openPreview(previewInfo, [](int i_type, const std::string& i_msgInfo)
        {
    
        }, surfaceId);
        if (i_err < 0)
        {
            HJLoge("open preview error", i_err);
            return nullptr;
        }    
    }
    napi_value nSurfaceId;
    napi_create_int64(env, (int64_t)surfaceId, &nSurfaceId);
    return nSurfaceId;
}
napi_value HJVerifyManager::trySetROIOffset(napi_env env, napi_callback_info info)
{

    if ((env == nullptr) || (info == nullptr))
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext env or info is null ");
        return nullptr;
    }

    size_t argCnt = 1;
    napi_value args[1];
    if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext napi_get_cb_info failed");
    }
    int32_t quantoffset = 0;
    napi_get_value_int32(env, args[0], &quantoffset);
    
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render->getLiveStream();
    if (liveStream)
    {
        liveStream->setVideoEncQuantOffset(quantoffset);
    }   
    return nullptr;
}

napi_value HJVerifyManager::tryNodeSetParam(napi_env env, napi_callback_info info)
{
    if ((env == nullptr) || (info == nullptr))
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "tryNodeSetParam env or info is null ");
        return nullptr;
    }

    size_t argCnt = 3;
    napi_value args[3] = { nullptr };
    if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok || argCnt < 3)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "tryNodeSetParam napi_get_cb_info failed");
        return nullptr;
    }

    std::string classStyle;
    std::string insName;
    std::string paramInfo;
    for (size_t i = 0; i < argCnt && i < 3; ++i)
    {
        size_t strLen = 0;
        if (args[i] == nullptr || napi_get_value_string_utf8(env, args[i], nullptr, 0, &strLen) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "tryNodeSetParam invalid string arg");
            return nullptr;
        }

        std::vector<char> buffer(strLen + 1);
        if (napi_get_value_string_utf8(env, args[i], buffer.data(), buffer.size(), nullptr) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "tryNodeSetParam read string arg failed");
            return nullptr;
        }

        if (i == 0)
        {
            classStyle = std::string(buffer.data());
        }
        else if (i == 1)
        {
            insName = std::string(buffer.data());
        }
        else
        {
            paramInfo = std::string(buffer.data());
        }
    }

    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render ? render->getLiveStream() : nullptr;
    if (liveStream)
    {
        HJBaseParam::Ptr param = HJBaseParam::Create();
        auto jsonDoc = HJYJsonDocument::createWithInfo(paramInfo);
        if (!jsonDoc)
        {
            HJFLoge("tryNodeSetParam parse json failed paramInfo:{}", paramInfo);
            return nullptr;
        }

        for (const auto& key : jsonDoc->getKeys())
        {
            HJYJsonObject::Ptr valueObj = jsonDoc->getObj(key);
            if (!valueObj)
            {
                continue;
            }
            if (valueObj->isBool())
            {
                HJ_CatchMapPlainSetVal(param, bool, key, valueObj->getBool());
                continue;
            }
            if (valueObj->isStr())
            {
                HJ_CatchMapPlainSetVal(param, std::string, key, valueObj->getString());
                continue;
            }
            if (valueObj->isReal())
            {
                HJ_CatchMapPlainSetVal(param, float, key, static_cast<float>(valueObj->getReal()));
                continue;
            }
            if (valueObj->isInt64())
            {
                HJ_CatchMapPlainSetVal(param, int64_t, key, valueObj->getInt64());
                continue;
            }
            if (valueObj->isUInt64())
            {
                HJ_CatchMapPlainSetVal(param, uint64_t, key, valueObj->getUInt64());
            }
        }

        liveStream->nodeSetParam(classStyle, insName, param);
    }
    return nullptr;
}

napi_value HJVerifyManager::trySetPreviewRotation(napi_env env, napi_callback_info info)
{
    if ((env == nullptr) || (info == nullptr))
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "trySetPreviewRotation env or info is null ");
        return nullptr;
    }

    size_t argCnt = 1;
    napi_value args[1];
    if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok)
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "trySetPreviewRotation napi_get_cb_info failed");
        return nullptr;
    }

    int32_t rotation = 0;
    napi_get_value_int32(env, args[0], &rotation);

    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render->getLiveStream();
    if (liveStream)
    {
        liveStream->setPreviewRotation(rotation);
    }
#if defined(HJ_ENABLE_RENDER_PRIO)    
    if (m_testLiveStream)
    {
        m_testLiveStream->setPreviewRotation(rotation);
    }
#endif
    return nullptr;
}

void HJVerifyManager::priTest(HJVerifyRender* render)
{
#if defined(HJ_ENABLE_RENDER_PRIO)    
    if (!m_testThreadPool)
    {
        m_testThreadPool = HJThreadPool::Create();
        m_testThreadPool->start();
    }    
    
    if (m_testThreadPool)
    {
#if 0
        m_testThreadPool->async([render](){
            HJPusherPNGSegInfo pngInfo;
            pngInfo.pngSeqUrl = std::string("/data/storage/el2/base/haps/entry/files/PK_ZUOJIA");
            return render->openPngSeq(pngInfo);
        }, 1000);
#endif
        
#if 0
        m_testThreadPool->async([render](){
            HJPusherRecordInfo recordInfo;
            recordInfo.recordUrl = std::string(s_localUrl);
            return render->openRecorder(recordInfo);
        }, 10000);
                
        m_testThreadPool->async([render](){
            render->closeRecorder();
            return 0;
        }, 53000);
#endif
    }
    
    if (!m_testThreadTimer)
    {
        m_testThreadTimer = HJTimerThreadPool::Create();
        m_testThreadTimer->startTimer(10000, [render]()
        {
            int ret = 0;
#if RESTART_RECORD
            m_testLiveStream->closeRecorder();
            HJPusherRecordInfo recordInfo;
            recordInfo.recordUrl = std::string(s_localUrl);
            ret = m_testLiveStream->openRecorder(recordInfo);
#endif
            
#if 1
            HJFLogi("restartpush close push enter");
            render->closePusher();
            HJFLogi("restartpush close push end, push create");
            HJPusherVideoInfo o_videoInfo;
            HJPusherAudioInfo o_audioInfo;
            HJPusherRTMPInfo o_rtmpInfo;
            priConstructParam(o_videoInfo, o_audioInfo, o_rtmpInfo);
            ret = render->openPusher(o_videoInfo, o_audioInfo, o_rtmpInfo);
#endif
            return ret;
        });  
    }
#endif
    
}
napi_value HJVerifyManager::tryOpenPush(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    if (render)
    {
#if 1
        HJPusherVideoInfo o_videoInfo;
        HJPusherAudioInfo o_audioInfo;
        HJPusherRTMPInfo o_rtmpInfo;
        priConstructParam(o_videoInfo, o_audioInfo, o_rtmpInfo);
        HJFLogi("openPusher enter");
        int i_err = render->openPusher(o_videoInfo, o_audioInfo, o_rtmpInfo);
        HJFLogi("openPusher end i_err:{}", i_err);
        if (i_err < 0)
        {
            HJLoge("openPush error", i_err);
            return nullptr;
        }
#else
        priTest(render);
#endif
    }  
    return nullptr;
}
napi_value HJVerifyManager::tryOpenRecord(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    if (render)
    {
        HJPusherRecordInfo recordInfo;
        recordInfo.recordUrl = std::string(s_localUrl);
        int i_err = render->openRecorder(recordInfo);
        if (i_err < 0)
        {
            HJFLoge("openRecorder error");
        }    
    }  
    return nullptr;    
}
napi_value HJVerifyManager::tryCloseRecord(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    if (render)
    {
        render->closeRecorder();
    }  
    return nullptr;   
}

napi_value HJVerifyManager::tryClosePreview(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    
    if (render)
    {
        render->closePreview();
    }
    return nullptr;    
}

napi_value HJVerifyManager::tryGiftOpen(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    
    if (render)
    {
        render->tryGiftOpen();
    }
    return nullptr;    
}
napi_value HJVerifyManager::tryDoubleScreen(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    
    if (render)
    {
        render->tryDoubleScreen();
    }
    return nullptr;    
}
napi_value HJVerifyManager::tryGiftPusher(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    
    if (render)
    {
        render->tryGiftPusher();
    } 
    return nullptr;    
}

napi_value HJVerifyManager::tryOpenImgReceiver(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render->getLiveStream();
    if (liveStream)
    {
        liveStream->openNativeSource(s_bGPUToRAMUsePBO);
        liveStream->openFaceu(s_faceuUrl);
        liveStream->setFaceProtected(s_bFaceProtected);
    }    
    return nullptr;  
}
napi_value HJVerifyManager::tryCloseImageReceiver(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render->getLiveStream();
    if (liveStream)
    {
        liveStream->closeFaceu();
        liveStream->closeNativeSource(); 
    } 
    return nullptr;  
}
napi_value HJVerifyManager::tryGetMediaData(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render->getLiveStream();
    std::shared_ptr<HJRGBAMediaData> data = nullptr;
    if (liveStream)
    {
       data = liveStream->acquireNativeSource();
    }
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
        
            int64_t t0 = HJCurrentSteadyMS();
            memcpy(pData, data->m_buffer->getBuf(), bufferSize);
            int64_t t1 = HJCurrentSteadyMS();
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
    return nullptr;  
}
napi_value HJVerifyManager::trySetFacePoints(napi_env env, napi_callback_info info)
{
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
    HJ::HJNAPILiveStream::Ptr liveStream = render->getLiveStream();
    if (liveStream)
    {
        if ((env == nullptr) || (info == nullptr))
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext env or info is null ");
            return nullptr;
        }

        size_t argCnt = 4;
        napi_value args[4] = {nullptr};
        if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext napi_get_cb_info failed");
            return nullptr;
        }

        std::string sourceInsName = HJRteGraphConfig::HJNodeClass_SourceBridge;
        int32_t w = 0;
        int32_t h = 0;
        std::string pointsInfo;

        napi_valuetype arg0Type = napi_undefined;
        if (argCnt > 0 && args[0] != nullptr)
        {
            napi_typeof(env, args[0], &arg0Type);
        }

        if (argCnt >= 4 && arg0Type == napi_string)
        {
            size_t nameLen = 0;
            napi_get_value_string_utf8(env, args[0], nullptr, 0, &nameLen);
            std::vector<char> nameBuf(nameLen + 1);
            napi_get_value_string_utf8(env, args[0], nameBuf.data(), nameBuf.size(), nullptr);
            sourceInsName = std::string(nameBuf.data());

            napi_get_value_int32(env, args[1], &w);
            napi_get_value_int32(env, args[2], &h);

            size_t strLen = 0;
            napi_get_value_string_utf8(env, args[3], nullptr, 0, &strLen);
            std::vector<char> buffer(strLen + 1);
            napi_get_value_string_utf8(env, args[3], buffer.data(), buffer.size(), nullptr);
            pointsInfo = std::string(buffer.data());
        }
        else if (argCnt >= 3)
        {
            // backward compatible: trySetFacePoints(width, height, faceInfo)
            napi_get_value_int32(env, args[0], &w);
            napi_get_value_int32(env, args[1], &h);

            size_t strLen = 0;
            napi_get_value_string_utf8(env, args[2], nullptr, 0, &strLen);
            std::vector<char> buffer(strLen + 1);
            napi_get_value_string_utf8(env, args[2], buffer.data(), buffer.size(), nullptr);
            pointsInfo = std::string(buffer.data());
        }
        else
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "trySetFacePoints invalid args");
            return nullptr;
        }
        
        liveStream->setFaceInfo(sourceInsName, w, h, pointsInfo, s_debugPoint);
    } 
    return nullptr;  
}


//void HJVerifyManager::priTimerRestartRecorder(HJVerifyRender* render)
//{
//    if (!m_threadTimer)
//    {
//        m_threadTimer = HJTimerThreadPool::Create();
//        m_threadTimer->startTimer(4000, [render]()
//        {
//            render->closeRecorder();
//            HJ::HJBaseParam::Ptr param = HJBaseParam::Create();
//            (*param)["localRecordUrl"] = std::string(s_localUrl);
//            return render->openRecorder(param);
//        });  
//    }
//}

void HJVerifyManager::priConstructParam(HJPusherVideoInfo& o_videoInfo, HJPusherAudioInfo& o_audioInfo, HJPusherRTMPInfo& o_rtmpInfo)
{
    int width = 0;
    int height = 0;  
    NativeWindow *window = nullptr;
    HJVerifyRender* render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);    
    int i_err = render->test(window, width, height);
    if (width > height)
    {
        o_videoInfo.videoWidth = s_width;
        o_videoInfo.videoHeight = s_height;
    }
    else
    {
        o_videoInfo.videoWidth = s_height;
        o_videoInfo.videoHeight = s_width;
    }    
    o_videoInfo.videoCodecId = s_videoCodecId;

    o_videoInfo.videoBitrateBit = s_videobitrate;
    o_videoInfo.videoFramerate = s_framerate;
    o_videoInfo.videoGopSize = s_gopsize;
    o_videoInfo.videoIsROIEnc = s_roiIsenc;
    
    o_audioInfo.audioCodecId = s_audioCodecId;
    o_audioInfo.audioBitrateBit = s_audiobitrate;
    o_audioInfo.audioSampleFormat = s_fmt;
    o_audioInfo.audioCaptureSampleRate = s_samplerate;
    o_audioInfo.audioCaptureChannels = s_channels;
    o_audioInfo.audioEncoderSampleRate = s_samplerate;
    o_audioInfo.audioEncoderChannels = s_channels;
        
    o_rtmpInfo.rtmpUrl = s_rtmpUrl;
}
////////////////////////////////////////////////

HJVerifyManager::~HJVerifyManager()
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "Callback", "~HJVerifyManager");
    for (auto iter = m_nativeXComponentMap.begin(); iter != m_nativeXComponentMap.end(); ++iter) {
        if (iter->second != nullptr) {
            delete iter->second;
            iter->second = nullptr;
        }
    }
    m_nativeXComponentMap.clear();

    for (auto iter = m_HJVerifyRenderMap.begin(); iter != m_HJVerifyRenderMap.end(); ++iter) {
        if (iter->second != nullptr) {
            delete iter->second;
            iter->second = nullptr;
        }
    }
    m_HJVerifyRenderMap.clear();
}

napi_value HJVerifyManager::GetContext(napi_env env, napi_callback_info info) {
    if ((env == nullptr) || (info == nullptr)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext env or info is null ");
        return nullptr;
    }

    size_t argCnt = 1;
    napi_value args[1] = { nullptr };
    if (napi_get_cb_info(env, info, &argCnt, args, nullptr, nullptr) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "GetContext napi_get_cb_info failed");
    }

    if (argCnt != 1) {
        napi_throw_type_error(env, NULL, "Wrong number of arguments");
        return nullptr;
    }

    napi_valuetype valuetype;
    if (napi_typeof(env, args[0], &valuetype) != napi_ok) {
        napi_throw_type_error(env, NULL, "napi_typeof failed");
        return nullptr;
    }

    if (valuetype != napi_number) {
        napi_throw_type_error(env, NULL, "Wrong type of arguments");
        return nullptr;
    }

    int64_t value;
    if (napi_get_value_int64(env, args[0], &value) != napi_ok) {
        napi_throw_type_error(env, NULL, "napi_get_value_int64 failed");
        return nullptr;
    }

    napi_value exports;
    if (napi_create_object(env, &exports) != napi_ok) {
        napi_throw_type_error(env, NULL, "napi_create_object failed");
        return nullptr;
    }

    return exports;
}

void HJVerifyManager::Export(napi_env env, napi_value exports) {
    if ((env == nullptr) || (exports == nullptr)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: env or exports is null");
        return;
    }

    napi_value exportInstance = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: napi_get_named_property fail");
        return;
    }

    OH_NativeXComponent* nativeXComponent = nullptr;
    if (napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent)) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: napi_unwrap fail");
        return;
    }

    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = { '\0' };
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        OH_LOG_Print(
            LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: OH_NativeXComponent_GetXComponentId fail");
        return;
    }

    std::string id(idStr);
    auto context = HJVerifyManager::GetInstance();
    if ((context != nullptr) && (nativeXComponent != nullptr)) {
        context->SetNativeXComponent(id, nativeXComponent);
        auto render = context->GetRender(id);
        if (render != nullptr) {
            render->RegisterCallback(nativeXComponent);
            render->Export(env, exports);
        }
    }
}

void HJVerifyManager::SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent) {
    if (nativeXComponent == nullptr) {
        return;
    }

    if (m_nativeXComponentMap.find(id) == m_nativeXComponentMap.end()) {
        m_nativeXComponentMap[id] = nativeXComponent;
        return;
    }

    if (m_nativeXComponentMap[id] != nativeXComponent) {
        OH_NativeXComponent* tmp = m_nativeXComponentMap[id];
        delete tmp;
        tmp = nullptr;
        m_nativeXComponentMap[id] = nativeXComponent;
    }
}

HJVerifyRender* HJVerifyManager::GetRender(const std::string& id) {
    if (m_HJVerifyRenderMap.find(id) == m_HJVerifyRenderMap.end()) {
        HJVerifyRender* instance = HJVerifyRender::GetInstance(id);
        m_HJVerifyRenderMap[id] = instance;
        return instance;
    }

    return m_HJVerifyRenderMap[id];
}
} 
