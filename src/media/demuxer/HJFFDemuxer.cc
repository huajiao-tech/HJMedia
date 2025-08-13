//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFFDemuxer.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJFFDemuxer::HJFFDemuxer()
    : HJBaseDemuxer()
{

}

HJFFDemuxer::HJFFDemuxer(const HJMediaUrl::Ptr& mediaUrl)
    : HJBaseDemuxer(mediaUrl)
{

}

HJFFDemuxer::~HJFFDemuxer()
{
    done();
}

int HJFFDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
{
    int res = HJBaseDemuxer::init(mediaUrl);
    if (HJ_OK != res) {
        HJLoge("init error:" + HJ2String(res));
        return res;
    }
    std::string url = m_mediaUrl->getUrl();
    if (url.empty()) {
        HJLoge("error, input url:" + url + " invalid");
        return HJErrInvalidUrl;
    }
    int64_t t1 = 0;
    int64_t t0 = HJTime::getCurrentMillisecond();
    HJFLogi("load stream url:{}, fmt:{}, start time:{}, end time:{}, timeout:{}", url, m_mediaUrl->getFmtName(), m_mediaUrl->getStartTime(), m_mediaUrl->getEndTime(), m_mediaUrl->getTimeout());
    do {
        AVFormatContext* ic = avformat_alloc_context();
        if (!ic) {
            HJLoge("avformate Could not allocate context");
            res = HJErrFFNewIC;
            break;
        }
        ic->interrupt_callback = *m_interruptCB;
//        ic->interrupt_callback.callback = m_interruptCB->callback;
//        ic->interrupt_callback.netCallback = m_interruptCB->netCallback;
//        ic->interrupt_callback.opaque = this;
        ic->opaque = (void *)this;
        ic->correct_ts_overflow = 0;
        //
        //auto cbit = m_mediaUrl->find(HJ_TYPE_NAME(HJBaseDemuxer::HJNetCallbackPtr));
        //if (cbit != m_mediaUrl->end()) {
        //    m_netCallback = HJANYCAST<HJBaseDemuxer::HJNetCallbackPtr>(cbit->second);
        //}
        if (m_mediaUrl->haveValue(HJBaseDemuxer::KEY_WORLDS_URLBUFFER))
        {
            HJBuffer::Ptr xioBuffer = m_mediaUrl->getValue<HJBuffer::Ptr>(HJBaseDemuxer::KEY_WORLDS_URLBUFFER);
            size_t avio_ctx_buffer_size = xioBuffer->size();
            uint8_t* avio_ctx_buffer = (uint8_t *)av_mallocz(avio_ctx_buffer_size);
            if (!avio_ctx_buffer || avio_ctx_buffer_size <= 0) {
                HJLoge("error, avio context buffer is NULL");
                res = HJErrMemAlloc;
                break;
            }
            memcpy(avio_ctx_buffer, xioBuffer->data(), avio_ctx_buffer_size);
            //
            AVIOContext* xioCtx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, this, NULL, NULL, NULL);
            if (!xioCtx) {
                HJLoge("error, alloc avio context failed");
                res = HJErrMemAlloc;
                break;
            }
            ic->pb = xioCtx;
            ic->flags = AVFMT_FLAG_CUSTOM_IO;
        }
        
        AVDictionary* fmtOpts = getFormatOptions();
        res = avformat_open_input(&ic, url.c_str(), NULL, &fmtOpts);
        av_dict_free(&fmtOpts);
        if (res < HJ_OK) {
            avformat_free_context(ic);
            HJLoge("error, avformat load url failed:" + HJ2String(res));
            res = AVErr2HJErr(res, HJErrFFLoadUrl);
            break;
        }
        m_ic = ic;
        //    av_format_inject_global_side_data(m_ic);
        ic->flags |= AVFMT_FLAG_GENPTS;
        if (std::string("mov,mp4,m4a,3gp,3g2,mj2").find(m_ic->iformat->name) != std::string::npos) {
            ic->fps_probe_size = 0;
        } else {
            ic->fps_probe_size = 5;
        }
        if (m_lowDelay) {
//            ic->fps_probe_size = 0;
            ic->flags |= AVFMT_FLAG_NOBUFFER;
        }
        t1 = HJTime::getCurrentMillisecond();

        //AVDictionary* opts = setup_find_stream_info_opts(m_ic, m_codecOpts);
        res = avformat_find_stream_info(ic, NULL);
        //av_dict_free(&opts);
        if (res < HJ_OK) {
            HJLoge("avformat find stream info error:" + HJ2String(res));
            res = AVErr2HJErr(res, HJErrFFFindInfo);
            break;
        }
//        if (!m_mediaUrl->getOutUrl().empty()) {
//            av_dump_format(m_ic, 0, m_mediaUrl->getOutUrl().c_str(), false);
//        }
        res = makeMediaInfo();
        if (HJ_OK != res) {
            break;
        }
        analyzeProtocol();

        m_runState = HJRun_Ready;
    } while (false);
    int64_t t2 = HJTime::getCurrentMillisecond();
    HJFLogi("init load stream end, res:{}, time:{},{},{}, url:{}", res, t1 - t0, t2 - t1, t2 - t0, url);
    
    return res;
}

int HJFFDemuxer::init() {
    return init(m_mediaUrl);
}

void HJFFDemuxer::done()
{
    HJBaseDemuxer::done();
    setQuit(true);
    if (m_ic) {
        avformat_close_input(&m_ic);
        m_ic = NULL;
    }
}

/**
 * seek(0)与init状态获取的第一帧可能不一致；
 */
int HJFFDemuxer::seek(int64_t pos)
{
    HJBaseDemuxer::seek(pos);
    if (!m_mediaInfo || !m_ic) {
        return HJErrNotAlready;
    }
    //HJLogi("seek entry");
    
    int64_t timestamp = (AV_NOPTS_VALUE != m_mediaInfo->m_startTime) ? (m_mediaInfo->m_startTime + pos) : pos;
    int res = avformat_seek_file(m_ic, -1, INT64_MIN, HJ_MS_TO_US(timestamp), INT64_MAX, 0);
    if (res < HJ_OK) {
        HJLoge("avformat seek file error:" + HJ2String(res) + ", seek time:" + HJ2String(timestamp));
        return HJErrFFSeek;
    }

    HJLogi("seek end pos:" + HJ2String(pos) + ", timestamp:" + HJ2String(timestamp));
    
    return HJ_OK;
}

int HJFFDemuxer::getFrame(HJMediaFrame::Ptr& frame)
{
    if (!m_ic) {
        return HJErrNotAlready;
    }
    //HJLogi("entry");
    int64_t t0 = HJTime::getCurrentMillisecond();
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        HJLogi("alloc AVPacket error");
        return HJErrNewObj;
    }
    int res = HJ_OK;
    AVFormatContext* ic = (AVFormatContext*)m_ic;
    for (;;) {
        res = av_read_frame(ic, pkt);
        if (res < HJ_OK)
        {
            if (AVERROR_EOF == res || avio_feof(ic->pb)) {
                procEofFrame();
                frame = HJMediaFrame::makeEofFrame();
                HJLogw("get frame eof, all track max duration:" + HJ2STR(getDuration()));
                res = HJ_EOF;
                break;
            }
            if (ic->pb && ic->pb->error) {
                HJLoge("get frame error:" + HJ2String(ic->pb->error));
                res = HJErrFatal;
                break;
            }
            res = HJ_WOULD_BLOCK;
            break;
        }
        //
//        HJMediaFrame::Ptr frame = std::make_shared<HJMediaFrame>();
        AVStream *st = ic->streams[pkt->stream_index];
        AVCodecParameters* codecparams = st->codecpar;
        
        HJMediaFrame::Ptr mvf = nullptr;
        if ((AVMEDIA_TYPE_VIDEO == codecparams->codec_type) && m_mediaUrl->isEnbleVideo())
        {
            HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo(m_mediaInfo->m_vstIdx);
            HJVideoInfo::Ptr mvinfo = m_mediaInfo->getVideoInfo();
            if (!mvinfo || !trackInfo) {
                res = HJErrESParse;
                HJLogi("error, get video info failed");
                break;
            }
            mvf = HJMediaFrame::makeVideoFrame();
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(mvf->m_info);
            videoInfo->setID(st->id);
            videoInfo->m_trackID = m_mediaInfo->m_vstIdx;
            videoInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            videoInfo->setCodecID(codecparams->codec_id);
            videoInfo->m_width = codecparams->width;
            videoInfo->m_height = codecparams->height;
            videoInfo->m_frameRate = mvinfo->m_frameRate;
            videoInfo->m_TimeBase = mvinfo->m_TimeBase;
            videoInfo->m_dataType = HJDATA_TYPE_ES;
//            AVRational fr = av_guess_frame_rate(ic, st, NULL);
//            videoInfo->m_frameRate = (0 != fr.den) ? (fr.num / (float)fr.den + 0.5f) : videoInfo->m_frameRate;
            
            if (pkt->flags & AV_PKT_FLAG_KEY) {
                mvf->setFrameType(HJFRAME_KEY);
                m_validVFlag = true;
            }
            mvf->setID(m_mediaInfo->m_vstIdx/*pkt->stream_index*/);     //track id
            mvf->setIndex(m_vfCnt);
            if(pkt->duration <= 1) {
                pkt->duration = av_rescale_q(1, { 1, videoInfo->m_frameRate }, { st->time_base.num, st->time_base.den });
            }
            // start, drop normal frame, until key frame; ex: srt, rist;
            if (!m_validVFlag) {
                break;
            }
            if (mvf->isKeyFrame()) 
            {
                if (!m_extraVBuffer || (mvinfo->getCodecID() != codecparams->codec_id)) 
                {
                    if (mvinfo->getCodecID() != codecparams->codec_id) 
                    {
                        HJFLogw("warning, video code id:{} change to :{}", mvinfo->getCodecID(), codecparams->codec_id);
                        //m_mediaInfo->m_vstIdx = pkt->stream_index;
                        mvinfo->setID(st->id);
                        mvinfo->setCodecID(codecparams->codec_id);
                        AVRational fr = av_guess_frame_rate(ic, st, NULL);
                        mvinfo->m_frameRate = (0 != fr.den) ? (fr.num / (float)fr.den + 0.5f) : mvinfo->m_frameRate;
                        if (HJ_BEYOND(mvinfo->m_frameRate, 10, 100)) {
                            trackInfo->m_guessDuration = std::make_shared<HJDataStati64>(100);
                            mvinfo->m_frameRate = 15;
                        }
                        mvinfo->m_TimeBase = HJMediaUtils::checkTimeBase({ st->time_base.num, st->time_base.den });
                        mvinfo->setTimeRange({ {st->start_time, {st->time_base.num, st->time_base.den}}, {st->duration, {st->time_base.num, st->time_base.den}} });
                        mvinfo->m_rotate = getRotation(st);
                        //
                        videoInfo->m_frameRate = mvinfo->m_frameRate;
                        videoInfo->m_TimeBase = mvinfo->m_TimeBase;
                    }
                    m_extraVBuffer = std::make_shared<HJBuffer>(codecparams->extradata, codecparams->extradata_size);
                    //
                    videoInfo->setAVCodecParams(av_dup_codec_params(codecparams));
                    mvf->setFlush();
                }
                else if (m_extraVBuffer)
                {
                    auto newExtraBuffer = hj_get_extradata_buffer(pkt);
                    if (newExtraBuffer && !HJUtilitys::memoryCompare(m_extraVBuffer->data(), m_extraVBuffer->size(), newExtraBuffer->data(), newExtraBuffer->size()))
                    {
                        m_extraVBuffer = newExtraBuffer;
                        //
                        const auto dupCodecParams = av_dup_codec_params(codecparams);
                        if (dupCodecParams->extradata) {
                            av_freep(&dupCodecParams->extradata);
                        }
                        //
                        dupCodecParams->extradata = (uint8_t*)av_mallocz(newExtraBuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
                        memcpy(dupCodecParams->extradata, newExtraBuffer->data(), newExtraBuffer->size());
                        dupCodecParams->extradata_size = (int)newExtraBuffer->size();
                        //
                        videoInfo->setAVCodecParams(av_dup_codec_params(dupCodecParams));
                        mvf->setFlush();
                    }
                }

                //if (!m_parser) {
                //    m_parser = std::make_shared<HJFrameParser>(m_extraVBuffer, codecparams->codec_id);
                //}
            }
            //if (m_parser) {
            //    auto inData = std::make_shared<HJBuffer>(pkt->data, pkt->size);
            //    m_parser->parse(inData);
            //}

            m_vfCnt++;
        } else if((AVMEDIA_TYPE_AUDIO == codecparams->codec_type) && m_mediaUrl->isEnableAudio())
        {
            HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo(m_mediaInfo->m_astIdx);
            HJAudioInfo::Ptr mainfo = m_mediaInfo->getAudioInfo();
            if (!mainfo || !trackInfo) {
                res = HJErrESParse;
                HJLogi("error, get video info failed");
                break;
            }
            mvf = HJMediaFrame::makeAudioFrame();
            mvf->setFrameType(HJFRAME_KEY);
            mvf->setID(m_mediaInfo->m_astIdx/*pkt->stream_index*/);         //track id
            mvf->setIndex(m_afCnt);

            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(mvf->m_info);
            audioInfo->setID(st->id);
            audioInfo->m_trackID = m_mediaInfo->m_astIdx;
            audioInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            audioInfo->setCodecID(codecparams->codec_id);
            audioInfo->setChannels(&codecparams->ch_layout);
            audioInfo->m_samplesRate = codecparams->sample_rate;
            audioInfo->m_samplesPerFrame = codecparams->frame_size;
            audioInfo->m_blockAlign = codecparams->block_align;
            audioInfo->m_sampleFmt = codecparams->format;
            audioInfo->m_dataType = HJDATA_TYPE_ES;
            if(pkt->duration <= 0) {
                pkt->duration = av_rescale_q(audioInfo->m_samplesPerFrame, { 1, audioInfo->m_samplesRate }, { st->time_base.num, st->time_base.den });
            }
            //flush frame
            if (!m_extraABuffer || (mainfo->getCodecID() != codecparams->codec_id)) 
            {
                if (mainfo->getCodecID() != codecparams->codec_id) 
                {
                    HJFLogw("warning, audio code id:{} change to :{}", mainfo->getCodecID(), codecparams->codec_id);
                    //m_mediaInfo->m_astIdx = pkt->stream_index;
                    mainfo->setID(st->id);
                    mainfo->setCodecID(codecparams->codec_id);
                }
                m_extraABuffer = std::make_shared<HJBuffer>(codecparams->extradata, codecparams->extradata_size);
                //
                audioInfo->setAVCodecParams(av_dup_codec_params(codecparams));
                mvf->setFlush();
            }
            else if (m_extraABuffer) 
            {
                auto newExtraBuffer = hj_get_extradata_buffer(pkt);
                if (newExtraBuffer && !HJUtilitys::memoryCompare(m_extraVBuffer->data(), m_extraVBuffer->size(), newExtraBuffer->data(), newExtraBuffer->size()))
                {
                    m_extraVBuffer = newExtraBuffer;
                    //
                    const auto dupCodecParams = av_dup_codec_params(codecparams);
                    if (dupCodecParams->extradata) {
                        av_freep(&dupCodecParams->extradata);
                    }
                    //
                    dupCodecParams->extradata = (uint8_t*)av_mallocz(newExtraBuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
                    memcpy(dupCodecParams->extradata, newExtraBuffer->data(), newExtraBuffer->size());
                    dupCodecParams->extradata_size = (int)newExtraBuffer->size();
                    //
                    audioInfo->setAVCodecParams(dupCodecParams);
                    mvf->setFlush();
                }
            }
            m_afCnt++;
        } else {
            HJLogi("warning, not support or disable AVPacket type:" + HJ2String(codecparams->codec_type));
            res = HJ_WOULD_BLOCK;
            continue;
        }
        pkt->time_base = st->time_base;
        if (!m_lowDelay && AV_NOPTS_VALUE != m_mediaInfo->m_startTime)
        {
            int64_t offsetTime = 0; // (HJ_NOTS_VALUE != m_atrs->m_offsetTime) ? m_atrs->m_offsetTime : 0;
            int64_t startTime = av_ms_to_time(m_mediaInfo->m_startTime + offsetTime, pkt->time_base);
            if (AV_NOPTS_VALUE != pkt->pts) {
                pkt->pts -= startTime;
            }
            if (AV_NOPTS_VALUE != pkt->dts) {
                pkt->dts -= startTime;
            }
        }
        mvf->setPTSDTS(pkt->pts, pkt->dts, av_rational_to_hj_rational(pkt->time_base));
        mvf->setDuration(pkt->duration, av_rational_to_hj_rational(pkt->time_base));
        mvf->setMTS(pkt->pts, pkt->dts, pkt->duration, av_rational_to_hj_rational(pkt->time_base));
        //
        mvf->setMPacket(pkt); pkt = NULL;
        //
        frame = std::move(mvf);
        //
#if HJ_HAVE_TRACKER
        frame->addTrajectory(HJakeTrajectory());
#endif
        HJNipInterval::Ptr nip = m_nipMuster->getNipById(frame->getType());
        if (frame && nip && nip->valid()) {
            HJLogi("name:" + getName() + ", " + frame->formatInfo());
        }
        break;
    }

    if (pkt) {
        av_packet_free(&pkt);
        pkt = NULL;
    }
    //HJFLogi("getFrame end");
    
    return res;
}

void HJFFDemuxer::reset()
{
    HJBaseDemuxer::reset();
    
    int64_t startTime = (HJ_NOTS_VALUE != m_mediaUrl->getStartTime()) ? m_mediaUrl->getStartTime() : 0;
    if(startTime >= 0 && startTime < m_mediaInfo->getDuration()) {
        int res = seek(startTime);
        if(HJ_OK != res) {
            HJLoge("seek time:" + HJ2String(startTime) + " error:" + HJ2STR(res));
        }
    }
    return;
}

int HJFFDemuxer::makeMediaInfo()
{
    if (!m_ic) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    AVFormatContext* ic = (AVFormatContext*)m_ic;
	if (ic->nb_streams <= 0) {
        HJLoge("error, ic->nb_streams is zero");
		return HJErrFatal;
	}
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
//        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
    }
    
    if (m_mediaUrl->isEnbleVideo()) {
        m_mediaInfo->m_vstIdx = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    }
    if (m_mediaUrl->isEnableAudio()) {
        m_mediaInfo->m_astIdx = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, m_mediaInfo->m_vstIdx, NULL, 0);
    }
    if (m_mediaUrl->isEnableSubtitle()) {
        m_mediaInfo->m_sstIdx = av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE, -1, m_mediaInfo->m_astIdx ? m_mediaInfo->m_astIdx : m_mediaInfo->m_vstIdx, NULL, 0);
    }
    int64_t minStartTime = HJ_INT64_MAX;
    std::vector<int> nidIdxs;
    if (m_mediaInfo->m_vstIdx >= 0)
    {
        AVStream* st = ic->streams[m_mediaInfo->m_vstIdx];
        if (st) {
            st->discard = AVDISCARD_DEFAULT;
            AVCodecParameters* codecparams = st->codecpar;
            if (!codecparams) {
                HJLoge("error, video codec params is NULL");
                return HJErrInvalid;
            }
            minStartTime = HJ_MIN(minStartTime, av_time_to_ms(st->start_time, st->time_base));
            m_mediaInfo->m_duration = HJ_MAX(m_mediaInfo->m_duration, av_time_to_ms(st->duration, st->time_base));
            //
            HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>(codecparams->width, codecparams->height);
            videoInfo->setID(st->id);
            videoInfo->m_trackID = m_mediaInfo->m_vstIdx;
            videoInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            videoInfo->setCodecID(codecparams->codec_id);
            AVRational fr = av_guess_frame_rate(ic, st, NULL);
            videoInfo->m_frameRate = (0 != fr.den) ? (fr.num / (float)fr.den + 0.5f) : videoInfo->m_frameRate;
            videoInfo->m_TimeBase = HJMediaUtils::checkTimeBase({st->time_base.num, st->time_base.den});
            videoInfo->setTimeRange({{st->start_time, {st->time_base.num, st->time_base.den}}, {st->duration, {st->time_base.num, st->time_base.den}}});
            videoInfo->m_rotate = getRotation(st);
            //
            AVCodecParameters* storeParams = av_dup_codec_params(codecparams);
            if (!storeParams) {
                HJLoge("dup codec params error");
                return HJErrNewObj;
            }
            videoInfo->setAVCodecParams(storeParams);
            //
            nidIdxs.emplace_back(videoInfo->getType());
            //
            m_mediaInfo->addStreamInfo(std::move(videoInfo));
            //m_mediaInfo->m_streamInfos.push_back(std::move(videoInfo));
            //m_mediaInfo->m_mediaTypes |= HJMEDIA_TYPE_VIDEO;
        }
    }
    if (m_mediaInfo->m_astIdx >= 0)
    {
        AVStream* st = ic->streams[m_mediaInfo->m_astIdx];
        if (st) {
            st->discard = AVDISCARD_DEFAULT;
            AVCodecParameters* codecparams = st->codecpar;
            if (!codecparams) {
                HJLoge("error, audio codec params is NULL");
                return HJErrInvalid;
            }
            minStartTime = HJ_MIN(minStartTime, av_time_to_ms(st->start_time, st->time_base));
            m_mediaInfo->m_duration = HJ_MAX(m_mediaInfo->m_duration, av_time_to_ms(st->duration, st->time_base));
            //
            HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
            audioInfo->setID(st->id);
            audioInfo->m_trackID = m_mediaInfo->m_astIdx;
            audioInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            audioInfo->setCodecID(codecparams->codec_id);
            audioInfo->setChannels(&codecparams->ch_layout);
            audioInfo->m_samplesRate = codecparams->sample_rate;
            audioInfo->m_samplesPerFrame = codecparams->frame_size;
            audioInfo->m_blockAlign = codecparams->block_align;
            audioInfo->m_sampleFmt = codecparams->format;
            audioInfo->m_bytesPerSample = (codecparams->bits_per_coded_sample > 0) ? (codecparams->bits_per_coded_sample * audioInfo->m_channels / 8) : (2 * audioInfo->m_channels);
            audioInfo->setTimeBase({ st->time_base.num, st->time_base.den });
            audioInfo->setTimeRange({{st->start_time, {st->time_base.num, st->time_base.den}}, {st->duration, {st->time_base.num, st->time_base.den}}});
//            audioInfo->m_sampleFmt = avctx->sample_fmt;
//            audioInfo->m_sampleCnt = codecparams->frame_size;
            //
            AVCodecParameters* storeParams = av_dup_codec_params(codecparams);
            if (!storeParams) {
                HJLoge("dup codec params error");
                return HJErrNewObj;
            }
            audioInfo->setAVCodecParams(storeParams);
            //
            nidIdxs.emplace_back(audioInfo->getType());
            //
             m_mediaInfo->addStreamInfo(std::move(audioInfo));
            //m_mediaInfo->m_streamInfos.push_back(std::move(audioInfo));
            //m_mediaInfo->m_mediaTypes |= HJMEDIA_TYPE_AUDIO;
        }
    }
    m_mediaInfo->m_startTime = HJ_MAX(0, minStartTime);
    if (AV_NOPTS_VALUE != ic->start_time) {
        m_mediaInfo->m_startTime = HJ_US_TO_MS(ic->start_time);
    }
    if (ic->duration > 0) {
        m_mediaInfo->m_duration = HJ_US_TO_MS(ic->duration);
    }
	m_mediaInfo->setTimeRange({{m_mediaInfo->m_startTime, HJ_TIMEBASE_MS}, {m_mediaInfo->m_duration, HJ_TIMEBASE_MS}});
    m_nipMuster = std::make_shared<HJNipMuster>(nidIdxs);

    HJLogi("media info:" + m_mediaInfo->formatInfo());
    
    return res;
}

AVDictionary* HJFFDemuxer::getFormatOptions()
{
    AVDictionary* fmtOpts = NULL;
    std::string thisStr = HJ2STR((size_t)this);
    av_dict_set(&fmtOpts, "opaque", thisStr.c_str(), 0);
    //
    std::string blobUrl = HJUtilitys::concateString(HJUtilitys::concatenatePath("E:/movies/blob/", HJUtilitys::getTimeToString()), ".flv");
    av_dict_set(&fmtOpts, "bloburl", blobUrl.c_str(), 0);
    
    if (m_mediaUrl->getTimeout() >= 0) {
        av_dict_set(&fmtOpts, "timeout", HJFMT("{}", m_mediaUrl->getTimeout()).c_str(), 0);
    }
    av_dict_set_int(&fmtOpts, "fpsprobesize", 0, 0);
    //av_dict_set(&fmtOpts, "headers", "Host: 150-138-141-134.6rooms.com", 0);
    //av_dict_set(&fmtOpts, "headers", "Host: al2-flv.live.huajiao.com", 0);
    if (m_lowDelay)
    {
        av_dict_set(&fmtOpts, "fflags", "nobuffer", 0);
        av_dict_set(&fmtOpts, "timeout", "5000000", 0);
//            av_dict_set(&fmtOpts, "listen_timeout", "5000000", 0);
        av_dict_set(&fmtOpts, "recv_buffer_size", "3000000", 0);
        av_dict_set(&fmtOpts, "send_buffer_size", "3000000", 0);
        if (!HJUtilitys::startWith(m_mediaUrl->getUrl(), "rist"))
        {
            av_dict_set(&fmtOpts, "buffer_size", "101024000", 0);
            //“‘tcp∑Ω Ω¥Úø™,»Áπ˚“‘udp∑Ω Ω¥Úø™Ω´tcpÃÊªªŒ™udp
            //av_dict_set(&fmtOpts, "rtsp_transport", transport.toUtf8().constData(), 0);
            //…Ë÷√≥¨ ±∂œø™¡¨Ω” ±º‰,µ•ŒªŒ¢√Î,3000000±Ì æ3√Î
            av_dict_set(&fmtOpts, "stimeout", "3000000", 0);
            //…Ë÷√◊Ó¥Û ±—”,µ•ŒªŒ¢√Î,1000000±Ì æ1√Î
            av_dict_set(&fmtOpts, "max_delay", "0", 0);
            av_dict_set(&fmtOpts, "probsize", "4096", 0);
            //av_dict_set(&fmtOpts, "packet - buffering", "0", 0);
            //av_dict_set(&fmtOpts, "fps", "30", 0);
        }
    }
//        {
//            char   *ptr, **pptr;
//            struct hostent *hptr;
//            char   str[32];
//            ptr = "120-201-0-21.6rooms.com";
//             if((hptr = gethostbyname(ptr)) == NULL)
//            {
//                return 0;
//            }
//        }
//        {
//            int ret = 0;
//            struct addrinfo hints = { 0 }, *ai, *cur_ai;
//            char* hostname = "120-201-0-21.6rooms.com";//150-138-141-134.6rooms.com";
//            char* portstr = "443";
//            hints.ai_family = AF_UNSPEC;
//            hints.ai_socktype = SOCK_STREAM;
//            hints.ai_flags = AI_DEFAULT;
//            hints.ai_protocol = IPPROTO_TCP;
//
////            hints.ai_flags = AI_DEFAULT;
////            hints.ai_family = PF_UNSPEC;
////            hints.ai_socktype = SOCK_STREAM;
////            snprintf(portstr, sizeof(portstr), "%d", );
////            av_log(h, AV_LOG_ERROR, "tcp_open hostname:%s, portstr:%s", hostname, portstr);
////            if (s->listen)
////                hints.ai_flags |= AI_PASSIVE;
//            if (!hostname[0])
//                ret = getaddrinfo(NULL, portstr, &hints, &ai);
//            else
//                ret = getaddrinfo(hostname, portstr, &hints, &ai);
//            if (ret) {
////                av_log(h, AV_LOG_ERROR,
////                       "Failed to resolve hostname %s: %s\n",
////                       hostname, gai_strerror(ret));
//                return AVERROR(EIO);
//            }
//        }
    
    return fmtOpts;
}

HJMediaFrame::Ptr HJFFDemuxer::procEofFrame()
{
    HJMediaFrame::Ptr mvf = nullptr;
    
    int64_t minDelta = 0;
    int64_t maxDelta = 0;
    for (auto &it : m_atrs->m_trackInfos)
    {
        auto& tinfo = it.second;
        if(tinfo->m_startDelta > minDelta) {
            m_atrs->m_minHoleTrack = tinfo;
            minDelta = tinfo->m_startDelta;
        }
        if(tinfo->m_endDelta < maxDelta) {
            m_atrs->m_maxHoleTrack = tinfo;
            maxDelta = tinfo->m_endDelta;
        }
    }
    HJLogi("{eof attrs:" + m_atrs->formatInfo() + "}");
    
    return mvf;
}

int64_t HJFFDemuxer::getDuration()
{
    if (m_mediaInfo) {
        int64_t duration = m_mediaInfo->getDuration();
        //HJLogi("file duration:" + HJ2STR(duration));
        return HJ_MAX(duration, m_atrs->getDuration());
    } else if (m_mediaUrl) {
        return m_mediaUrl->getDuration();
    }
    return 0;
}

double HJFFDemuxer::getRotation(AVStream *st)
{
    AVDictionaryEntry *rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
    uint8_t* displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    double theta = 0;

    if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
        char *tail;
        theta = av_strtod(rotate_tag->value, &tail);
        if (*tail)
            theta = 0;
    }
    if (displaymatrix && !theta)
        theta = -av_display_rotation_get((int32_t*) displaymatrix);

    theta -= 360*floor(theta/360 + 0.9/360);

    //    if (fabs(theta - 90*round(theta/90)) > 2)
    //        av_log_ask_for_sample(NULL, "Odd rotation angle\n");

    return theta;
}

int HJFFDemuxer::analyzeProtocol()
{
    if (!m_ic) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    do
    {
        if (m_ic->protocol_whitelist) {
            const std::vector<std::string> liveProtocols = HJUtilitys::split("http,https,tls,rtp,tcp,udp,httpproxy,rtmp", ",");
            std::string whitelist = HJ2SSTR(m_ic->protocol_whitelist);
            HJLogi("analyze protocols:" + whitelist);
            auto protocols = HJUtilitys::split(whitelist, ",");
            for (std::string pro : protocols) {
                if (std::find(liveProtocols.begin(), liveProtocols.end(), pro) != liveProtocols.end()) {
                    m_isLiving = true;
                    break;
                }
            }
        } else {
            std::string url = m_mediaUrl->getUrl();
            if (HJUtilitys::startWith(url, "rtmp") ||
                HJUtilitys::startWith(url, "http") ||
                HJUtilitys::startWith(url, "srt") ||
                HJUtilitys::startWith(url, "rist")) {
                m_isLiving = true;
                break;
            }
        }
    } while (false);

    return res;
}


//***********************************************************************************//
NS_HJ_END
