//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNodeVDecoder.h"
#include "HJContext.h"
#include "HJVDecFFMpeg.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJNodeVDecoder::HJNodeVDecoder(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, scheduler, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_VIDEO;
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeVDecoder)));
}

HJNodeVDecoder::HJNodeVDecoder(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto/* = true*/)
    : HJMediaNode(env, executor, isAuto)
{
    m_mediaType = HJMEDIA_TYPE_VIDEO;
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeVDecoder)));
}

HJNodeVDecoder::~HJNodeVDecoder()
{
    done();
}

int HJNodeVDecoder::init(const HJVideoInfo::Ptr& info, const HJDeviceType devType/* = HJDEVICE_TYPE_NONE*/)
{
    if (!info) {
        return HJErrInvalidParams;
    }
    HJLogi("init entry");
    int res = HJMediaNode::init(HJ_VDEC_STOREAGE_CAPACITY);
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    m_info = info;
    //
    HJBaseCodec::Ptr decoder = HJBaseCodec::createVDecoder(devType);
    if (!decoder) {
        HJLoge("error, create video decoder failed");
        return HJErrNewObj;
    }
    //decoder->setLowDelay(m_env->getLowDelay());
    //
    res = decoder->init(m_info);
    if (HJ_OK != res) {
        HJFLoge("error, video decoder init failed�� res:{}", res);
        return res;
    }
    m_dec = decoder;
    m_runState = HJRun_Init;
    HJFLogi("init end, res:{}", res);
    
    return res;
}

void HJNodeVDecoder::done()
{
    if (!m_scheduler) {
        return;
    }
    m_scheduler->sync([&] {
        HJLogi("done entry");
        m_runState = HJRun_Done;
        m_outFrame = nullptr;
        m_dec = nullptr;

        HJMediaNode::done();
        HJLogi("done end");
    });
    m_scheduler = nullptr;

    return;
}

int HJNodeVDecoder::flush(const HJSeekInfo::Ptr info/* = nullptr*/)
{
//    HJLogi("vdecoder flush begin");
    auto running = [=]()
    {
        if (!m_dec) {
            return;
        }
        HJLogi("vdecoder sync flush start");
        int res = HJ_OK;
        do {
            res = m_dec->flush();
            if (HJ_OK != res) {
                HJLoge("flush error:" + HJ2String(res));
                break;
            }
            if(getEOF()) {
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
            notify(HJMakeNotification(HJNotify_Error, res, std::string("vdecoder flush error")));
        }
        HJLogi("vdecoder sync flush end");
    };
    m_scheduler->sync(running);
//    HJLogi("vdecoder flush end");
    return HJ_OK;
}

int HJNodeVDecoder::proRun()
{
    HJOnceToken token(nullptr, [&] {
        setIsBusy(false);
        //HJFLogi("proRun] end, name:{}, proc run done, is busy:{}", getName(), isBusy());
    });
    if (!m_dec || !m_scheduler) {
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
        if (!m_outFrame)
        {
            HJMediaFrame::Ptr rawFrame = nullptr;
            res = m_dec->getFrame(rawFrame);
            if (res < HJ_OK) {
                HJLoge("video deoder get frame error:" + HJ2String(res));
                break;
            }
            if (!rawFrame)
            {
                HJMediaFrame::Ptr esFrame = pop();
                if (!esFrame) {
                    res = HJ_IDLE;
                    //HJLogw("warning, pop frame is null");
                    break;
                }
                res = m_dec->run(std::move(esFrame));
                if (res < HJ_OK) {
                    HJLoge("video decoder run frame error:" + HJ2String(res));
                    break;
                }
                res = m_dec->getFrame(rawFrame);
                if (res < HJ_OK) {
                    HJLoge("video decoder get frame error:" + HJ2String(res));
                    break;
                }
            }
            m_outFrame = rawFrame;
        }
        if (!m_outFrame) {
            //HJLoge("get out frame null");
            res = HJ_WOULD_BLOCK;
            break;
        }
        //HJFLogi("have frame type:{}, pts:{}, dts:{}, frame type:{}", HJMediaType2String(m_outFrame->getType()), m_outFrame->getPTS(), m_outFrame->getDTS(), m_outFrame->getFrameTypeStr());
        if (HJFRAME_EOF == m_outFrame->getFrameType())
        {
            auto nextNode = getNext();
            if (nextNode && !nextNode->isFull()) {
                res = nextNode->deliver(m_outFrame);
                if (res < HJ_OK) {
                    HJLoge("deoder deliver error:" + HJ2String(res));
                    break;
                }
            }
            if (getNextsEOF()) {
                setEOF(true);
                clearInOutStorage();
                m_outFrame = nullptr;
                m_runState = HJRun_Stop;
                HJLogi("decoder end, eof");
                return HJ_EOF;
            }
        }
        else
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
            if (m_outFrame->isFlushFrame()) {
                const auto minfo = m_outFrame->getInfo()->dup();
                HJNotification::Ptr ntf = HJMakeNotification(HJNotify_MediaChanged, res, std::string("audio changed"));
                (*ntf)[HJStreamInfo::KEY_WORLDS] = minfo;
                notify(ntf);
            }
            res = nextNode->deliver(std::move(m_outFrame));
            if (res < HJ_OK) {
                HJLoge("deoder deliver error:" + HJ2String(res));
                break;
            }
        }
    } while (false);
//    HJLogi("end");

    return res;
}

void HJNodeVDecoder::run()
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
