#pragma once
#include <cstdint>
#include <deque>
#include <mutex>
#include <any>
#include "HJUtilitys.h"

#define JP_HAVE_AUTO_PARAMS 1

NS_HJ_BEGIN
//using HJParams = std::unordered_map<std::string, std::any>;
//***********************************************************************************//
class HJAutoCacheController : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAutoCacheController>;
    HJAutoCacheController(
        int64_t cacheTargetDuration = 3000,  // 缓冲池目标大小(ms)
        int64_t cacheLimitedRange = 2000,    // 允许波动范围(ms)
        double playbackSpeed = 1.0f,   // 初始播放速度
        double stutterRatioMax = 0.03f,// 最大卡顿率(3%)
		double stutterRatioDelta = 0.005f, // 卡顿率变化阈值(0.5%)
		double stutterFactorMax = 3.0f, // 卡顿率最大放大系数
        int64_t observeTime = 15000   // 观察期时间(ms)
    );

    // 更新控制参数并返回建议的播放速度
    double update(
        int64_t curDuration,      // 当前缓冲池大小(ms)
        double stutterRatio,       // 当前卡顿率
        int64_t durationMax,      // 观察期内最大缓冲(ms)
        int64_t durationMin,      // 观察期内最小缓冲(ms)
        int64_t durationAvg,      // 观察期内平均缓冲(ms)
        int64_t currentTime       // 当前时间戳(ms)
    );
    void setOnOptionsCallback(std::function<void(std::shared_ptr<HJParams> params)> callback) {
        m_onOptionsCallback = callback;
    }
    
    // 获取当前目标缓冲大小和波动范围
    int64_t getCurrentTargetDuration() const;
    int64_t getCurrentLimitedRange() const;
private:
    int64_t getCurUpperLimit() const {
        return m_currentTargetDuration + m_currentLimitedRange;
    }
    int64_t getCurLowerLimit() const {
        return m_currentTargetDuration - 0.5 * m_currentLimitedRange;
    }
    int64_t getCurUltraLimit() const {
        return 0.35 * m_currentTargetDuration;
    }
    int64_t getCacheTargetDurationMin() const {
        return 0.5 * m_cacheTargetDuration;
    }
    int64_t getCacheTargetDurationMax() const {
        return 2.0 * m_cacheTargetDuration;
    }
    int64_t getCacheLimitedRangeMin() const {
        return 0.5 * m_cacheLimitedRange;
    }
private:
    // 控制状态
    enum class ControlState {
        OBSERVATION,  // 观察期
        CONTROL       // 控制期
    };

    // 参数历史记录，用于检测趋势
    struct BufferHistory {
        int64_t duration;
        int64_t timestamp;
    };

    // 基础参数
    const int64_t   m_cacheTargetDuration;
    const int64_t   m_cacheLimitedRange;
    const double    m_stutterRatioMax;
    const double    m_stutterRatioDelta;
    const double    m_stutterFactorMax;
    const int64_t   m_observeTime;

    // 当前参数
    ControlState    m_state;
    int64_t         m_currentTargetDuration;
    int64_t         m_currentLimitedRange;
    double          m_playbackSpeed;
    int64_t         m_startTime = HJ_NOPTS_VALUE;
    bool            m_stutterDetected;
    double          m_preStutterRatio{0.0f};

    // 历史记录和趋势检测
    std::deque<BufferHistory> m_history;
//    mutable std::mutex m_historyMutex;
    int64_t         m_lastUpdateTime;
    int64_t         m_directionChangeTime;
    double          m_lastPlaybackSpeed;
    double          m_slope;
    double          m_slopeMagnitude = 0.5f;
    std::string     m_calcName = "normal";
    
    //
    int64_t         m_controlPeriod = 5000;
    int64_t         m_controlPreTime = HJ_NOPTS_VALUE;
    int64_t		    m_updateTime = 0;
    
    //
    std::shared_ptr<HJParams> m_params = nullptr;
    std::string m_updateParamsInfo = "";
    std::function<void(std::shared_ptr<HJParams>)> m_onOptionsCallback = nullptr;

    // 辅助方法
    void updateControlParams(
        int64_t curDuration,
        double stutterRatio,
        int64_t durationMax,
        int64_t durationMin,
        int64_t durationAvg,
        int64_t currentTime
    );

    // 播放速度计算函数类型
    std::function<double(int64_t curDuration, double stutterRatio)> calculatePlaybackSpeed;
    double normalSpeedCalculation(int64_t curDuration, double stutterRatio);
    double acceleratedSpeedCalculation(int64_t curDuration, double stutterRatio);
    double deceleratedSpeedCalculation(int64_t curDuration, double stutterRatio);
    double ultraSpeedCalculation(int64_t curDuration, double stutterRatio);
    
    void addHistorySample(int64_t curDuration, int64_t currentTime);
    bool isBufferTrendingUp(int64_t currentTime) const;
    bool isBufferTrendingDown(int64_t currentTime) const;
    double calculateSlopeNormalized(std::deque<BufferHistory>& values);
    double calculateSlope(std::deque<BufferHistory>& values);
    //
    std::string getStateString();
};
NS_HJ_END
