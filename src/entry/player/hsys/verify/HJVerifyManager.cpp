#include "HJVerifyManager.h"

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <cstdint>
#include <hilog/log.h>
#include <string>
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJFFHeaders.h"
#include "HJTransferInfo.h"
#include "HJPrioUtils.h"
#include "HJTime.h"

NS_HJ_USING

namespace NativeXComponentSample
{

    std::string HJVerifyManager::s_path = "/data/storage/el2/base/haps/entry/files/";
    //std::string HJVerifyManager::s_url = s_path + "ShuangDanCaiShen.mp4";//"huajiaot.mp4";//"huajiaoline2_noaudio.flv";

    std::string HJVerifyManager::s_url = //s_path + "ShuangDanCaiShen.mp4";//"http://www.w3school.com.cn/example/html5/mov_bbb.mp4";
    "https://live-pull-1.huajiao.com/main/HJ_0_ali_1_main__h265_139879955_1758079178061_9753_O.flv?auth_key=1758165659-0-0-356f26c174c655d814726a8ce60c08cb";
    
    //s_path + "PK_ZUOJIA.mp4";
    int HJVerifyManager::s_fps = 30;
    int HJVerifyManager::s_videoCodecType = HJPlayerVideoCodecType_OHCODEC; // HJPlayerVideoCodecType_SoftDefault;//HJPlayerVideoCodecType_OHCODEC;//HJPlayerVideoCodecType_SoftDefault;
    int HJVerifyManager::s_sourceType = HJPrioComSourceType_SERIES;
   
    HJThreadPool::Ptr HJVerifyManager::m_exitThread = nullptr;
    HJTimerThreadPool::Ptr HJVerifyManager::m_playerThread = nullptr;
    std::deque<HJNAPIPlayer::Ptr> HJVerifyManager::m_playerActiveQueue;
    std::deque<HJNAPIPlayer::Ptr> HJVerifyManager::m_playerDisposeQueue;
    std::atomic<bool> HJVerifyManager::m_bQuit{false};
    int HJVerifyManager::m_restartTime = 1000;//60 * 60 * 1000;
    int HJVerifyManager::s_logCnt = 5;

    HJVerifyManager HJVerifyManager::m_HJVerifyManager;

    HJNAPITestPlayer::Ptr HJVerifyManager::m_testPlayer = nullptr;
    HJTimerThreadPool::Ptr HJVerifyManager::m_testThreadTimer = nullptr;
    HJThreadPool::Ptr HJVerifyManager::m_testThreadPool = nullptr;

    napi_value HJVerifyManager::tryOpen(napi_env env, napi_callback_info info)
    {
        HJFLogi("playerexit tryOpen enter");
        static bool s_bCtxInit = false;
        if (!s_bCtxInit)
        {
            HJEntryContextInfo contextInfo;
            contextInfo.logIsValid = true;
            contextInfo.logDir = "/data/storage/el2/base/haps/entry/files/log_player";
            contextInfo.logLevel = HJLOGLevel_INFO;
            contextInfo.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
            contextInfo.logMaxFileSize = 1024 * 1024 * 5;
            contextInfo.logMaxFileNum = s_logCnt;
            HJNAPIPlayer::contextInit(contextInfo);
            s_bCtxInit = true;
        }
        if (m_playerThread)
        {
            HJFLogi("playerexit m_playerThread is not nullptr, return");
            return nullptr;
        }
        NativeWindow *window = nullptr;
        int width = 0;
        int height = 0;
        HJVerifyRender *render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
        int i_err = render->test(window, width, height);
        if (i_err < 0)
        {
            return nullptr;
        }
        render->setRenderChangeFunc([](void *window, int width, int height)
        {
            HJFLogi("setRenderChangeFunc width:{} height:{}", width, height);
            if (m_playerThread)
            {
                m_playerThread->async([window, width, height]
                {
                    if (!m_playerActiveQueue.empty())
                    {
                        HJNAPIPlayer::Ptr player = *m_playerActiveQueue.begin();
                        if (player)
                        {
                            int ret = player->setNativeWindow(window, width, height, HJ::HJTargetState_Change);
                            return ret;
                        }
                    }
                });
            }
        });

        if (!m_playerThread)
        {
            m_playerThread = HJTimerThreadPool::Create();
            m_playerThread->startTimer(m_restartTime, [window, width, height]()
            {
                HJFLogi("playerexit startTimer enter");
                if (!m_bQuit)
                {
                    static int s_idxCnt = 0;
                    HJFLogi("playerexit restart idx:{}", s_idxCnt++);
                    priTryStop();
                    priTryStart(window, width, height);
                }
                return HJ_OK; 
            });
        }

        return nullptr;
    }
    napi_value HJVerifyManager::tryMute(napi_env env, napi_callback_info info)
    {
        HJFLogi("tryMute");
        if (m_playerThread)
        {
            m_playerThread->async([]
            {
                if (!m_playerActiveQueue.empty())
                {
                    HJNAPIPlayer::Ptr player = *m_playerActiveQueue.begin();
                    if (player)
                    {
                        static bool sMute = true;
                        int ret = player->setMute(sMute);
                        sMute = !sMute;
                        return ret;
                    }
                }
            });
        }
        return nullptr;
    }
    napi_value HJVerifyManager::tryClose(napi_env env, napi_callback_info info)
    {
        HJFLogi("playerexit tryClose enter");
        if (!m_playerThread)
        {
            return nullptr;
        }
    
        m_bQuit = true;
        if (m_playerThread)
        {
            m_playerThread->sync([]
            {
                HJFLogi("playerexit activequeue close size:{}", m_playerActiveQueue.size());
                for (auto active : m_playerActiveQueue)
                {
                    m_playerDisposeQueue.push_back(active);
                    active->closePlayer();
                }
                HJFLogi("playerexit dispose queue exit size:{}", m_playerDisposeQueue.size());
                m_playerActiveQueue.clear();
                return HJ_OK;
            });
        }
        
        m_exitThread = HJThreadPool::Create();
        m_exitThread->start();
        m_exitThread->sync([]()
        {
            HJFLogi("playerexit wait m_playerDisposeQueue size:{}", m_playerDisposeQueue.size());
            while (!m_playerDisposeQueue.empty())
            {
                HJ_SLEEP(10);
            }
            return HJ_OK;
        });
        HJFLogi("playerexit m_exitThread done enter");
        m_exitThread->done();
        m_exitThread = nullptr;
    
        HJFLogi("playerexit m_playerThread stop timer enter");
        m_playerThread->stopTimer();
        m_playerThread = nullptr;
        m_bQuit = false;
        HJFLogi("playerexit end==========");
        return nullptr;
    }
    int HJVerifyManager::priTryStart(void *window, int width, int height)
    {
        int ret = HJ_OK;
        do
        {
            HJNAPIPlayer::Ptr player = HJNAPIPlayer::Create();
            m_playerActiveQueue.push_back(player);
            HJPlayerInfo playerInfo;

            // s_url = "https://static.s3.huajiao.com/Object.access/hj-video/MTY1MDI3MjcyNDYyMzg5Lm1wNA==";
            playerInfo.m_url = s_url;//s_path + "264_1-264_2-265_1-265_1-265_2-265_1-264_1-264_1.flv"; // s_url;
            playerInfo.m_fps = s_fps;
            playerInfo.m_videoCodecType = /*HJPlayerVideoCodecType_SoftDefault;/*/ s_videoCodecType;
            playerInfo.m_sourceType = (HJPrioComSourceType)s_sourceType;

            if ((playerInfo.m_url.compare(0, 8, "https://") == 0) || (playerInfo.m_url.compare(0, 7, "http://") == 0))
            {
                playerInfo.m_playerType = HJPlayerType_LIVESTREAM;
            }
            else
            {
                playerInfo.m_playerType = HJPlayerType_VOD;
            }

            HJEntryStatInfo statInfo{};
            statInfo.bUseStat = true;
            statInfo.device = "harmonyOS";
            statInfo.interval = 10;
            statInfo.sn = "sntest";
            statInfo.uid = 1235;
            statInfo.statNotify = ([](const std::string& i_name, int i_nType, const std::string& i_info)
            {
                //HJFLogi("stat name:{} type:{} info:{}", i_name, i_nType, i_info);
            });
        
            HJNAPIPlayer::Wtr weakPlayer = player;
            ret = player->openPlayer(playerInfo, [weakPlayer](int i_type, const std::string &i_msgInfo)
                                     {
                                            if (i_type == HJ_RENDER_NOTIFY_NEED_SURFACE)
                                            {
                                                 HJFLogi("need surface notify");
                                            }
                                            else if (i_type == HJ_PLAYER_NOTIFY_CLOSEDONE)
                                            {
                                                HJFLogi("playerexit priTryExit");
                                                priTryExit(weakPlayer);
                                            }
                                     }, statInfo);
            if (ret < 0)
            {
                HJFLoge("openPlayer error");
                break;
            }

            ret = player->setNativeWindow(window, width, height, HJ::HJTargetState_Create);
            if (ret < 0)
            {
                HJLoge("setNativeWindow error:{}", ret);
                break;
            }

        } while (false);
        return ret;
    }

    int HJVerifyManager::priTryStop()
    {
        if (!m_playerActiveQueue.empty())
        {
            HJNAPIPlayer::Ptr player = *m_playerActiveQueue.begin();
            if (player)
            {
                int64_t t0 = HJCurrentSteadyMS();
                // HJFLogi("test setNativeWindow end");
                // m_Player->setNativeWindow(window, 0, 0, HJ::HJTargetState_Destroy);
                HJFLogi("test closePlayer enter");
                int64_t t1 = HJCurrentSteadyMS();

                // cache, after real destroy;
                m_playerDisposeQueue.push_back(player);

                player->closePlayer();
                m_playerActiveQueue.erase(m_playerActiveQueue.begin());
                int64_t t2 = HJCurrentSteadyMS();
                HJFLogi("test closePlayer end total:{} setWindowEmpty:{} close:{}", (t2 - t0), (t1 - t0), (t2 - t1));
            }
        }

        return HJ_OK;
    }
    void HJVerifyManager::priTryExit(HJNAPIPlayer::Wtr i_Player)
    {
        if (m_playerThread)
        {
            m_playerThread->async([i_Player]()
            {
                HJNAPIPlayer::Ptr player = HJNAPIPlayer::GetPtrFromWtr(i_Player);
                if (player)
                {
                    player->exitPlayer();
                    for (auto it = m_playerDisposeQueue.begin(); it != m_playerDisposeQueue.end(); it++)
                    {
                        if ((*it) == player)
                        {
                            m_playerDisposeQueue.erase(it);
                            HJFLogi("playerexit m_playerQueue find success, erase size:{}", m_playerDisposeQueue.size());
                            break;
                        }
                    }
                }
                return HJ_OK;
            });
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    void HJVerifyManager::priTest()
    {
        if (!m_testThreadPool)
        {
            m_testThreadPool = HJThreadPool::Create();
            m_testThreadPool->start();
        }

        if (m_testThreadPool)
        {
#if 0
        m_testThreadPool->async([](){
            HJPusherPNGSegInfo pngInfo;
            pngInfo.pngSeqUrl = std::string("/data/storage/el2/base/haps/entry/files/ShuangDanCaiShen");
            return m_testLiveStream->openPngSeq(pngInfo);
        }, 1000);
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
            return ret; });
        }
    }
    napi_value HJVerifyManager::testOpen(napi_env env, napi_callback_info info)
    {
        static bool s_btestInit = false;
        if (!s_btestInit)
        {
            HJEntryContextInfo contextInfo;
            contextInfo.logIsValid = true;
            contextInfo.logDir = "/data/storage/el2/base/haps/entry/files/log_player";
            contextInfo.logLevel = HJLOGLevel_INFO;
            contextInfo.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
            contextInfo.logMaxFileSize = 1024 * 1024 * 5;
            contextInfo.logMaxFileNum = 5;
            HJNAPITestPlayer::contextInit(contextInfo);
            s_btestInit = true;
        }

        priTestClose();

        static int s_openIdx = 0;
        //    if (s_openIdx == 0)
        //    {
        //        s_sourceType = HJPrioComSourceType_Bridge;
        //        s_url = s_path + "huajiaoline2_noaudio.flv";
        //    }
        //    else if (s_openIdx == 1)
        //    {
        //        s_sourceType = HJPrioComSourceType_SPLITSCREEN;
        //        s_url = s_path + "PK_ZUOJIA.mp4";
        //    }
        //    else if (s_openIdx == 2)
        {
            s_sourceType = HJPrioComSourceType_SERIES;
            s_url = s_url;//s_path + "264_1-264_2-265_1-265_1-265_2-265_1-264_1-264_1.flv"; //"H264-265-264_RES.flv";//"HEVC.flv"; //"264_1-264_2-265_1-265_1-265_2-265_1-264_1-264_1.flv";//"H264-265-264_RES.flv";//"H264_H264_ResChange.flv";//"huajiaoline2_noaudio.flv";
        }
        s_openIdx++;
        if (s_openIdx == 3)
        {
            s_openIdx = 0;
        }

        NativeWindow *window = nullptr;
        int width = 0;
        int height = 0;
        HJVerifyRender *render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
        int i_err = render->test(window, width, height);
        if (i_err < 0)
        {
            return nullptr;
        }

        m_testPlayer = HJNAPITestPlayer::Create();
        HJPlayerInfo playerInfo;
        playerInfo.m_url = s_path + "huajiaoline2_noaudio.flv";//s_url; //"https://static.s3.huajiao.com/Object.access/hj-video/MTY1MDI3MjcyNDYyMzg5Lm1wNA==";//s_url;
        playerInfo.m_fps = s_fps;
        playerInfo.m_videoCodecType = s_videoCodecType;
        playerInfo.m_sourceType = (HJPrioComSourceType)s_sourceType;

        if ((playerInfo.m_url.compare(0, 8, "https://") == 0) || (playerInfo.m_url.compare(0, 7, "http://") == 0))
        {
            playerInfo.m_playerType = HJPlayerType_LIVESTREAM;
        }
        else
        {
            playerInfo.m_playerType = HJPlayerType_VOD;
        }
        i_err = m_testPlayer->openPlayer(playerInfo, nullptr);
        if (i_err < 0)
        {
            HJFLoge("openPlayer error");
            return nullptr;
        }

        i_err = m_testPlayer->setNativeWindow(window, width, height, HJ::HJTargetState_Create);
        if (i_err < 0)
        {
            HJLoge("setNativeWindow error", i_err);
            return nullptr;
        }
        //    priTest();
        return nullptr;
    }
    void HJVerifyManager::priTestClose()
    {
        if (m_testPlayer)
        {
            HJFLogi("test close enter");

            NativeWindow *window = nullptr;
            int width = 0;
            int height = 0;
            HJVerifyRender *render = HJVerifyManager::GetInstance()->GetRender(S_XCOM_ID);
            int i_err = render->test(window, width, height);
            if (i_err < 0)
            {
                return;
            }

            int64_t t0 = HJCurrentSteadyMS();
            HJFLogi("test setNativeWindow end");
            m_testPlayer->setNativeWindow(window, 0, 0, HJ::HJTargetState_Destroy);
            int64_t t1 = HJCurrentSteadyMS();
            HJFLogi("test closePlayer enter");
            m_testPlayer->closePlayer();
            int64_t t2 = HJCurrentSteadyMS();
            HJFLogi("test closePlayer end total:{} setWindowEmpty:{} close:{}", (t2 - t0), (t1 - t0), (t2 - t1));
            m_testPlayer = nullptr;
        }

        //    if (m_testThreadTimer)
        //    {
        //        HJFLogi("test close m_testThreadTimer enter");
        //        m_testThreadTimer->stopTimer();
        //        m_testThreadTimer = nullptr;
        //    }
        //    if (m_testThreadPool)
        //    {
        //        HJFLogi("test close m_testThreadPool sync enter");
        //        m_testThreadPool->sync([]()
        //        {
        //           if (m_testPlayer)
        //           {
        //                HJFLogi("test close closePush enter");
        //
        //                m_testPlayer->closePlayer();
        //                HJFLogi("test close closePreview enter");
        //           }
        //           return 0;
        //        });
        //        HJFLogi("test close m_testThreadPool done enter");
        //        m_testThreadPool->done();
        //        HJFLogi("test close m_testThreadPool done end");
        //        m_testThreadPool = nullptr;
        //    }
    }
    napi_value HJVerifyManager::testClose(napi_env env, napi_callback_info info)
    {
        priTestClose();
        return nullptr;
    }
    napi_value HJVerifyManager::testEffectGray(napi_env env, napi_callback_info info)
    {
        if (m_testPlayer)
        {
            static bool s_bGray = true;
            if (s_bGray)
            {
                m_testPlayer->openEffect(HJPrioEffect_Gray);
            }
            else
            {
                m_testPlayer->closeEffect(HJPrioEffect_Gray);
            }
            s_bGray = !s_bGray;
        }
        return nullptr;
    }
    napi_value HJVerifyManager::testEffectBlur(napi_env env, napi_callback_info info)
    {
        if (m_testPlayer)
        {
            static bool s_bBlur = true;
            if (s_bBlur)
            {
                m_testPlayer->openEffect(HJPrioEffect_Blur);
            }
            else
            {
                m_testPlayer->closeEffect(HJPrioEffect_Blur);
            }
            s_bBlur = !s_bBlur;
        }
        return nullptr;
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
            OH_LOG_Print(
                LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "HJVerifyManager", "Export: OH_NativeXComponent_GetXComponentId fail");
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
}