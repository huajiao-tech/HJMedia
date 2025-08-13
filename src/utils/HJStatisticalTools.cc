//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJStatisticalTools.h"
#include "HJFLog.h"
#include "HJTime.h"

NS_HJ_BEGIN
//***********************************************************************************//
float HJBitrateTracker::addData(const size_t length)
{
    return addData(length, HJCurrentSteadyMS());
}

float HJBitrateTracker::addData(const size_t length, const int64_t time)
{
    checkData(time);
//    m_datas.push_back(HJBitrateData(length, time));
    m_datas.push_back(std::make_pair(length, time));
    
    float bps = 0.0f;
    float fps = 1.0f;
    if(m_datas.size() >= 2) {
        const auto& first = m_datas.front();
        const auto& end = m_datas.back();
        double deltaTime = std::abs(end.second - first.second);
        deltaTime = HJ_MAX(deltaTime, m_slidingWinLen * 0.7) / (double)HJ_MS_PER_SEC;
        m_fps = (m_datas.size() - 1) / deltaTime;
#if 1
        bps = (m_winDataLength * 8) / deltaTime;
        //double frameDuraton = 1.0 / m_fps;
        //bps = (m_winDataLength * 8) / (double)(deltaTime + frameDuraton);
#else
        bps = (m_totalDataLength - end.first) * 8 * HJ_MS_PER_SEC / (double)deltaTime;
#endif
        m_bps = bps;
    }
    m_winDataLength += length;
    if (HJ_NOPTS_VALUE == m_startTime) {
        m_startTime = time;
    }
    m_endTime = time;
    m_totalDataLength += length;
    if (m_endTime > m_startTime) {
        m_totalBPS = (m_totalDataLength * 8) * HJ_MS_PER_SEC / (double)(m_endTime - m_startTime);
    }
    
    return bps;
}

void HJBitrateTracker::reset()
{
    m_bps = 0.0f;
    m_fps = 0.0f;
    m_winDataLength = 0;
    m_totalDataLength = 0;
    m_datas.clear();
}

void HJBitrateTracker::checkData(const int64_t curTime)
{
    for (auto it = m_datas.begin(); it != m_datas.end(); ) {
        if(it->second + m_slidingWinLen < curTime) {
            m_winDataLength -= it->first;
            it = m_datas.erase(it);
        } else {
            it++;
        }
    }
}
//***********************************************************************************//
HJNetworkSimulator::HJNetworkSimulator(int bandWidthLimit)
    : m_bandWidthLimit(bandWidthLimit)
{
    m_lastOutputTime = std::chrono::time_point<std::chrono::high_resolution_clock>{};
    m_trackerMuster = std::make_shared<HJBitrateTrackerMuster>();
}

HJNetworkSimulator::~HJNetworkSimulator()
{
    done();
}

void HJNetworkSimulator::addData(int8_t* data, int len)
{
    {
        HJ_AUTOU_LOCK(m_mutex);
        m_dataBuffer.insert(m_dataBuffer.end(), data, data + len);
        if (std::chrono::time_point<std::chrono::high_resolution_clock>{} == m_lastOutputTime) {
            m_lastOutputTime = std::chrono::high_resolution_clock::now();
        }
        //
        float bps = m_trackerMuster->getInObj()->addData(len);
        HJFLogi("networks in bitrate:{}, len:{}, buffer size:{}", bps, len, m_dataBuffer.size());
    }
    m_dataAvailable.notify_one();

    return;
}

void HJNetworkSimulator::addData(HJBuffer::Ptr& data)
{
	if (!data || data->size() <= 0) {
		return;
	}
    addData((int8_t *)data->data(), data->size());
	return;
}

HJBuffer::Ptr HJNetworkSimulator::getData()
{
    HJ_AUTOU_LOCK(m_mutex);
    while (m_dataBuffer.empty()) {
        m_dataAvailable.wait(lock);
    }
    //
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastOutputTime).count();
    int64_t maxBytesToSend = !m_bandWidthLimit ? INT64_MAX : (m_bandWidthLimit * elapsed) / 8000000;
    //
    HJBuffer::Ptr buffer = nullptr;
    int64_t bytesToSend = std::min(maxBytesToSend, static_cast<int64_t>(m_dataBuffer.size()));
    if (bytesToSend > 0) {
        buffer = std::make_shared<HJBuffer>(bytesToSend);
        std::memcpy(buffer->data(), m_dataBuffer.data(), bytesToSend);
        buffer->setSize(bytesToSend);
        //
        m_dataBuffer.erase(m_dataBuffer.begin(), m_dataBuffer.begin() + bytesToSend);
        //
        m_trackerMuster->getOutObj()->addData(bytesToSend);
    }
    m_lastOutputTime = now;
    //
	float bps = m_trackerMuster->getOutObj()->getBPS();
    HJFLogi("networks out bitrate:{}, bytesToSend:{}, buffer size:{}", bps, bytesToSend, m_dataBuffer.size());

    return buffer;
}

HJBuffer::Ptr HJNetworkSimulator::getWaitData()
{
    HJ_AUTOU_LOCK(m_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastOutputTime).count();
    int64_t bytesToSend = m_dataBuffer.size();
    int64_t willElapsed = !m_bandWidthLimit ? 0 : (bytesToSend * 8000000.0) / m_bandWidthLimit;       //US
    if (willElapsed > elapsed) {
        int64_t waitTime = willElapsed; //willElapsed - elapsed;
		HJFLogi("wait for data time:{} us", waitTime);
        m_dataAvailable.wait_for(lock, std::chrono::microseconds(waitTime), [&] { return m_done; });
    }
    m_lastOutputTime = now;
    //
    HJBuffer::Ptr buffer = nullptr;
    if (bytesToSend > 0) {
        buffer = std::make_shared<HJBuffer>(bytesToSend);
        std::memcpy(buffer->data(), m_dataBuffer.data(), bytesToSend);
        buffer->setSize(bytesToSend);
        //
        m_dataBuffer.erase(m_dataBuffer.begin(), m_dataBuffer.begin() + bytesToSend);
        //
        m_trackerMuster->getOutObj()->addData(bytesToSend);
    }
    float bps = m_trackerMuster->getOutObj()->getBPS();
    HJFLogi("networks out bitrate:{}, bytesToSend:{}, buffer size:{}", bps, bytesToSend, m_dataBuffer.size());

    return buffer;
}

void HJNetworkSimulator::waitData(int64_t len)
{
    HJ_AUTOU_LOCK(m_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::time_point<std::chrono::high_resolution_clock>{} == m_lastOutputTime) {
        m_lastOutputTime = now;
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastOutputTime).count();
    int64_t willElapsed = !m_bandWidthLimit ? 0 : (len * 8000000.0) / m_bandWidthLimit;       //US
    if (willElapsed > elapsed) {
        int64_t waitTime = m_waitFactor *(willElapsed - elapsed);
        //HJFLogi("wait for data time:{} us", waitTime);
        m_dataAvailable.wait_for(lock, std::chrono::microseconds(waitTime), [&] { return m_done; });
    }
    m_lastOutputTime = now;
    //
    //float bps = m_trackerMuster->getOutObj()->addData(len);
    //if (bps > 0.0) {
    //    m_waitFactor = HJ_CLIP(bps / m_bandWidthLimit, 0.5f, 1.5f);
    //}

    return;
}

void HJNetworkSimulator::done()
{
    m_done = true;
    m_dataAvailable.notify_one();
}

//***********************************************************************************//
HJNetBitrateTracker::HJNetBitrateTracker(size_t winLen)
    : m_winLen(winLen)
{

}

float HJNetBitrateTracker::addData(const size_t length, const int64_t time)
{
    float kbps = 0.0f;
    //check
    if (m_datas.size() >= m_winLen) {
        const auto& it = m_datas.front();
        m_totalDataSize -= it.first;
        m_totalTime -= it.second;
        //
        m_datas.pop_front();
    }
    m_totalDataSize += length;
    m_totalTime += time;
    m_datas.push_back(std::make_pair(length, time));
    if (m_datas.size() >= 2 && m_totalTime > 0) {
        double duration = m_totalTime / (double)HJ_MS_PER_SEC;   //HJ_US_PER_SEC
        kbps = static_cast<float>(m_totalDataSize * 8) / duration;
        //
        m_kbps = kbps;
    }
    return kbps;
}
void HJNetBitrateTracker::reset()
{
    m_datas.clear();
    m_totalDataSize = 0;
    m_totalTime = 0;
}

//***********************************************************************************//
HJLossRatioTracker::HJLossRatioTracker(int64_t winLen)
    : windowSize(winLen)
{
}

double HJLossRatioTracker::update(int length, int64_t time, bool isDropped)
{
    cleanExpiredPackets(time);

    DataInfo info{length, time, isDropped};
    packets.push_back(info);

    totalLength += length;
    if (isDropped) {
        droppedLength += length;
    }

    return getLossRate();
}

double HJLossRatioTracker::getLossRate()
{
    if (totalLength == 0) {
        return 0.0;
    }

    return static_cast<double>(droppedLength) / totalLength;
}

void HJLossRatioTracker::cleanExpiredPackets(int64_t currentTime)
{
    while (!packets.empty()) {
        int64_t oldestTime = packets.front().time;
        if (currentTime - oldestTime > windowSize) {
            totalLength -= packets.front().length;
            if (packets.front().isDropped) {
                droppedLength -= packets.front().length;
            }
            packets.pop_front();
        } else {
            break;
        }
    }
    return;
}

void HJLossRatioTracker::reset()
{
    droppedLength = 0;
    totalLength = 0;
    packets.clear();
}

NS_HJ_END
