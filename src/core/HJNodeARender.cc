//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNodeARender.h"
#include "HJNotify.h"
#include "HJPlayerBase.h"
#include "HJContext.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJNodeARender::HJNodeARender(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, scheduler, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_AUDIO;
    setName(HJContext::Instance().getGlobalName(HJ_TYPE_NAME(HJNodeARender)));
    setDriveType(HJNODE_DRIVE_PRE);
}

HJNodeARender::HJNodeARender(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto/* = true*/)
    : HJMediaNode(env, executor, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_AUDIO;
    setName(HJContext::Instance().getGlobalName(HJ_TYPE_NAME(HJNodeARender)));
    setDriveType(HJNODE_DRIVE_PRE);
}

HJNodeARender::~HJNodeARender()
{
    done();
}

int HJNodeARender::init(const HJAudioInfo::Ptr& info)
{
    int res = HJMediaNode::init(HJ_AREND_STOREAGE_CAPACITY);
    if (HJ_OK != res) {
        HJLoge("error, res:" + HJ2String(res));
        return res;
    }
    m_info = info;
    
    HJLogi("entry");
    m_render = HJARenderBase::createAudioRender();
    if (!m_render) {
        HJLoge("error, create audio render failed");
        return HJErrNewObj;
    }
    res = m_render->init(m_info, [&](const HJMediaFrame::Ptr rawFrame) {
        if (!rawFrame) {
            return;
        }
        int64_t pts = rawFrame->getPTS();
        if (HJ_NOTS_VALUE == m_startElapse) {
            m_startElapse = (int64_t)HJCurrentSteadyMS() - pts;
        }
        //HJFLogi("[checkFrame] out audio frame pts:{}, elapse delta:{}", pts, (int64_t)(HJUtils::getCurrentMillisecond() - pts - m_startElapse));
        if (m_env) {
            m_env->getTimeline()->update(pts);
        }
        //
        notifyMediaFrame(rawFrame);
    });
    if (HJ_OK != res) {
        HJLoge("error, audio render init failed");
        return res;
    }
    m_runState = HJRun_Init;
    m_renderState = HJRender_Init;
    HJLogi("end");
    
    return HJ_OK;
}

void HJNodeARender::done()
{
    if (!m_scheduler) {
        return;
    }
    m_scheduler->sync([&] {
        HJLogi("done entry");
        m_runState = HJRun_Done;
        m_rawFrame = nullptr;
        m_render = nullptr;

        HJMediaNode::done();
        HJLogi("done end");
    });
    m_scheduler = nullptr;

    return;
}

int HJNodeARender::play()
{
    auto running = [&]()
    {
        int res = HJ_OK;
        HJLogi("play entry");
        do {
            if (!m_render) {
                res = HJErrNotAlready;
                break;
            }
            m_runState = HJRun_Ready;
            res = asyncSelf();
            if (HJ_OK != res) {
                HJLoge("error:" + HJ2String(res));
                break;
            }
            res = m_render->start();
            if (HJ_OK != res) {
                HJLoge("audio render start error:" + HJ2String(res));
                break;
            }
            res = HJMediaNode::play();
        } while (false);

        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("pause error")));
        }
    };
    m_scheduler->async(running);

    return HJ_OK;
}

int HJNodeARender::pause()
{
    auto running = [&]()
    {
        int res = HJ_OK;
        HJLogi("pause entry");
        do {
            if (!m_render) {
                res = HJErrNotAlready;
                break;
            }
            res = HJMediaNode::pause();
            if (HJ_OK != res) {
                break;
            }
            res = m_render->pause();
            if (HJ_OK != res) {
                HJFLoge("error, audio kernel pause failed, res:{}", res);
                break;
            }
            m_runState = HJRun_Pause;
        } while (false);
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("pause error")));
        }
    };
    m_scheduler->async(running);
    
    return HJ_OK;
}

//int HJNodeARender::resume()
//{
//    auto running = [&]()
//    {
//        int res = HJ_OK;
//        HJLogi("entry");
//        do {
//            m_runState = HJRun_Ready;
//            if (m_render) {
//                res = m_render->resume();
//                if (HJ_OK != res) {
//                    break;
//                }
//            }
//            res = asyncSelf();
//            if (HJ_OK != res) {
//                break;
//            }
//            res = HJMediaNode::resume();
//        } while (false);
//        if (HJ_OK != res) {
//            notify(HJMakeNotification(HJNotify_Error, res, std::string("pause error")));
//        }
//    };
//    m_scheduler->async(running);
//    
//    return HJ_OK;
//}

void HJNodeARender::setPreFlush(const bool flag)
{
    HJLogi("entry");
    m_preFlushFlag = flag;
    auto running = [=]()
    {
        if (!m_render) {
            return;
        }
        m_render->pause();
    };
    m_scheduler->sync(running);
    HJLogi("end");
   
    return;
}

int HJNodeARender::flush(const HJSeekInfo::Ptr info/* = nullptr*/)
{
//    HJLogi("202204301245 vrender flush begin");
    auto running = [=]()
    {
        if (!m_render) {
            return;
        }
        HJLogi("arender sync flush start");
        int res = HJ_OK;
        do {
            m_preFlushFlag = false;
            //if(info) {
            //    m_seekInfos.emplace_back(std::move(info));
            //    HJLogi("render add seek info size:" + HJ2String(m_seekInfos.size()));
            //}
            res = m_render->reset();
            if (HJ_OK != res) {
                HJLoge("flush error:" + HJ2String(res));
                break;
            }
            if(getEOF()) {
                setEOF(false);
                m_runState = HJRun_Ready;
            }
            m_rawFrame = nullptr;
            m_renderCount = 0;
            m_startElapse = HJ_NOTS_VALUE;
        
            res = tryAsyncSelf();
            if (HJ_OK != res) {
                HJLoge("error, try async self" + HJ2String(res));
                break;
            }
            res = HJMediaNode::flush(info);
            if (HJ_OK != res) {
                HJFLogi("error, flush next failed res:{}", res);
                break;
            }
            if (HJRun_Ready != m_runState) {
                m_runState = HJRun_Start;
            }
        } while (false);
        //
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("arender flush error")));
        }
        HJLogi("arender sync flush end");
    };
    m_scheduler->sync(running);
//    HJLogi("202204301245 vrender flush end");
    return HJ_OK;
}

/**
* 1��error, pause, EOF��done -- break
* 2��no frame,
*/
int HJNodeARender::proRun()
{
    HJOnceToken token(nullptr, [&] {
        setIsBusy(false);
        //HJFLogi("proRun] end, name:{}, proc run done, is busy:{}", getName(), isBusy());
    });
    if (!m_render || !m_scheduler) {
        HJLoge("error, not ready");
        return HJErrNotAlready;
    }
    //HJFLogi("entry, name:{}, m_runState:{}", getName(), m_runState);
    if (HJRun_Pause == m_runState || m_preFlushFlag || isEndOfRun()) {
        HJFLogw("warning, run state:{}, pre flush:{}", HJRunStateToString(m_runState), m_preFlushFlag);
        return HJ_EOF;
    }
    int res = HJ_OK;
    do
    {
        m_waitTime = HJMediaNode::MTASK_DELAY_TIME_DEFAULT;
        if (!m_rawFrame) {
            m_rawFrame = pop();
        }
        if (m_rawFrame)
        {
            //HJLogi("stream id:" + HJ2STR(m_info->getStreamIndex()) + ", audio frame info:" + m_rawFrame->formatInfo());
            if (HJFRAME_EOF == m_rawFrame->getFrameType()) {
                setEOF(true);
                clearInOutStorage();
                m_rawFrame = nullptr;
                m_render->pause();
                m_runState = HJRun_Stop;
                HJLogi("video render end, eof");
                return HJ_EOF;
            }
            else
            {
                if (m_render->getSpeed() != m_env->getSpeed()) {
                    m_render->setSpeed(m_env->getSpeed());
                }
                if (m_render->getVolume() != m_env->getVolume()) {
                    m_render->setVolume(m_env->getVolume());
                }
                res = m_render->write(m_rawFrame);
                if (res < HJ_OK) {
                    HJLoge("render error:" + HJ2String(res));
                    break;
                }
                else if (HJ_OK == res) {
                    //if (m_env) {
                    //    m_env->getTimeline()->update(m_rawFrame->getPTS());
                    //}
                    m_waitTime = 0;
                    m_rawFrame = nullptr;
                    m_renderCount++;
                    //m_renderState = HJRender_Running;
                }
                else {  //HJ_WOULD_BLOCK
                    //HJFLogw("warning, audio kernel render is full, res:{}", res);
                    if ((HJRun_Start == m_runState) || (HJRun_Pause == m_runState)) 
                    {
                        if (m_renderState < HJRender_Prepare) {
                            m_renderState = HJRender_Ready;
                            if (HJMEDIA_TYPE_AUDIO == m_env->getEnableMediaTypes()) {
                                notify(HJMakeNotification(HJNotify_Already, res, std::string("play aready")));
                            }
                        }
                        return HJ_IDLE;
                    } else 
                    {
                        if (HJRender_Ready == m_renderState) {
                            m_renderState = HJRender_Running;
                            if (HJMEDIA_TYPE_AUDIO == m_env->getEnableMediaTypes()) {
                                notify(HJMakeNotification(HJNotify_PlaySuccess, res, std::string("play success")));
                            }
                        }
                    }
                }
            }
        }
        else {
            res = HJ_WOULD_BLOCK;
        } //if (m_rawFrame)
    } while (false);    
    //HJLogi("end");

    return res;
}

void HJNodeARender::run()
{
    int res = proRun();
    if (res < HJ_OK) {
        notify(HJMakeNotification(HJNotify_Error, res, getName() + "error"));
        return;
    }
    if (HJ_EOF == res || HJ_IDLE == res) {
        return;
    }
    tryAsyncSelf(m_waitTime);
    return;
}

int HJNodeARender::notifySeekInfo(const HJSeekInfo::Ptr info)
{
    if (m_env) {
        m_env->emitEvent(HJBroadcast::EVENT_PLAYER_SEEKINFO, info);
    }   
    return HJ_OK;
}

NS_HJ_END


