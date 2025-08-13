//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJRTMPBitrateAdapter : public HJObject {
public: 
    HJ_DECLARE_PUWTR(HJRTMPBitrateAdapter)
    HJRTMPBitrateAdapter(int bitrate, int minBitrate = 400, int adjustInterval = 5000, int dropThreshold = 1500);
    virtual ~HJRTMPBitrateAdapter() = default;
    
    int update2(int inBitrate, int outBitrate, int netBitrate, float dropRatio, int64_t queueDuration);
    float update(int inBitrate, int sentBitrate, int64_t queueDuration, int dropCount, float dropRatio);
    
    void reset();

    bool valabe() {
        auto now = HJCurrentSteadyMS();
        if (HJ_NOTS_VALUE == m_valableTime) {
            m_valableTime = now;
            return true;
        }
        if (now - m_valableTime > 1000) {
            m_valableTime = now;
            return true;
        }
        return false;
    }
private:
    int m_recommendedBitrate = 0;
    int m_bitrate = 0;
    int m_minBitrate = 400;
    int m_adjustInterval = 5000;
    int m_dropThreshold = 1500;
    float m_k1 = 0.5f;
    float m_k2 = 0.1f;
    int64_t m_lastTime = HJ_NOTS_VALUE;
    int64_t m_goodTime = 0;
    int64_t m_badTime = 0;
    //
    int64_t m_valableTime = HJ_NOTS_VALUE;
};



NS_HJ_END