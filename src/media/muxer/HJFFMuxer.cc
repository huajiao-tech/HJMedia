//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFFMuxer.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

#define HJ_MUXER_DELAY_INIT   1

NS_HJ_BEGIN
//***********************************************************************************//
HJFFMuxer::HJFFMuxer()
{

}

HJFFMuxer::HJFFMuxer(HJListener listener)
{

}

HJFFMuxer::~HJFFMuxer()
{
    destroyAVIO();
}

int HJFFMuxer::init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts)
{
    int res = HJBaseMuxer::init(mediaInfo, opts);
    if (HJ_OK != res) {
        return res;
    }
#if !defined(HJ_MUXER_DELAY_INIT)
    res = createAVIO();
    if (HJ_OK != res) {
        destroyAVIO();
    }
#endif
    return res;
}

int HJFFMuxer::init(const std::string url, int mediaTypes, HJOptions::Ptr opts)
{
    int res = HJBaseMuxer::init(url, mediaTypes, opts);
    if (HJ_OK != res) {
        return res;
    }
    if (m_url.empty()) {
        HJFLoge("error, invalid url:{}", m_url);
        return HJErrInvalidParams;
    }
    m_mediaInfo = HJCreates<HJMediaInfo>();

    return res;
}

int HJFFMuxer::gatherMediaInfo(const HJMediaFrame::Ptr& frame)
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

int64_t HJFFMuxer::waitStartDTSOffset(HJMediaFrame::Ptr frame)
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
    return startDTSOffset;
}

int HJFFMuxer::createAVIO()
{
    int res = HJ_OK;
    //const HJMediaUrl::Ptr mediaUrl = m_mediaInfo->getMediaUrl();
    if (m_url.empty()) {
        HJFLoge("error, invalid url");
        return HJErrInvalidParams;
    }
    HJFLogi("init begin, url:{}", m_url);
    AVFormatContext* ic = NULL;
    do {
        const AVOutputFormat* output_format = NULL;
        if (HJUtilitys::startWith(m_url, "rtmp")) {
            output_format = av_guess_format("flv", m_url.c_str(), NULL);
        } else if (HJUtilitys::startWith(m_url, "srt")) {
            output_format = av_guess_format("mpegts", NULL, "video/M2PT");
        } else {
            output_format = av_guess_format(NULL, m_url.c_str(), NULL);
        }
        res = avformat_alloc_output_context2(&ic, output_format, NULL, m_url.c_str());
        //res = avformat_alloc_output_context2(&ic, nullptr, mediaUrl->getFmtName().empty() ? NULL : mediaUrl->getFmtName().c_str(), mediaUrl->getUrl().c_str());
        if (res < HJ_OK) {
            HJFLogi("alloc output error:{},{}", res, HJ_AVErr2Str(res));
            break;
        }
        ic->interrupt_callback = *m_interruptCB;
//        ic->interrupt_callback.callback = avInterruptCallback;
//        ic->interrupt_callback.opaque = this;

        std::vector<int> nidIdxs;
        m_ic = ic;
        res = m_mediaInfo->forEachInfo([&](const HJStreamInfo::Ptr& info) -> int {
            int ret = HJ_OK;
            HJMediaType mediaType = info->getType();
            if (HJMEDIA_TYPE_VIDEO != mediaType &&
                HJMEDIA_TYPE_AUDIO != mediaType &&
                HJMEDIA_TYPE_SUBTITLE != mediaType) {
                return HJ_OK;
            }
            HJTrackHolder::Ptr holder = std::make_shared<HJTrackHolder>();
            m_tracks.emplace(mediaType, holder);
            //
            AVStream* outStream = nullptr;
            AVCodecParameters* codecParam = info->getAVCodecParams();
            if(!codecParam)
            {
                HJBaseCodec::Ptr encoder = getEncoder(mediaType);
                if(!encoder) {
                    return HJErrCodecCreate;
                }
                const HJStreamInfo::Ptr encInfo = encoder->getInfo();
                codecParam = encInfo->getAVCodecParams();
                holder->m_codec = encoder;
            }
            if (!codecParam) {
                HJFLoge("need codec params error");
                return HJErrFatal;
            }
            const AVCodec* codec = avcodec_find_encoder(codecParam->codec_id);
            if (!codec) {
                HJFLogw("warning, can't find codec id:{}", codecParam->codec_id);
//                return HJErrNotSupport;
            }
            outStream = avformat_new_stream(ic, codec);
            if (!outStream) {
                HJFLoge("new stream error");
                return HJErrNewObj;
            }
            ret = avcodec_parameters_copy(outStream->codecpar, codecParam);
            if (ret < HJ_OK) {
                HJLoge("copy codec param error:{}, {}", ret, HJ_AVErr2Str(ret));
                return HJErrFatal;
            }
            /* macOS High Sierra Quicktime or IOS requires by using -tag:v hvc1 */
            if ((AV_CODEC_ID_HEVC == codecParam->codec_id) && std::string("mov,mp4,m4a,3gp,3g2,mj2").find(m_ic->oformat->name) != std::string::npos) {
                outStream->codecpar->codec_tag = MKTAG('h', 'v', 'c', '1');
            } else {
                outStream->codecpar->codec_tag = 0; //MKTAG('h', 'e', 'v', '1');
            }
            outStream->id = ic->nb_streams - 1;
            //if (ic->oformat->flags & AVFMT_GLOBALHEADER) {
            //    (*output_codec_context)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            //    HJLogw("sdf");
            //}
            //
            switch (mediaType)
            {
            case HJMEDIA_TYPE_VIDEO:
            {
                HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(info);
                HJTimeBase timebase = HJMediaUtils::getSuitableTimeBase({ 1, videoInfo->m_frameRate });
                outStream->time_base = {timebase.num, timebase .den};
                //outStream->r_frame_rate = { videoInfo->m_frameRate, 1 };
                //outStream->avg_frame_rate = { videoInfo->m_frameRate, 1 };
                break;
            }
            case HJMEDIA_TYPE_AUDIO:
            {
                HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(info);
                outStream->time_base = { 1, audioInfo->m_samplesRate };
                break;
            }
            default: {
                break;
            }
            }
            //
            holder->m_st = outStream;
            nidIdxs.emplace_back(info->getType());

            return ret;
         });
        if (res < HJ_OK) {
            HJLogi("m_mediaInfo->forEachInfo error:" + HJ2STR(res) + ", " + HJ_AVErr2Str(res));
            break;
        }

        if (!(ic->oformat->flags & AVFMT_NOFILE)) {
            res = avio_open2(&ic->pb, m_url.c_str(), AVIO_FLAG_WRITE, &ic->interrupt_callback, NULL);
            if (res < HJ_OK) {
                HJLoge("error, Could not open output file:" + m_url + ", " + HJ_AVErr2Str(res));
                break;
            }
        }
        AVDictionary* hdOpts = getFormatOptions();
        res = avformat_write_header(ic, &hdOpts);
        av_dict_free(&hdOpts);
        if (res < HJ_OK) {
            HJLoge("Error occurred when opening output file, res:" + HJ2STR(res) + ", " + HJ_AVErr2Str(res));
            break;
        }
        m_nipMuster = std::make_shared<HJNipMuster>(nidIdxs);
    } while (false);
    HJLogi("init end");

    return res;
}

void HJFFMuxer::destroyAVIO()
{
    setQuit(true);
    if (m_ic) {
        if (!(m_ic->oformat->flags & AVFMT_NOFILE)) {
            avio_close(m_ic->pb);
        }
        avformat_free_context(m_ic);
        m_ic = nullptr;
    }
}

/**
* dts > pst
* interleave
* 
*/
int HJFFMuxer::addFrame(const HJMediaFrame::Ptr& frame)
{
    return procFrame(frame, false);
}

int HJFFMuxer::writeFrame(const HJMediaFrame::Ptr& frame)
{
    return procFrame(frame, true);
}

void HJFFMuxer::done()
{
    if (m_ic) {
        av_write_trailer(m_ic);
    }
    HJLogi("done frame cnt video:" + HJ2STR(m_videoFrameCnt) + ", audio:" + HJ2STR(m_audioFrameCnt) + ", total:" + HJ2STR(m_totalFrameCnt));
}

int HJFFMuxer::procFrame(const HJMediaFrame::Ptr& frame, bool isDup)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do {
        if (m_mediaTypes != m_mediaInfo->getMediaTypes()) 
        {
            if (m_keyFrameCheck)
            {
                if (frame->isKeyFrame()) {
                    m_firstKeyFrame = true;
                }
                if (!m_firstKeyFrame) {
                    HJFLogw("warning, before first key frame, drop frame:{} ", frame->formatInfo());
                    return HJ_MORE_DATA;
                }
            }
            res = gatherMediaInfo(frame);
            if (HJ_OK != res) {
                break;
            }
        }
        if (HJ_NOTS_VALUE == m_startDTSOffset) {
            m_startDTSOffset = waitStartDTSOffset(frame);
            res = HJ_MORE_DATA;
            break;
        }
        if (m_framesQueue.size() > 0) {
            for (const auto& mavf : m_framesQueue) {
                res = deliver(mavf, isDup);
                if (HJ_OK != res) {
                    HJFLoge("error, deliver frame info:{}", mavf->formatInfo());
                    break;
                }
            }
            m_framesQueue.clear();
        }

        res = deliver(frame, isDup);
    } while (false);

    return res;
}

int HJFFMuxer::deliver(const HJMediaFrame::Ptr& frame, bool isDup)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do {
#if defined(HJ_MUXER_DELAY_INIT)
        if (!m_ic)
        {
            res = createAVIO();
            if (HJ_OK != res) {
                destroyAVIO();
                break;
            }
        }
#endif //HJ_MUXER_DELAY_INIT
        if (!m_ic) {
            res = HJErrNotAlready;
            HJFLoge("error, format context is null");
            break;
        }
        HJMediaFrame::Ptr mavf = nullptr;
        if (isDup) {
            mavf = frame->deepDup();
            if (!mavf) {
                res = HJErrNotAlready;
                HJFLoge("error, dup frame failed");
                break;
            }
        }
        else {
            mavf = frame;
        }
        if(m_timestampZero) {
            mavf->setPTSDTS(mavf->m_pts - m_startDTSOffset, mavf->m_dts - m_startDTSOffset);
        }

        HJTrackHolder::Ptr track = nullptr;
        HJMediaType mediaType = mavf->getType();
        auto it = m_tracks.find(mediaType);
        if (it != m_tracks.end()) {
            track = it->second;
        }
        if (!track || !track->m_st) {
            HJLoge("can't find property track");
            return HJ_OK;
        }

        AVStream* outStream = (AVStream*)track->m_st;
        AVPacket* pkt = NULL;
        if (mavf && (HJFRAME_EOF != mavf->getFrameType())) {
            pkt = mavf->getAVPacket();
        }
        if (pkt)
        {
            if (outStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                if ((pkt->dts < m_lastVideoDTS) && (m_lastVideoDTS != HJ_INT64_MIN)) {
                    HJLoge("error, muxer video input dts:" + HJ2STR(pkt->dts) + " < last dts:" + HJ2STR(m_lastVideoDTS));
                    return HJErrMuxerDTS;
                }
                if (pkt->pts < pkt->dts) {
                    pkt->pts = pkt->dts + 1;
                    HJLogw("muxer write video AVPacket warning, pts:" + HJ2STR(pkt->pts) + " < dts:" + HJ2STR(pkt->dts));
                }
                m_lastVideoDTS = pkt->dts;
                m_videoFrameCnt++;
            }
            else if (outStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                if ((pkt->dts < m_lastAudioDTS) && (m_lastAudioDTS != HJ_INT64_MIN)) {
                    HJLoge("error, muxer audio input dts:" + HJ2STR(pkt->dts) + " < last dts:" + HJ2STR(m_lastAudioDTS));
                    return HJErrMuxerDTS;
                }
                if (pkt->pts < pkt->dts) {
                    pkt->pts = pkt->dts + 1;
                    HJLogw("muxer write audio AVPacket warning, pts:" + HJ2STR(pkt->pts) + " < dts:" + HJ2STR(pkt->dts));
                }
                m_lastAudioDTS = pkt->dts;
                m_audioFrameCnt++;
            }

            //
#if HJ_HAVE_TRACKER
            mavf->addTrajectory(HJakeTrajectory());
#endif
            HJNipInterval::Ptr nip = m_nipMuster->getNipById(mediaType);
            if (mavf && nip && nip->valid()) {
                HJLogi("name:" + getName() + ", " + mavf->formatInfo());
            }
            //
            if (av_cmp_q(outStream->time_base, pkt->time_base)) {
                av_packet_rescale_ts(pkt, pkt->time_base, outStream->time_base);
                pkt->time_base = outStream->time_base;
            }
            pkt->pos = -1;
            pkt->stream_index = outStream->index;

            if (pkt->pts == AV_NOPTS_VALUE || pkt->dts == AV_NOPTS_VALUE) {
                HJLogw("warning, pkt pts or dts is AV_NOPTS_VALUE");
            }
            //HJFLogi("AVPacket pts:{}, dts:{}, time base:{}:{}", copy->pts, copy->dts, copy->time_base.num, copy->time_base.den);
        }
        //
//        auto t0 = HJCurrentSteadyMS();
        res = av_interleaved_write_frame(m_ic, pkt);
        if (pkt) {
            m_totalFrameCnt++;
        }
//        HJFLogw("muxer write frame use time:{} ms", HJCurrentSteadyMS() - t0);
    } while (false);

    if (res < HJ_OK) {
        HJLoge("name:" + getName() + ", muxer write frame error:" + HJ2STR(res) + ", " + HJ_AVErr2Str(res));
    }

    return res;
}

//int HJFFMuxer::avInterruptCallback(void* opaque)
//{
//    HJFFMuxer* muxer = (HJFFMuxer*)opaque;
//    //if (muxer) {
//    //    return muxer->getQuit();
//    //}
//    if (muxer && muxer->m_interruptCB) {
//        return muxer->m_interruptCB();
//    }
//    return false;
//}

HJBaseCodec::Ptr HJFFMuxer::getEncoder(const HJMediaType mediaType)
{
    int res = HJ_OK;
    HJTrackHolder::Ptr holder = nullptr;
    HJBaseCodec::Ptr encoder = nullptr;
    auto it = m_tracks.find(mediaType);
    if (it != m_tracks.end()) {
        holder = it->second;
        encoder = holder->m_codec;
    }
    if (!holder) {
        return nullptr;
    }
    if (!encoder)
    {
        const HJStreamInfo::Ptr info = m_mediaInfo->getStreamInfo(mediaType);
        if (!info) {
            return nullptr;
        }
        encoder = HJBaseCodec::createEncoder(mediaType);
        if (!encoder) {
            return nullptr;
        }
        res = encoder->init(info);
        if (HJ_OK != res) {
            return nullptr;
        }
        holder->m_codec = encoder;
    }
    return encoder;
}

AVDictionary* HJFFMuxer::getFormatOptions()
{
    AVDictionary* opts = NULL;
    if (m_isLive) {
        av_dict_set(&opts, "movflags", "faststart", 0);
    }
//    av_dict_set(&opts, "stimeout", "10000000", 0);
//    av_dict_set(&opts, "timeout", "10000000", 0);
    //av_dict_set(&options, "buffer_size", "1024000", 0);  //设置udp的接收缓冲
//    av_dict_set(&opts, "flvflags", "no_duration_filesize", 0);
    
    return opts;
}

NS_HJ_END
