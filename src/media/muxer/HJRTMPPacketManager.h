//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJRTMPUtils.h"
#include "flv/HJFLVUtils.h"
#include "HJStatisticalTools.h"
#include "HJNotify.h"
#include "HJRTMPBitrateAdapter.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef struct HJRTMPNetParams
{
    bool m_dropEnable = true;
    int m_dropThreshold = 1500;            //700 ms  B
    int m_dropPFrameThreshold = 2000;      //900 ms  P  
    int m_dropIFrameThreshold = 5000;      //3000 ms  I
    //
    int m_bitrate = 2200;                   //kbps
    int m_lowBitrate = 300;                 //kbps
    int m_adjustInterval = 5000;            //ms
} HJRTMPNetParams;

typedef enum HJRTMPStatType
{
    HJRTMP_STATTYPE_NONE = 0,
    HJRTMP_STATTYPE_QUEUED,
    HJRTMP_STATTYPE_SENT,
    HJRTMP_STATTYPE_DROPPED,
} HJRTMPStatType;

//***********************************************************************************//
typedef struct HJRTMPStatsValue
{
    int         videoFrames = 0;
    float       videoDropRatio = 0.0;
    int         video_kbps = 0;
    int         video_fps = 0;
    //
    int         audioFrames = 0;
    float       audioDropRatio = 0.0;
    int         audio_kbps = 0;
    int         audio_fps = 0;
    //
    int64_t     bytes = 0;
    float       dropRatio = 0.0;
} HJRTMPStatsValue;

typedef struct HJRTMPStreamStats
{
    int64_t          time = 0;
    HJRTMPStatsValue inStats;
    HJRTMPStatsValue outStats;
    int64_t          cacheDuration = 0;
    int64_t          cacheSize = 0;
    int64_t          delay = 0;
} HJRTMPStreamStats;

class HJRTMPStatsInfo : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJRTMPStatsInfo);
    HJRTMPStatsInfo() {
        m_videoTracker = HJCreates<HJBitrateTracker>(5 * HJ_MS_PER_SEC);
        m_audioTracker = HJCreates<HJBitrateTracker>(5 * HJ_MS_PER_SEC);
    }
    virtual ~HJRTMPStatsInfo() {};
public:
    HJRTMPStatsValue      m_val;
    //
    HJBitrateTracker::Ptr m_videoTracker = nullptr;
    HJBitrateTracker::Ptr m_audioTracker = nullptr;
};

class HJRTMPPacketStats : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJRTMPPacketStats);
    virtual ~HJRTMPPacketStats();

    int update(HJRTMPStatType statType, const HJFLVPacket::Ptr packet, const HJBuffer::Ptr tag);

    std::string formatInfo();

    HJRTMPStatsInfo::Ptr getStats(HJRTMPStatType statType) {
        auto it = m_stats.find(statType);
        if (it != m_stats.end()) {
            return it->second;
        }
        m_stats[statType] = HJCreates<HJRTMPStatsInfo>();
        return m_stats[statType];
    }

    HJRTMPStreamStats getStreamStats() {
        HJRTMPStreamStats ret;
        auto stat = getStats(HJRTMP_STATTYPE_QUEUED);
        if (stat) {
            ret.inStats = stat->m_val;
        }
        stat = getStats(HJRTMP_STATTYPE_SENT);
        if (stat) {
            ret.outStats = stat->m_val;
        }
        ret.cacheDuration = m_cacheDuration;
        ret.cacheSize = m_cacheSize;
        ret.delay = m_delay;
        //
        ret.time = HJCurrentSteadyMS() - m_startTime;

        return ret;
    }
    int64_t getStartTime() const {
        return m_startTime;
    }
    int64_t getDuration() const {
        return m_duration;
    }
    
    const int getOutBitrate () {
        auto stat = getStats(HJRTMP_STATTYPE_SENT);
        if(stat) {
            return stat->m_val.video_kbps;
        }
        return 0;
    }
    const int getInBitrate () {
        auto stat = getStats(HJRTMP_STATTYPE_QUEUED);
        if(stat) {
            return stat->m_val.video_kbps;
        }
        return 0;
    }
    const float getDropRatio() {
        return m_dropRatio;
        // auto stat = getStats(HJRTMP_STATTYPE_DROPPED);
        // if(stat) {
        //     return stat->m_val.dropRatio;
        // }
        // return 0.0f;
    }
public:
    std::map<HJRTMPStatType, HJRTMPStatsInfo::Ptr> m_stats;
    int64_t     m_cacheDuration = 0;
    int64_t     m_cacheSize = 0;
    int64_t     m_delay = 0;
    int64_t     m_startTime = HJ_NOTS_VALUE;
    int64_t     m_duration = 0;
    int         m_dropCount = 0;
    HJLossRatioTracker m_lossTracker;
    float       m_dropRatio = 0.0f;
    //
    int64_t     m_logTime = HJ_NOTS_VALUE;
};

//***********************************************************************************//
class HJRTMPPacketManager : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJRTMPPacketManager);
    HJRTMPPacketManager(HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts = nullptr, HJListener listener = nullptr);
    virtual ~HJRTMPPacketManager();
    
    int push(HJFLVPacket::Ptr packet);
    HJBuffer::Ptr getTag(bool isHeader = false);
    HJBuffer::Ptr waitTag(int64_t timeout = 0, bool isHeader = false);

    virtual void setQuit(const bool isQuit = true) {
        HJLogi("entry");
        {
            HJ_AUTOU_LOCK(m_mutex);
            m_isQuit = isQuit;
        }
        m_cv.notify_all();
        HJLogi("end");
    }
    HJRTMPStreamStats getStats() {
        HJ_AUTOU_LOCK(m_mutex);
        return m_stats.getStreamStats();
    }
    void setNetBitrate(int bitrate) {
        m_netBitrate = bitrate;
    }
    void setNetBlock(const bool block) {
        m_isNetBlocking = block;
    }
private:
    int drop(bool dropAudio = true);
    void printPackets();
    std::tuple<int64_t, int64_t, int64_t> getDuartion();
    //
    HJBuffer::Ptr buildTagA(HJFLVPacket::Ptr packet, bool isHeader = false);
    HJBuffer::Ptr buildTagB(HJFLVPacket::Ptr packet, bool isHeader, bool isFooter = false);
    HJBuffer::Ptr buildTag(HJFLVPacket::Ptr packet, bool isHeader = false, bool isFooter = false);
    //
    HJBuffer::Ptr buildAudioHeader();
    HJBuffer::Ptr buildVideoHeader();
    HJBuffer::Ptr buildHDRVideoHeader();
    HJBuffer::Ptr buildFooter();

    int64_t waitStartDTSOffset();
    //
    void notify(uint32_t n = 1) {
        m_count += n;
        if (1 == n) {
            m_cv.notify_one();
        }
        else {
            m_cv.notify_all();
        }
    }
    bool tryWait() {
        if (m_count > 0) {
            --m_count;
            return true;
        }
        return false;
    }
    void netWouldBlock() {
        while (m_isNetBlocking) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            HJLogi("net blocking");
        }
    }
private:
    HJListener              m_listener{ nullptr };
    //int                     m_mediaTypes{ HJMEDIA_TYPE_NONE };
    HJMediaInfo::Ptr        m_mediaInfo = nullptr;
    HJOptions::Ptr          m_opts{ nullptr };
    HJFLVPacketList         m_packets;
    int                     m_sentStatus = HJ_RTMP_SENT_NONE;
    bool                    m_enhancedRtmp = false;
    bool                    m_firstKeyFrame = false;
    int64_t				    m_startDTSOffset = HJ_NOTS_VALUE;
    bool                    m_gotFirtFrame = false;
    //
    std::mutex              m_mutex;
    std::condition_variable m_cv;
    size_t                  m_count = 0;
    bool                    m_isQuit{ false };
    //
    HJRTMPNetParams         m_netParams;
    int64_t                 m_netBitrate = 0; //kbps
    bool                    m_isNetBlocking{ false };
    HJRTMPPacketStats       m_stats;
    HJRTMPBitrateAdapter::Utr    m_bitrateAdapter = nullptr;
    int                     m_adaptiveBitrate = 0;
    //
    int64_t                 m_waitTagTime = HJ_NOTS_VALUE;
};

NS_HJ_END

