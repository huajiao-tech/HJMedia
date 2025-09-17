//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNodeVRender.h"
#include "HJNotify.h"
#include "HJPlayerBase.h"
#include "HJContext.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJNodeVRender::HJNodeVRender(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, scheduler, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_VIDEO;
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeVRender)));
    setDriveType(HJNODE_DRIVE_PRE);
}

HJNodeVRender::HJNodeVRender(const HJEnvironment::Ptr& env/* = nullptr*/, const HJExecutor::Ptr& executor/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, executor, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_VIDEO;
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeVRender)));
    setDriveType(HJNODE_DRIVE_PRE);
}

HJNodeVRender::~HJNodeVRender()
{
    done();
}

int HJNodeVRender::init(const HJVideoInfo::Ptr& info)
{
    HJLogi("entry");
    int res = HJMediaNode::init(HJ_VREND_STOREAGE_CAPACITY);
    if (HJ_OK != res) {
        HJLoge("error, res:" + HJ2String(res));
        return res;
    }
    m_info = info;
    
    m_render = HJVRenderBase::createVideoRender();
    if (!m_render) {
        HJLoge("error, create video render failed");
        return HJErrNewObj;
    }
    res = m_render->init(m_info);
    if (HJ_OK != res) {
        HJLoge("error, video render init failed");
        return res;
    }
    m_clockDeltas = std::make_shared<HJDataStati64>(30);

    m_runState = HJRun_Init;
    m_renderState = HJRender_Init;
    HJFLogi("end, res:{}", res);

    return res;
}

void HJNodeVRender::done()
{
    if (!m_scheduler) {
        return;
    }
    m_scheduler->sync([&]{
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

int HJNodeVRender::play()
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
                break;
            }
            res = HJMediaNode::play();
        } while (false);

        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("play error")));
        }
    };
    m_scheduler->async(running);

    return HJ_OK;
}

int HJNodeVRender::pause()
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
            m_runState = HJRun_Pause;
        } while (false);
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("pause error")));
        }
    };
    m_scheduler->async(running);
    
    return HJ_OK;
}

//int HJNodeVRender::resume()
//{
//    auto running = [&]()
//    {
//        int res = HJ_OK;
//        HJLogi("entry");
//        do {
//            m_runState = HJRun_Ready;
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

void HJNodeVRender::setPreFlush(const bool flag)
{
    m_preFlushFlag = flag;
    return;
}

int HJNodeVRender::flush(const HJSeekInfo::Ptr info/* = nullptr*/)
{
//    HJLogi("202204301245 vrender flush begin");
    auto running = [=]()
    {
        if (!m_render) {
            return;
        }
        HJLogi("vrender sync flush start");
        int res = HJ_OK;
        do {
            m_preFlushFlag = false;
            if(info) {
                m_seekInfos.emplace_back(info);
                //HJLogi("render add seek info size:" + HJ2String(m_seekInfos.size()));
            }
            res = m_render->flush();
            if (HJ_OK != res) {
                HJLoge("flush error:" + HJ2String(res));
                break;
            }
            if(getEOF()) {
                setEOF(false);
                m_runState = HJRun_Running;
            }
            m_rawFrame = nullptr;
            m_renderCount = 0;
        
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
            if (HJRun_Running != m_runState) {
                m_runState = HJRun_Start;
            }
        } while (false);
        //
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("vrender flush error")));
        }
        HJLogi("vrender sync flush end");
    };
    m_scheduler->sync(running);
//    HJLogi("202204301245 vrender flush end");
    return HJ_OK;
}

/**
 * 1��error\pause\done\EOF\first frame -- break
 * 2��no window, no frame, get frame, full -- async self(delaytime or 0)
 */
int HJNodeVRender::proRun()
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
        if (!m_render->isReady())
        {
            res = HJ_WOULD_BLOCK;
            if (m_renderState < HJRender_Prepare) {
                m_renderState = HJRender_Prepare;
                notify(HJMakeNotification(HJNotify_NeedWindow));
            }
        }
        else
        {
            if (!m_rawFrame) {
                m_rawFrame = pop();
            }
            if (m_rawFrame)
            {
                //HJLogi("stream id:" + HJ2STR(m_info->getStreamIndex()) + ", video frame info:" + m_rawFrame->formatInfo());
                if (HJFRAME_EOF == m_rawFrame->getFrameType()) {
                    setEOF(true);
                    clearInOutStorage();
                    m_rawFrame = nullptr;
                    m_runState = HJRun_Stop;
                    HJLogi("video render end, eof");
                    return HJ_EOF;
                }
                else
                {
                    res = checkFrame(m_rawFrame);
                    if (res < HJ_OK) {
                        HJLoge("check frame error:" + HJ2String(res));
                        break;
                    }
                    else if (HJ_OK == res)
                    {
                        notifyMediaFrame(m_rawFrame);
                        //
                        res = m_render->render(std::move(m_rawFrame));
                        if (res < HJ_OK) {
                            HJLoge("render error:" + HJ2String(res));
                            break;
                        }
                        m_renderCount++;

                        if ((HJRun_Start == m_runState) || (HJRun_Pause == m_runState))
                        {
                            /**
                                * 1: start; 2: start | seek; 3: pause | seek;
                                */
                            if (HJRender_Prepare == m_renderState) {
                                m_renderState = HJRender_Ready;
                                notify(HJMakeNotification(HJNotify_Already, res, std::string("play aready")));
                            }
                            if (!m_seekInfos.empty()) {
                                notifySeekInfo(m_seekInfos.front());
                                m_seekInfos.pop_front();
                            }
                            res = HJ_IDLE;
                            break;
                        }
                        else
                        {
                            /**
                                * 1: play; 2: play | seek
                                */
                            if (HJRender_Ready == m_renderState) {
                                m_renderState = HJRender_Running;
                                notify(HJMakeNotification(HJNotify_PlaySuccess, res, std::string("play success")));
                            }
                            m_runState = HJRun_Running;
                        }
                    }
                    else {
                        //HJLogw("warning, video frame wounld wait systime or audio time");
                    }
                }
            } //if (m_rawFrame)
        } //if (!m_render->isReady())
    } while (false);
//    HJLogi("end");

    return res;
}

void HJNodeVRender::run()
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

int HJNodeVRender::setWindow(const std::any& window)
{
    auto running = [&]() {
        int res = m_render->setWindow(window);
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("set window error")));
        }
    };
    m_scheduler->async(running);

    return HJ_OK;
}

int HJNodeVRender::checkFrame(const HJMediaFrame::Ptr& mvf)
{
    HJVideoInfo::Ptr videoInfo = mvf->getStreamInfo<HJVideoInfo::Ptr>();
    if (videoInfo)
    {
        if (videoInfo->m_width != m_info->m_width ||
            videoInfo->m_height != m_info->m_height) {
            notify(HJMakeNotification(HJNotify_WindowChanged, HJ_OK, std::string("window info changed")));
        }
    }
    /**
    * 1��audio and video sync;
    * 2��video with systime sync;
    * 3��first frame update reftime
    */
    int64_t pts = mvf->getPTS();
    if (!m_renderCount) {   //first frame
        m_env->getTimeline()->update(pts);
    }
    int64_t refTime = m_env->getTimeline()->getTimestamp();
    int64_t waitTime = pts - refTime;
    //if ((pts + m_clockDeltaValue) > refTime) {
    if(pts > refTime) {
        m_waitTime = HJ_MIN(waitTime, mvf->getDuration());
        //m_delayTime = HJ_CLIP(m_delayTime + m_clockDeltaValue, 0, m_delayTime);
        //HJFLogi("202407221140 would block, pts:{}, reftime:{}, real wait time:{}, count wait time:{}", pts, refTime, m_waitTime, waitTime);
        return HJ_WOULD_BLOCK;
    } else {
        m_waitTime = 0;
        m_clockDeltaValue = m_clockDeltas->add(waitTime);
    }
    //HJFLogi("202407221140 will render frame pts:{}, reftime:{}, wait time:{}, avg wait time:{}", pts, refTime, waitTime, m_clockDeltaValue);

    return HJ_OK;
}

int HJNodeVRender::notifySeekInfo(const HJSeekInfo::Ptr info)
{
    if (m_env) {
        m_env->emitEvent(HJBroadcast::EVENT_PLAYER_SEEKINFO, info);
    }    
    return HJ_OK;
}

NS_HJ_END

