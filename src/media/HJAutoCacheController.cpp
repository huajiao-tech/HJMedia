#include <algorithm>
#include <cmath>
#include "HJFLog.h"
#include "HJAutoCacheController.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJAutoCacheController::HJAutoCacheController(
    int64_t cacheTargetDuration,
    int64_t cacheLimitedRange,
    double playbackSpeed,
    double stutterRatioMax,
    double stutterRatioDelta,
    double stutterFactorMax,
    int64_t observeTime
) : m_cacheTargetDuration(cacheTargetDuration),
    m_cacheLimitedRange(cacheLimitedRange),
    m_stutterRatioMax(stutterRatioMax),
    m_stutterRatioDelta(stutterRatioDelta),
    m_stutterFactorMax(stutterFactorMax),
    m_observeTime(observeTime),
    m_state(ControlState::OBSERVATION),
    m_currentTargetDuration(cacheTargetDuration),
    m_currentLimitedRange(cacheLimitedRange),
    m_playbackSpeed(playbackSpeed),
    m_startTime(HJ_NOPTS_VALUE),
    m_stutterDetected(false),
    m_lastUpdateTime(0),
    m_directionChangeTime(0),
    m_slope(0.0f),
    m_lastPlaybackSpeed(playbackSpeed)
{
    calculatePlaybackSpeed = std::bind(&HJAutoCacheController::normalSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
    HJFNLogi("entry, cacheTargetDuration:{}, cacheLimitedRange:{}, playbackSpeed:{}, stutterRatioMax:{}, stutterRatioDelta:{}, observeTime:{}", cacheTargetDuration, cacheLimitedRange, playbackSpeed, stutterRatioMax, stutterRatioDelta, observeTime);
}

double HJAutoCacheController::update(
    int64_t curDuration,
    double stutterRatio,
    int64_t durationMax,
    int64_t durationMin,
    int64_t durationAvg,
    int64_t currentTime
) {
    if (m_onOptionsCallback) {
        m_params = std::make_shared<HJParams>();
    }
    m_updateParamsInfo = "";
    if (m_startTime == HJ_NOPTS_VALUE) {
        m_startTime = currentTime;
    }

    // 添加当前缓冲状态到历史记录
    addHistorySample(curDuration, currentTime);

    // 更新控制参数（目标缓冲和波动范围）
    updateControlParams(curDuration, stutterRatio, durationMax, durationMin, durationAvg, currentTime);

    // 计算并应用新的播放速度
    m_playbackSpeed = (nullptr != calculatePlaybackSpeed) ? calculatePlaybackSpeed(curDuration, stutterRatio) : m_playbackSpeed;
    if (m_playbackSpeed != m_lastPlaybackSpeed) {
        HJFNLogi("speed control, duration rangs:[{}, {}, {}], curDuration:{}, stutterRatio:{:.4f}, speed:{:.3f}", getCurUpperLimit(), m_currentTargetDuration, getCurLowerLimit(), curDuration, stutterRatio, m_playbackSpeed);
    }
    m_lastPlaybackSpeed = m_playbackSpeed;
    m_lastUpdateTime = currentTime;
    
    //JFLogi("end, state:{}, calc:{}, duration:{}, stutterRatio:{}, delta:{}, Avg:{}, speed:{}, m_slope:{:.3f}, time:{}", getStateString(), m_calcName, curDuration, stutterRatio, durationMax - durationMin, durationAvg, m_playbackSpeed, m_slope, m_lastUpdateTime - m_startTime);
    
#if defined(JP_HAVE_AUTO_PARAMS)
    if(m_params) 
    {
        (*m_params)["stat"] = getStateString();
        (*m_params)["calc"] = m_calcName;
        (*m_params)["dura"] = curDuration;
        (*m_params)["max"] = durationMax;
        (*m_params)["min"] = durationMin;
        (*m_params)["avg"] = durationAvg;
        (*m_params)["stut"] = stutterRatio;
        (*m_params)["slop"] = m_slope;
        (*m_params)["speed"] = m_lastPlaybackSpeed;
        (*m_params)["watch"] = m_lastUpdateTime;
        //(*m_params)["edura"] = expectTargetDuration;
        //(*m_params)["elimit"] = expectLimitedRange;
        //(*m_params)["rdura"] = m_currentTargetDuration;
        //(*m_params)["rlimit"] = m_currentLimitedRange;
        if (m_onOptionsCallback) {
            m_onOptionsCallback(m_params);
        }
    }
#endif

    auto time  = HJCurrentSteadyMS();
    if (time - m_updateTime >= 2000) {
        std::string fmtInfo = HJFMT("[stat:{}, calc:{}] - [stutter ratio:{:.4f}, slope:{:.3f}], [speed:{:.3f}, watch:{}] - [duration:{}, max-min:{}, avg:{}]", getStateString(), m_calcName, stutterRatio, m_slope, m_lastPlaybackSpeed, m_lastUpdateTime, curDuration, durationMax - durationMin, durationAvg);
        //HJFLogi(fmtInfo + m_updateParamsInfo);
        m_updateTime = time;
    }

    return m_playbackSpeed;
}

void HJAutoCacheController::updateControlParams(
    int64_t curDuration,
    double stutterRatio,
    int64_t durationMax,
    int64_t durationMin,
    int64_t durationAvg,
    int64_t currentTime
) {
    // 检查是否需要从观察期切换到控制期
    if (m_state == ControlState::OBSERVATION)
    {
        // 观察期：检查是否有卡顿发生
        if (stutterRatio > m_stutterRatioMax) {
            m_stutterDetected = true;
        }

        // 如果观察期结束或检测到卡顿，则切换到控制期
        if (currentTime - m_startTime >= m_observeTime || m_stutterDetected) {
            m_state = ControlState::CONTROL;
        }
    }

    // 控制期：动态调整目标缓冲和波动范围
    int64_t durationDelta = durationMax - durationMin;
    std::string fmtInfo = HJFMT("current: [duration:{}, max-min:{}, avg:{}] - [stutter ratio:{:.4f}, slope:{:.3f}]", curDuration, durationDelta, durationAvg,  stutterRatio, m_slope);
    //
    if (m_state == ControlState::CONTROL)
    {
        double stutterDelta = stutterRatio - m_preStutterRatio;
        auto time = HJCurrentSteadyMS();
        if(HJ_NOPTS_VALUE == m_controlPreTime ||
           (time - m_controlPreTime > m_controlPeriod) ||
           std::fabs(stutterDelta) > m_stutterRatioDelta)
        {
            // 计算实际波动范围
            int64_t targetDurationMin = getCacheTargetDurationMin();
            int64_t targetDurationMax = getCacheTargetDurationMax();
            int64_t limitedRangeMin = getCacheLimitedRangeMin();
            
            // 计算预期目标缓冲
            double stutterFactor = std::fmin(stutterRatio / m_stutterRatioMax, m_stutterFactorMax);
            //to do durationDelta <-> stutterFactor
            
            int64_t expectTargetDuration = static_cast<int64_t>(targetDurationMin + targetDurationMin * stutterFactor);
            
            // 限制预期目标缓冲范围
            expectTargetDuration = HJ_CLIP(expectTargetDuration, targetDurationMin, targetDurationMax);
            
            // 计算当前目标缓冲
            //int64_t newTargetDuration = static_cast<int64_t>(0.5 * (durationAvg + m_cacheLimitedRange));
            //        newTargetDuration = std::max(
            //            static_cast<int64_t>(targetDurationMin),
            //            newTargetDuration);
            
            // 平滑更新目标缓冲
            if (expectTargetDuration > m_currentTargetDuration) {
                m_currentTargetDuration = static_cast<int64_t>(0.3 * m_currentTargetDuration + 0.7 * expectTargetDuration);
            } else  {
                m_currentTargetDuration = static_cast<int64_t>(0.7 * m_currentTargetDuration + 0.3 * expectTargetDuration);
            }
            // 计算并平滑更新波动范围
//            int64_t newLimitedRange = static_cast<int64_t>(0.5 * (durationDelta + m_cacheLimitedRange));
            double rangeFactor = (m_currentTargetDuration - targetDurationMin) /(double)m_cacheTargetDuration;
            int64_t expectLimitedRange = limitedRangeMin + rangeFactor * limitedRangeMin;
            expectLimitedRange = HJ_CLIP(expectLimitedRange, 0.5 * m_cacheLimitedRange, m_cacheLimitedRange);
            m_currentLimitedRange = static_cast<int64_t>(0.7 * m_currentLimitedRange + 0.3 * expectLimitedRange);
            
            //
            m_controlPreTime = time;
            m_preStutterRatio = stutterRatio;
            
#if defined(JP_HAVE_AUTO_PARAMS)
            if(m_params) {
                (*m_params)["edura"] = expectTargetDuration;
                (*m_params)["elimit"] = expectLimitedRange;
            }
#endif
            m_updateParamsInfo += HJFMT(" - [expect:{}, {}, stutter delta:{:.4f}]", expectTargetDuration, expectLimitedRange, stutterDelta);
            HJFNLogi("control out duration:[{}, {}, {}], expect:[{}, {}], stutterDelta:{:.4f} -- {}", getCurUpperLimit(), m_currentTargetDuration, getCurLowerLimit(), expectTargetDuration, expectLimitedRange, stutterDelta, fmtInfo);
        } 
        //else {
        //    JFLogi("control out duration:[{}, {}], stutterDelta:{:.4f} -- {}", m_currentTargetDuration, m_currentLimitedRange, stutterDelta, fmtInfo);
        //}
    } 
    //else {
    //    JFLogi("observe out duration:[{}, {}] -- {}", m_currentTargetDuration, m_currentLimitedRange, fmtInfo);
    //}
#if defined(JP_HAVE_AUTO_PARAMS)
    if(m_params) {
        (*m_params)["rdura"] = getCurUpperLimit();
        (*m_params)["rlimit"] = getCurLowerLimit();
    }
#endif
    m_updateParamsInfo += HJFMT(" - [limits:{}, {}, {}]", getCurUpperLimit(), m_currentTargetDuration, getCurLowerLimit());
    
    return;
}

double HJAutoCacheController::normalSpeedCalculation(int64_t curDuration, double stutterRatio)
{
//    JFLogi("entry, normal Calculation speed");
    m_calcName = "normal";
    double speed = 1.0f;
    if (curDuration > getCurUpperLimit()) {
        calculatePlaybackSpeed = std::bind(&HJAutoCacheController::acceleratedSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
        speed = calculatePlaybackSpeed(curDuration, stutterRatio);
    } else if ((curDuration < getCurUltraLimit()) && (m_state == ControlState::OBSERVATION)) {
        calculatePlaybackSpeed = std::bind(&HJAutoCacheController::ultraSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
        speed = calculatePlaybackSpeed(curDuration, stutterRatio);
    } else if ((curDuration < getCurLowerLimit() && stutterRatio > 0.0f))
    {
        calculatePlaybackSpeed = std::bind(&HJAutoCacheController::deceleratedSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
        speed = calculatePlaybackSpeed(curDuration, stutterRatio);
    } else {
        speed = 1.0f;
    }
    
    return speed;
}
double HJAutoCacheController::acceleratedSpeedCalculation(int64_t curDuration, double stutterRatio)
{
//    JFLogi("entry, accelerated Calculation speed");
    m_calcName = "accelerate";
    if (curDuration <= m_currentTargetDuration)
    {
        calculatePlaybackSpeed = std::bind(&HJAutoCacheController::normalSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
        return 1.0f;
    }
    //if(m_slope > m_slopeMagnitude) {
    //    return 1.2f;
    //}
    return 1.2f;
}
double HJAutoCacheController::deceleratedSpeedCalculation(int64_t curDuration, double stutterRatio)
{
//    JFLogi("entry, decelerated Calculation speed");
    m_calcName = "decelerate";
    if(curDuration >= m_currentTargetDuration) {
        calculatePlaybackSpeed = std::bind(&HJAutoCacheController::normalSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
        return 1.0f;
    }
    //double magnitude = -m_slopeMagnitude;
    //if(m_slope < magnitude) {
    //    return 0.8f;
    //}
    return 0.8f;
}

double HJAutoCacheController::ultraSpeedCalculation(int64_t curDuration, double stutterRatio)
{
    m_calcName = "ultra";
    if(curDuration >= 0.5 * m_currentTargetDuration) {
        calculatePlaybackSpeed = std::bind(&HJAutoCacheController::normalSpeedCalculation, this, std::placeholders::_1, std::placeholders::_2);
        return 1.0f;
    }
    return 0.8f;
}

void HJAutoCacheController::addHistorySample(int64_t curDuration, int64_t currentTime)
{
    if(m_history.size() >= 10) {
        m_history.pop_front();
    }
    m_history.push_back({curDuration, currentTime});
    if(m_history.size() < 5) {
        return;
    }
    m_slope = calculateSlopeNormalized(m_history);
    
    return;
}

double HJAutoCacheController::calculateSlopeNormalized(std::deque<BufferHistory>& values)
{
    int n = (int)values.size();
    double sum_dy = 0.0;
    for (int i = 1; i < n; i++) {
        sum_dy += static_cast<double>(values[i].duration - values[i-1].duration);
    }
    double avg_dy_per_point = sum_dy / (n - 1);

    const double time_interval_ms = 25.0f;
    return avg_dy_per_point / time_interval_ms;
}

double HJAutoCacheController::calculateSlope(std::deque<BufferHistory>& values)
{
    int n = (int)values.size();
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xy = 0.0;
    double sum_x2 = 0.0;

    for (int i = 0; i < n; i++) {
        double x = i * 25.0;
        double y = static_cast<double>(values[i].duration);
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    // 计算斜率 k = (n*sum_xy - sum_x*sum_y) / (n*sum_x2 - sum_x*sum_x)
    double denominator = n * sum_x2 - sum_x * sum_x;
    if (denominator == 0.0) return 0.0f;

    return static_cast<double>((n * sum_xy - sum_x * sum_y) / denominator);
}

bool HJAutoCacheController::isBufferTrendingUp(int64_t currentTime) const {
//    std::lock_guard<std::mutex> lock(m_historyMutex);
    
    // 至少需要5个样本才能确定趋势
    if (m_history.size() < 5) {
        return false;
    }
    
    // 趋势必须持续至少1秒
    if (currentTime - m_directionChangeTime < 1000) {
        return false;
    }
    
    // 检查最近大部分样本是上升的
    int increasingCount = 0;
    for (size_t i = 1; i < m_history.size(); ++i) {
        if (m_history[i].duration > m_history[i-1].duration) {
            increasingCount++;
        }
    }
    
    return increasingCount > static_cast<int>(m_history.size() * 0.7);
}

bool HJAutoCacheController::isBufferTrendingDown(int64_t currentTime) const {
//    std::lock_guard<std::mutex> lock(m_historyMutex);
    
    // 至少需要5个样本才能确定趋势
    if (m_history.size() < 5) {
        return false;
    }
    
    // 趋势必须持续至少1秒
    if (currentTime - m_directionChangeTime < 1000) {
        return false;
    }
    
    // 检查最近大部分样本是下降的
    int decreasingCount = 0;
    for (size_t i = 1; i < m_history.size(); ++i) {
        if (m_history[i].duration < m_history[i-1].duration) {
            decreasingCount++;
        }
    }
    
    return decreasingCount > static_cast<int>(m_history.size() * 0.7);
}

int64_t HJAutoCacheController::getCurrentTargetDuration() const {
    return m_currentTargetDuration;
}

int64_t HJAutoCacheController::getCurrentLimitedRange() const {
    return m_currentLimitedRange;
}
std::string HJAutoCacheController::getStateString() {
    return (m_state == ControlState::OBSERVATION) ? "observe" : "control";
}

NS_HJ_END
