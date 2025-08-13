//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRTMPPacketManager.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "flv/HJESParser.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJRTMPPacketStats::~HJRTMPPacketStats()
{

}

int HJRTMPPacketStats::update(HJRTMPStatType statType, const HJFLVPacket::Ptr packet, const HJBuffer::Ptr tag)
{
    switch (statType)
    {
        case HJRTMP_STATTYPE_QUEUED: {
            auto stat = getStats(statType);
            if(stat && packet)
            {
                if (packet->isVideo()) {
                    stat->m_val.videoFrames++;
                    stat->m_videoTracker->addData(packet->getSize());
                    stat->m_val.video_fps = stat->m_videoTracker->getFPS() + 0.5f;
                    stat->m_val.video_kbps = stat->m_videoTracker->getBPS() / 1000;
                }
                else if (packet->isAudio()) {
                    stat->m_val.audioFrames++;
                    stat->m_audioTracker->addData(packet->getSize());
                    stat->m_val.audio_fps = stat->m_audioTracker->getFPS() + 0.5f;
                    stat->m_val.audio_kbps = stat->m_audioTracker->getBPS() / 1000;
                }
                stat->m_val.bytes += packet->getSize();
                //
                m_dropRatio = m_lossTracker.update(packet->getSize(), packet->m_dts, false);
            }
            break;
        }
        case HJRTMP_STATTYPE_SENT: {
            auto stat = getStats(statType);
            if (stat) 
            {
                if (packet) {
                    if (packet->isVideo()) {
                        stat->m_val.videoFrames++;
                        if (tag) {
                            stat->m_videoTracker->addData(tag->size());
                            stat->m_val.video_fps = stat->m_videoTracker->getFPS() + 0.5f;
                            stat->m_val.video_kbps = stat->m_videoTracker->getBPS() / 1000;
                        }
                    }
                    else if (packet->isAudio()) {
                        stat->m_val.audioFrames++;
                        if (tag) {
                            stat->m_audioTracker->addData(tag->size());
                            stat->m_val.audio_fps = stat->m_audioTracker->getFPS() + 0.5f;
                            stat->m_val.audio_kbps = stat->m_audioTracker->getBPS() / 1000;
                        }
                    }   
                }
                if (tag) {
                    stat->m_val.bytes += tag->size();
                }
            }
            break;
        }
        case HJRTMP_STATTYPE_DROPPED: {
            auto stat = getStats(statType);
            if (stat && packet) 
            {
                auto queuedStat = getStats(HJRTMP_STATTYPE_QUEUED);
                if (packet->isVideo()) {
                    stat->m_val.videoFrames++;
                    if(queuedStat) {
                        stat->m_val.videoDropRatio = (float)stat->m_val.videoFrames / (queuedStat->m_val.videoFrames + 1);
                    }
                }
                else if (packet->isAudio()) {
                    stat->m_val.audioFrames++;
                    if (queuedStat) {
                        stat->m_val.audioDropRatio = (float)stat->m_val.audioFrames / (queuedStat->m_val.audioFrames + 1);
                    }
                }
                stat->m_val.bytes += packet->getSize();
                if (queuedStat) {
                    stat->m_val.dropRatio = (float)stat->m_val.bytes / (queuedStat->m_val.bytes + 1);
                }
                //
                m_dropRatio = m_lossTracker.update(packet->getSize(), packet->m_dts, true);
            }
            break;
        }
        default: {
            break;
        }
    }
    auto now = HJCurrentSteadyMS();
    if (HJ_NOTS_VALUE == m_startTime) {
        m_startTime = now;
    }
    m_duration = now - m_startTime;
    //
    if(m_logTime == HJ_NOTS_VALUE) {
        m_logTime = now;
    }
    if(now - m_logTime > 2000) {
        m_logTime = now;
        HJFLogi("packet stats info:{}", formatInfo());
    }

    return HJ_OK;
}

std::string HJRTMPPacketStats::formatInfo()
{
    std::string info = "";
    auto stat = getStats(HJRTMP_STATTYPE_QUEUED);
    if(stat) {
        auto& val = stat->m_val;
        info += HJFMT("data:[v - fps:{}, kbps:{}, count:{}; a - fps:{}, kpbs:{}, count:{}] ", 
            val.video_fps, val.video_kbps, val.videoFrames, val.audio_fps, val.audio_kbps, val.audioFrames);
    }
    stat = getStats(HJRTMP_STATTYPE_SENT);
    if (stat) {
        auto& val = stat->m_val;
        info += HJFMT("sent:[v - fps:{}, kbps:{}, count:{}; a - fps:{}, kpbs:{}, count:{}] ",
            val.video_fps, val.video_kbps, val.videoFrames, val.audio_fps, val.audio_kbps, val.audioFrames);
    }
    info += HJFMT(" duration:{}", m_duration);

    return info;
}

//***********************************************************************************//
HJRTMPPacketManager::HJRTMPPacketManager(HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts, HJListener listener)
    : m_mediaInfo(mediaInfo)
    , m_opts(opts)
    , m_listener(listener)
{
    if (m_opts) {
        m_enhancedRtmp = m_opts->getBool("enhanced");
        if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_DROP_ENABLE)) {
            m_netParams.m_dropEnable = m_opts->getBool(HJRTMPUtils::STORE_KEY_DROP_ENABLE);
        }
        if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_DROP_THRESHOLD)) {
            m_netParams.m_dropThreshold = m_opts->getInt64(HJRTMPUtils::STORE_KEY_DROP_THRESHOLD);
        }
        if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_DROP_PFRAME_THRESHOLD)) {
            m_netParams.m_dropPFrameThreshold = m_opts->getInt64(HJRTMPUtils::STORE_KEY_DROP_PFRAME_THRESHOLD);
        }
        if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_DROP_IFRAME_THRESHOLD)) {
            m_netParams.m_dropIFrameThreshold = m_opts->getInt64(HJRTMPUtils::STORE_KEY_DROP_IFRAME_THRESHOLD);
        }
        if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_BITRATE)) {
            m_netParams.m_bitrate = m_opts->getInt64(HJRTMPUtils::STORE_KEY_BITRATE);
        }
        if (m_opts->haveValue(HJRTMPUtils::STORE_KEY_LOW_BITRATE)) {
            m_netParams.m_lowBitrate = m_opts->getInt64(HJRTMPUtils::STORE_KEY_LOW_BITRATE);
        }
    }
    HJFLogi("m_enhancedRtmp:{}, m_dropThreshold:{}, m_dropPFrameThreshold:{}, m_dropIFrameThreshold:{}, m_bitrate:{}, m_lowBitrate:{}", 
        m_enhancedRtmp, m_netParams.m_dropThreshold, m_netParams.m_dropPFrameThreshold, m_netParams.m_dropIFrameThreshold, m_netParams.m_bitrate, m_netParams.m_lowBitrate);
    //
    m_bitrateAdapter = HJCreateu<HJRTMPBitrateAdapter>(m_netParams.m_bitrate, m_netParams.m_lowBitrate, m_netParams.m_adjustInterval, m_netParams.m_dropThreshold);

    return;
}

HJRTMPPacketManager::~HJRTMPPacketManager()
{

}

std::tuple<int64_t, int64_t, int64_t> HJRTMPPacketManager::getDuartion()
{
    HJFLVPacket::Ptr videoFront = nullptr;
    HJFLVPacket::Ptr videoBack = nullptr;
    HJFLVPacket::Ptr audioFront = nullptr;
    HJFLVPacket::Ptr audioBack = nullptr;
    for (auto it = m_packets.begin(); it != m_packets.end(); ++it) {
        auto &packet = *it;
        if (!videoFront && packet->isVideo()) {
            videoFront = packet;
        }
        if (!audioFront && packet->isAudio()) {
            audioFront = packet;
        }
        if (videoFront && audioFront) {
            break;
        }
    }
    for (auto it = m_packets.rbegin(); it != m_packets.rend();++it) {
        auto &packet = *it;
        if (!videoBack && packet->isVideo()) {
            videoBack = packet;
        }
        if (!audioBack && packet->isAudio()) {
            audioBack = packet;
        }
        if (videoBack && audioBack) {
            break;
        }
    }
    //
    int64_t duration = 0;
    int64_t videoDuration = 0;
    int64_t audioDuration = 0;
    int64_t minTime = HJ_INT64_MAX;
    int64_t maxTime = HJ_INT64_MIN;
    if (videoFront != videoBack) {
        videoDuration = videoBack->m_dts - videoFront->m_dts;
        minTime = HJ_MIN(minTime, videoFront->m_dts);
        maxTime = HJ_MAX(maxTime, videoBack->m_dts);
    }
    
    if (audioFront != audioBack) {
        audioDuration = audioBack->m_dts - audioFront->m_dts;
        minTime = HJ_MIN(minTime, audioFront->m_dts);
        maxTime = HJ_MAX(maxTime, audioBack->m_dts);
    }
    if (HJ_INT64_MAX != minTime && HJ_INT64_MIN != maxTime) {
        duration = maxTime - minTime;
    }
    //HJFLogi("getDuartion - duartion:{}, videoDuration:{}, audioDuration:{}", duration, videoDuration, audioDuration);
    return std::make_tuple(duration, videoDuration, audioDuration);
}

int HJRTMPPacketManager::drop(bool dropAudio)
{
    if (m_packets.size() < 5) {
        m_stats.m_cacheDuration = m_packets.size() > 0 ? m_packets.back()->m_dts - m_packets.front()->m_dts : 0;
        return HJ_OK;
    }
    int64_t duration = 0;
    int64_t videoDuration = 0;
    int64_t audioDuration = 0;
    std::tie(duration, videoDuration, audioDuration) = getDuartion();
    //
    m_stats.m_cacheDuration = duration;
    //
    if (!m_netParams.m_dropEnable) {
        if (duration > m_netParams.m_dropThreshold) {
            //HJFLogi("entry, duration:{}, videoDuration:{}, audioDuration:{}", duration, videoDuration, audioDuration);
        }
        return HJ_OK;
    }
    if (videoDuration <= m_netParams.m_dropThreshold && duration <= m_netParams.m_dropIFrameThreshold) {
        return HJ_OK;
    }
    HJFLogw("entry, duration:{}, videoDuration:{}, audioDuration:{}", duration, videoDuration, audioDuration);
    int dropCount = 0;
    if (duration > m_netParams.m_dropIFrameThreshold) 
    {
        HJList<HJFLVPacket::Ptr>::iterator dropGuard = m_packets.begin();
        int64_t lastDTS = m_packets.back()->m_dts;
        for (auto it = m_packets.begin(); it != m_packets.end(); ++it) {
            auto& packet = *it;
            const int64_t offset = lastDTS - packet->m_dts;
            if (packet->isKeyVFrame() && offset < m_netParams.m_dropIFrameThreshold) {
                dropGuard = it;
                break;
            }
        }
        //
        for (auto it = m_packets.begin(); it != dropGuard;) {
            auto& packet = *it;
            const int64_t offset = lastDTS - packet->m_dts;
            if (packet->getPriority() <= HJFLV_PKT_PRIORITY_HIGH && (dropAudio || !packet->isAudio())) {
                HJFLogw("drop packet, packet:{}, offset:{}", packet->formatInfo(), offset);
                dropCount++;
                m_stats.update(HJRTMP_STATTYPE_DROPPED, packet, nullptr);
                it = m_packets.erase(it);
            } else {
                ++it;
            }
        }
    } 
    else if (duration > m_netParams.m_dropPFrameThreshold) 
    {
        HJList<HJFLVPacket::Ptr>::iterator dropGuard = m_packets.begin();
        int64_t lastDTS = m_packets.back()->m_dts;
        for (auto it = m_packets.begin(); it != m_packets.end(); ++it) {
            auto& packet = *it;
            const int64_t offset = lastDTS - packet->m_dts;
            if (packet->isKeyVFrame() && offset < m_netParams.m_dropPFrameThreshold) {
                dropGuard = it;
                break;
            }
        }
        //
        for (auto it = m_packets.begin(); it != dropGuard;) {
            auto& packet = *it;
            const int64_t offset = lastDTS - packet->m_dts;
            if (packet->getPriority() <= HJFLV_PKT_PRIORITY_MEDIUM && packet->isVideo()) {
                HJFLogw("drop packet, packet:{}, offset:{}", packet->formatInfo(), offset);
                dropCount++;
                m_stats.update(HJRTMP_STATTYPE_DROPPED, packet, nullptr);
                it = m_packets.erase(it);
            } else {
                ++it;
            }
        }
    }
    else if (duration > m_netParams.m_dropThreshold)
    {
        HJList<HJFLVPacket::Ptr>::iterator dropGuard = m_packets.begin();
        int64_t lastDTS = m_packets.back()->m_dts;
        for (auto it = m_packets.begin(); it != m_packets.end(); ++it) {
            auto& packet = *it;
            const int64_t offset = lastDTS - packet->m_dts;
            if (packet->isKeyVFrame() && offset < m_netParams.m_dropThreshold) {
                dropGuard = it;
                break;
            }
        }
        //
        for (auto it = m_packets.begin(); it != dropGuard;) {
            auto& packet = *it;
            const int64_t offset = lastDTS - packet->m_dts;
            if (packet->getPriority() <= HJFLV_PKT_PRIORITY_LOW && packet->isVideo()) {
                HJFLogw("drop packet, packet:{}, offset:{}", packet->formatInfo(), offset);
                dropCount++;
                m_stats.update(HJRTMP_STATTYPE_DROPPED, packet, nullptr);
                it = m_packets.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (m_listener && dropCount > 0) {
        m_listener(std::move(HJMakeNotification(HJRTMP_EVENT_DROP_FRAME, HJFMT("{}", dropCount))));
        //
        // printPackets();
    }

    return dropCount;
}

void HJRTMPPacketManager::printPackets()
{
    HJFLogi("entry");
    for (auto it = m_packets.begin(); it != m_packets.end(); ++it)  {
        auto& packet = *it;
        if(packet->isVideo()) {
            HJFLogi("left packet:{}", packet->formatInfo());
        }
    }
    HJFLogi("end");
}

int HJRTMPPacketManager::push(HJFLVPacket::Ptr packet)
{
    //HJFLogi("packet:{}", packet->formatInfo());
    {
        HJ_AUTOU_LOCK(m_mutex);
        m_packets.push_back(packet);
        //
        m_stats.update(HJRTMP_STATTYPE_QUEUED, packet, nullptr);
        //
        m_stats.m_dropCount = drop();
        m_stats.m_cacheSize = m_packets.size();
        //
        if(m_bitrateAdapter && m_stats.getDuration() > 5000)
        {
            auto targetBitrate = m_netParams.m_bitrate;
            auto inBitrate = m_stats.getInBitrate();
            auto outBitrate = m_stats.getOutBitrate();
            auto dropRatio = m_stats.getDropRatio();
        //    int adaptiveBitrate = m_bitrateAdapter->update2(inBitrate, outBitrate, m_netBitrate, dropRatio, m_stats.m_cacheDuration);
            int adaptiveBitrate = (int)m_bitrateAdapter->update(inBitrate, outBitrate, m_stats.m_cacheDuration, m_stats.m_dropCount, dropRatio);
            //
            // HJFLogi("bitrate adapter, target:{}, in:{}, out:{}, dropRatio:{}, adaptive:{}, packets - [drop count:{},  size:{}], duration:{}", 
            //    targetBitrate, inBitrate, outBitrate, dropRatio, (int)adaptiveBitrate, m_stats.m_dropCount, m_stats.m_cacheSize, m_stats.getDuration());
            //
            if(adaptiveBitrate > 0 && (adaptiveBitrate != m_adaptiveBitrate) && m_listener) {
                m_adaptiveBitrate = adaptiveBitrate;
                HJFLogw("adaptive bitrate changed:{}", m_adaptiveBitrate);
                m_listener(std::move(HJMakeNotification(HJRTMP_EVENT_AUTOADJUST_BITRATE, (int64_t)m_adaptiveBitrate, HJFMT("{}", m_adaptiveBitrate))));
            }
        }
    }
    //notify();
    m_cv.notify_one();
    //HJFLogi("push packet size:{}", m_packets.size());
    return HJ_OK;
}

HJBuffer::Ptr HJRTMPPacketManager::waitTag(int64_t timeout, bool isHeader)
{
    //HJFLogi("entry, timeout:{}", timeout);
    //netWouldBlock();
    //
	auto t0 = HJCurrentSteadyMS();
    HJBuffer::Ptr tag = nullptr;
    HJ_AUTOU_LOCK(m_mutex);
    do
    {
        if (m_isQuit) {
            break;
        }
        tag = getTag(isHeader);
        if (tag) {
            break;
        } 
        //HJFLogi("wait_for before");
        auto ret = m_cv.wait_for(lock, std::chrono::milliseconds(timeout));
        if (ret == std::cv_status::timeout) 
        {
        }
        //auto ret = m_cv.wait_for(lock, std::chrono::milliseconds(timeout), [&] { return m_isQuit; });
        //if (ret) {
        //    HJFLogi("wait_for ret:{}, quit:{}", ret, m_isQuit);
        //}
    } while (false);

    if(HJ_NOTS_VALUE == m_waitTagTime) {
        m_waitTagTime = t0;
    }
    if (tag && (t0 - m_waitTagTime > 1000)) {
        m_waitTagTime = t0;
        HJFLogi("end, tag size:{}, dts:{}, packets:{}, time:{}, delay:{}", tag->size(), tag->getTimestamp(), m_packets.size(), HJCurrentSteadyMS() - t0, m_stats.m_delay);
    }
    return tag;
}

HJBuffer::Ptr HJRTMPPacketManager::getTag(bool isHeader)
{
    HJBuffer::Ptr tag = nullptr;
    enum HJVideoColorspace colorspace = HJ_VIDEO_CS_709; //to do //info->colorspace; send metadata only if HDR
    if (isHeader) {
        m_packets.clear();
        m_sentStatus = HJ_RTMP_SENT_NONE;
        m_firstKeyFrame = false;
        m_gotFirtFrame = false;
        m_startDTSOffset = HJ_NOTS_VALUE;
        //
        HJFLogi("get header tag");
    }
    //
    HJFLVPacket::Ptr packet = nullptr;
    if (!(m_sentStatus & HJ_RTMP_SENT_META)) {
        auto buffer = HJFLVUtils::buildMetaDataTag(m_mediaInfo, true);
        if (!buffer) {
            HJLoge("error, build meta data tag failed");
            return nullptr;
        }
        buffer->setID(0);
        //
        tag = buffer;
        m_sentStatus |= HJ_RTMP_SENT_META;
        HJFLogi("build meta data tag");
    }
//    else if (!(m_sentStatus & HJ_RTMP_SENT_EXTRA_META)) {  //audio track 1
//        auto buffer = HJFLVUtils::buildAdditionalMetaTag();
//        if (!buffer) {
//            HJLoge("error, build additional meta data tag failed");
//            return nullptr;
//        }
//        buffer->setID(0);
//        //
//        tag = buffer;
//        m_sentStatus |= HJ_RTMP_SENT_EXTRA_META;
//    }
    else if (!(m_sentStatus & HJ_RTMP_SENT_A_HEADER)) {
        tag = buildAudioHeader();
        m_sentStatus |= HJ_RTMP_SENT_A_HEADER;
        HJFLogi("build audio header tag");
    }
    else if (!(m_sentStatus & HJ_RTMP_SENT_V_HEADER)) {
        tag = buildVideoHeader();
        m_sentStatus |= HJ_RTMP_SENT_V_HEADER;
        HJFLogi("build video header tag");
    }
    else if (!(m_sentStatus & HJ_RTMP_SENT_HDR_HEADER) && (colorspace == HJ_VIDEO_CS_2100_PQ || colorspace == HJ_VIDEO_CS_2100_HLG)) {
        tag = buildHDRVideoHeader();
        m_sentStatus |= HJ_RTMP_SENT_HDR_HEADER;
    }
    
    //HJ_RTMP_SENT_FOOTER
    else 
    {
        if (m_packets.size() <= 0) {
            return nullptr;
        }
        if (!m_firstKeyFrame) 
        {
            for (auto it = m_packets.begin(); it != m_packets.end();) {
                const auto& pkt = *it;
                if (pkt->isVideo() && pkt->isKeyVFrame()) {
                    m_firstKeyFrame = true;
                    break;
                } else {
                    it = m_packets.erase(it);
                }
            } 
            //
            // auto iframeGuard = m_packets.begin();
            // for (auto it = m_packets.begin(); it != m_packets.end(); ++it) {
            //     const auto& pkt = *it;
            //     if (pkt->isVideo() && pkt->isKeyVFrame()) {
            //         packet = pkt;
            //         iframeGuard = it;
            //         m_firstKeyFrame = true;
            //     }
            // }
            // if (m_firstKeyFrame) {
            //     m_packets.erase(m_packets.begin(), iframeGuard);
            //     HJFLogi("first key packet:{}", packet->formatInfo());
            // } else {
            //     m_packets.clear();
            // }
        } 
        if (m_firstKeyFrame) 
        {
            if (HJ_NOTS_VALUE == m_startDTSOffset) {
                m_startDTSOffset = waitStartDTSOffset();
                return nullptr;
            }
            packet = m_packets.front();
            if(!m_gotFirtFrame && packet) {
                m_gotFirtFrame = true;
                HJFLogi("got first frame:{}, m_startDTSOffset:{}", packet->formatInfo(), m_startDTSOffset);
            }
            m_packets.pop_front();
            //
            tag = buildTag(packet);
        } else {
            HJFLogw("warning, waiting key frame");
        }
    }
    if (tag) {
        m_stats.update(HJRTMP_STATTYPE_SENT, packet, tag);
        //
        if (packet && HJ_NOTS_VALUE != packet->m_extraTS) {
            m_stats.m_delay = HJCurrentSteadyMS() - packet->m_extraTS;
            //
            // if (m_stats.m_delay > 100) {
            //     HJFLogi("packet tag delay:{}", m_stats.m_delay);
            // }
        }
    }

    return tag;
}

int64_t HJRTMPPacketManager::waitStartDTSOffset()
{
    int64_t startDTSOffset = HJ_NOTS_VALUE;
    HJFLVPacket::Ptr videoPkt = nullptr;
    HJFLVPacket::Ptr audioPkt = nullptr;
    //
    HJFLogi("wait start dts offset");
    for (auto it = m_packets.begin(); it != m_packets.end(); ++it) {
        const auto& pkt = *it;
        if (pkt->isVideo() && !videoPkt) {
            videoPkt = pkt;
        } else if (pkt->isAudio() && !audioPkt) {
            audioPkt = pkt;
        }
        if(videoPkt && audioPkt) {
            startDTSOffset = HJ_MIN(videoPkt->m_dts, audioPkt->m_dts);
            break;
        }
    }
    return startDTSOffset;
}

HJBuffer::Ptr HJRTMPPacketManager::buildTagA(HJFLVPacket::Ptr packet, bool isHeader)
{
    size_t idx = packet->m_trackIdx;
    HJBuffer::Ptr tag = nullptr;
    if (idx > 0) {
        tag = HJFLVUtils::buildAdditionalPacket(packet, isHeader ? 0 : m_startDTSOffset, isHeader, idx);
    } else {
        tag = HJFLVUtils::buildPacket(packet, isHeader ? 0 : m_startDTSOffset, isHeader);
    }
    tag->setID(idx);

    return tag;
}

HJBuffer::Ptr HJRTMPPacketManager::buildTagB(HJFLVPacket::Ptr packet, bool isHeader, bool isFooter)
{
    HJBuffer::Ptr tag = nullptr;
    if (isHeader) {
        tag = HJFLVUtils::buildPacketStart(packet, packet->m_codecid);
    }
    else if (isFooter) {
        tag = HJFLVUtils::buildPacketEnd(packet, packet->m_codecid);
    }
    else {
        tag = HJFLVUtils::buildPacketFrames(packet, packet->m_codecid, 0/*m_start_dts_offset*/);
    }
    return tag;
}

HJBuffer::Ptr HJRTMPPacketManager::buildTag(HJFLVPacket::Ptr packet, bool isHeader, bool isFooter)
{
    HJBuffer::Ptr tag = nullptr;
    if (m_enhancedRtmp) {
        tag = buildTagB(packet, isHeader, isFooter);
    } else {
        tag = buildTagA(packet, isHeader);
    }
    //HJFLogi("build tag, size:{}, packet:{}", tag->size(), packet->formatInfo());

    return tag;
}

HJBuffer::Ptr HJRTMPPacketManager::buildAudioHeader()
{
    auto audioInfo = m_mediaInfo->getAudioInfo();
    if (!audioInfo) {
        HJLogw("warning, have no audio info");
        return nullptr;
    }
    AVCodecParameters* codecParam = audioInfo->getAVCodecParams();
    if (!codecParam) {
        HJLoge("error, have no audio info codec param");
        return nullptr;
    }
    HJFLVPacket::Ptr packet = HJFLVPacket::makeAudioPacket();
    packet->m_data = std::make_shared<HJBuffer>(codecParam->extradata, codecParam->extradata_size);
    HJFLogi("audio extra data:0x{:x}, 0x{:x}", codecParam->extradata[0], codecParam->extradata[1]);
    //
    return buildTag(packet, true, false);
}

HJBuffer::Ptr HJRTMPPacketManager::buildVideoHeader()
{
    auto videoInfo = m_mediaInfo->getVideoInfo();
    if (!videoInfo) {
        HJLogw("warning, have no video info");
        return nullptr;
    }
    AVCodecParameters* codecParam = videoInfo->getAVCodecParams();
    if (!codecParam) {
        HJLoge("have no video info codec param");
        return nullptr;
    }
    int codecid = videoInfo->getCodecID();
    HJFLVPacket::Ptr packet = HJFLVPacket::makeVideoPacket(codecid, true);
    switch (codecid) {
    case AV_CODEC_ID_H264:
        packet->m_data = HJESParser::parse_avc_header(codecParam->extradata, codecParam->extradata_size);
        break;
    case AV_CODEC_ID_HEVC:
        packet->m_data = HJESParser::parse_hevc_header(codecParam->extradata, codecParam->extradata_size);
        break;
    case AV_CODEC_ID_AV1:
        packet->m_data = HJESParser::parse_av1_header(codecParam->extradata, codecParam->extradata_size);
        break;
    default:
        HJFLogw("not support video codec id:{}", codecid);
        break;
    }
    if(!packet->m_data) {
        HJFLoge("error, parse vide header failed, codecid:{}", codecid);
        return nullptr;
    }
    //
    return buildTag(packet, true, false);
}

HJBuffer::Ptr HJRTMPPacketManager::buildHDRVideoHeader()
{
    auto videoInfo = m_mediaInfo->getVideoInfo();
    if (!videoInfo) {
        HJLogw("warning, have no video info");
        return nullptr;
    }
    HJBuffer::Ptr tag = nullptr;
    int codecid = videoInfo->getCodecID();
    // Y2023 spec
    if (AV_CODEC_ID_H264 == codecid)
    {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get((enum AVPixelFormat)videoInfo->m_pixFmt);
        int bits_per_raw_sample = av_get_bits_per_pixel(desc);
        //enum video_format format = info->format;
        enum HJVideoColorspace colorspace = HJ_VIDEO_CS_709;//info->colorspace;
        
        //int bits_per_raw_sample;
        //switch (format) {
        //case VIDEO_FORMAT_I010:
        //case VIDEO_FORMAT_P010:
        //case VIDEO_FORMAT_I210:
        //    bits_per_raw_sample = 10;
        //    break;
        //case VIDEO_FORMAT_I412:
        //case VIDEO_FORMAT_YA2L:
        //    bits_per_raw_sample = 12;
        //    break;
        //default:
        //    bits_per_raw_sample = 8;
        //}
        
        int pri = 0, trc = 0, spc = 0;
        switch (colorspace) {
            case HJ_VIDEO_CS_601:
                pri = HJCOL_PRI_SMPTE170M;
                trc = HJCOL_PRI_SMPTE170M;
                spc = HJCOL_PRI_SMPTE170M;
                break;
            case HJ_VIDEO_CS_DEFAULT:
            case HJ_VIDEO_CS_709:
                pri = HJCOL_PRI_BT709;
                trc = HJCOL_PRI_BT709;
                spc = HJCOL_PRI_BT709;
                break;
            case HJ_VIDEO_CS_SRGB:
                pri = HJCOL_PRI_BT709;
                trc = HJCOL_TRC_IEC61966_2_1;
                spc = HJCOL_PRI_BT709;
                break;
            case HJ_VIDEO_CS_2100_PQ:
                pri = HJCOL_PRI_BT2020;
                trc = HJCOL_TRC_SMPTE2084;
                spc = HJCOL_SPC_BT2020_NCL;
                break;
            case HJ_VIDEO_CS_2100_HLG:
                pri = HJCOL_PRI_BT2020;
                trc = HJCOL_TRC_ARIB_STD_B67;
                spc = HJCOL_SPC_BT2020_NCL;
        }
        
        int max_luminance = 0;
        if (trc == HJCOL_TRC_ARIB_STD_B67)
            max_luminance = 1000;
        else if (trc == HJCOL_TRC_SMPTE2084) {
            //max_luminance =
            //(int)obs_get_video_hdr_nominal_peak_level();
        }
        tag = HJFLVUtils::buildPacketMetadata(codecid, bits_per_raw_sample, pri, trc, spc, 0, max_luminance);
    }
    return tag;
}

HJBuffer::Ptr HJRTMPPacketManager::buildFooter()
{
    auto videoInfo = m_mediaInfo->getVideoInfo();
    if (!videoInfo) {
        HJLogw("warning, have no video info");
        return nullptr;
    }
    int codecid = videoInfo->getCodecID();
    if (AV_CODEC_ID_H264 == codecid || !m_enhancedRtmp) {
        return nullptr;
    }
    HJFLVPacket::Ptr packet = HJFLVPacket::makeVideoPacket(codecid);
    if(!packet) {
        return nullptr;
    }
    return buildTag(packet, false, true);
}

NS_HJ_END
