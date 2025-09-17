//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFFDemuxerEx.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJFFDemuxerEx::HJFFDemuxerEx()
    : HJFFDemuxer() 
{

}

HJFFDemuxerEx::HJFFDemuxerEx(const HJMediaUrl::Ptr& mediaUrl)
    : HJFFDemuxer(mediaUrl)
{

}

int HJFFDemuxerEx::init(const HJMediaUrl::Ptr& mediaUrl)
{
	int res = HJFFDemuxer::init(mediaUrl);
	if (HJ_OK != res) {
		return res;
	}
	auto& vinfo = m_mediaInfo->getVideoInfo();
	if (vinfo) {
		if (HJ_BEYOND(vinfo->m_frameRate, 10, 100)) {
			HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo(m_mediaInfo->m_vstIdx);
			trackInfo->m_guessDuration = std::make_shared<HJDataStati64>(100);
			vinfo->m_frameRate = 15;
		}
	}
	return HJ_OK;
}

int HJFFDemuxerEx::init() {
    return init(m_mediaUrl);
}

int HJFFDemuxerEx::getFrame(HJMediaFrame::Ptr& frame)
{
    int res = HJFFDemuxer::getFrame(frame);
    if (HJ_OK != res || !frame) {
        return res;
    }
    res = checkFrame(frame);
    //HJFNLogi(frame->formatInfo());

    return res;
}

/**
 * (1) ����ʱ�����
 * (2) ��ʼʱ��PTS��DTS�Ǹ�ֵ(A: ������ʼʱ�䣻B������duration)
 * (3) ʱ�����䣨���ϡ����£���A���ؼ�֡���䣨ʱ���޸�OK����B���ǹؼ�֡���䣨DTS�޸���;  C����֡ʱ�� <= ǰ֡ʱ�䣻D����ʼ��ʱ����Ч֡��
 * (4) PTS��DTSʱ�佻�����ã�ͳ�Ʋ�ֵ
 * (5) ƽ�� duration
 * (6) �޸� guess frame rate��������
 *
 */
int HJFFDemuxerEx::checkFrame(const HJMediaFrame::Ptr& frame)
{
    //HJLogi("entry");
    HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo((int)frame->getID());
    if (!trackInfo) {
        return HJErrFatal;
    }
    int64_t delta = 0;
    if (HJ_NOTS_VALUE != trackInfo->m_lastTS.m_dts) {
        delta = frame->getDTS() - (trackInfo->m_lastTS.m_dts + trackInfo->m_frameDurations.getAvg());
    }
    int64_t avgDelta = trackInfo->m_deltas.add(delta);
    //if (HJ_ABS(avgDelta) > frame->getDuration())
    if(delta < 0 && HJ_ABS(delta) > (m_gapFrameMax * frame->getDuration()))
    {
        HJFLogw("avg guess delta:{}, duration:{}, delta:{}, dts:{}, last dts:{} avg duration:{}", avgDelta, frame->getDuration(), delta, frame->getDTS(), trackInfo->m_lastTS.m_dts, trackInfo->m_frameDurations.getAvg());
        auto atrs = std::make_shared<HJSourceAttribute>();
        atrs->m_preDuration = m_atrs->m_preDuration + HJ_MAX(m_atrs->getDuration(), m_atrs->getMaxTS());
        //atrs->setPre(m_atrs);
        m_atrs = atrs;
        //
        checkFrame(frame);
        return HJ_OK;
    }

    if (frame->getPTS() < trackInfo->m_minTS.m_pts) //(HJ_INT64_MAX == trackInfo->m_minTS.m_pts) //
    {
        // (2)
        int64_t pts = frame->getPTS();
        //if((0 != pts) && (!m_atrs->m_offsetTime || pts < m_atrs->m_offsetTime)) {
        if (HJ_NOTS_VALUE == m_atrs->m_offsetTime || pts < m_atrs->m_offsetTime) {
            int64_t offsetTime = (HJ_NOTS_VALUE != m_atrs->m_offsetTime) ? m_atrs->m_offsetTime : 0;
            int64_t plus = offsetTime - pts;
            m_atrs->m_offsetTime = pts;
            //frame->setPTSDTS(frame->getPTS() + plus, frame->getDTS() + plus);
        }
        trackInfo->m_minTS.m_pts = frame->getPTS();
        trackInfo->m_minTS.m_dts = frame->getDTS();
        trackInfo->m_minTS.m_duration = frame->getDuration();
    }
    if (frame->getPTS() > trackInfo->m_maxTS.m_pts) {
        trackInfo->m_maxTS.m_pts = frame->getPTS();
        trackInfo->m_maxTS.m_dts = frame->getDTS();
        trackInfo->m_maxTS.m_duration = frame->getDuration();
    }

    // (3)
    if (HJ_NOTS_VALUE == trackInfo->m_lastTS.m_dts) {
        trackInfo->m_lastTS.m_dts = frame->getDTS();
    }
    else {
        // (6)
        if (trackInfo->m_guessDuration && !trackInfo->m_guessDuration->isFull())
        {
            trackInfo->m_guessDuration->add(frame->getDTS() - trackInfo->m_lastTS.m_dts);
            int64_t guessDuration = trackInfo->m_guessDuration->getAvg();
            if (guessDuration > 0) {
                HJVideoInfo::Ptr mvinfo = m_mediaInfo->getVideoInfo();
                mvinfo->m_frameRate = (int)(HJ_MS_PER_SEC / (float)guessDuration + 0.5f);
            }
        }
    }
    //��5��
    trackInfo->m_frameDurations.add(frame->getDuration());
    trackInfo->m_lastTS.m_pts = frame->getPTS();
    trackInfo->m_lastTS.m_dts = frame->getDTS();
    trackInfo->m_lastTS.m_duration = frame->getDuration();

    // (4)
    m_atrs->m_interDelta = 0;
    for (auto& it : m_atrs->m_trackInfos) {
        if (trackInfo->getID() != it.first)
        {
            auto& other = it.second;
            if (HJ_NOTS_VALUE != trackInfo->m_lastTS.m_dts && HJ_NOTS_VALUE != other->m_lastTS.m_dts) {
                int64_t interDelta = HJ_ABS(trackInfo->m_lastTS.m_dts - other->m_lastTS.m_dts);
                m_atrs->m_interDelta = HJ_MAX(interDelta, m_atrs->m_interDelta);
            }
            if (HJ_INT64_MAX != trackInfo->m_minTS.m_pts && HJ_INT64_MAX != other->m_minTS.m_pts) {
                trackInfo->m_startDelta = trackInfo->m_minTS.m_pts - other->m_minTS.m_pts;
                other->m_startDelta = other->m_minTS.m_pts - trackInfo->m_minTS.m_pts;
                //                int64_t minDelta = trackInfo->m_minTS.m_pts - tinfo->m_minTS.m_pts;
                //                if(HJ_ABS(minDelta) > HJ_ABS(trackInfo->m_startDelta)) {
                //                    trackInfo->m_startDelta = minDelta;
                //                }
            }
            if (HJ_INT64_MAX != trackInfo->m_maxTS.m_pts && HJ_INT64_MAX != other->m_maxTS.m_pts) {
                trackInfo->m_endDelta = trackInfo->m_maxTS.m_pts - other->m_maxTS.m_pts;
                other->m_endDelta = other->m_maxTS.m_pts - trackInfo->m_maxTS.m_pts;
            }
            //            int64_t maxDelta = trackInfo->m_maxTS.m_pts - tinfo->m_maxTS.m_pts;
            //            if(HJ_ABS(maxDelta) > HJ_ABS(trackInfo->m_endDelta)) {
            //                trackInfo->m_endDelta = maxDelta;
            //            }
        }
    }
    m_atrs->m_interDeltaMax = HJ_MAX(m_atrs->m_interDelta, m_atrs->m_interDeltaMax);
    //
    HJMediaTime mts = frame->getMTS();
    if (m_atrs->m_preDuration > 0) {
        HJUtilTime offset = { m_atrs->m_preDuration };
        mts += offset;
        frame->setMTS(mts);
        frame->setPTSDTS(mts.getPTS().getValue(), mts.getDTS().getValue(), mts.getPTS().getTimeBase());
    }
    //HJLogi("{attrs:" + m_atrs->formatInfo() + ", {track:" + trackInfo->formatInfo() + "} }");

    return HJ_OK;
}

NS_HJ_END
