//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNodeMuxer.h"
#include "HJTicker.h"
#include "HJContext.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJNodeMuxer::HJNodeMuxer(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
    : HJMediaNode(env, scheduler, isAuto)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeMuxer)));
}

HJNodeMuxer::HJNodeMuxer(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto/* = true*/)
    : HJMediaNode(env, executor, isAuto)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJNodeMuxer)));
}

HJNodeMuxer::~HJNodeMuxer()
{
    done();
}

int HJNodeMuxer::init(const HJMediaInfo::Ptr& mediaInfo)
{
    HJLogi("entry");
    int res = HJMediaNode::init();
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    m_mediaInfo = mediaInfo;

    m_muxer = std::make_shared<HJFFMuxer>();
    res = m_muxer->init(mediaInfo);
    if (HJ_OK != res) {
        HJLoge("source init error:" + HJ2String(res));
        return res;
    }
    m_runState = HJRun_Init;
    HJFLogi("end, res:{}", res);
    
    return HJ_OK;
}

void HJNodeMuxer::done()
{
    if (!m_scheduler) {
        return;
    }
    m_scheduler->sync([&] {
        HJLogi("done entry");
        m_runState = HJRun_Done;
        //m_esFrame = nullptr;
        if (m_muxer) {
            m_muxer->done();
            m_muxer = nullptr;
        }
        HJMediaNode::done();
        HJLogi("done end");
    });
    m_scheduler = nullptr;
}

/**
 * start, ready,
 */
int HJNodeMuxer::proRun()
{
    HJOnceToken token(nullptr, [&] {
        setIsBusy(false);
        //HJFLogi("proRun] end, name:{}, proc run done, is busy:{}", getName(), isBusy());
    });
    if (!m_muxer || !m_scheduler) {
        HJLoge("error, not ready");
        return HJErrNotAlready;
    }
    //HJFLogi("entry, name:{}, m_runState:{}", getName(), m_runState);
    if (HJRun_Pause == m_runState || isEndOfRun()) {
        HJFLogw("warning, run state:{}", HJRunStateToString(m_runState));
        return HJ_EOF;
    }
    int res = HJ_OK;
    HJMediaFrame::Ptr esFrame = nullptr;
    do {
        HJMediaStorage::Ptr inStorage = getMinStorage();
        if (inStorage) {
            HJLogi("get min storage:" + inStorage->getName() + ", size:" + HJ2STR(inStorage->size()));
            esFrame = inStorage->pop();
        } else {
            setEOF(true);
            clearInOutStorage();
            if (m_muxer) {
                m_muxer->done();
                m_muxer = nullptr;
            }
            m_runState = HJRun_Stop;
            HJLogi("muxer eof");
            return HJ_EOF;
        }
        if (!esFrame) {
            res = HJ_IDLE;
            break;
        }
        if (HJMEDIA_TYPE_VIDEO == esFrame->getType()) {
            HJLogi("muxer receive frame video pts:" + HJ2STR(esFrame->getPTS()) + ", dts:" + HJ2STR(esFrame->getDTS()));
            m_vfCnt++;
        }
        else if (HJMEDIA_TYPE_AUDIO == esFrame->getType()) {
            HJLogi("muxer receive frame audio pts:" + HJ2STR(esFrame->getPTS()) + ", dts:" + HJ2STR(esFrame->getDTS()));
            m_afCnt++;
        }
        res = m_muxer->writeFrame(std::move(esFrame));
        if (HJ_OK != res) {
            HJLoge("muxer add frame error:" + HJ2STR(res));
            break;
        }
        m_runState = HJRun_Running;
    } while (false);    
    //HJLogi("end");

    return res;
}

void HJNodeMuxer::run()
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

/**
* 1: dts < 0, drop
* 2: (dts >= next_dts), write
*/
HJMediaFrame::Ptr HJNodeMuxer::getPropertyFrame()
{
    HJMediaFrame::Ptr esFrame = nullptr;
    HJMediaStorage::Ptr inStorage = getMinStorage();
    if (inStorage)
    {
        HJLogi("get min storage:" + inStorage->getName() + ", size:" + HJ2STR(inStorage->size()));
        esFrame = inStorage->pop();
    } else {
        HJLogi("maybe eof");
        setEOF(true);
        clearInOutStorage();
        if (m_muxer) {
            m_muxer->done();
            m_muxer = nullptr;
        }
        m_runState = HJRun_Stop;
    }
    return esFrame;
}

NS_HJ_END

