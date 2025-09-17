#pragma once
#include <deque>
#include <utility>
#include <map>
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJCacheObserver :  public HJObject
{
public:
    struct Stats {
        int64_t duration{0};
        int64_t average{0};    // 平均缓存大小
        int64_t maximum{0};    // 最大缓存大小
        int64_t minimum{0};    // 最小缓存大小
        double stddev{0.0};
    };
    using Ptr = std::shared_ptr<HJCacheObserver>;
    using Wtr = std::weak_ptr<HJCacheObserver>;
    HJCacheObserver() = default;
    explicit HJCacheObserver(int64_t windowMs);
    ~HJCacheObserver() = default;

    HJCacheObserver::Stats addCacheDuration(int64_t duration);
    HJCacheObserver::Stats getStats() {
        HJ_AUTOU_LOCK(m_mutex)
        return m_stats;
    }
    void reset();
    //
    void setWatchStartTime(int64_t time = HJ_NOTS_VALUE) {
        m_watchStartTime = (HJ_NOTS_VALUE != time) ? time : HJCurrentSteadyMS();
    }
    void setStutter(bool start = true);

    int64_t getWatchTime(bool end = false);
    double getWinStutterRatio();
    double getStutterRatio() {
        return m_stutterDuration / (double)getWatchTime();
    }
    int64_t get100StutterRatio() {
        return (int64_t)(100 * getStutterRatio() + 0.5);
    }
    int64_t getStutterDuration() {
        return m_stutterDuration;
    }
    void  setStutterWindowMax(int64_t time) {
        m_stutterWindowMax = time;
    }
    void  setStutterWindowMin(int64_t time) {
        m_stutterWindowMin = time;
    }
private:
    void cleanExpiredData(int64_t currentTimeMs);
    double calculateStdDev(int64_t mean) const;
    int64_t getInnerWatchTime(bool end = false);
private:
    std::recursive_mutex                    m_mutex;
    std::deque<std::pair<int64_t, int64_t>> m_cacheDatas;
    int64_t                                 m_windowMs = 15*1000;
    //
    int64_t m_watchStartTime = 0;
    int64_t m_watchEndTime = HJ_NOPTS_VALUE;
    int64_t m_stutterDuration = 0;
    int64_t m_stutterStart = HJ_NOPTS_VALUE;
    int64_t m_stutterCnt = 0;

	std::vector<std::pair<int64_t, int64_t>> m_stutterDatas;    // {start, end}
    int64_t m_stutterWindowMin = 150 * 1000;
	int64_t m_stutterWindowMax = 600 * 1000;                 //  10min, 3s / 0.005 = 600s

    HJCacheObserver::Stats m_stats;
};


NS_HJ_END
