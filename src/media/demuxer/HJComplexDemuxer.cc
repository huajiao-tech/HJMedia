//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJComplexDemuxer.h"
#include "HJFLog.h"
#include "HJHLSParser.h"

/**
 * 场景：
 * 一、播放、转码 、合流（demuxer --> decoder）；
 * 1:  V Time  >  A Time，加NULL Audio Frame，decoder输出静音帧
 * 2: V Time < A Time，加NULL Video Frame，decoder输出最后一帧
 * 3: V Time ～= A Time
 * 二、转封装（demuxer --> muxer）
 * 1:  V Time  >  A Time，（添加静音帧 ？）
 * 2: V Time < A Time，（输出最后一帧ES流回事什么结果，丢弃多余音频帧）
 * 3: V Time ～= A Time
 */
NS_HJ_BEGIN
//***********************************************************************************//
HJComplexDemuxer::HJComplexDemuxer()
{
    
}

HJComplexDemuxer::~HJComplexDemuxer()
{
    m_mediaUrls.clear();
    m_sources.clear();
    m_curSource = nullptr;
}

int HJComplexDemuxer::init(const HJMediaUrlVector& mediaUrls)
{
    if (mediaUrls.size() <= 0) {
        return HJErrInvalidParams;
    }
    int res = HJBaseDemuxer::init(mediaUrls);
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    m_mediaUrls = mediaUrls;
    checkUrls();
    
    HJLogi("entry, urls size:" + HJ2STR(m_mediaUrls.size()));
    do {
        HJBaseDemuxer::Ptr preSource = nullptr;
        int64_t totalDuration = 0;
        for (const auto& murl : m_mediaUrls)
        {
            auto source = std::make_shared<HJFFDemuxerEx>(murl);
            source->setLowDelay(m_lowDelay);
            //
            m_sources.emplace_back(source);
            //
            if (!m_curSource) 
            {
                m_curSource = source;
                //
                res = source->init();
                if (HJ_OK != res) {
                    HJFLoge("error, url:{} source init failed:{}", murl->getUrl(), res);
                    break;
                }
                auto& minf = m_curSource->getMediaInfo();
                if (!minf) {
                    HJFLoge("error, url:{} get media info failed", murl->getUrl());
                    return HJErrInvalid;
                }
                m_mediaInfo = minf->dup();
            } else if (murl->getDuration() <= 0) 
            {
                res = source->init();
                if (HJ_OK != res) {
                    break;
                }
                m_mediaInfo->addSubMInfo(source->getMediaInfo());
            }
            if (preSource) {
                preSource->setNext(source);
            }
            //
            totalDuration += source->getDuration() * source->getLoopCnt();
            preSource = source;
        }
        m_mediaInfo->setDuration(totalDuration);
    } while (false);
    HJFLogi("end, res:{}", res);

    return res;
}

int HJComplexDemuxer::seek(int64_t pos)
{
    HJBaseDemuxer::seek(pos);
    if (m_sources.size() <= 0 || !m_curSource) {
        return HJErrNotAlready;
    }
    m_segOffset = 0;

    int res = HJ_OK;
    HJBaseDemuxer::Ptr bestSource = nullptr;
    int64_t totalDuration = 0;
    for (const auto& source : m_sources) 
    {
        int64_t nextDuration = source->getDuration() * source->getLoopCnt();
        if (pos <= (totalDuration + nextDuration)) 
        {
            bestSource = source;
            if (!bestSource->isReady()) {
                res = bestSource->init();
                if (HJ_OK != res) {
                    HJFLoge("error, url:{} source init failed:{}", bestSource->getMediaUrl()->getUrl(), res);
                    break;
                }
            }
            //
            int64_t posOffset = pos - totalDuration;
            int64_t seekPos = posOffset % source->getDuration();
            int loopIdx = posOffset / source->getDuration();
            //
            m_segOffset += loopIdx * source->getDuration();
            //HJFLogi("loop idx:{}, posOffset:{}, seekPos:{}, m_segOffset:{}, url:{}", loopIdx, posOffset, seekPos, m_segOffset, bestSource->getMediaUrl()->getUrl());
            //if (bestSource->getMediaUrl()->getUrlHash() != m_curSource->getMediaUrl()->getUrlHash()) {
            //    HJFLogi("best url：{}, cur url:{}", bestSource->getMediaUrl()->getUrl(), m_curSource->getMediaUrl()->getUrl());
            //    bestSource->reset();
            //    m_curSource->reset();
            //    //notify
            //}
            bestSource->setLoopIdx(loopIdx + 1);
            bestSource->seek(seekPos);
            //
            if (m_curSource != bestSource) {
                //m_curSource->done();
                m_curSource = bestSource;
            }
            break;
        }
        totalDuration += nextDuration;
        m_segOffset += nextDuration;
    }
    return HJ_OK;
}

/**
 * (1) source loop
 * (2) next source
 * (3) next NULL
 * (4) EOF --> EOF Frame
 * single -> loop -> single -> loop -> single
 */
int HJComplexDemuxer::getFrame(HJMediaFrame::Ptr& frame)
{
    if (m_sources.size() <= 0 || !m_curSource) {
        return HJErrNotAlready;
    }
    //HJLogi("entry");
    int res = HJ_OK;
    do {
        HJMediaFrame::Ptr mvf = nullptr;
        res = m_curSource->getFrame(mvf);
        if (res < HJ_OK || !mvf) {
            break;
        }
        if (HJ_EOF == res)
        {
            HJLogi("EOF");
            HJBaseDemuxer::Ptr nextSource = getNextSource();
            if(nextSource)
            {
                if (m_curSource != nextSource) 
                {
                    HJFLogw("start next segment: url:{}", nextSource->getMediaUrl()->getUrl());
                    if (!nextSource->isReady()) {
                        res = nextSource->init();
                        if (HJ_OK != res) {
                            HJFLoge("error, url:{} source init failed:{}", nextSource->getMediaUrl()->getUrl(), res);
                            break;
                        }
                    }
                    //notify
                    m_curSource->done();
                    m_curSource = nextSource;
                }
                //
                mvf = nullptr;
                res = m_curSource->getFrame(mvf);
                if (res < HJ_OK || !mvf) {
                    return res;
                }
            }
        }
        if (m_segOffset > 0 && mvf->isValidFrame()) 
        {
            HJUtilTime offset = {m_segOffset};
            auto mts = mvf->getMTS() + offset;
            mvf->setMTS(mts);
            mvf->setPTSDTS(mts.getPTS().getValue(), mts.getDTS().getValue(), mts.getPTS().getTimeBase());
        }
        frame = std::move(mvf);
        //HJLogi("name:" + getName() + ", " + frame->formatInfo() + ", duration:" + HJ2STR(frame->getDuration()));
    } while (false);

    return res;
}

void HJComplexDemuxer::reset()
{
    HJBaseDemuxer::reset();
    m_segOffset = 0;
    for (const auto& source : m_sources){
        source->reset();
    }
    return;
}

int64_t HJComplexDemuxer::getDuration()
{
    if (!m_mediaInfo) {
        return 0;
    }
    return m_mediaInfo->getDuration();
}

HJBaseDemuxer::Ptr HJComplexDemuxer::getNextSource()
{
    HJBaseDemuxer::Ptr nextSource = nullptr;
    if (m_curSource->getLoopIdx() < m_curSource->getLoopCnt())
    {
        m_segOffset += m_curSource->getDuration();
        m_curSource->reset();
        m_curSource->setLoopIdx(m_curSource->getLoopIdx() + 1);
        nextSource = m_curSource;
    }
    else if (m_curSource->getNext())
    {
        m_segOffset += m_curSource->getDuration();
        nextSource = m_curSource->getNext();
    }
    return nextSource;
}

void HJComplexDemuxer::checkUrls()
{
    int res = HJ_OK;
    HJMediaUrlVector mediaUrls;
    for (const auto& murl : m_mediaUrls) 
    {
        const auto& url = murl->getUrl();
        if (HJUtilitys::endWith(url, "m3u8"))
        {
            HJHLSParser::Ptr hlsParser = std::make_shared<HJHLSParser>();
            res = hlsParser->init(murl);
            if (HJ_OK != res) {
                HJFLoge("error, hls parser url failed:{}", res);
                break;
            }
            const auto& hlsMUrls = hlsParser->getHLSMdiaUrls();
            mediaUrls.insert(mediaUrls.end(), hlsMUrls.begin(), hlsMUrls.end());
            //for (const auto & item : hlsMUrls) {
            //    mediaUrls.emplace_back(item);
            //}
        } else {
            mediaUrls.emplace_back(murl);
        }
    }
    m_mediaUrls.swap(mediaUrls);
    return;
}

NS_HJ_END
