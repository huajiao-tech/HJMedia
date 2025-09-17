//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJTicker.h"
#include "HJStatisticalTools.h"
#include "HJTrackInfo.h"
#include "HJMediaFrame.h"
#include "HJXIOBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJSourceAttribute : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJSourceAttribute>;
    
    void reset() {
        //seek(0)与init状态获取的第一帧可能不一致；init获取状态为主；
        for (const auto& it : m_trackInfos) {
            HJTrackInfo::Ptr trackInfo = it.second;
            trackInfo->reset();
        }
        m_trackInfos.clear();
        m_minHoleTrack = nullptr;
        m_maxHoleTrack = nullptr;
        m_offsetTime = HJ_NOTS_VALUE;
        m_interDeltaMax = 0;
        m_interDelta = 0;
        m_preDuration = 0;

        m_pre = nullptr;
        return;
    }
    
    HJTrackInfo::Ptr getTrackInfo(int idx) {
        auto it = m_trackInfos.find(idx);
        if (it != m_trackInfos.end()) {
            return it->second;
        }
        HJTrackInfo::Ptr trackInfo = std::make_shared<HJTrackInfo>(idx);
        addTrackInfo(trackInfo);
        
        return trackInfo;
    }
    void addTrackInfo(HJTrackInfo::Ptr trackInfo) {
        m_trackInfos[(int)trackInfo->getID()] = trackInfo;
    }
    
    int64_t getDuration() {
        int64_t duration = 0;
        for (const auto& it : m_trackInfos) {
            HJTrackInfo::Ptr trackInfo = it.second;
            duration = HJ_MAX(trackInfo->getDuration(), duration);
            HJLogi("source attribute track i:" + HJ2STR(it.first) + ", duration:" + HJ2STR(trackInfo->getDuration()));
        }
        return duration;
    }
    int64_t getMaxTS() {
        int64_t maxTS = 0;
        for (const auto& it : m_trackInfos) {
            HJTrackInfo::Ptr trackInfo = it.second;
            maxTS = HJ_MAX(trackInfo->getMaxTS(), maxTS);
        }
        return maxTS;
    }
    std::string formatInfo() {
        std::string info = "offset time:" + HJ2STR(m_offsetTime) + ", interlace delta:" + HJ2STR(m_interDelta) + ", max:" + HJ2STR(m_interDeltaMax);
        if (m_minHoleTrack) {
            info += ", min hole id:" + HJ2STR(m_minHoleTrack->getID()) + ", start hole:" + HJ2STR(m_minHoleTrack->m_startDelta);
        }
        if (m_maxHoleTrack) {
            info += ", max hole id:" + HJ2STR(m_maxHoleTrack->getID()) + ", end hole:" + HJ2STR(m_maxHoleTrack->m_endDelta);
        }
        return info;
    }
    void setPre(const HJSourceAttribute::Ptr& pre) {
        m_pre = pre;
    }
    auto& getPre() {
        return m_pre;
    }
public:
    HJTrackInfoMap         m_trackInfos;
    HJTrackInfo::Ptr       m_minHoleTrack = nullptr;
    HJTrackInfo::Ptr       m_maxHoleTrack = nullptr;
    int64_t                 m_offsetTime = HJ_NOTS_VALUE;
    int64_t                 m_interDeltaMax = 0;
    int64_t                 m_interDelta = 0;           //instant
    int64_t                 m_preDuration = 0;
    //
    HJSourceAttribute::Ptr m_pre = nullptr;
};

class HJBaseDemuxer : public HJXIOInterrupt, public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJBaseDemuxer)
    HJBaseDemuxer() {};
    HJBaseDemuxer(const HJMediaUrl::Ptr& mediaUrl);
    virtual ~HJBaseDemuxer();
    
    virtual int init() { return  HJ_OK; }
    virtual int init(const HJMediaUrl::Ptr& mediaUrl);
    virtual int init(const HJMediaUrlVector& mediaUrls);
    virtual void done();
    virtual int seek(int64_t pos);
    virtual int getFrame(HJMediaFrame::Ptr& frame) { return  HJ_OK; }
    virtual void reset();
    //
    void setMediaUrl(const HJMediaUrl::Ptr mediaUrl) {
        m_mediaUrl = mediaUrl;
    }
    HJMediaUrl::Ptr getMediaUrl() {
        return m_mediaUrl;
    }
    int getLoopCnt() {
        if (m_mediaUrl) {
            return m_mediaUrl->getLoopCnt();
        }
        return 0;
    }
    virtual int64_t getDuration() {
        return 0;
    }
    const HJMediaInfo::Ptr& getMediaInfo() {
        return m_mediaInfo;
    }
    void setLowDelay(const bool lowDelay) {
        m_lowDelay = lowDelay;
    }
    const bool getLowDelay() const {
        return m_lowDelay;
    }
    void setLoopIdx(const int loopIdx) {
        m_loopIdx = loopIdx;
    }
    const int getLoopIdx() {
        return m_loopIdx;
    }
    void setIsLiving(const bool isLiving) {
        m_isLiving = isLiving;
    }
    bool getIsLiving() {
        return m_isLiving;
    }
    void setTimeOffsetEnable(const bool enable) {
        m_timeOffsetEnable = enable;
    }
    const bool getTimeOffsetEnable() const {
        return m_timeOffsetEnable;
    }

    void setNext(HJBaseDemuxer::Ptr next) {
        m_next = next;
    }
    HJBaseDemuxer::Ptr getNext() {
        return m_next.lock();
    }
    const auto& getRunState() {
        return m_runState;
    }
    const bool isReady() {
        return (HJRun_Ready == m_runState);
    }
public:
    static const std::string KEY_WORLDS_URLBUFFER;
protected:
    HJRunState              m_runState{ HJRun_NONE };
    HJMediaUrl::Ptr         m_mediaUrl = nullptr;
    HJMediaInfo::Ptr        m_mediaInfo = nullptr;
    size_t                  m_vfCnt = 0;
    size_t                  m_afCnt = 0;
    bool                    m_validVFlag = false;
    bool                    m_lowDelay = false;
    int                     m_loopIdx = 1;
    bool                    m_isLiving = false;
    int				        m_streamID{ 0 };
    bool                    m_timeOffsetEnable{ false };
    //
    HJSourceAttribute::Ptr m_atrs = nullptr;
    HJBuffer::Ptr          m_extraVBuffer = nullptr;
    HJBuffer::Ptr          m_extraABuffer = nullptr;
    HJBaseDemuxer::Wtr     m_next;
    //Debug
    HJNipMuster::Ptr       m_nipMuster = nullptr;
};
using HJBaseDemuxerList = HJList<HJBaseDemuxer::Ptr>;
using HJBaseDemuxerMap = std::unordered_map<size_t, HJBaseDemuxer::Ptr>;

NS_HJ_END
