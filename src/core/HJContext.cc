//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFLog.h"
// #include "HJFileUtil.h"
#include "HJContext.h"
#include "HJFFUtils.h"
#include "HJException.h"
#include <csignal>
#if defined(HJ_OS_WIN)
#include <mmsystem.h>
#endif

NS_HJ_BEGIN
//***********************************************************************************//
HJ_INSTANCE_IMP(HJContext);
//HJContext::Ptr HJSingleton<HJContext>::mSingleton = nullptr;

HJContext::HJContext()
{
    // std::atomic_init(&m_globalIDCounter, 0);
}

HJContext::~HJContext()
{
    //done();
}

int HJContext::init(const HJContextConfig& config)
{
    m_cfg = config;
    static HJOnceToken token( [&]{
        HJLog::Instance().init(config.mIsLog, m_cfg.mLogDir, m_cfg.mLogLevel, m_cfg.mLogMode, false);
    	//HJExceptionHandler::Instance().init(m_cfg.mDumpsDir);
        //
        HJExecutorManager::setPoolSize(m_cfg.mThreadNum);
        
        //std::signal(SIGSEGV, HJContext::onHandleSignal);
#if defined(HJ_OS_WIN)
        timeBeginPeriod((UINT)m_timerPeriod);
#endif

        av_log_set_level(AV_LOG_TRACE);
        av_log_set_callback(HJContext::onHandleFFLogCallback);
        
        avformat_network_init();
    });
    HJLogi("init end");
    
    return HJ_OK;
}

int HJContext::init()
{
    HJContextConfig cfg;
    return init(cfg);
}

int HJContext::done()
{
    HJLogi("done entry");
    
#if defined(HJ_OS_WIN)
    timeEndPeriod((UINT)m_timerPeriod);
#endif

    avformat_network_deinit();
#if 0
    HJDictionary dald = {{"daflaf", std::string("dadfa")}, {"dafa", 1.f}, {"dfa", 2.4545}};
    HJDictionaryPtr dad = std::make_shared<HJDictionary>(dald);
    float d = std::any_cast<float>((*dad)["dafa"]);
    double ds = std::any_cast<double>((*dad)["dfa"]);
#endif
    
#if 0
    HJDictionary dald = {{"daflaf", std::string("dadfa")}, {"dafa", 1.f}, {"dfa", 2.4545}};
    
//    HJDictionary dald = HJDictionary::makeDictionary("dafdf", 1.0);
//    dald["daflaf"] = std::string("dadfa");
//    dald["dafa"] = 1.01f;
//    dald["dfa"] = 4.089;
    
    std::string da = std::any_cast<std::string>(dald["daflaf"]);
    float d = std::any_cast<float>(dald["dafa"]);
    double ds = std::any_cast<double>(dald["dfa"]);
    
    std::string uuid = HJUtils::generateUUID();
#endif
    
#if 0
    std::string ss = "dsdfsf";
    HJObject* tsk;
    
    HJAnyVector vecs = HJMakeAnyVector({ss, 5, 6.0233, 1.0f, tsk});
    vecs.emplace_back(tsk);
    std::string v0 = std::any_cast<std::string>(vecs[0]);
    int v1 = std::any_cast<int>(vecs[1]);
    double v2 = std::any_cast<double>(vecs[2]);
    float v3 = std::any_cast<float>(vecs[3]);
    HJObject* v4 = std::any_cast<HJObject *>(vecs[4]);
#endif//
    
    HJLogi("done entry");
#if 0
    HJLogi("tasks before");
    for (size_t i = 0; i < 100; i++) {
        HJScheduler::Ptr scheduler = std::make_shared<HJScheduler>(nullptr, true);
        m_schedulers.push_back(scheduler);
    
        usleep(100*1000);
    }
    
    HJLogi("tasks before dad");
    for (size_t i = 0; i < 10; i++) {
        for (auto it = m_schedulers.begin(); it != m_schedulers.end(); it++) {
            size_t tid = (*it)->getTid();
            size_t schID = (*it)->getID();
            (*it)->asyncCoro([tid, schID, i]{
                usleep(100*1000);
                HJLogi("task run end, tid:" + HJ2String(tid)  + ", scheduler id:" + HJ2String(schID) + ", task id:" + HJ2String(i));
            });
            usleep(100*1000);
        }
    }

    HJLogi("tasks end");
#endif //
    
#if 0
    {
        HJScheduler::Ptr scheduler = std::make_shared<HJScheduler>();
    //    m_schedulers.push_back(scheduler);
        
        HJLogi("tasks before");
        for (size_t i = 0; i < 100; i++) {
            scheduler->asyncCoro([i]{
                usleep(100*1000);
                HJLogi("task run end, id:" + HJ2String(i));
            });
    //        usleep(200*1000);
        }
        usleep(3000*1000);
        
        HJLogi("begin del before");
        scheduler = nullptr;
        
        HJLogi("tasks end");
    }
#endif

#if 0
    {
        HJScheduler::Ptr scheduler = std::make_shared<HJScheduler>();
        m_schedulers.push_back(scheduler);
        
        HJLogi("tasks before");
        for (size_t i = 0; i < 100; i++) {
            scheduler->async([i]{
                usleep(110*1000);
                HJLogi("task run end, id:" + HJ2String(i));
            });
            usleep(100*1000);
        }
        HJLogi("tasks end");
    }
#endif
    
#if 0
    {
        HJScheduler::Ptr scheduler = std::make_shared<HJScheduler>();
        m_schedulers.push_back(scheduler);
        
        HJLogi("tasks before");
        scheduler->asyncDelayTask([&]{
            HJLogi("task run end 0");
        }, 3000);
        scheduler->asyncDelayTask([&]{
            HJLogi("task run end 1");
        }, 10000);
        scheduler->asyncDelayTask([&]{
            HJLogi("task run end 2");
        }, 1000);
        scheduler->async([&]{
            HJLogi("task run end 3");
        });
        
        HJLogi("tasks end");
    }
#endif
   
#if 0
    {
        HJScheduler::Ptr scheduler = std::make_shared<HJScheduler>();
        m_schedulers.push_back(scheduler);
        
        HJLogi("tasks before");
        scheduler->syncCoro([&]{
            HJLogi("task run end 0");
        });
        scheduler->syncCoro([&]{
            HJLogi("task run end 1");
        });
        scheduler->asyncDelayTask([&]{
            HJLogi("task run end 2");
        }, 1000);
        scheduler->async([&]{
            HJLogi("task run end 3");
        });
        
        HJLogi("tasks end");
    }
#endif
    
#if 0
    {
        HJLogi("coro task before");
        for (int i = 0; i < 6; i++) {
            HJScheduler::Ptr scheduler = std::make_shared<HJScheduler>(HJExecutorManager::Instance().getExecutor(i));
            m_schedulers.push_back(scheduler);
        }
        
        HJTask::Ptr tsk0 = m_schedulers[0]->asyncCoro([&]{
            HJLogi("coro task 0 - 0 begin, id:" + HJ2String(tsk0->getID()));
            m_schedulers[1]->syncCoro([&]{
                HJLogi("coro task 1 - 1 begin, id");
                
                m_schedulers[0]->syncCoro([&]{
                    HJLogi("coro task 0 - 2 begin");
                    
                    m_schedulers[2]->syncCoro([&]{
                        HJLogi("coro task 2 - 3 begin");

                        m_schedulers[3]->syncCoro([&]{
                            HJLogi("coro task 3 - 4 begin");

                            m_schedulers[4]->syncCoro([&]{
                                HJLogi("coro task 4 - 5 begin");

                                m_schedulers[5]->syncCoro([&]{
                                    HJLogi("coro task 5 - 6 begin");

                                    m_schedulers[0]->syncCoro([&]{
                                        HJLogi("coro task 0 - 7 begin");

                                        
                                        HJLogi("coro task 0 - 7 end");
                                    });
                                    
                                    HJLogi("coro task 5 - 6 end");
                                });
                                
                                
                                HJLogi("coro task 4 - 5 end");
                            });
                            
                            HJLogi("coro task 3 - 4 end");
                        });
        
                        HJLogi("coro task 2 - 3 end");
                    });
//                    m_tasks.push_back(tsk3);
                    HJLogi("coro task 0 - 2 end");
                });
//                m_tasks.push_back(tsk2);
                HJLogi("coro task 1 - 1 end");
            });
//            m_tasks.push_back(tsk1);
            HJLogi("coro task 0 - 0 end");
        });
//        m_tasks.push_back(tsk0);
        HJLogi("coro task task end");
    }
#endif

    HJExecutorManager::Instance().done();
    HJLogi("done end");
    HJLog::Instance().done();
    
    return HJ_OK;
}

#if 0
static FILE* s_fp = NULL;
void HJContext::ffmpeg_log_callback(void* avcl, int level, const char* fmt, va_list vl)
{
    if (level > av_log_get_level())
        return;

    {
        if (!s_fp) {
            fopen_s(&s_fp, "D:/log/ffmpeg_recieve.log", "wb+");
            if (!s_fp) {
                return;
            }
        }

        char buffer[32768];
        vsnprintf(buffer, sizeof(buffer), fmt, vl); // support %td
        // sprintf(buffer, "ffmpeg:%s\n ", buffer);
        fwrite(buffer, strlen(buffer), 1, s_fp);
        fflush(s_fp);
    }
}
#endif

#if 1
void HJContext::onHandleFFLogCallback(void* avcl, int level, const char *fmt, va_list vl)
{
    switch (level) {
    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
    case AV_LOG_ERROR:
    case AV_LOG_WARNING:
    case AV_LOG_INFO:
    {
        char* strp = NULL;
        int res = vasprintf(&strp, fmt, vl);
        if (res > 0 && strp) {
            HJLogi("time:" + HJ2STR(HJCurrentMicrosecond()) + ", ffmpeg: " + HJ2SSTR(strp));
            free(strp);
        }
//        char buffer[8*1024];
//#if (defined WIN32_LIB) && _MSC_VER >= 1310
//        vsnprintf(buffer, sizeof(buffer), fmt, vl);
//#else
//        vsprintf(buffer, fmt, vl);
//#endif
        //HJLogi("time:" + HJ2STR(HJUtils::getCurrentMicrosecond()) + ", ffmpeg: " + HJ2SSTR(buffer));
    }
    break;

    case AV_LOG_QUIET:

        break;
    //case AV_LOG_INFO:

    //    break;
    case AV_LOG_VERBOSE:

        break;

    case AV_LOG_DEBUG:

        break;
    default:
        break;
    }
}
#endif

void HJContext::onHandleSignal(int signal)
{
    HJFLoge("error, catch signal:{}", signal);
    return;
}

NS_HJ_END
