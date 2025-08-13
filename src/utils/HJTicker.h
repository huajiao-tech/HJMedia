//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJTicker {
public:
    HJTicker(uint64_t min_ms = 0,
              HJLogStream::Ptr ctx = std::make_shared<HJLogStream>(HJLOG_WARN, __FILENAME__, __LINE__, ""),
              bool print_log = false) : m_ctx(std::move(ctx)) {
        if (!print_log) {
            m_ctx->clear();
        }
        m_created = m_begin = HJTime::getCurrentMillisecond();
        m_minMS = min_ms;
    }

    ~HJTicker() {
        uint64_t tm = createdTime();
        if (tm > m_minMS) {
            *m_ctx.get() << "take time:" << tm << "ms" << ", thread may be overloaded";
        } else {
            m_ctx->clear();
        }
    }

    /**
     * 获取创建时间，单位毫秒
     */
    uint64_t elapsedTime() const {
        return HJTime::getCurrentMillisecond() - m_begin;
    }

    /**
     * 获取上次resetTime后至今的时间，单位毫秒
     */
    uint64_t createdTime() const {
        return HJTime::getCurrentMillisecond() - m_created;
    }

    /**
     * 重置计时器
     */
    void resetTime() {
        m_begin = HJTime::getCurrentMillisecond();
    }

private:
    uint64_t        m_minMS;
    uint64_t        m_begin;
    uint64_t        m_created;
    HJLogStream::Ptr    m_ctx;
};

class HJSmoothTicker {
public:
    /**
     * 此对象用于生成平滑的时间戳
     * @param reset_ms 时间戳重置间隔，没间隔reset_ms毫秒, 生成的时间戳会同步一次系统时间戳
     */
    HJSmoothTicker(uint64_t reset_ms = 10000) {
        _reset_ms = reset_ms;
        _ticker.resetTime();
    }

    ~HJSmoothTicker() {}
    /**
     * 返回平滑的时间戳，防止由于网络抖动导致时间戳不平滑
     */
    uint64_t elapsedTime() {
        auto now_time = _ticker.elapsedTime();
        if (_first_time == 0) {
            if (now_time < _last_time) {
                auto last_time = _last_time - _time_inc;
                double elapse_time = (now_time - last_time);
                _time_inc += (elapse_time / ++_pkt_count) / 3;
                auto ret_time = last_time + _time_inc;
                _last_time = (uint64_t) ret_time;
                return (uint64_t) ret_time;
            }
            _first_time = now_time;
            _last_time = now_time;
            _pkt_count = 0;
            _time_inc = 0;
            return now_time;
        }

        auto elapse_time = (now_time - _first_time);
        _time_inc += elapse_time / ++_pkt_count;
        auto ret_time = _first_time + _time_inc;
        if (elapse_time > _reset_ms) {
            _first_time = 0;
        }
        _last_time = (uint64_t) ret_time;
        return (uint64_t) ret_time;
    }

    /**
     * 时间戳重置为0开始
     */
    void resetTime() {
        _first_time = 0;
        _pkt_count = 0;
        _ticker.resetTime();
    }

private:
    double      _time_inc = 0;
    uint64_t    _first_time = 0;
    uint64_t    _last_time = 0;
    uint64_t    _pkt_count = 0;
    uint64_t    _reset_ms;
    HJTicker   _ticker;
};

#if !defined(NDEBUG)
#define HJTimeTicker()         HJTicker __ticker(5, HJLOG_WARN, true)
#define HJTimeTicker1(tm)      HJTicker __ticker1(tm, HJLOG_WARN, true)
#define HJTimeTicker2(tm, log) HJTicker __ticker2(tm, log, true)
#else
#define HJTimeTicker()
#define HJTimeTicker1(tm)
#define HJTimeTicker2(tm,log)
#endif

//***********************************************************************************//
class HJTimeCounter
{
public:
    using Ptr = std::shared_ptr<HJTimeCounter>;
    using TimesQueue = std::deque<uint64_t>;
    HJTimeCounter(const size_t period = 100)
        : m_period(period) {

    }
    void begin() {
        m_inTime = HJTime::getCurrentMicrosecond();
        addInTime(m_inTime);
    }
    void end() {
        addTime(HJTime::getCurrentMicrosecond() - m_inTime);
    }
    void reset()
    {
		m_delayCnt = 2;
		m_inTime = 0;
		m_preTime = HJ_UINT64_MAX;
		m_preInTime = HJ_UINT64_MAX;
		m_totalTime = 0;
		m_totalGapTime = 0;
		m_times.clear();
		m_gapTimes.clear();
    }
    uint64_t getAvgTime()
    {
		if (m_times.size() <= 0) {
			return 0;
		}
		size_t cnt = HJ_MIN(m_period, m_times.size());
		return m_totalTime / cnt;
    }
    uint64_t getAvgGapTime()
    {
		if (m_gapTimes.size() <= 0) {
			return 0;
		}
		size_t cnt = HJ_MIN(m_period, m_gapTimes.size());
		return m_totalGapTime / cnt;
    }
    uint64_t getTime() {
		if (m_times.size() > 0) {
			return m_times.back();
		}
		return 0;
    }
    uint64_t getGapTime() {
		if (m_gapTimes.size() > 0) {
			return m_gapTimes.back();
		}
		return 0;
    }
private:
    void addTime(uint64_t time) 
    {
		if (m_preTime >= HJ_UINT64_MAX) {
            m_preTime = time;
		} else
		{
			if (m_times.size() >= m_period) {
                m_totalTime -= m_times[0];
                m_times.erase(m_times.begin());
			}
			if (time > 0)
			{
                m_totalTime += time;
                m_times.emplace_back(time);
			}
		}
    }
    void addInTime(uint64_t time)
    {
		if (m_preInTime >= HJ_UINT64_MAX || m_delayCnt > 0) {
            m_preInTime = time;
		} else
		{
			uint64_t gapTime = time - m_preInTime;
			if (m_gapTimes.size() >= m_period) {
                m_totalGapTime -= m_gapTimes[0];
                m_gapTimes.erase(m_gapTimes.begin());
			}
			if (gapTime > 0)
			{
                m_preInTime = time;
                m_totalGapTime += gapTime;
                m_gapTimes.emplace_back(gapTime);
			}
		}
        m_delayCnt--;
    }
private:
    size_t      m_period = 100;
    size_t      m_delayCnt = 2;
    uint64_t    m_inTime = 0;
    uint64_t    m_preTime   = HJ_UINT64_MAX;
    uint64_t    m_preInTime = HJ_UINT64_MAX;
    uint64_t    m_totalTime = 0;
    uint64_t    m_totalGapTime = 0;
    TimesQueue  m_times;
    TimesQueue  m_gapTimes;
};

#define HJ_NIP_INTERVAL_DEFAULT    2       //s
class HJNipInterval
{
public:
    using Ptr = std::shared_ptr<HJNipInterval>;
    HJNipInterval(int64_t interval = HJ_SEC_TO_MS(HJ_NIP_INTERVAL_DEFAULT)) {       //ms
        m_interval = interval;
    }
    bool valid() {
        int64_t cur = HJTime::getCurrentMillisecond();
        if (HJ_NOPTS_VALUE == m_time || (cur - m_time) >= m_interval) {
            m_time = cur;
            return true;
        }
        return false;
    }
    bool valid(int64_t val) {
        if (m_preVal == HJ_NOPTS_VALUE) {
            m_preVal = val;
        }
        int64_t delta = val - m_preVal;
        if (delta > m_interval) {
            size_t n = (size_t)(delta / m_interval);
            m_preVal += n * m_interval;
            return true;
        }
        return false;
    }
    void setInterval(int64_t interval) {
        m_interval = interval;
    }
private:
    int64_t    m_interval = HJ_MS_PER_SEC;
    int64_t    m_time = HJ_NOPTS_VALUE;
    int64_t    m_preVal = HJ_NOPTS_VALUE;
};

class HJNipMuster
{
public:
    using Ptr = std::shared_ptr<HJNipMuster>;
    HJNipMuster(uint64_t interval = HJ_SEC_TO_MS(HJ_NIP_INTERVAL_DEFAULT)) {
        addNip("in", interval);
        addNip("out", interval);
    }
    HJNipMuster(std::vector<int> idxs, uint64_t interval = HJ_SEC_TO_MS(HJ_NIP_INTERVAL_DEFAULT)) {
        for (auto idx : idxs) {
            addNipById(idx, interval);
        }
    }
    HJNipMuster(std::vector<std::string> names, uint64_t interval = HJ_SEC_TO_MS(HJ_NIP_INTERVAL_DEFAULT)) {
        for (auto name : names) {
            addNip(name, interval);
        }
    }

    HJNipInterval::Ptr addNipById(const int idx, uint64_t interval = HJ_SEC_TO_MS(HJ_NIP_INTERVAL_DEFAULT)) {
        std::string name = HJ2STR(idx);
        return addNip(name, interval);
    }
    HJNipInterval::Ptr addNip(const std::string& name, uint64_t interval = HJ_SEC_TO_MS(HJ_NIP_INTERVAL_DEFAULT)) {
        HJNipInterval::Ptr nip = std::make_shared<HJNipInterval>(interval);
        m_nips.emplace(name, nip);
        return nip;
    }

    const HJNipInterval::Ptr getNipById(const int idx) {
        std::string name = HJ2STR(idx);
        return getNipByName(name);
    }
    const HJNipInterval::Ptr getNipByName(const std::string& name) {
        auto it = m_nips.find(name);
        if (it != m_nips.end()) {
            return it->second;
        }
        return nullptr;
    }
    const HJNipInterval::Ptr getInNip() {
        return getNipByName("in");
    }
    const HJNipInterval::Ptr getOutNip() {
        return getNipByName("out");
    }
private:
    std::unordered_map<std::string, HJNipInterval::Ptr> m_nips;
};

//***********************************************************************************//
template <typename T>
class HJIntervalDeterminer
{
public:
    using Ptr = std::shared_ptr<HJIntervalDeterminer<T>>;
    HJIntervalDeterminer(T interval = (T)HJ_MS_PER_SEC) {
        m_interval = interval;
    }
    bool valid(T val) {
        if (m_preVal == (T)HJ_NOPTS_VALUE) {
            m_preVal = val;
        }
        m_val = val;
        T delta = m_val - m_preVal;
        if (delta > m_interval) {
            size_t n = (size_t)(delta / m_interval);
            m_preVal += n * m_interval;
            return true;
        }
        return false;
    }
    void setInterval(T interval) {
        m_interval = interval;
    }
private:
    T   m_interval = (T)HJ_MS_PER_SEC;
    T   m_preVal = (T)HJ_NOPTS_VALUE;
    T   m_val = (T)0;
};
using HJIntervalDeterminerUI64 = HJIntervalDeterminer<uint64_t>;

//***********************************************************************************//
template <typename T> 
class HJDataStatistic
{
public:
    using Ptr = std::shared_ptr<HJDataStatistic<T>>;
    HJDataStatistic(int swinLen = 3) {
        m_swinLen = swinLen;
    }
    T add(T data) {
        if (m_datas.size() >= m_swinLen) {
            m_total -= m_datas.front();
            m_datas.pop_front();
        }
        m_datas.push_back(data);
        m_total += data;
        //
        return getAvg();
    }
    T getAvg() {
        if(m_datas.size() <= 0) {
            return (T)0;
        }
        return (T)(m_total / (double)m_datas.size());
//        T total = 0;
//        for (T a : m_datas) {
//            total += a;
//        }
//        return total / (T)m_datas.size();
    }
    void reset() {
        m_datas.clear();
        m_total = 0;
    }
    bool isFull() {
        return (m_datas.size() >= m_swinLen);
    }
private:
    int         m_swinLen = 3;
    HJList<T>  m_datas;
    double      m_total = 0;
};
using HJDataStati64 = HJDataStatistic<int64_t>;

template <typename T>
class HJDataStatMuster
{
public:
    using Ptr = std::shared_ptr<HJDataStatMuster<T>>;
    HJDataStatMuster(int swinLen = 3) {
        addStat("in", swinLen);
        addStat("out", swinLen);
    }
    HJNipInterval::Ptr addStat(const std::string& name, uint64_t swinLen = 3) {
        auto stat = std::make_shared<HJDataStatistic<T>>(swinLen);
        m_stats.emplace(name, stat);
        return stat;
    }
    const HJNipInterval::Ptr getStatByName(const std::string& name) {
        auto it = m_stats.find(name);
        if (it != m_stats.end()) {
            return it->second;
        }
        return nullptr;
    }
    const HJNipInterval::Ptr getIn() {
        return getStatByName("in");
    }
    const HJNipInterval::Ptr getOut() {
        return getStatByName("out");
    }
private:
    std::unordered_map<std::string, std::shared_ptr<HJDataStatistic<T>>> m_stats;
};
using HJDataStatMusteri64 = HJDataStatMuster<int64_t>;

//***********************************************************************************//
class HJTrajectory : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJTrajectory>;
    HJTrajectory(const char* file, int line, const char* function, const std::string& info);
    virtual ~HJTrajectory();

    size_t getKey();
    std::string& getTrace() {
        return m_trace;
    }
public:
    int64_t         m_time = 0;
    std::string     m_file = "";
    std::string     m_func = "";
    int             m_line = 0;
    std::string     m_info = "";
    //
    std::string     m_trace = "";
    size_t          m_key = 0;
};
#define HJakeTrajectory() std::make_shared<::HJ::HJTrajectory>(__FILENAME__, __LINE__, __FUNCTION__, "")
#define HJakeTrajectoryInfo(info) std::make_shared<::HJ::HJTrajectory>(__FILENAME__, __LINE__, __FUNCTION__, info)

class HJTracker : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJTracker>;
    HJTracker() {};
    virtual ~HJTracker() {};

    void addTrajectory(const HJTrajectory::Ptr& trajectory) {
        if (!trajectory) {
            return;
        }
        m_trajectorys.push_back(trajectory);
    }

    virtual void clone(const HJTracker::Ptr& other) {
        m_trajectorys = HJList<HJTrajectory::Ptr>(other->m_trajectorys.begin(), other->m_trajectorys.end());
    }
    virtual HJTracker::Ptr dup() {
        HJTracker::Ptr tracker = std::make_shared<HJTracker>();
        tracker->clone(sharedFrom(this));
        return tracker;
    }
    std::string formatInfo();
private:
    HJList<HJTrajectory::Ptr>     m_trajectorys;
};
using HJTrackerMap = std::unordered_map<std::string, HJTracker::Ptr>;
//***********************************************************************************//
NS_HJ_END

