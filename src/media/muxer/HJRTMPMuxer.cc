//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRTMPMuxer.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJRTMPMuxer::HJRTMPMuxer()
{

}
HJRTMPMuxer::HJRTMPMuxer(HJListener listener)
    : m_listener(listener)
{

}

HJRTMPMuxer::~HJRTMPMuxer()
{
    
}

int HJRTMPMuxer::init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts)
{
    int res = HJBaseMuxer::init(mediaInfo, opts);
    if (HJ_OK != res || !mediaInfo || !m_mediaInfo->getMediaUrl()) {
        return res;
    }
    HJFLogi("entry, version:{}", HJ_VERSION);
    do
    {
        m_url = m_mediaInfo->getMediaUrl()->getUrl();
        m_localUrl = m_mediaInfo->getMediaUrl()->getOutUrl();
        m_mediaTypes = mediaInfo->getMediaTypes();
        if (m_url.empty()) {
            HJFLoge("error, invalid url:{}", m_url);
            res = HJErrInvalidParams;
            break;
        }
        if (opts) {
            m_localUrl = opts->getString(HJRTMPUtils::STORE_KEY_LOCALURL);
            //
            m_statCtx = opts->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
        }
        auto videoInfo = m_mediaInfo->getVideoInfo();
        if (videoInfo) {
            int codecID = videoInfo->getCodecID();
            if (AV_CODEC_ID_H264 != codecID &&
                AV_CODEC_ID_H265 != codecID &&
                AV_CODEC_ID_AV1 != codecID) {
                HJFLoge("not support codec id:{}" + codecID);
                return HJErrNotSupport;
            }
        }
        if (!m_localUrl.empty()) {
            m_localFile = std::make_shared<HJXIOFile>();
            res = m_localFile->open(std::make_shared<HJUrl>(m_localUrl, HJ_XIO_WRITE));
            if (HJ_OK != res) {
                HJFLoge("error, open local url:{} failed", m_localUrl);
                break;
            }
        }
        m_rtmpWrapper = HJCreates<HJRTMPAsyncWrapper>(sharedFrom(this));
        res = m_rtmpWrapper->init(m_url);
        if (HJ_OK != res) {
            HJFLoge("error, rtmp wrapper create failed:{}", res);
            break;
        }
    } while (false);
    HJFLogi("end");

    return res;
}

int HJRTMPMuxer::init(const std::string url, int mediaTypes, HJOptions::Ptr opts)
{
    int res = HJ_OK;
    HJFLogi("entry, version:{}", HJ_VERSION);
    do {
        m_url = url;
        m_mediaTypes = mediaTypes;
        m_opts = opts;
        if (m_url.empty()) {
            HJFLoge("error, invalid url:{}", m_url);
            res = HJErrInvalidParams;
            break;
        }
        //
        if (opts) {
            m_localUrl = opts->getString(HJRTMPUtils::STORE_KEY_LOCALURL);
            //
            m_statCtx = opts->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
        }
        if (!m_localUrl.empty()) {
            m_localFile = std::make_shared<HJXIOFile>();
            res = m_localFile->open(std::make_shared<HJUrl>(m_localUrl, HJ_XIO_WRITE));
            if (HJ_OK != res) {
                HJFLoge("error, open local url:{} failed", m_localUrl);
                break;
            }
        }
        m_mediaInfo = HJObject::creates<HJMediaInfo>();
        //
        m_rtmpWrapper = HJCreates<HJRTMPAsyncWrapper>(sharedFrom(this));
        res = m_rtmpWrapper->init(m_url, opts);
        if (HJ_OK != res) {
            HJFLoge("error, rtmp wrapper create failed:{}", res);
            break;
        }
    } while (false);
    HJFLogi("end");

    return res;
}


/**
* dts > pst
* interleave
* 
*/
int HJRTMPMuxer::addFrame(const HJMediaFrame::Ptr& frame)
{
    if (!frame || !m_mediaInfo) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do {
        if (m_mediaTypes != m_mediaInfo->getMediaTypes()) {
            res = gatherMediaInfo(frame);
            if (HJ_OK != res) {
                break;
            }
            //
            notifyStatInfo();
        }
        if (HJ_NOTS_VALUE == m_startDTSOffset) {
            m_startDTSOffset = waitStartDTSOffset(frame);
            res = HJ_MORE_DATA;
            break;
        }
        //
        addRTMPPacket(frame, m_startDTSOffset);
    } while (false);

    return res;
}

int HJRTMPMuxer::writeFrame(const HJMediaFrame::Ptr& frame)
{
    return addFrame(frame);
}

void HJRTMPMuxer::done()
{
    HJFLogi("entry");
    //sendFooters();

//    setQuit();
    if (m_rtmpWrapper) {
        m_rtmpWrapper->done();
        m_rtmpWrapper = nullptr;
        //
        notifyStatDoneInfo();
    }
    m_packetManager = nullptr;
    m_localFile = nullptr;
    m_statCtx.reset();

    HJFLogi("end");
}

void HJRTMPMuxer::setQuit(const bool isQuit)
{
    if(m_isQuit == isQuit) {
        return;
    }
    HJFLogi("entry, isQuit:{}", isQuit);
    HJBaseMuxer::setQuit(isQuit);
    if (m_packetManager) {
        m_packetManager->setQuit(isQuit);
    }
    if (m_rtmpWrapper) {
        m_rtmpWrapper->setQuit(isQuit);
    }
    HJLogi("end");
}

void HJRTMPMuxer::setNetBlock(const bool block)
{
    if (m_packetManager) {
        m_packetManager->setNetBlock(block);
    }
}
void HJRTMPMuxer::setNetSimulater(int bitrate)
{
    if (m_rtmpWrapper) {
		m_rtmpWrapper->setNetSimulBitrate(bitrate);
    }
}

int HJRTMPMuxer::onRTMPWrapperNotify(HJNotification::Ptr ntf)
{
    if (!ntf) {
        return HJ_OK;
    }
    switch (ntf->getID())
    {
        case HJRTMP_EVENT_STREAM_CONNECTED:
        {
            m_rtmpWrapper->start();
            break;
        }
        case HJRTMP_EVENT_NET_BITRATE: {
            if(m_packetManager) {
                m_packetManager->setNetBitrate(ntf->getVal());
            }
            return HJ_OK;
        }
        case HJRTMP_EVENT_DISCONNECTED: {
            if(m_packetManager) {
                m_packetManager->setWrapperGoing(true);
            }
        }
        case HJRTMP_EVENT_CONNECT_FAILED:
        case HJRTMP_EVENT_STREAM_CONNECT_FAIL: 
        {
            auto statPtr = m_statCtx.lock();
            if (statPtr && m_rtmpWrapper) {
                statPtr->notify("connectFailed", JNotify_Publish_Failed, m_rtmpWrapper->getRetryCount());
            }
            break;
        }
        default: {
            break;
        }
    }
    if (m_listener) {
        return m_listener(ntf);
    }
    return HJ_OK;
}

HJBuffer::Ptr HJRTMPMuxer::onAcquireMediaTag(int64_t timeout, bool isHeader)
{
    if (!m_packetManager) {
        return nullptr;
    }
    auto tag = m_packetManager->waitTag(timeout, isHeader);
    if (tag && m_localFile) {
        m_localFile->write(tag->data(), tag->size());
    }
    return tag;
}

int HJRTMPMuxer::addRTMPPacket(HJMediaFrame::Ptr frame, int64_t tsOffset)
{
    if (m_mediaTypes != m_mediaInfo->getMediaTypes()) {
        HJFLoge("error, media types not match, media types:{}, frame type:{}", m_mediaTypes, m_mediaInfo->getMediaTypes());
        return HJErrNotAlready;
    }

    //HJFLogi("entry, frame info:{}", frame->formatInfo());
    HJFLVPacket::Ptr packet = std::make_shared<HJFLVPacket>();
    int res = packet->init(frame, tsOffset);
    if (HJ_OK != res) {
        HJFLoge("error, flv packet init");
        return HJErrInvalidParams;
    }
    if (!m_packetManager) {
        m_packetManager = HJCreateu<HJRTMPPacketManager>(m_mediaInfo, m_opts, m_listener);
    }
    m_packetManager->push(packet);

    //
    HJPERIOD_RUN1([&]() {notifyStatLiveInfo();});
    
    return HJ_OK;
}

int HJRTMPMuxer::gatherMediaInfo(const HJMediaFrame::Ptr& frame)
{
    auto mtype = frame->getType();
    if (mtype & m_mediaTypes)
    {
        auto mediaInfo = m_mediaInfo->getStreamInfo(mtype);
        if (!mediaInfo) {
            m_mediaInfo->addStreamInfo(frame->getInfo());
            HJFLogi("add stream info:{}, key frame info:{}", mtype, frame->formatInfo());
        }
    }
    else {
        HJFLoge("deliver not support media type:{}", mtype);
        return HJErrNotSupport;
    }
    return HJ_OK;
}

int64_t HJRTMPMuxer::waitStartDTSOffset(HJMediaFrame::Ptr frame)
{
    HJFLogi("entry, frame info:{}", frame->formatInfo());
    m_framesQueue.push_back(frame);
    //
    int64_t startDTSOffset = HJ_NOTS_VALUE;
    if ((m_mediaTypes & HJMEDIA_TYPE_VIDEO) && (m_mediaTypes & HJMEDIA_TYPE_AUDIO))
    {
        HJMediaFrame::Ptr videoFrame = nullptr;
        HJMediaFrame::Ptr audioFrame = nullptr;
        for (const auto& mavf : m_framesQueue) {
            if (mavf->isVideo() && !videoFrame) {
                videoFrame = mavf;
            }
            else if (mavf->isAudio() && !audioFrame) {
                audioFrame = mavf;
            }
            if (videoFrame && audioFrame) {
                startDTSOffset = HJ_MIN(videoFrame->m_dts, audioFrame->m_dts);
                break;
            }
        }
    }
    else
    {
        startDTSOffset = frame->m_dts;
    }
    //
    if (HJ_NOTS_VALUE != startDTSOffset)
    {
        for (const auto& mavf : m_framesQueue) {
            addRTMPPacket(mavf, startDTSOffset);
        }
        m_framesQueue.clear();
    }
    return startDTSOffset;
}

void HJRTMPMuxer::notifyStatInfo()
{
    auto statPtr = m_statCtx.lock();
    if (statPtr) {
        std::string ntfyName = "muxer" + getName();
        JStatPublishLiveInfo liveInfo;
        auto& videoInfo = m_mediaInfo->getVideoInfo();
        if (videoInfo) {
            liveInfo.m_encodeType = AVCodecIDToString((AVCodecID)videoInfo->m_codecID);
            liveInfo.m_width = videoInfo->m_width;
            liveInfo.m_height = videoInfo->m_height;
            liveInfo.m_preFps = videoInfo->m_frameRate;
            liveInfo.m_preBps = videoInfo->m_bitrate / 1000;
        }

        liveInfo.m_highCpu = 0;
        //liveInfo.m_gamut
        liveInfo.m_version = HJ_VERSION;
        statPtr->notify(ntfyName, JNotify_Publish_LiveInfo, liveInfo);
        HJFLogi("[publish stat notify JNotify_Publish_LiveInfo, m_encodeType:{}, m_width:{}, m_height:{}, m_preFps:{}, m_preBps:{}, m_version:{}]", liveInfo.m_encodeType, liveInfo.m_width, liveInfo.m_height, liveInfo.m_preFps, liveInfo.m_preBps, liveInfo.m_version);
    }
}

void HJRTMPMuxer::notifyStatLiveInfo()
{
    auto streamStat = m_packetManager->getStats();
    auto outKbps = streamStat.outStats.video_kbps + streamStat.outStats.audio_kbps;
    auto outFps = streamStat.outStats.video_fps;
    auto outDelay = streamStat.delay;
    //
    auto statPtr = m_statCtx.lock();
    if (statPtr && m_packetManager) {
        statPtr->notify("vido bps", JNotify_Publish_FinalBPS, outKbps);
        statPtr->notify("vido fps", JNotify_Publish_FinalFPS, outFps);
        //
        statPtr->notify("push delay", JNotify_Publish_PushDelay, outDelay);
        HJFLogi("[publish stat notify JNotify_Publish_FinalBPS, JNotify_Publish_FinalFPS, JNotify_Publish_PushDelay, m_outKbps:{}, m_outFps:{}, m_delay:{}]", outKbps, outFps, outDelay);
    }
    //
    if(m_listener) {
        auto ntfy = HJMakeNotification(HJRTMP_EVENT_LIVE_INFO, "rtmp muxer live info");
        (*ntfy)["kbps"] = (int)outKbps;
        (*ntfy)["fps"] = (int)outFps;
        (*ntfy)["delay"] = (int)outDelay;
        m_listener(std::move(ntfy));
        HJFLogi("notify HJRTMP_EVENT_LIVE_INFO: outKbps:{}, outFps:{}, outDelay:{}", outKbps, outFps, outDelay);
    }
    return;
}

void HJRTMPMuxer::notifyStatDoneInfo()
{
    auto statPtr = m_statCtx.lock();
    if(statPtr) {
        statPtr->notify("disconnected", JNotify_Publish_Disconnected, "rtmp link disconnected");
    }
}

NS_HJ_END
