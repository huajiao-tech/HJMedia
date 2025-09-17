//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include <optional>

NS_HJ_BEGIN
//***********************************************************************************//
using HJBitrateData = std::pair<size_t, int64_t>;
class HJBitrateTracker : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJBitrateTracker>;
    HJBitrateTracker(const int64_t slidingWinLen = HJ_MS_PER_SEC)
        : m_slidingWinLen(slidingWinLen) {};
    virtual ~HJBitrateTracker() {};
    
    float addData(const size_t length);
    float addData(const size_t length, const int64_t time);
    void reset();
    
    const float getBPS() {
        return m_bps;
    }
    const float getTotalBPS() {
        return m_totalBPS;
    }
    const float getFPS() {
        return m_fps;
    }
private:
    void checkData(const int64_t curTime);
private:
    int64_t                 m_slidingWinLen = HJ_MS_PER_SEC;      //ms
    float                   m_bps = 0.0f;       //bps
    float                   m_totalBPS = 0.0f;
    float                   m_fps = 0.0f;
    int64_t                 m_winDataLength = 0;
    int64_t                 m_totalDataLength = 0;
    int64_t                 m_startTime = HJ_NOPTS_VALUE;
    int64_t                 m_endTime = HJ_NOPTS_VALUE;
    HJList<HJBitrateData> m_datas;
};

//***********************************************************************************//
class HJBitrateTrackerMuster : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJBitrateTrackerMuster>;
    HJBitrateTrackerMuster(uint64_t slidingWinLen = HJ_MS_PER_SEC) {
        addObject("in", slidingWinLen);
        addObject("out", slidingWinLen);
    }
    HJBitrateTrackerMuster(std::vector<int> idxs, uint64_t slidingWinLen = HJ_MS_PER_SEC) {
        for (auto idx : idxs) {
            addObjectById(idx, slidingWinLen);
        }
    }
    HJBitrateTrackerMuster(std::vector<std::string> names, uint64_t slidingWinLen = HJ_MS_PER_SEC) {
        for (auto name : names) {
            addObject(name, slidingWinLen);
        }
    }

    HJBitrateTracker::Ptr addObjectById(const int idx, uint64_t slidingWinLen = HJ_MS_PER_SEC) {
        std::string name = HJ2STR(idx);
        return addObject(name, slidingWinLen);
    }
    HJBitrateTracker::Ptr addObject(const std::string& name, uint64_t slidingWinLen = HJ_MS_PER_SEC) {
        HJBitrateTracker::Ptr obj = std::make_shared<HJBitrateTracker>(slidingWinLen);
        m_objs.emplace(name, obj);
        return obj;
    }
    const HJBitrateTracker::Ptr getObjById(const int idx) {
        std::string name = HJ2STR(idx);
        return getObjByName(name);
    }
    const HJBitrateTracker::Ptr getObjByName(const std::string& name) {
        auto it = m_objs.find(name);
        if (it != m_objs.end()) {
            return it->second;
        }
        return nullptr;
    }
    const HJBitrateTracker::Ptr getInObj() {
        return getObjByName("in");
    }
    const HJBitrateTracker::Ptr getOutObj() {
        return getObjByName("out");
    }
private:
    std::unordered_map<std::string, HJBitrateTracker::Ptr> m_objs;
};

//***********************************************************************************//
class HJAVCounter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAVCounter>;

    void incIn() {
        m_inCnt++;
    }
    void incOut() {
        m_outCnt++;
    }
    const size_t getInCnt() const {
        return m_inCnt;
    }
    const size_t getOutCnt() const {
        return m_outCnt;
    }
    void reset() {
        m_inCnt = 0;
        m_outCnt = 0;
    }
public:
    size_t      m_inCnt = 0;
    size_t      m_outCnt = 0;
};
//***********************************************************************************//
class HJNetworkSimulator : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJNetworkSimulator)
    HJNetworkSimulator(int bandWidthLimit = 0);
    virtual ~HJNetworkSimulator();

    void setBandWidthLimit(int bandWidthLimit) {
        m_bandWidthLimit = bandWidthLimit;
    }
    void addData(int8_t* data, int len);
    void addData(HJBuffer::Ptr& data);
    HJBuffer::Ptr getData();
    HJBuffer::Ptr getWaitData();
    void waitData(int64_t len);

    void done();
private:
    int64_t				m_bandWidthLimit = 0;			//bps
    std::vector<int8_t> m_dataBuffer;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastOutputTime;
    std::condition_variable	m_dataAvailable;
    std::mutex			m_mutex;
    HJBitrateTrackerMuster::Ptr m_trackerMuster{ nullptr };
    bool                m_done = false;
    float               m_waitFactor = 0.96f;
};

//***********************************************************************************//
class HJNetBitrateTracker : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJNetBitrateTracker>;
    HJNetBitrateTracker(size_t winLen = 100);

    float addData(const size_t length, const int64_t time);  //us
    void reset();
    //
    float getKbps() {
        return m_kbps;
    }
    int64_t getTotalTime() {
        return m_totalTime;
    }
    int64_t getTotalDataSize() {
        return m_totalDataSize;
    }
private:
    size_t                  m_winLen = 100;
    HJList<HJBitrateData>   m_datas;
    int64_t                 m_totalDataSize{0};
    int64_t                 m_totalTime{0};
    float                   m_kbps = 0.0f;
};

//***********************************************************************************//
class HJLossRatioTracker : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJLossRatioTracker);
    HJLossRatioTracker(int64_t winLen = 5000);  //ms
    virtual ~HJLossRatioTracker() = default;
    //
    struct DataInfo {
        int length;
        int64_t time;
        bool isDropped;
    };
    double update(int length, int64_t time, bool isDropped);

    void reset();
public:

private:
    void cleanExpiredPackets(int64_t currentTime);
    double getLossRate();
private:
    std::deque<DataInfo> packets;   
    int64_t windowSize = 5000;
    int64_t totalLength = 0;
    int64_t droppedLength = 0;
};

//***********************************************************************************//
template <typename T>
class HJValueStatistics : public HJObject {
private:
    struct DataPoint {
        T value;
        int64_t time;
    };
    
    std::deque<DataPoint> dataPoints;
    int64_t windowSize;
    
    void cleanExpiredData(int64_t currentTime) {
        int64_t threshold = currentTime - windowSize;
        dataPoints.erase(
            std::remove_if(dataPoints.begin(), dataPoints.end(),
                [threshold](const DataPoint& dp) { return dp.time < threshold; }),
            dataPoints.end()
        );
    }
    
public:
    HJ_DECLARE_PUWTR(HJValueStatistics);
    explicit HJValueStatistics(int64_t window = 5 * 1000) : windowSize(window) {}
    void setWindowSize(int64_t window) {
        windowSize = window;
        if (!dataPoints.empty()) {
            auto latestTime = std::max_element(
                dataPoints.begin(), 
                dataPoints.end(),
                [](const DataPoint& a, const DataPoint& b) { return a.time < b.time; }
            )->time;
            cleanExpiredData(latestTime);
        }
    }
    void addValue(T value, int64_t time) {
        dataPoints.push_back({value, time});
        cleanExpiredData(time);
    }
    
    std::optional<T> getMax() const {
        if (dataPoints.empty()) {
            return std::nullopt;
        }
        
        auto maxIt = std::max_element(
            dataPoints.begin(), 
            dataPoints.end(),
            [](const DataPoint& a, const DataPoint& b) { return a.value < b.value; }
        );
        return maxIt->value;
    }
    
    std::optional<T> getMin() const {
        if (dataPoints.empty()) {
            return std::nullopt;
        }
        
        auto minIt = std::min_element(
            dataPoints.begin(), 
            dataPoints.end(),
            [](const DataPoint& a, const DataPoint& b) { return a.value < b.value; }
        );
        return minIt->value;
    }
    
    std::optional<T> get95thValue(bool min = false) const {
        if (dataPoints.empty()) {
            return std::nullopt;
        }
        
        std::vector<T> values;
        values.reserve(dataPoints.size());
        
        for (const auto& dp : dataPoints) {
            values.push_back(dp.value);
        }
        size_t idx = 0;
        if(min) {
            idx  = static_cast<size_t>(values.size() * 0.05);
            std::nth_element(values.begin(), values.begin() + idx, values.end());
            return values[idx];
        } else {
            idx = static_cast<size_t>(std::ceil(values.size() * 0.95)) - 1;
            std::nth_element(values.begin(), values.begin() + idx, values.end());
        }
        return values[idx];
    }
    
    size_t getCount() const {
        return dataPoints.size();
    }
    
    void reset() {
        dataPoints.clear();
    }
    
    void cleanExpired(int64_t currentTime) {
        cleanExpiredData(currentTime);
    }
};
using HJValueStatisticsInt64 = HJValueStatistics<int64_t>;


NS_HJ_END
