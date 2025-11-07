#include "HJFLog.h"
#include "HJCacheObserver.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJCacheObserver::HJCacheObserver(int64_t windowMs)
    : m_windowMs(windowMs)
{

}

HJCacheObserver::Stats HJCacheObserver::addCacheDuration(int64_t duration)
{
    HJ_AUTOU_LOCK(m_mutex)
    int64_t now = HJCurrentSteadyMS();
    cleanExpiredData(now);
    //
    m_cacheDatas.push_back(std::make_pair(now, duration));
    if (m_cacheDatas.size() < 5) {
        return Stats{0, 0, 0, 0, 0.0};
    }
    int64_t sum = 0;
    int64_t max = m_cacheDatas[0].second;
    int64_t min = m_cacheDatas[0].second;

    for (const auto& data : m_cacheDatas) {
        sum += data.second;
        max = std::max(max, data.second);
        min = std::min(min, data.second);
    }
    int64_t avg = sum / static_cast<int64_t>(m_cacheDatas.size());
    double stddev = 0.0; //calculateStdDev(avg);
    m_stats = { 
        duration,
        sum / static_cast<int64_t>(m_cacheDatas.size()),
        max,
        min,
        stddev
    };
    HJFPERNLogi("speed control, current duration:{}, max:{}, min:{}", duration, max, min);
    return m_stats;
    //return {
    //    sum / static_cast<int64_t>(m_cacheDatas.size()),
    //    max,
    //    min,
    //    stddev
    //};
}

void HJCacheObserver::cleanExpiredData(int64_t currentTimeMs)
{
    while (!m_cacheDatas.empty()) {
        if (currentTimeMs - m_cacheDatas.front().first > m_windowMs) {
            m_cacheDatas.pop_front();
        } else {
            break;
        }
    }
    return;
}

double HJCacheObserver::calculateStdDev(int64_t mean) const {
    if (m_cacheDatas.empty()) return 0.0;
    
    double sumSquares = 0.0;
    for (const auto& data : m_cacheDatas) {
        double diff = data.second - mean;
        sumSquares += diff * diff;
    }
    return std::sqrt(sumSquares / m_cacheDatas.size());
}

void HJCacheObserver::reset()
{
    HJ_AUTOU_LOCK(m_mutex)
    m_cacheDatas.clear();
    m_watchStartTime = 0;
    m_watchEndTime = HJ_NOPTS_VALUE;
    m_stutterDuration = 0;
    m_stutterStart = HJ_NOPTS_VALUE;
}

int64_t HJCacheObserver::getWatchTime(bool end)
{
    HJ_AUTOU_LOCK(m_mutex)
    return getInnerWatchTime(end);
}

int64_t HJCacheObserver::getInnerWatchTime(bool end)
{
    int64_t watchTime = 0;
    if (end)
    {
        if (HJ_NOPTS_VALUE == m_watchEndTime) {
            m_watchEndTime = HJCurrentSteadyMS();
        }
        watchTime = m_watchEndTime - m_watchStartTime;
    }
    else {
        watchTime = HJCurrentSteadyMS() - m_watchStartTime;
    }
    //    HJFLogi("end, watchTime:{}", watchTime);

    return watchTime;
}

void HJCacheObserver::setStutter(bool start)
{
    HJ_AUTOU_LOCK(m_mutex)
    if(start) {
        m_stutterStart = getInnerWatchTime(); //HJCurrentSteadyMS();
        HJFLogi("stutter start:{}", m_stutterStart);
    } else {
        if(HJ_NOPTS_VALUE != m_stutterStart)
        {
            auto stutterEnd = getInnerWatchTime(); //HJCurrentSteadyMS();
            if(m_stutterCnt > 0 && stutterEnd > 3000) 
            {
                m_stutterDuration += stutterEnd - m_stutterStart;
                //
                m_stutterDatas.push_back(std::make_pair(m_stutterStart, stutterEnd));
				HJFLogi("stutter end, m_stutterDuration:{}, m_stutterCnt:{}, m_stutterStart:{}, stutterEnd:{}", m_stutterDuration, m_stutterCnt, m_stutterStart, stutterEnd);
            } else {
                HJFLogw("warning, stutter end, stutter duration:{}", stutterEnd - m_stutterStart);
            }
            m_stutterStart = HJ_NOPTS_VALUE;
        }
        m_stutterCnt++;
    }
//    HJFLogi("end, m_stutterDuration:{}, m_stutterCnt:{}, m_stutterStart:{}, start:{}", m_stutterDuration, m_stutterCnt, m_stutterStart, start);
}

double HJCacheObserver::getWinStutterRatio()
{
	HJ_AUTOU_LOCK(m_mutex)
	if (m_stutterDatas.empty()) {
		return 0.0;
	}
	int64_t stutterDuration = 0;
	int64_t watchTime = getInnerWatchTime();
    int64_t watchTimeMin = HJ_MAX(0, watchTime - m_stutterWindowMax);
    std::string fmtInfo = "";
	for (const auto& stutter : m_stutterDatas) {
        if (stutter.second >= watchTimeMin) {
			int64_t segMin = std::max(stutter.first, watchTimeMin);
            stutterDuration += stutter.second - segMin;
        }
        fmtInfo += HJFMT(" [{}, {}] ", stutter.first, stutter.second);
	}

	if (watchTime <= 0) {
		HJFLoge("watchTime is zero, stutterDuration:{}, stutterDatas:{}", stutterDuration, fmtInfo);
		return 0.0;
	}
	int64_t windowLen = HJ_CLIP(watchTime, m_stutterWindowMin, m_stutterWindowMax);
	double stutterRatio = static_cast<double>(stutterDuration) / static_cast<double>(windowLen);

	//HJFLogi("stutterRatio:{:.3f}, stutterDuration:{}, watchTimeMin:{}, watchTime:{}, stutterDatas:{}, total ratio:{:.3f}", stutterRatio, stutterDuration, watchTimeMin, watchTime, fmtInfo, getStutterRatio());
    
	return stutterRatio;
}

NS_HJ_END
