//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaInfo.h"
#include "HJTicker.h"
#include "HJStatisticalTools.h"

NS_HJ_BEGIN
//***********************************************************************************//
class MTS
{
public:
    MTS(const int64_t pts, const int64_t dts)
        : m_pts(pts)
        , m_dts(dts)
        , m_duration(0)
    { }
    MTS(const int64_t pts, const int64_t dts, const int64_t duration)
        : m_pts(pts)
        , m_dts(dts)
        , m_duration(duration)
    { }
public:
    static const MTS MTS_ZERO;
    static const MTS MTS_NOPTS;
    static const MTS MTS_MAX;
public:
    int64_t     m_pts = HJ_NOTS_VALUE;
    int64_t     m_dts = HJ_NOTS_VALUE;
    int64_t     m_duration = HJ_NOTS_VALUE;
};

class HJTrackInfo : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJTrackInfo>;
    HJTrackInfo(int idx) {
        setID(idx);
    }
    HJTrackInfo(const std::string key) {
        setName(key);
    }
    void reset() {
        m_lastTS = MTS::MTS_NOPTS;
        m_frameDurations.reset();
        m_counter.reset();
    }
    int64_t getDuration() {
        return m_maxTS.m_pts + m_maxTS.m_duration - m_minTS.m_pts;
    }
    int64_t getMaxTS() {
        return m_maxTS.m_pts + m_maxTS.m_duration;
    }
    std::string formatInfo() {
        std::string info = "id:" + HJ2STR(getID()) + ", pts:" + HJ2STR(m_lastTS.m_pts) + ", dts:" + HJ2STR(m_lastTS.m_dts) +
            ", min pts:" + HJ2STR(m_minTS.m_pts) + ", dts:" + HJ2STR(m_minTS.m_dts) +
            ", max pts:" + HJ2STR(m_maxTS.m_pts) + ", dts:" + HJ2STR(m_maxTS.m_dts) +
            ", start delta:" + HJ2STR(m_startDelta) + ", end delta:" + HJ2STR(m_endDelta) +
            ", avg duration:" + HJ2STR(m_frameDurations.getAvg()) + ", dts jump:" + HJ2STR(m_deltas.getAvg());
        return info;
    }
    HJAVCounter& getCounter() {
        return m_counter;
    }
    void incIn() {
        m_counter.incIn();
    }
    void incOut() {
        m_counter.incOut();
    }
public:
    MTS                 m_minTS = MTS::MTS_MAX;    //ms
    int64_t             m_startDelta = 0;
    MTS                 m_maxTS = MTS::MTS_ZERO;
    int64_t             m_endDelta = 0;
    MTS                 m_lastTS = MTS::MTS_NOPTS;
    HJDataStati64      m_frameDurations = HJDataStati64(10);
    HJDataStati64      m_deltas = HJDataStati64(10);
    HJDataStati64::Ptr m_guessDuration = nullptr;
    //
    HJAVCounter        m_counter;
};
using HJTrackInfoMap = std::map<int, HJTrackInfo::Ptr>;

//***********************************************************************************//
class HJTrackInfoHolder
{
public:
    using Ptr = std::shared_ptr<HJTrackInfoHolder>;

    HJTrackInfo::Ptr getTrackInfo(const std::string key) {
        auto it = m_tracks.find(key);
        if (it != m_tracks.end()) {
            return it->second;
        }
        HJTrackInfo::Ptr trackInfo = std::make_shared<HJTrackInfo>(key);
        addTrackInfo(trackInfo);

        return trackInfo;
    }

    void addTrackInfo(const HJTrackInfo::Ptr track) {
        m_tracks[track->getName()] = track;
    }
    const size_t getTrackSize() {
        return m_tracks.size();
    }
private:
    std::map<std::string, HJTrackInfo::Ptr>    m_tracks;
};

NS_HJ_END