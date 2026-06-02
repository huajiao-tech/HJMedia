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
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJComplexDemuxer)));
}

HJComplexDemuxer::~HJComplexDemuxer()
{
    done();
}

void HJComplexDemuxer::done()
{
    HJFLogi("entry");
    HJBaseDemuxer::done();
    //
    for (const auto& source : m_sources) {
        if (source) {
            source->done();
        }
    }
    m_sources.clear();
    m_mediaUrls.clear();
    m_curSource = nullptr;
    m_segOffset = 0;
    m_streamTotalIndex = 0;
    HJFLogi("end");
}

int HJComplexDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
{
    if (!mediaUrl) {
        return HJErrInvalidParams;
    }
    int res = HJBaseDemuxer::init(mediaUrl);
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    m_mediaUrls.push_back(m_mediaUrl);
    res = checkUrls();
    if (HJ_OK != res) {
        return res;
    }

    res = procMediaUrls();
    if (HJ_OK != res) {
        return res;
    }
	m_runState = HJRun_Ready;

    return res;
}

int HJComplexDemuxer::init(const HJMediaUrlVector& mediaUrls)
{
    if (mediaUrls.empty()) {
        return HJErrInvalidParams;
    }
    int res = HJBaseDemuxer::init(mediaUrls);
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    m_mediaUrls = mediaUrls;
    res = checkUrls();
    if (HJ_OK != res) {
        return res;
    }

    res = procMediaUrls();
    if (HJ_OK != res) {
        return res;
    }
    m_runState = HJRun_Ready;

    return res;
}

int HJComplexDemuxer::procMediaUrls()
{
    if (m_mediaUrls.empty()) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;

    HJFLogi("entry, urls size:{}", m_mediaUrls.size());
    do {
        HJBaseDemuxer::Ptr preSource = nullptr;
        int64_t totalDuration = 0;
        for (const auto& murl : m_mediaUrls)
        {
            auto source = std::make_shared<HJFFDemuxer>(murl);  //HJFFDemuxerEx
            source->setLowDelay(m_lowDelay);
            source->setTimeOffsetEnable(m_timeOffsetEnable);
            source->setSeekKeyFrame(m_seekKeyFrame);
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
                    // 清理已添加的 sources
                    for (auto& s : m_sources) {
                        if (s) s->done();
                    }
                    m_sources.clear();
                    break;
                }
                auto& minf = m_curSource->getMediaInfo();
                if (!minf) {
                    HJFLoge("error, url:{} get media info failed", murl->getUrl());
                    return HJErrInvalid;
                }
                m_mediaInfo = minf->dup();
            }
            else if (murl->getDuration() <= 0)
            {
                res = source->init();
                if (HJ_OK != res) {
                    HJFLoge("error, url:{} source init failed:{}", murl->getUrl(), res);
                    // 清理已添加的 sources
                    for (auto& s : m_sources) {
                        if (s) s->done();
                    }
                    m_sources.clear();
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
    if (m_sources.empty() || !m_curSource) {
        return HJErrNotAlready;
    }
    // Clamp pos to valid range
    pos = HJ_CLIP(pos, 0, m_mediaInfo->getDuration() - 500);
    m_segOffset = 0;

    int res = HJ_OK;
    HJBaseDemuxer::Ptr bestSource = nullptr;
    int64_t totalDuration = 0;
    for (const auto& source : m_sources) 
    {
        int64_t sourceDuration = source->getDuration();
        int loopCnt = source->getLoopCnt();
        int64_t nextDuration = sourceDuration * loopCnt;
        
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
            int64_t seekPos = 0;
            int loopIdx = 0;
            
            // 除零保护
            if (sourceDuration > 0) {
                seekPos = posOffset % sourceDuration;
                loopIdx = static_cast<int>(posOffset / sourceDuration);
            }
            //
            m_segOffset += loopIdx * sourceDuration;
            bestSource->setLoopIdx(loopIdx + 1);
            res = bestSource->seek(seekPos);
            if (HJ_OK != res) {
                HJFLoge("error, seek to {} failed:{}", seekPos, res);
                break;
            }
            //
            if (m_curSource != bestSource) {
                m_curSource = bestSource;
                m_streamTotalIndex++;
            }
            break;
        }
        totalDuration += nextDuration;
        m_segOffset += nextDuration;
    }
    
    // 处理 pos 超出总时长的情况
    if (!bestSource) {
        HJFLogw("warning, seek pos:{} exceeds total duration:{}, seeking to end", pos, totalDuration);
        if (!m_sources.empty()) {
            bestSource = m_sources.back();
            if (bestSource && bestSource->isReady()) {
                int64_t duration = bestSource->getDuration();
                if (duration > 0) {
                    res = bestSource->seek(duration - 1);
                }
            }
        }
    }
    return res;
}

int HJComplexDemuxer::switchAudioTrack(int trackID)
{
    if (!m_curSource) {
        return HJErrNotAlready;
    }
    int res = m_curSource->switchAudioTrack(trackID);
    if (res == HJ_OK && m_mediaInfo) {
        m_mediaInfo->setSelectedAudioTrackID(trackID);
    }
    return res;
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
    if (m_sources.empty() || !m_curSource) {
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
                    m_streamTotalIndex++;
                }
                //
                mvf = nullptr;
                res = m_curSource->getFrame(mvf);
                if (res < HJ_OK || !mvf) {
                    return res;
                }
            } else {
                HJFLogw("get next source null");
            }
        }
        if (m_timeOffsetEnable && m_segOffset > 0 && mvf->isValidFrame())
        {
            HJUtilTime offset = {m_segOffset};
            auto mts = mvf->getMTS() + offset;
            mvf->setMTS(mts);
            mvf->setPTSDTS(mts.getPTS().getValue(), mts.getDTS().getValue(), mts.getPTS().getTimeBase());
        }
        mvf->m_streamIndex += m_streamTotalIndex;

        frame = std::move(mvf);
        HJFPERNLogi("{}, duration:{}", frame->formatInfo(), frame->getDuration());
    } while (false);

    return res;
}

void HJComplexDemuxer::reset()
{
    HJBaseDemuxer::reset();
    m_segOffset = 0;
    m_streamTotalIndex = 0;
    
    for (const auto& source : m_sources) {
        if (source) {
            source->reset();
        }
    }
    
    // 重置当前 source 到第一个
    if (!m_sources.empty()) {
        m_curSource = m_sources.front();
    } else {
        m_curSource = nullptr;
    }
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

int HJComplexDemuxer::checkUrls()
{
    int res = HJ_OK;
    HJMediaUrlVector mediaUrls;
    for (const auto& murl : m_mediaUrls) 
    {
        const auto& url = murl->getUrl();
        if(murl->isM3u8())
        {
            HJHLSParser::Ptr hlsParser = std::make_shared<HJHLSParser>();
            res = hlsParser->init(murl);
            if (HJ_OK != res) {
                HJFLoge("error, hls parser url failed:{}", res);
                return res;
            }
            const auto& hlsMUrls = hlsParser->getHLSMediaUrls();
            for (size_t i = 0; i < murl->getLoopCnt(); i++) {
                mediaUrls.insert(mediaUrls.end(), hlsMUrls.begin(), hlsMUrls.end());
            }
        } else {
            mediaUrls.emplace_back(murl);
        }
    }
    m_mediaUrls.swap(mediaUrls);

    return res;
}

NS_HJ_END
