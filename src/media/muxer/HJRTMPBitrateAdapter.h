//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJBitrateEvaluator : public HJObject
{
public: 
    HJ_DECLARE_PUWTR(HJBitrateEvaluator);
    HJBitrateEvaluator(int64_t expireTime, int maxBitrate = 2200, int minBitrate = 400) 
        : m_expireTime(expireTime)
        , m_maxBitrate(maxBitrate)
        , m_minBitrate(minBitrate) {
            m_startTime = HJCurrentSteadyMS();
        }
    virtual ~HJBitrateEvaluator() = default;

    virtual int evaluate(int recommendedBitrate, int receivableBitrate) = 0;
    virtual bool expire() {
        return HJCurrentSteadyMS() - m_startTime > m_expireTime;
    };
protected:
    int64_t m_startTime{0};
    int64_t m_expireTime{0};
    int m_maxBitrate{2200};
    int m_minBitrate{400};
};

class HJBitrateIncEvaluator : public HJBitrateEvaluator
{ 
public: 
    HJ_DECLARE_PUWTR(HJBitrateIncEvaluator);
    HJBitrateIncEvaluator(int64_t expireTime, int maxBitrate = 2200, int minBitrate = 400)
        : HJBitrateEvaluator(expireTime, maxBitrate, minBitrate)
    { }
    virtual int evaluate(int recommendedBitrate, int receivableBitrate);
private:

};

class HJBitrateDecEvaluator : public HJBitrateEvaluator
{ 
public: 
    HJ_DECLARE_PUWTR(HJBitrateDecEvaluator);
    HJBitrateDecEvaluator(int64_t expireTime, int maxBitrate = 2200, int minBitrate = 400)
        : HJBitrateEvaluator(expireTime, maxBitrate, minBitrate)
    { }
    virtual int evaluate(int recommendedBitrate, int receivableBitrate);
private:
};

class HJRTMPBitrateAdapter : public HJObject {
public: 
    HJ_DECLARE_PUWTR(HJRTMPBitrateAdapter)
    HJRTMPBitrateAdapter(int bitrate, int minBitrate = 400, int adjustInterval = 4000, int dropThreshold = 2000);
    virtual ~HJRTMPBitrateAdapter() = default;
    
    int evaluateBitrate(int inBitrate, int outBitrate, int netBitrate, float dropRatio, int64_t queueDuration);
    int update2(int inBitrate, int outBitrate, int netBitrate, float dropRatio, int64_t queueDuration);
    float update(int inBitrate, int sentBitrate, int64_t queueDuration, int dropCount, float dropRatio);
    
    void reset();
private:
    int m_recommendedBitrate = 0;
    int m_bitrate = 0;
    int m_minBitrate = 400;
    int m_adjustInterval = 5000;
    int m_dropThreshold = 2000;
    int m_queueDurationMin = 150;
    //
    HJBitrateEvaluator::Ptr m_evaluator = nullptr;
    //
    float m_k1 = 0.5f;
    float m_k2 = 0.1f;
    int64_t m_lastTime = HJ_NOTS_VALUE;
    int64_t m_goodTime = 0;
    int64_t m_badTime = 0;
};



NS_HJ_END