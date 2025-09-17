//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRTMPBitrateAdapter.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
int HJBitrateIncEvaluator::evaluate(int recommendedBitrate, int receivableBitrate)
{
    int step = HJ_CLIP(receivableBitrate - recommendedBitrate, 50, 2 * 50);
    recommendedBitrate += step;
    recommendedBitrate = HJ_MIN(m_maxBitrate, recommendedBitrate);
    return recommendedBitrate;
}

int HJBitrateDecEvaluator::evaluate(int recommendedBitrate, int receivableBitrate)
{
    int step = HJ_MAX(recommendedBitrate - 0.9 * receivableBitrate, 100);
    recommendedBitrate -= step;
    recommendedBitrate = HJ_MAX(m_minBitrate, recommendedBitrate);
    return recommendedBitrate;
}

//***********************************************************************************//
HJRTMPBitrateAdapter::HJRTMPBitrateAdapter(int bitrate, int minBitrate, int adjustInterval, int dropThreshold)
    : m_bitrate(bitrate)
    , m_minBitrate(minBitrate)
    , m_adjustInterval(adjustInterval)
    , m_dropThreshold(dropThreshold)
{
     m_recommendedBitrate = m_bitrate;
}

int HJRTMPBitrateAdapter::evaluateBitrate(int inBitrate, int outBitrate, int netBitrate, float dropRatio, int64_t queueDuration)
{
    if(m_recommendedBitrate <= 0) {
        m_recommendedBitrate = inBitrate;
        HJFLogi("init m_recommendedBitrate:{}", m_recommendedBitrate);
    }
    if(m_evaluator && !m_evaluator->expire()) {
        return m_recommendedBitrate;
    }
    m_evaluator = nullptr;
    //
    // HJFLogi("entry, inBitrate:{}, outBitrate:{}, netBitrate:{}, dropRatio:{}, queueDuration:{}, m_recommendedBitrate:{}", inBitrate, outBitrate, netBitrate, dropRatio, queueDuration, m_recommendedBitrate);
    int receivableBitrate = HJ_MAX(HJ_MIN(outBitrate, netBitrate), m_minBitrate);
    if((dropRatio > 0.0f || queueDuration > m_dropThreshold) && m_recommendedBitrate > receivableBitrate)
    {
        m_evaluator = HJCreates<HJBitrateDecEvaluator>(m_adjustInterval, m_bitrate, m_minBitrate);
        m_recommendedBitrate = m_evaluator->evaluate(m_recommendedBitrate, receivableBitrate);
        //
        HJFLogi("decrease, m_recommendedBitrate:{}, receivableBitrate:{}, queueDuration:{}, m_dropThreshold:{}, dropRatio:{}", m_recommendedBitrate, receivableBitrate, queueDuration, m_dropThreshold, dropRatio);
    } else if(dropRatio <= 0.0 && queueDuration < m_queueDurationMin && m_recommendedBitrate < HJ_MIN(m_bitrate, netBitrate))
    {
        m_evaluator = HJCreates<HJBitrateIncEvaluator>(2 * m_adjustInterval, m_bitrate, m_minBitrate);
        m_recommendedBitrate = m_evaluator->evaluate(m_recommendedBitrate, netBitrate);
        //
        HJFLogi("increase, m_recommendedBitrate:{}, netBitrate:{}, queueDuration:{}, m_queueDurationMin:{}, dropRatio:{}", m_recommendedBitrate, netBitrate, queueDuration, m_queueDurationMin, dropRatio);
    }
    return m_recommendedBitrate;
}

int HJRTMPBitrateAdapter::update2(int inBitrate, int outBitrate, int netBitrate, float dropRatio, int64_t queueDuration)
{
    int recommendedBitrate = 0;
    if(m_recommendedBitrate <= 0) {
        m_recommendedBitrate = inBitrate;
    }
    HJFLogi("entry, inBitrate:{}, outBitrate:{}, netBitrate:{}, dropRatio:{}, queueDuration:{}, m_recommendedBitrate:{}", inBitrate, outBitrate, netBitrate, dropRatio, queueDuration, m_recommendedBitrate);
    int64_t gapTime = (HJ_NOTS_VALUE != m_lastTime) ? (HJCurrentSteadyMS() - m_lastTime) : 0;
    int receivableBitrate = HJ_MIN(outBitrate, netBitrate);
    if (dropRatio > 0.0f && receivableBitrate < m_recommendedBitrate * 0.7)
    {
        m_recommendedBitrate -= HJ_MAX(m_recommendedBitrate - receivableBitrate, 100);
        m_recommendedBitrate = HJ_MAX(m_minBitrate, m_recommendedBitrate);
    }



    if(dropRatio > 0.0f || receivableBitrate < m_recommendedBitrate * 0.7)
    {
        m_goodTime = 0;
        m_badTime += gapTime;
        if(m_badTime > m_adjustInterval && (outBitrate < 1.5 * netBitrate)) {
            m_recommendedBitrate = netBitrate;
            //
            m_badTime = 0;
            HJFLogw("m_recommendedBitrate:{}", m_recommendedBitrate);
            return m_recommendedBitrate;
        }
    } else {
        m_badTime = 0;
        //
        if (queueDuration < 150) {
            m_goodTime += gapTime;
        } else {
            m_goodTime -= gapTime;
        }
        m_goodTime = HJ_MAX(0, m_goodTime);
        if (m_goodTime > m_adjustInterval && (outBitrate * 1.5 < netBitrate))
        {
            auto bitrate = HJ_MIN(m_bitrate, netBitrate);
            float step = m_k2 * (bitrate - m_recommendedBitrate);
            step = HJ_MAX(50.0f, step);
            m_recommendedBitrate += step;
            m_recommendedBitrate = HJ_CLIP(m_recommendedBitrate, m_minBitrate, m_bitrate);
            //
            m_goodTime = 0;
            HJFLogw("m_recommendedBitrate:{}, step:{}", m_recommendedBitrate, step);
            return m_recommendedBitrate;
        }
    }

    m_lastTime = HJCurrentSteadyMS();

    return recommendedBitrate;
}

float HJRTMPBitrateAdapter::update(int inBitrate, int sentBitrate, int64_t queueDuration, int dropCount, float dropRatio)
{
    if(m_recommendedBitrate <= 0) {
        m_recommendedBitrate = inBitrate;
        // m_recommendedBitrate = m_bitrate = inBitrate;
    }
    // HJFLogi("entry, inBitrate:{}, sentBitrate:{}, queueDuration:{}, dropCount:{}, dropRatio:{}, m_recommendedBitrate:{}", inBitrate, sentBitrate, queueDuration, dropCount, dropRatio, m_recommendedBitrate);
    int64_t gapTime = (HJ_NOTS_VALUE != m_lastTime) ? (HJCurrentSteadyMS() - m_lastTime) : 0;
    if(dropRatio > 0.0f)
    {
        m_goodTime = 0;
        if(sentBitrate < m_bitrate) {
            m_badTime += gapTime;
        } else {
			m_badTime -= gapTime;
        }
        m_badTime = HJ_MAX(0, m_badTime);
        //
        //HJFLogi("m_badTime:{}, gapTime:{}, m_adjustInterval:{}, sentBitrate:{}", m_badTime, gapTime, m_adjustInterval, sentBitrate);
        if(m_badTime > m_adjustInterval && (sentBitrate < m_bitrate)) {
            HJFLogi("entry, m_bitrate:{}, inBitrate:{}, sentBitrate:{}, queueDuration:{}, dropCount:{}, dropRatio:{}, m_recommendedBitrate:{}", m_bitrate, inBitrate, sentBitrate, queueDuration, dropCount, dropRatio, m_recommendedBitrate);
            float step = m_k1 * (m_bitrate - sentBitrate);
            step = HJ_MAX(50.0f, step);
            m_recommendedBitrate -= step;
            m_recommendedBitrate = HJ_CLIP(m_recommendedBitrate, sentBitrate, m_bitrate);
            m_recommendedBitrate = HJ_MAX(m_minBitrate, m_recommendedBitrate);
            //
            m_badTime = 0;
            HJFLogw("m_recommendedBitrate:{}, step:{}", m_recommendedBitrate, step);
            return m_recommendedBitrate;
        }
    } else
    {
        m_badTime = 0;
        if (queueDuration < 150) {
            m_goodTime += gapTime;
        } else {
            m_goodTime -= gapTime;
        }
        m_goodTime = HJ_MAX(0, m_goodTime);
        //
        //HJFLogi("m_goodTime:{}, gapTime:{}, m_adjustInterval:{}, queueDuration:{}, ", m_goodTime, gapTime, m_adjustInterval, queueDuration, sentBitrate, m_bitrate);
        if (m_goodTime > m_adjustInterval && (sentBitrate < m_bitrate))
        {
            HJFLogi("entry, m_bitrate:{}, inBitrate:{}, sentBitrate:{}, queueDuration:{}, dropCount:{}, dropRatio:{}, m_recommendedBitrate:{}", m_bitrate, inBitrate, sentBitrate, queueDuration, dropCount, dropRatio, m_recommendedBitrate);
            float step = m_k2 * (m_bitrate - sentBitrate);
            step = HJ_MAX(50.0f, step);
            m_recommendedBitrate += step;
            m_recommendedBitrate = HJ_CLIP(m_recommendedBitrate, m_minBitrate, m_bitrate);
            //
            m_goodTime = 0;
            HJFLogw("m_recommendedBitrate:{}, step:{}", m_recommendedBitrate, step);
            return m_recommendedBitrate;
        }
    }
    //
    m_lastTime = HJCurrentSteadyMS();

    return 0.0f;
}

void HJRTMPBitrateAdapter::reset() {
    m_lastTime = HJ_NOTS_VALUE;
    m_goodTime = 0;
    m_recommendedBitrate = m_bitrate;
}

NS_HJ_END