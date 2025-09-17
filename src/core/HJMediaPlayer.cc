//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaPlayer.h"
#include "HJTicker.h"
#include "HJFLog.h"
#include "HJContext.h"

NS_HJ_BEGIN
const int64_t HJSeekLimiter::SEEK_INTERVAL_MAX = 200;  //ms
const std::string HJMediaPlayer::PLAYER_KEY_SEEK = "26a1e2bc-a1ac-4901-a9a6-61c665a785d3";
//***********************************************************************************//
HJMediaPlayer::HJMediaPlayer(const HJScheduler::Ptr& scheduler/* = nullptr*/)
    : HJPlayerBase(scheduler)
{
    m_seekLimiter = std::make_shared<HJSeekLimiter>();
}

HJMediaPlayer::~HJMediaPlayer()
{
    done();
}

int HJMediaPlayer::init(const HJMediaUrl::Ptr& mediaUrl, const HJListener& listener/* = nullptr*/)
{
    if (!mediaUrl) {
        return HJErrInvalidParams;
    }
    HJMediaUrlVector mediaUrls = { mediaUrl };
    return init(mediaUrls, listener);
}

int HJMediaPlayer::init(const HJMediaUrlVector& mediaUrls, const HJListener& listener/* = nullptr*/)
{
    if (HJ_OK != HJPlayerBase::init(mediaUrls)) {
        HJLoge("graph init error");
        return HJErrFatal;
    }
    m_listener = listener;
    m_env->setLowDelay(false);
    m_env->addNotify([=](const HJNotification::Ptr ntf){
        //HJFLogi("EVENT_PLAYER_NOTIFY msgID:{}, msgVal:{}, msgVal:{}", HJNotifyIDToString((HJNotifyID)ntf->getID()), HJ2STR(ntf->getVal()), ntf->getMsg());
        switch (ntf->getID())
        {
            case HJNotify_Prepared: 
            {
                HJ_AUTO_LOCK(m_mutex);
                auto minfo = ntf->getValue<HJMediaInfo::Ptr>(HJMediaInfo::KEY_WORLDS);
                if (minfo) {
                    m_minfo = minfo;
                }
                break;
            }
            case HJNotify_ProgressStatus:
            {
                HJ_AUTO_LOCK(m_mutex);
                auto progInfo = ntf->getValue<HJProgressInfo::Ptr>(HJProgressInfo::KEY_WORLDS);
                if (!progInfo) {
                    return;
                }
                if (m_progInfo && HJ_ABS(progInfo->getPos() - m_progInfo->getPos()) < HJ_INTERVAL_DEFAULT) {
                    //HJFLogw("warning, discard progress info pos:{}, pre pos:{}", progInfo->getPos(), m_progInfo->getPos());
                    return;
                }
                m_progInfo = progInfo;
                //
                //HJFLogi("progress info pos:{}", progInfo->getPos());
                break;
            }
        }
        //
        m_scheduler->asyncAlwaysTask([=](){
            switch (ntf->getID())
            {
                case HJNotify_Already: {
                    m_runState = HJRun_Ready;
                    break;
                }
                default: {
                    break;
                }
            }
            notify(ntf);
        });
    });

    m_env->addListener(HJBroadcast::EVENT_PLAYER_SEEKINFO, [=](const HJSeekInfo::Ptr info){
        if(info) {
            HJLogi("seek success info, pos:" + HJ2String(info->getPos()) + ", seek id:" + HJ2String(info->getID()));
            //m_seekLimiter->remove(info);
        }
    });
    m_renderExecutor = HJExecutorByName("HJRender");  //HJCreateExecutor(HJFMT("{}_{}", getName(), "render"));
    //
    auto running = [=]()
    {
        HJLogi("init begin, url:" + m_mediaUrls[0]->getUrl());
        auto graphDec = std::make_shared<HJGraphDec>(m_env);
        int ret = graphDec->init(m_mediaUrls);
        if (HJ_OK != ret) {
            HJFLoge("error, decode graph init failed:{}", ret);
            return;
        }
        m_graphDec = graphDec;
        m_runState = HJRun_Init;
    };
    m_scheduler->asyncAlwaysTask(running);
    
    return HJ_OK;
}

void HJMediaPlayer::done()
{
    if(!m_scheduler) {
        return;
    }
    HJLogi("begin, name:" + getName());
    HJPlayerBase::done();
    auto running = [=]() {
        HJLogi("sync done etnry");
        if(m_graphDec) {
            m_graphDec->done();
            m_graphDec = nullptr;
        }
        if (m_audioRender) {
            m_audioRender->done();
            m_audioRender = nullptr;
        }
        if(m_videoRender) {
            m_videoRender->done();
            m_videoRender = nullptr;
        }
        m_renderExecutor = nullptr;
        m_listener = nullptr;
        m_mavfListener = nullptr;
        HJLogi("sync done end");
    };
    m_scheduler->sync(running);
    m_scheduler = nullptr;
    HJLogi("end, name:" + getName());
}

int HJMediaPlayer::start()
{
    if (HJ_OK != HJPlayerBase::start()) {
        HJLoge("graph start error");
        return HJErrFatal;
    }
    auto running = [&]()
    {
        int res = HJ_OK;
        HJLogi("start begin, name:" + getName());
        do {
            if (!m_graphDec || m_runState < HJRun_Init) {
                res = HJErrNotAlready;
                break;
            }
            m_runState = HJRun_Start;
            //
            res = HJPlayerBase::start();
            if (HJ_OK != res) {
                HJLoge("start error:" + HJ2String(res));
                break;
            }
            res = m_graphDec->buildGraph();
            if (HJ_OK != res) {
                HJLogi("demuxer start error:" + HJ2STR(res));
                break;
            }
            HJMediaInfo::Ptr mediaInfo = m_graphDec->getMediaInfo();
            if (!mediaInfo || mediaInfo->getStreamInfos().size() <= 0) {
                HJLoge("media info error");
                res = HJErrNotMInfo;
                break;
            }
            HJLogi("start begin, build graph");
            const auto& streamInfos = mediaInfo->getStreamInfos();
            for (auto it = streamInfos.begin(); it != streamInfos.end(); it++)
            {
                HJMediaType mediaType = (HJMediaType)(*it)->getType();
                switch (mediaType)
                {
                    case HJMEDIA_TYPE_VIDEO:
                    {
                        HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(*it);
                        //render
                        m_videoRender = std::make_shared<HJNodeVRender>(m_env, m_renderExecutor);
                        res = m_videoRender->init(videoInfo);
                        if (HJ_OK != res) {
                            HJFLoge("error, video render init res:{}", res);
                            break;
                        }
                        res = m_graphDec->connectVOut(m_videoRender);
                        if (HJ_OK != res) {
                            HJFLoge("error, decode graph connect video render failed, res:{}", res);
                            break;
                        }
                        break;
                    }
                    case HJMEDIA_TYPE_AUDIO:
                    {
                        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(*it);
                        //render
                        m_audioRender = std::make_shared<HJNodeARender>(m_env, m_renderExecutor);
                        res = m_audioRender->init(audioInfo);
                        if (HJ_OK != res) {
                            HJLoge("audio render init error:" + HJ2String(res));
                            break;
                        }
                        res = m_graphDec->connectAOut(m_audioRender);
                        if (HJ_OK != res) {
                            HJFLoge("error, decode graph connect audio render failed, res:{}", res);
                            break;
                        }
                        break;
                    }
                    default: {
                        HJLogi("not support media info, mediaType:" + HJ2String(mediaType));
                        break;
                    }
                } //switch (mediaType)
                if (HJ_OK != res) {
                    HJFLoge("error, proc media type:{} failed");
                    break;
                }
            }//for
            if (HJ_OK != res) {
                break;
            }
            res = m_graphDec->start();
            if (HJ_OK != res) {
                HJLogi("demuxer start error:" + HJ2STR(res));
                break;
            }
        } while (false);
        
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("build graph error")));
        }
        HJLogi("start end, name:" + getName());
    };
    m_scheduler->asyncAlwaysTask(running);
    
    return HJ_OK;
}

int HJMediaPlayer::play()
{
    if (HJ_OK != HJPlayerBase::play()) {
        HJLoge("play graph error:");
        return HJErrFatal;
    }
    m_isPause = false;
    auto running = [&]()
    {
        int res = HJ_OK;
        HJLogi("play begin, name:" + getName());
        do {
            if (!m_graphDec || m_runState < HJRun_Ready) {
                HJFLoge("error, player is not ready");
                res = HJErrNotAlready;
                break;
            }
            res = m_graphDec->play();
        } while(false);
        
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("play error")));
        } else {
            m_runState = HJRun_Running;
        }
        HJLogi("play end, name:" + getName());
    };
    m_scheduler->asyncAlwaysTask(running);
    
    return HJ_OK;
}

int HJMediaPlayer::pause()
{
    if (HJ_OK != HJPlayerBase::pause()) {
        HJLoge("error, playe base pause failed");
        return HJErrFatal;
    }
    m_isPause = true;
    auto running = [=]()
    {
        int res = HJ_OK;
        HJLogi("pause begin, name:" + getName());
        do {
            if (!m_graphDec || m_runState < HJRun_Running) {
                res = HJErrNotAlready;
                break;
            }
            res = m_graphDec->pause();
        } while (false);
        
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("pause error")));
        } else {
            m_runState = HJRun_Pause;
        }
        HJLogi("pause end, name:" + getName());
    };
    m_scheduler->asyncAlwaysTask(running);
    
    return HJ_OK;
}
//
//int HJMediaPlayer::resume()
//{
//    if (HJ_OK != HJPlayerBase::resume()) {
//        HJLoge("error, playe base resume failed");
//        return HJErrFatal;
//    }
//    m_isPause = false;
//    auto running = [=]()
//    {
//        int res = HJ_OK;
//        HJLogi("resume entry");
//        do {
//            if (!m_graphDec || m_runState < HJRun_Running) {
//                res = HJErrNotAlready;
//                break;
//            }
//            res = m_graphDec->resume();
//        } while (false);
//        
//        if (HJ_OK != res) {
//            notify(HJMakeNotification(HJNotify_Error, res, std::string("resume error")));
//        } else {
//            m_runState = HJRun_Running;
//        }
//    };
//    m_scheduler->asyncAlwaysTask(running);
//    
//    return HJ_OK;
//}

int HJMediaPlayer::setWindow(const std::any& window)
{
    if (HJ_OK != HJPlayerBase::setWindow(window)) {
        HJLoge("play graph error:");
        return HJErrFatal;
    }
    auto running = [=]()
    {
        int res = HJ_OK;
        do {
            if (!m_videoRender) {
                res = HJErrNotAlready;
                break;
            }
            HJLogi("setWindow entry");
            res = m_videoRender->setWindow(window);
        } while (false);

        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("set window error:" + HJ2String(res))));
        }
    };
    m_scheduler->asyncAlwaysTask(running);
    
    return HJ_OK;
}

int HJMediaPlayer::seek(int64_t pos)
{
    int res = HJPlayerBase::seek(pos);
    if (HJ_OK != res) {
        HJLoge("error, res:" + HJ2String(res));
        return res;
    }
    HJLogi("entry, seek pos:" + HJ2String(pos));
    //HJSeekInfo::Ptr seekInfo = std::make_shared<HJSeekInfo>(pos);
    HJSeekInfo::Ptr seekInfo = m_seekLimiter->checkInfo(std::make_shared<HJSeekInfo>(pos));
    if (!seekInfo) {
        HJLogi("seek info drop pos:" + HJ2String(pos));
        return HJ_OK;
    }
    auto running = [=]()
    {
        int res = HJ_OK;
        HJTicker ticker;
        HJLogi("seek begin, pos:" + HJ2String(pos) + ", seek id:" + HJ2String(seekInfo->getID()) + ", task name:" + getName());
        do {
            if (!m_graphDec || m_runState < HJRun_Ready) {
                res = HJErrNotAlready;
                break;
            }
            //
            if(m_audioRender) {
                m_audioRender->setPreFlush(true);
            }
            if(m_videoRender) {
                m_videoRender->setPreFlush(true);
            }
            res = m_graphDec->seek(seekInfo);
        } while (false);
        
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("seek error")));
        }
        HJLogi("seek end, time:" + HJ2String(ticker.elapsedTime()));
    };
    m_scheduler->asyncRemoveBefores(running, 0, getName());
    
    return HJ_OK;
}

int HJMediaPlayer::seek(float progress)
{
    int res = HJPlayerBase::seek(progress);
    if (HJ_OK != res) {
        HJLoge("seek error:" + HJ2String(res));
        return res;
    }
    //int64_t pos
    
    return HJ_OK;
}

int HJMediaPlayer::speed(float speed)
{
    int res = HJPlayerBase::speed(speed);
    if (HJ_OK != res) {
        HJLoge("speed error:" + HJ2String(res));
        return res;
    }
    auto running = [=]() {
        m_env->setSpeed(speed);
    };
    m_scheduler->asyncAlwaysTask(running);

    return HJ_OK;
}

int64_t HJMediaPlayer::getCurrentPos()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_progInfo) {
        return m_progInfo->getPos();
    }
    return 0;
}

float HJMediaPlayer::getProgress()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_progInfo && m_minfo && m_minfo->getDuration() > 0) {
        return m_progInfo->getPos() / m_minfo->getDuration();
    }
    return 0.0f;
}

void HJMediaPlayer::setVolume(const float volume)
{
    HJPlayerBase::setVolume(volume);
    auto running = [=]() {
        m_env->setVolume(volume);
    };
    m_scheduler->asyncAlwaysTask(running);

    return;
}

bool HJMediaPlayer::isReady()
{
    if (m_runState >= HJRun_Ready) {
        return true;
    }
    return false;
}

void HJMediaPlayer::setMediaFrameListener(const HJMediaFrameListener listener) 
{
    auto running = [=]()
    {
        HJLogi("set media frame listener entry");
        if (!m_mavfListener && listener)
        {
            m_mavfListener = listener;
            m_env->addMediaFrameNotify([=](const HJMediaFrame::Ptr mavf) {
                m_scheduler->asyncAlwaysTask([=]() {
                    notifyMediaFrame(mavf);
                });
            });
        } else if(!listener) {
            m_env->delMediaFrameNotify();
            m_mavfListener = nullptr;
        }
        HJLogi("set media frame listener end");
    };
    m_scheduler->asyncAlwaysTask(running);

    return;
}

void HJMediaPlayer::setSourceFrameListener(const HJMediaFrameListener listener)
{
    auto running = [=]()
    {
        HJLogi("set source frame listener entry");
        if (!m_savfListener && listener) 
        {
            m_savfListener = listener;
            m_env->addSourceFrameNotify([=](const HJMediaFrame::Ptr mavf) {
                m_scheduler->asyncAlwaysTask([=]() {
                    notifySourceFrame(mavf);
                });
            });
        }
        else if(!listener){
            m_env->delSourceFrameNotify();
            m_savfListener = nullptr;
        }
        HJLogi("set source frame listener end");
    };
    m_scheduler->asyncAlwaysTask(running);

    return;
}

NS_HJ_END
