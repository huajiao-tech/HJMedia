//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNodeDemuxer.h"
#include "HJTicker.h"
#include "HJContext.h"
#include "HJFLog.h"

NS_HJ_BEGIN
const std::string HJNodeDemuxer::DEMUXER_KEY_SEEK = "42d0947a-1941-4b2c-a589-fc648de57d15";
//***********************************************************************************//
HJNodeDemuxer::HJNodeDemuxer(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, scheduler, isAuto)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeDemuxer)));
}

HJNodeDemuxer::HJNodeDemuxer(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto/* = true*/)
    : HJMediaNode(env, executor, isAuto)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeDemuxer)));
}

HJNodeDemuxer::~HJNodeDemuxer()
{
    done();
}

int HJNodeDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
{
    int res = HJMediaNode::init();
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    auto running = [=]()
    {
        int ret = HJ_OK;
        do
        {
            HJFFDemuxer::Ptr source = std::make_shared<HJFFDemuxer>();
            source->setLowDelay(m_env->getLowDelay());
            ret = source->init(mediaUrl);
            if (HJ_OK != ret) {
                HJLoge("source init error:" + HJ2String(ret));
                break;
            }
            m_source = source;
            //
            int64_t startTime = mediaUrl->getStartTime();
            if (HJ_NOTS_VALUE != startTime && startTime < m_source->getMediaInfo()->m_duration)
            {
                ret = m_source->seek(startTime);
                if (HJ_OK != ret) {
                    HJLoge("seek time:" + HJ2String(startTime) + " error:" + HJ2STR(ret));
                    break;
                }
            }
            m_demuxerState = HJDemuxer_Init;
        } while (false);
        //
        if (HJ_OK != ret) {
            notify(HJMakeNotification(HJNotify_Error, ret, std::string("demuxer init error, url" + mediaUrl->getUrl())));
        } else {
            m_runState = HJRun_Init;
            //
            HJNotification::Ptr ntf = HJMakeNotification(HJNotify_Prepared);
            (*ntf)[HJMediaInfo::KEY_WORLDS] = m_source->getMediaInfo();
            notify(ntf);
        }
        HJFLogi("init end, res:{}, url:{}", res, mediaUrl->getUrl());
    };
    m_scheduler->async(running);

    return HJ_OK;
}
int HJNodeDemuxer::init(const HJMediaUrlVector& mediaUrls)
{
    int res = HJMediaNode::init();
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    auto running = [=]() 
    {
        int ret = HJ_OK;
        auto mediaUrl = mediaUrls[0];
        do
        {
            HJComplexDemuxer::Ptr source = std::make_shared<HJComplexDemuxer>();
            source->setLowDelay(m_env->getLowDelay());
            ret = source->init(mediaUrls);
            if (HJ_OK != ret) {
                HJLoge("source init error:" + HJ2String(ret));
                break;
            }
            m_source = source;
            //
            int64_t startTime = mediaUrl->getStartTime();
            if (HJ_NOTS_VALUE != startTime && startTime < m_source->getMediaInfo()->m_duration)
            {
                ret = m_source->seek(startTime);
                if (HJ_OK != ret) {
                    HJLoge("seek time:" + HJ2String(startTime) + " error:" + HJ2STR(ret));
                    break;
                }
            }
            m_demuxerState = HJDemuxer_Init;
        } while (false);
        //
        if (HJ_OK != ret) {
            notify(HJMakeNotification(HJNotify_Error, ret, std::string("demuxer init error, url" + mediaUrl->getUrl())));
        } else {
            m_runState = HJRun_Init;
            //
            const auto& mediaInfo = m_source->getMediaInfo();
            m_env->setEnableMediaTypes(mediaInfo->m_mediaTypes);
            //
            HJNotification::Ptr ntf = HJMakeNotification(HJNotify_Prepared);
            (*ntf)[HJMediaInfo::KEY_WORLDS] = mediaInfo;
            notify(ntf);
        }
        HJFLogi("init end, res:{}, url:{}", res, mediaUrl->getUrl());
    };
    m_scheduler->async(running);

    return HJ_OK;
}

void HJNodeDemuxer::done()
{
    if (!m_scheduler) {
        return;
    }
    m_scheduler->sync([&] {
        HJLogi("done entry");
        m_runState = HJRun_Done;
        m_esFrame = nullptr;
        m_source = nullptr;

        HJMediaNode::done();
        HJLogi("done end");
    });
    m_scheduler = nullptr;

    return;
}

int HJNodeDemuxer::seek(const HJSeekInfo::Ptr info)
{
    auto running = [=]()
    {
        if (!m_source) {
            return;
        }
        int res = HJ_OK;
        HJTicker ticker;
        uint64_t pos = info->getPos();
        HJLogi("demuxer async seek begin, pos:" + HJ2String(pos));
        do {
            res = m_source->seek(pos);
            if (HJ_OK != res) {
                HJLoge("flush error:" + HJ2String(res));
                break;
            }
            if(getEOF()) {
                setEOF(false);
                m_runState = HJRun_Running;
            }
            m_esFrame = nullptr;
            m_demuxerState = HJDemuxer_Init;
            //
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
        } while (false);
        //
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("demuxer seek error")));
        }
        HJLogi("demuxer async seek end, time:" + HJ2String(ticker.elapsedTime()));
    };
    m_scheduler->sync(running);
    
    return HJ_OK;
}

/**
* 1��error, EOF��done��next full
* 2��no frame,
*/
int HJNodeDemuxer::proRun()
{
    HJOnceToken token(nullptr, [&] {
        setIsBusy(false);
        //HJFLogi("proRun] token end, name:{}, proc run done, is busy:{}", getName(), isBusy());
    });
    if (!m_source || !m_scheduler) {
        HJLoge("not ready");
        return HJErrNotAlready;
    }
    //HJFLogi("entry, name:{}, m_runState:{}", getName(), HJRunStateToString(m_runState));
    if (isEndOfRun()) {
        HJLogw("warning, is end of run");
        return HJ_EOF;
    }
    int res = HJ_OK;
    do
    {
        if (!m_esFrame)
        {
            HJMediaFrame::Ptr esFrame = nullptr;
            res = m_source->getFrame(esFrame);
            if (res < HJ_OK) {
                HJLoge("error, source get frame error:" + HJ2String(res));
                break;
            }
            m_esFrame = esFrame;
        }
        if (!m_esFrame) {
            //HJLoge("get out frame null");
            res = HJ_WOULD_BLOCK;
            break;
        }
        //HJFLogi("frame type:{}, dts:{}, pts:{}, frame type:{}", HJMediaType2String(m_esFrame->getType()), m_esFrame->getDTS(), m_esFrame->getPTS(), m_esFrame->getFrameTypeStr());
        if (HJFRAME_EOF == m_esFrame->getFrameType())
        {
            if (m_demuxerState < HJDemuxer_End) {
                notify(HJMakeNotification(HJNotify_DemuxEnd, res, std::string("demux end")));
                m_demuxerState = HJDemuxer_End;
            }
            notifySourceFrame(m_esFrame);

            //HJFLogi("eof deliver frame type:{}, dts:{}, pts:{}, frame type:{}", HJMediaType2String(m_esFrame->getType()), m_esFrame->getDTS(), m_esFrame->getPTS(), m_esFrame->getFrameTypeStr());
            res = deliver(m_esFrame);
            if (HJ_OK != res) {
                HJFLoge("error, eof deliver failed:{}", res);
                break;
            }
            if (getNextsEOF()) {
                setEOF(true);
                m_esFrame = nullptr;
                m_runState = HJRun_Stop;
                if (HJDemuxer_Done != m_demuxerState) {
                    notify(HJMakeNotification(HJNotify_Complete, res, std::string("play complete")));
                    m_demuxerState = HJDemuxer_Done;
                }
                HJLogi("demuxer end, eof");
                return HJ_EOF;
            }
        }
        else
        {
            m_runState = HJRun_Running;
            //
            auto nextNode = getNext(m_esFrame->getType());
            if (!nextNode) {
                m_esFrame = nullptr;            //drop
                break;
            }
            if (nextNode->isFull()) {
                //HJLogw("warning, next node:" + HJMediaType2String(nextNode->getMediaType()) + " storage is full:" + HJ2STR(nextNode->getInStorageSize()));
                return HJ_IDLE;
            }
            notifySourceFrame(m_esFrame);

            //HJFLogi("deliver frame type:{}, dts:{}, pts:{}, frame type:{}", HJMediaType2String(m_esFrame->getType()), m_esFrame->getDTS(), m_esFrame->getPTS(), m_esFrame->getFrameTypeStr());
            res = nextNode->deliver(std::move(m_esFrame));
            if (res < HJ_OK) {
                HJLoge("error, demuxer deliver error:" + HJ2String(res));
               break;
            }
        }
    } while (false);
    //HJLogi("end");
 
    return res;
}

/**
 * error, eof, done --
 * OK, WOUNLD_BLOCK --
 */
void HJNodeDemuxer::run()
{
    int res = proRun();
    if (res < HJ_OK) {
        notify(HJMakeNotification(HJNotify_Error, res, getName() + "error"));
        return;
    }
    if (HJ_EOF == res || HJ_IDLE == res) {
        return;
    }
    tryAsyncSelf();
    return;
}

int HJNodeDemuxer::deliver(HJMediaFrame::Ptr mediaFrame, const std::string& key/* = ""*/)
{
    int res = HJ_OK;
    for (const auto& it : m_nexts) {
        auto node = it.second.lock();
        if (node && !node->isFull()) {
            res = node->deliver(mediaFrame);
            if (HJ_OK != res) {
                break;
            }
        }
    }
    return res;
}

//int HJNodeDemuxer::onMessage(const HJMessage::Ptr msg)
//{
//    if (!msg) {
//        return HJ_OK;
//    }
//    int res = HJ_OK;
//    switch (msg->getID())
//    {
//        case HJ_MSG_Seek:
//        {
//            const std::any* anyObj = msg->getStorage(HJSeekInfo::KEY_WORLDS);
//            if (!anyObj) {
//                break;
//            }
//            HJSeekInfo::Ptr seekInfo = std::any_cast<HJSeekInfo::Ptr>(*anyObj);
//            if(!seekInfo) {
//                break;
//            }
//            res = onSeek(seekInfo);
//            break;
//        }
//        default:
//            break;
//    }
//    return res;
//}

//int HJNodeDemuxer::onSeek(const HJSeekInfo::Ptr info)
//{
//    auto running = [=]()
//    {
//        int res = HJ_OK;
//        HJTicker ticker;
//        uint64_t pos = info->getPos();
//        HJLogi("202204301245 demuxer async seek begin, pos:" + HJ2String(pos));
//        do {
//            res = HJMediaNode::flush();
//            if (HJ_OK != res) {
//                HJLoge("flush error:" + HJ2String(res));
//                break;
//            }
//            res = m_source->seek(pos);
//            //
//            m_esFrame = nullptr;
//        } while (false);
//        //
//        if (HJ_OK != res) {
//            notify(HJMakeNotification(HJNotify_Error, res, std::string("demuxer seek error")));
//        }
//        HJLogi("202204301245 demuxer async seek end, time:" + HJ2String(ticker.elapsedTime()));
//    };
//    m_scheduler->asyncCoro(running);
//
//    return HJ_OK;
//}

NS_HJ_END
