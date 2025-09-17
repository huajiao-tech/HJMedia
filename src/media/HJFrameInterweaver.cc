//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFrameInterweaver.h"
#include "HJMediaUtils.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJFrameInterweaver::HJFrameInterweaver()
{

}

HJFrameInterweaver::~HJFrameInterweaver()
{
    
}

int HJFrameInterweaver::init(const HJMediaInfo::Ptr& mediaInfo)
{
    int res = HJ_OK;
    
    do {
        m_mediaInfo = mediaInfo->dup();
        res = m_mediaInfo->forEachInfo([&](const HJStreamInfo::Ptr& info) -> int {
            int ret = HJ_OK;
            const std::string key = info->getTrackKey();
            m_trackCaches[key] = std::make_shared<HJMediaFrameCache>(HJ_INT_MAX, key);
            HJLogi("create stream name:" + key);
            return ret;
         });
        if (res < HJ_OK) {
            HJLogi("m_mediaInfo->forEachInfo error:" + HJ2STR(res) + ", " + HJ_AVErr2Str(res));
            break;
        }
        
    } while (false);
    
    return res;
}

int HJFrameInterweaver::addFrame(const HJMediaFrame::Ptr& frame)
{
    if(!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    const std::string key = frame->getTrackKey();
    HJMediaFrameCache::Ptr trackCache = getTrackCache(key);
    if(!trackCache) {
        HJLoge("error, not find stream name:" + key);
        return HJErrNotAlready;
    }
    HJMediaFrame::Ptr mvf = frame;
    checkFrame(mvf, trackCache);
    res = trackCache->push(std::move(mvf));
    if(HJ_OK != res) {
        return res;
    }
    HJFLogi("add cache:{} size:{}, frame info:{}", key, trackCache->size(), frame->formatInfo());
    
    return res;
}

/**
 * 1: all  track ready
 * 2: Using DTS as a discriminant basis
 * 3: DTS0 < DTS1
 * 4: DTS < PTS
 * 5: over 2 track ready, output min dts frame
 * 6: beyond HJ_INTERWEAVE_DELTA_MAX, output any frame
 * 7: EOF
 */
HJMediaFrame::Ptr HJFrameInterweaver::getFrame()
{
    HJMediaFrameCache::Ptr suitableTrack = checkTracks();
    if(!suitableTrack) {
        //HJLogw("warning, wait suitable tracks frame");
        return nullptr;
    }
    HJMediaFrame::Ptr outFrame = suitableTrack->pop();
    if (outFrame) {
        HJFLogi("get cache:{} size:{}, frame info:{}", suitableTrack->getName(), suitableTrack->size(), outFrame->formatInfo());
    }
    return std::move(outFrame);
}

HJMediaFrameCache::Ptr HJFrameInterweaver::checkTracks()
{
    HJMediaFrameCache::Ptr suitableTrack = nullptr;
    do {
        if (1 == m_trackCaches.size()) {
            return m_trackCaches.begin()->second;
        }
        //At least two tracks have data
        HJList<HJMediaFrameCache::Ptr> validTracks;
        for (const auto& it : m_trackCaches) {
            HJMediaFrameCache::Ptr cache = it.second;
            if(cache->size() > 0) {
                validTracks.emplace_back(cache);
            }
        }
        if (validTracks.size() >= 2) {
            //Find the track with the smallest dts
            suitableTrack = validTracks.front();
            int64_t minDTS = suitableTrack->getTopDTS();
            for (const auto& it : validTracks) {
                int64_t dts = it->getTopDTS();
                if (dts < minDTS) {
                    dts = minDTS;
                    suitableTrack = it;
                }
            }
        } else if (1 == validTracks.size()){
            HJMediaFrameCache::Ptr cache = validTracks.front();
            //interleave HJ_INTERWEAVE_DELTA_MAX, force output
            if ((cache->getDTSDuration() > HJ_INTERWEAVE_DELTA_MAX) || cache->hasEofFrame()) {
                suitableTrack = cache;
            }
        }
    } while (false);
    
    return suitableTrack;
}

int HJFrameInterweaver::checkFrame(const HJMediaFrame::Ptr& frame, HJMediaFrameCache::Ptr trackCache)
{
    int res = HJ_OK;
    AVPacket* pkt = NULL;
    if (frame && (HJFRAME_EOF != frame->getFrameType())) {
        pkt = (AVPacket *)frame->getAVPacket();
    }
    if(pkt)
    {
        //HJFLogi("check in frame type:{} pkt dts:{}, pts:{}", HJMediaType2String(frame->getType()), pkt->dts, pkt->pts);
        int64_t mts = trackCache->getMTS().getValue();
        if(HJ_NOTS_VALUE != mts && pkt->dts <= mts) {
            HJLogw(HJFMT("warning, pkt dts:{} < pre pkt dts:{}", pkt->dts, mts));
            pkt->dts = mts + 1;
        }
        if(pkt->pts < pkt->dts) {
            HJLogw(HJFMT("waning, pkt pts:{} < dts:{}", pkt->pts, pkt->dts));
            pkt->pts = pkt->dts + 1;
        }
        trackCache->setMTS({pkt->dts, {pkt->time_base.num, pkt->time_base.den}});
        //HJFLogi("check frame type:{} pkt dts:{}, pts:{}", HJMediaType2String(frame->getType()), pkt->dts, pkt->pts);
    }
    
    return res;
}

//***********************************************************************************//
HJAVInterweaver::HJAVInterweaver()
{

}

HJAVInterweaver::~HJAVInterweaver()
{
    done();
}

int HJAVInterweaver::init(const HJMediaInfo::Ptr& mediaInfo)
{
    int res = HJ_OK;

    do {
        m_mediaInfo = mediaInfo;
        res = m_mediaInfo->forEachInfo([&](const HJStreamInfo::Ptr& info) -> int {
            int ret = HJ_OK;
            const std::string key = info->getTrackKey();
            HJTrackInfo::Ptr trackInfo = std::make_shared<HJTrackInfo>(key);
            m_trackHolder.addTrackInfo(trackInfo);
            HJLogi("create stream track name:" + key);
            return ret;
        });
        if (res < HJ_OK) {
            HJLogi("m_mediaInfo->forEachInfo error:" + HJ2STR(res) + ", " + HJ_AVErr2Str(res));
            break;
        }

    } while (false);

    return res;
}

void HJAVInterweaver::done()
{
    m_mediaInfo = nullptr;
    m_frames.clear();
}

/**
* 1: DTS monotonic growth 
*/
int HJAVInterweaver::addFrame(const HJMediaFrame::Ptr& frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    if (HJFRAME_EOF == frame->getFrameType()) {
        m_eof = true;
        HJLogi("add eof frame");
        return HJ_OK;
    }
    int res = HJ_OK;
    do
    {
        res = checkFrame(frame);
        if (HJ_OK != res) {
            break;
        }
        const auto dts = frame->getDTS();
        m_frames.emplace(dts, frame);
        //
        HJFLogi("add cache key:{} size:{}, frame info:{}", frame->getTrackKey(), m_frames.size(), frame->formatInfo());
    } while (false);
    //
    return HJ_OK;
}
/**
* 1: one track -- output
* 2: track accumulated for more than HJ_INTERWEAVE_DELTA_MAX
* 3: EOF
*/
HJMediaFrame::Ptr HJAVInterweaver::getFrame()
{
    HJMediaFrame::Ptr frame = nullptr;
    if (m_trackHolder.getTrackSize() <= 1) {
        frame = popFrame();
    } else 
    {
        const auto front = frontFrame();
        if (!front) {
            return nullptr;
        }

        for (const auto& it : m_frames) {
            const auto& mavf = it.second;
            if (front->getTrackKey() != mavf->getTrackKey()) {
                frame = popFrame();
                break;
            }
        }
        if (!frame && m_eof) {
            frame = popFrame();
        }
    }
    if (frame) {
        HJFLogi("get frame key:{} size:{}, frame info:{}", frame->getTrackKey(), m_frames.size(), frame->formatInfo());
    }

    return frame;
}

int HJAVInterweaver::checkFrame(const HJMediaFrame::Ptr& frame)
{
    int res = HJ_OK;
    do
    {
        const auto key = frame->getTrackKey();
        const auto trackInfo = m_trackHolder.getTrackInfo(key);
        if (!trackInfo) {
            res = HJErrNotAlready;
            break;
        }
        int64_t pts = frame->getPTS();
        int64_t dts = frame->getDTS();
        int64_t duration = frame->getDuration();
        //
        if (HJ_NOTS_VALUE != trackInfo->m_lastTS.m_dts && dts <= trackInfo->m_lastTS.m_dts) {
            HJLogw(HJFMT("warning, pkt dts:{} < pre pkt dts:{}", dts, trackInfo->m_lastTS.m_dts));
            dts = trackInfo->m_lastTS.m_dts + 1;
        }
        if (pts < dts) {
            HJLogw(HJFMT("waning, pkt pts:{} < dts:{}", pts, dts));
            pts = dts + 1;
        }
        frame->setPTSDTS(pts, dts);
        //
        int64_t delta = 0;
        if (HJ_NOTS_VALUE != trackInfo->m_lastTS.m_dts) {
            delta = dts - (trackInfo->m_lastTS.m_dts + trackInfo->m_frameDurations.getAvg());
        }
        //
        if (dts < trackInfo->m_minTS.m_dts) {
            trackInfo->m_minTS.m_pts = pts;
            trackInfo->m_minTS.m_dts = dts;
            trackInfo->m_minTS.m_duration = duration;
        }
        if (dts > trackInfo->m_maxTS.m_dts) {
            trackInfo->m_maxTS.m_pts = pts;
            trackInfo->m_maxTS.m_dts = dts;
            trackInfo->m_maxTS.m_duration = duration;
        }
        trackInfo->m_frameDurations.add(duration);
        trackInfo->m_lastTS.m_pts = pts;
        trackInfo->m_lastTS.m_dts = dts;
        trackInfo->m_lastTS.m_duration = duration;
    } while (false);

    return res;
}

const HJMediaFrame::Ptr HJAVInterweaver::popFrame()
{
    const auto& it = m_frames.begin();
    if (it != m_frames.end()) {
        const HJMediaFrame::Ptr mavf = it->second;
        m_frames.erase(it);
        return mavf;
    }
    return  nullptr;
}

const HJMediaFrame::Ptr HJAVInterweaver::frontFrame()
{
    const auto& it = m_frames.begin();
    return (it != m_frames.end()) ? it->second : nullptr;
}

NS_HJ_END

