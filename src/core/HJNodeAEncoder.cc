//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNodeAEncoder.h"
#include "HJContext.h"
#include "HJAEncFFMpeg.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJNodeAEncoder::HJNodeAEncoder(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, scheduler, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_AUDIO;
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeAEncoder)));
}

HJNodeAEncoder::HJNodeAEncoder(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto/* = true*/)
    : HJMediaNode(env, executor, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_AUDIO;
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeAEncoder)));
}

HJNodeAEncoder::~HJNodeAEncoder()
{
    done();
}

int HJNodeAEncoder::init(const HJAudioInfo::Ptr& info)
{
    if (!info) {
        return HJErrInvalidParams;
    }
    HJLogi("init entry");
    int res = HJMediaNode::init(HJ_ADEC_STOREAGE_CAPACITY);
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    m_info = info;
    //
    HJAEncFFMpeg::Ptr coder = std::make_shared<HJAEncFFMpeg>();
    if (!coder) {
        return HJErrNewObj;
    }
    res = coder->init(m_info);
    if (HJ_OK != res) {
        return res;
    }
    m_coder = coder;
    m_runState = HJRun_Init;
    HJFLogi("init end, res:{}", res);
    
    return HJ_OK;
}

void HJNodeAEncoder::done()
{
    if (!m_scheduler) {
        return;
    }
    m_scheduler->sync([&] {
        HJLogi("done entry");
        m_runState = HJRun_Done;
        m_outFrame = nullptr;
        m_coder = nullptr;

        HJMediaNode::done();
        HJLogi("done end");
    });
    m_scheduler = nullptr;

    return;
}

int HJNodeAEncoder::flush(const HJSeekInfo::Ptr info/* = nullptr*/)
{
    //HJLogi("vcoder flush begin");
    auto running = [=]()
    {
        if (!m_coder) {
            return;
        }
        HJLogi("audio coder sync flush start");
        int res = HJ_OK;
        do {
            res = m_coder->flush();
            if (HJ_OK != res) {
                HJLoge("flush error:" + HJ2String(res));
                break;
            }
            if (getEOF()) {
                setEOF(false);
                m_runState = HJRun_Running;
            }
            m_outFrame = nullptr;
            
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
            notify(HJMakeNotification(HJNotify_Error, res, std::string("audio coder flush error")));
        }
        HJLogi("audio coder sync flush end");
    };
    m_scheduler->sync(running);
    //HJLogi("vcoder flush end");
    return HJ_OK;
}

int HJNodeAEncoder::proRun()
{
    HJOnceToken token(nullptr, [&] {
        setIsBusy(false);
    //HJFLogi("proRun] end, name:{}, proc run done, is busy:{}", getName(), isBusy());
    });
    if (!m_coder || !m_scheduler) {
        HJLoge("not ready");
        return HJErrNotAlready;
    }
    //HJFLogi("entry, name:{}, m_runState:{}", getName(), HJRunStateToString(m_runState));
    if (isEndOfRun()) {
        HJLogw("warning, is end of run");
        return HJ_EOF;
    }
    int res = HJ_OK;
    do {
        if (!m_outFrame)
        {
            HJMediaFrame::Ptr outFrame = nullptr;
            res = m_coder->getFrame(outFrame);
            if (res < HJ_OK) {
                HJLoge("audio coder get frame error:" + HJ2String(res));
                break;
            }
            if (!outFrame)
            {
                HJMediaFrame::Ptr inFrame = pop();
                if (!inFrame) {
                    res = HJ_IDLE;
                    //HJLogw("warning, pop frame is null");
                    break;
                }
                res = m_coder->run(std::move(inFrame));
                if (res < HJ_OK) {
                    HJLoge("audio coder run frame error:" + HJ2String(res));
                    break;
                }
                res = m_coder->getFrame(outFrame);
                if (res < HJ_OK) {
                    HJLoge("audio coder get frame error:" + HJ2String(res));
                    break;
                }
            }
            m_outFrame = outFrame;
        }
        if (!m_outFrame) {
            //HJLoge("get out frame null");
            res = HJ_WOULD_BLOCK;
            break;
        }
        
        HJLogi("have frame type:" + HJMediaType2String(m_outFrame->getType()) + ", dts:" + HJ2String(m_outFrame->getDTS()) + ", pts:" + HJ2String(m_outFrame->getPTS()) + ", frame type:" + m_outFrame->getFrameTypeStr());
        if (HJFRAME_EOF == m_outFrame->getFrameType())
        {
            auto nextNode = getNext();
            if (nextNode && !nextNode->isFull(m_name)) {
                res = nextNode->deliver(m_outFrame, m_name);
                if (res < HJ_OK) {
                    HJLoge("audio coder deliver error:" + HJ2String(res));
                    break;
                }
            }
            if(getNextsEOF()) {
                setEOF(true);
                clearInOutStorage();
                m_outFrame = nullptr;
                m_runState = HJRun_Stop;
                HJLogi("video coder end, eof");
                return HJ_EOF;
            }
        } else
        {
            m_runState = HJRun_Running;
            //
            auto nextNode = getNext();
            if (!nextNode) {
                //HJFLogw("warning, has no next node, drop frame:", m_outFrame->formatInfo());
                m_outFrame = nullptr;            //drop
                break;
            }
            if (nextNode->isFull()) {
                //HJLogw("warning, next node:" + HJMediaType2String(nextNode->getMediaType()) + " storage is full:" + HJ2STR(nextNode->getInStorageSize()));
                res = HJ_IDLE;
                break;
            }
            res = nextNode->deliver(std::move(m_outFrame));
            if (res < HJ_OK) {
                HJLoge("deoder deliver error:" + HJ2String(res));
                break;
            }
        }
    } while (false);
    //HJLogi("run end");

    return res;
}

void HJNodeAEncoder::run()
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

NS_HJ_END

