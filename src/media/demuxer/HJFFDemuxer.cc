//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFFDemuxer.h"
#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJGlobalSettings.h"
#include "HJSEIWrapper.h"
#include "HJDataSourceKit.h"
#include "HJLocalFileManager.h"
#include <cctype>

NS_HJ_BEGIN
namespace {
void destroyFormatContext(AVFormatContext*& context)
{
    if (!context) {
        return;
    }
    if ((context->flags & AVFMT_FLAG_CUSTOM_IO) && context->pb) {
        avio_context_free(&context->pb);
    }
    avformat_free_context(context);
    context = NULL;
}

void closeInputContext(AVFormatContext*& context)
{
    if (!context) {
        return;
    }
    AVIOContext* customIO = ((context->flags & AVFMT_FLAG_CUSTOM_IO) && context->pb) ? context->pb : NULL;
    avformat_close_input(&context);
    if (customIO) {
        avio_context_free(&customIO);
    }
}

std::string getMetadataValue(const AVDictionary* metadata, const char* key)
{
    if (!metadata || !key) {
        return {};
    }
    const AVDictionaryEntry* entry = av_dict_get(metadata, key, NULL, 0);
    if (!entry || !entry->value) {
        return {};
    }
    return entry->value;
}

std::string trimTrackLabel(const std::string& value)
{
    if (value.empty()) {
        return {};
    }
    size_t begin = 0;
    size_t end = value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::string makeTrackDisplayName(const std::string& title,
                                 const std::string& language,
                                 const std::string& handlerName,
                                 bool isCommentary,
                                 int trackID)
{
    std::string displayName = !title.empty() ? title : (!language.empty() ? language : handlerName);
    displayName = trimTrackLabel(displayName);
    if (displayName.empty()) {
        displayName = HJFMT("音轨 {}", trackID);
    }
    if (isCommentary && displayName.find("comment") == std::string::npos &&
        displayName.find("Comment") == std::string::npos &&
        displayName.find("解说") == std::string::npos) {
        displayName += " (解说)";
    }
    return displayName;
}
}
//***********************************************************************************//
HJFFDemuxer::HJFFDemuxer()
    : HJBaseDemuxer()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJFFDemuxer)));
}

HJFFDemuxer::HJFFDemuxer(const HJMediaUrl::Ptr& mediaUrl)
    : HJBaseDemuxer(mediaUrl)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJFFDemuxer)));
}

HJFFDemuxer::~HJFFDemuxer()
{
    done();
}

int HJFFDemuxer::switchAudioTrack(int trackID)
{
    if (!m_mediaInfo) {
        return HJErrNotAlready;
    }
    if (!m_mediaInfo->findAudioInfoByTrackID(trackID)) {
        HJFNLoge("invalid audio track:{}", trackID);
        return HJErrNotFind;
    }
    if (m_mediaInfo->getSelectedAudioTrackID() == trackID && m_pendingAudioTrackID.load() != trackID) {
        return HJ_OK;
    }
    m_pendingAudioTrackID.store(trackID);
    HJFNLogi("request switch audio track:{}", trackID);
    return HJ_OK;
}

int HJFFDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
{
    int res = HJBaseDemuxer::init(mediaUrl);
    if (HJ_OK != res) {
        HJFNLoge("init error:{}", res);
        return res;
    }
    std::string url = m_mediaUrl->getUrl();
    if (url.empty()) {
        HJFNLoge("error, input url:{} invalid", url);
        return HJErrInvalidUrl;
    }
    int64_t t1 = 0;
    int64_t t0 = HJTime::getCurrentMillisecond();
    AVFormatContext* ic = NULL;
    HJFNLogi("load stream url:{}, fmt:{}, start time:{}, end time:{}, timeout:{}", url, m_mediaUrl->getFmtName(), m_mediaUrl->getStartTime(), m_mediaUrl->getEndTime(), m_mediaUrl->getTimeout());
    do {
        ic = avformat_alloc_context();
        if (!ic) {
            HJFNLoge("avformate Could not allocate context");
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
                HJFNLoge("error, avio context buffer is NULL");
                res = HJErrMemAlloc;
                break;
            }
            memcpy(avio_ctx_buffer, xioBuffer->data(), avio_ctx_buffer_size);
            //
            AVIOContext* xioCtx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, this, NULL, NULL, NULL);
            if (!xioCtx) {
                HJFNLoge("error, alloc avio context failed");
                av_freep(&avio_ctx_buffer);
                res = HJErrMemAlloc;
                break;
            }
            //
            ic->pb = xioCtx;
            ic->flags = AVFMT_FLAG_CUSTOM_IO;
            //
            setIOTimeout(m_mediaUrl->getTimeout());
        }

        if (mediaUrl->canUseFast() && HJGlobalSettingsManager::getUseHTTPPool()) {
            url = hj_replace_fasthttp(url);
        }
        AVDictionary* fmtOpts = getFormatOptions();
        res = avformat_open_input(&ic, url.c_str(), NULL, &fmtOpts);
        av_dict_free(&fmtOpts);
        if (res < HJ_OK) {
            HJFNLoge("error, avformat load url failed:{}, {}", res, HJ_AVErr2Str(res));            
            destroyFormatContext(ic);
            
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
        ic->mflag_disable = m_mediaUrl->getDisableMFlag();
        ic->mflag_timeout = m_mediaUrl->getMFlagTimeout();

        t1 = HJTime::getCurrentMillisecond();

        //AVDictionary* opts = setup_find_stream_info_opts(ic, m_codecOpts);
        res = avformat_find_stream_info(ic, NULL);
        //av_dict_free(&opts);
        if (res < HJ_OK) {
            HJFNLoge("avformat find stream info error:{}", res);
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

        //updateStatus();

        m_runState = HJRun_Ready;
    } while (false);
    if (HJ_OK != res) {
        if (m_ic) {
            closeInputContext(m_ic);
        } else {
            destroyFormatContext(ic);
        }
    }
    int64_t t2 = HJTime::getCurrentMillisecond();
    HJFNLogi("init load stream end, res:{}, time:{},{},{}, url:{}", res, t1 - t0, t2 - t1, t2 - t0, url);
    
    return res;
}

int HJFFDemuxer::init() {
    return init(m_mediaUrl);
}

void HJFFDemuxer::done()
{
    HJFLogi("entry");
    HJBaseDemuxer::done();
    setQuit(true);
    if (m_ic) {
        closeInputContext(m_ic);
    }
    m_gopFrames.clear();
    m_lastAudioDTS = HJ_NOTS_VALUE;
    m_pendingAudioTrackID.store(-1);
    m_switchingAudioTrackID = -1;
    HJFLogi("end");
}

/**
 * seek(0)与init状态获取的第一帧可能不一致；
 */
int HJFFDemuxer::seek(int64_t pos)
{
    HJBaseDemuxer::seek(pos);
    m_gopFrames.clear();
    m_lastAudioDTS = HJ_NOTS_VALUE;
//    m_pendingAudioTrackID.store(-1);
//    m_switchingAudioTrackID = -1;
    m_runState = HJRun_Ready;
    if (!m_mediaInfo || !m_ic) {
        return HJErrNotAlready;
    }
    HJLogi("seek entry");
    int64_t t0 = HJCurrentSteadyMS();
    int64_t timestamp = (AV_NOPTS_VALUE != m_mediaInfo->m_startTime) ? (m_mediaInfo->m_startTime + pos) : pos;
    int64_t seek_pos = HJ_MS_TO_US(timestamp);
    int flags = getSeekFlags();
    //int res = avformat_seek_file(m_ic, -1, seek_pos - AV_TIME_BASE, seek_pos, seek_pos + AV_TIME_BASE, 0);
    int res = avformat_seek_file(m_ic, -1, INT64_MIN, seek_pos, INT64_MAX, flags);
    if (res < HJ_OK) {
        HJFNLoge("avformat seek file error:{}, {}, seek time:{}", res, HJ_AVErr2Str(res), timestamp);
        return HJErrFFSeek;
    }
    if (m_seekKeyFrame && m_mediaUrl->isM3u8())
    {
        res = seek_keyframe(m_timeOffsetEnable ? pos : timestamp);
        if (HJ_OK != res) {
            HJFNLogw("warning, seek keyframe failed:{}", res);
            res = avformat_seek_file(m_ic, -1, INT64_MIN, seek_pos, INT64_MAX, flags);
            if (res < HJ_OK) {
                HJFNLoge("avformat seek file retry error:{}, {}, seek time:{}", res, HJ_AVErr2Str(res), timestamp);
                return HJErrFFSeek;
            }
        }
    }
    HJFNLogi("seek end, pos:{}, timestamp:{}, consume:{}", pos, timestamp, HJCurrentSteadyMS() - t0);
    
    return HJ_OK;
}

int HJFFDemuxer::seek_keyframe(int64_t target_pos)
{
    m_gopFrames.clear();
    if (m_mediaInfo->m_vstIdx < 0) {
        return HJ_OK;
    }
    int res = HJ_OK;
    int videoStreamIndex = m_mediaInfo->m_vstIdx;
    AVStream* st = m_ic->streams[videoStreamIndex];
    int64_t targetTimestamp = av_rescale(target_pos, st->time_base.den, HJ_MS_PER_SEC * (int64_t)st->time_base.num);
    std::list<HJMediaFrame::Ptr> curGopBuffer;
    int64_t lastKeyframePts = AV_NOPTS_VALUE;
    int64_t lastFramePts = AV_NOPTS_VALUE;
    HJFNLogi("entry, target_pos:{}, targetTimestamp:{}", target_pos, targetTimestamp);
    
    // Read frames until we reach the target position
    while (true) 
    {
        //int64_t t0 = HJCurrentSteadyMS();
        HJMediaFrame::Ptr frame = nullptr;
        res = readFrameInternal(frame);    
        if (res == HJ_EOF || res < HJ_OK || !frame) {
            res = HJErrNotSupport;
            break;
        }
        if (frame->isVideo()) 
        {
            auto pkt = frame->getAVPacket();
            if (!pkt) {
                res = HJErrNotSupport;
                break;
            }
            lastFramePts = pkt->pts;
            if (frame->isKeyFrame()) {
                lastKeyframePts = pkt->pts;
                curGopBuffer.clear();
            }
            // Check if we've reached the target position
            if (pkt->pts >= targetTimestamp) {
                curGopBuffer.push_back(frame);
                break;
            }
			//HJFNLogi("time:{}, buffering video frame:{}", HJCurrentSteadyMS() - t0, frame->formatInfo());
        }
        curGopBuffer.push_back(frame);
    }
    
    //// Trim buffer: only keep frames from the last keyframe onwards
    //if (lastKeyframePts != AV_NOPTS_VALUE && lastKeyframeIndex > 0) {
    //    auto it = tempBuffer.begin();
    //    size_t idx = 0;
    //    while (it != tempBuffer.end() && idx < lastKeyframeIndex - 1) {
    //        // Keep audio frames that are close to the keyframe (within 500ms)
    //        if ((*it)->isAudio()) {
    //            int64_t audioPtsMs = (*it)->getPTS();
    //            int64_t keyframePtsMs = av_rescale(lastKeyframePts, 1000 * st->time_base.num, st->time_base.den);
    //            if (keyframePtsMs - audioPtsMs < 500) {
    //                ++it;
    //                ++idx;
    //                continue;
    //            }
    //        }
    //        it = tempBuffer.erase(it);
    //        // Don't increment idx since we erased, but we need to track position relative to original
    //    }
    //}
    if (HJ_OK != res) {
        return res;
    }
    m_gopFrames = std::move(curGopBuffer);
    HJFNLogi("seek_keyframe buffered {} frames, target_pos:{}, lastKeyframePts:{}, lastFramePts:{}", m_gopFrames.size(), target_pos, lastKeyframePts, lastFramePts);
    
    return res;
}

int HJFFDemuxer::getFrame(HJMediaFrame::Ptr& frame)
{
    if (!m_ic) {
        return HJErrNotAlready;
    }
    
    if (!m_gopFrames.empty()) {
        frame = m_gopFrames.front();
        m_gopFrames.pop_front();
        return HJ_OK;
    }
    
    return readFrameInternal(frame);
}

int HJFFDemuxer::readFrameInternal(HJMediaFrame::Ptr& frame)
{
    if (!m_ic) {
        return HJErrNotAlready;
    }
    frame = nullptr;
    //HJLogi("entry");
    int64_t t0 = HJCurrentSteadyMS();
    if (HJRun_EOF == m_runState) {
        frame = HJMediaFrame::makeEofFrame();
        frame->m_streamIndex = m_streamID;
        return HJ_EOF;
    }
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        HJFNLoge("alloc AVPacket error");
        return HJErrNewObj;
    }
    int res = HJ_OK;
    AVFormatContext* ic = (AVFormatContext*)m_ic;
    for (;;) {
        applyPendingAudioTrackSwitch();
        av_packet_unref(pkt);
        setIORunTime(HJCurrentSteadyUS());
        //
        res = av_read_frame(ic, pkt);
        if (res < HJ_OK)
        {
            if (AVERROR_EOF == res || (ic->pb && avio_feof(ic->pb))) {
                procEofFrame();
                frame = HJMediaFrame::makeEofFrame();
                frame->m_streamIndex = m_streamID;
                m_runState = HJRun_EOF;
                HJFNLogw("get frame eof, all track max duration:{}", getDuration());
                res = HJ_EOF;
                break;
            }
            if (ic->pb && ic->pb->error) {
                HJFNLoge("get frame error:{}", ic->pb->error);
                res = HJErrFatal;
                break;
            }
            res = HJ_WOULD_BLOCK;
            break;
        }
        if (pkt->flags & AV_PKT_FLAG_CORRUPT) {
            res = HJErrFFAVPacket;
            HJFNLoge("error, get packet corrupt, flags:{}", pkt->flags);
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
                HJFNLoge("error, get video info failed");
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
                continue;
            }
            if (mvf->isKeyFrame()) 
            {
                if (!m_extraVBuffer || (mvinfo->getCodecID() != codecparams->codec_id)) 
                {
                    if (m_extraVBuffer) {
                        m_extraABuffer = nullptr;
                        m_streamID++;
                        HJFNLogw("warning video m_streamID:{}", m_streamID);
                    }
                    if (mvinfo->getCodecID() != codecparams->codec_id) 
                    {
                        HJFNLogw("warning, video code id:{} change to :{}",
                                 AVCodecIDToString(static_cast<AVCodecID>(mvinfo->getCodecID())),
                                 AVCodecIDToString(codecparams->codec_id));
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
                    auto newExtraBuffer = hj_get_extradata_buffer(pkt);
                    if (newExtraBuffer) {
                        m_extraVBuffer = newExtraBuffer;
                        //
                        AVCodecParameters* dupCodecParams = av_dup_codec_params(codecparams);
                        if (!dupCodecParams) {
                            res = HJErrNewObj;
                            break;
                        }
                        if (dupCodecParams->extradata) {
                            av_freep(&dupCodecParams->extradata);
                        }
                        //
                        dupCodecParams->extradata = (uint8_t*)av_mallocz(newExtraBuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
                        if (!dupCodecParams->extradata) {
                            avcodec_parameters_free(&dupCodecParams);
                            res = HJErrMemAlloc;
                            break;
                        }
                        memcpy(dupCodecParams->extradata, newExtraBuffer->data(), newExtraBuffer->size());
                        dupCodecParams->extradata_size = (int)newExtraBuffer->size();
                        //
                        videoInfo->setAVCodecParams(dupCodecParams);
                        HJFNLogw("warning, video media frame flush, switch with side data, m_streamID:{}, codec:{}, extra data size:{}", m_streamID, dupCodecParams->codec_id, dupCodecParams->extradata_size);
                    } else {
                        m_extraVBuffer = std::make_shared<HJBuffer>(codecparams->extradata, codecparams->extradata_size);
                        //
                        videoInfo->setAVCodecParams(av_dup_codec_params(codecparams));
                        HJFNLogw("warning, video media frame flush, switch, m_streamID:{}, codec:{}, extra data size:{}", m_streamID, codecparams->codec_id, codecparams->extradata_size);
                    }
                    mvf->setFlush();
                }
                else if (m_extraVBuffer)
                {
                    auto newExtraBuffer = hj_get_extradata_buffer(pkt);
                    if (newExtraBuffer && !HJUtilitys::memoryCompare(m_extraVBuffer->data(), m_extraVBuffer->size(), newExtraBuffer->data(), newExtraBuffer->size()))
                    {
                        m_extraVBuffer = newExtraBuffer;
                        m_extraABuffer = nullptr;
                        m_streamID++;
                        //
                        AVCodecParameters* dupCodecParams = av_dup_codec_params(codecparams);
                        if (!dupCodecParams) {
                            res = HJErrNewObj;
                            break;
                        }
                        if (dupCodecParams->extradata) {
                            av_freep(&dupCodecParams->extradata);
                        }
                        //
                        dupCodecParams->extradata = (uint8_t*)av_mallocz(newExtraBuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
                        if (!dupCodecParams->extradata) {
                            avcodec_parameters_free(&dupCodecParams);
                            res = HJErrMemAlloc;
                            break;
                        }
                        memcpy(dupCodecParams->extradata, newExtraBuffer->data(), newExtraBuffer->size());
                        dupCodecParams->extradata_size = (int)newExtraBuffer->size();
                        //
                        videoInfo->setAVCodecParams(dupCodecParams);
                        mvf->setFlush();
                        HJFNLogw("warning, video media frame flush, compare, m_streamID:{}, codec:{}, extra data size:{}", m_streamID, codecparams->codec_id, codecparams->extradata_size);
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
            const int selectedAudioTrackID = m_mediaInfo ? m_mediaInfo->getSelectedAudioTrackID() : -1;
            if (selectedAudioTrackID < 0 || pkt->stream_index != selectedAudioTrackID) {
                continue;
            }
            HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo(pkt->stream_index);
            HJAudioInfo::Ptr mainfo = m_mediaInfo->findAudioInfoByTrackID(pkt->stream_index);
            if (!mainfo || !trackInfo) {
                res = HJErrESParse;
                HJFNLoge("error, get audio info failed");
                break;
            }
            HJAudioInfo::Ptr audioInfo = makeAudioStreamInfo(ic, st, pkt->stream_index, false);
            if (!audioInfo) {
                res = HJErrNewObj;
                HJFNLoge("error, make audio info failed");
                break;
            }
            mvf = HJMediaFrame::makeAudioFrame(audioInfo);
            mvf->setFrameType(HJFRAME_KEY);
            mvf->setID(pkt->stream_index);         //track id
            mvf->setIndex(m_afCnt);
            if(pkt->duration <= 2) {
                pkt->duration = av_rescale_q(audioInfo->m_samplesPerFrame, { 1, audioInfo->m_samplesRate }, { st->time_base.num, st->time_base.den });
            }
            const bool audioTrackSwitch = isPendingAudioTrackSwitch(pkt->stream_index);
            //flush frame
            if (!m_extraABuffer || (mainfo->getCodecID() != codecparams->codec_id)) 
            {
                if (m_extraABuffer && !audioTrackSwitch) {
                    m_extraVBuffer = nullptr;
                    m_streamID++;
                    HJFNLogw("warning audio m_streamID:{}", m_streamID);
                }
                const auto prevCodecID = mainfo->getCodecID();
                mainfo->clone(audioInfo);
                if (prevCodecID != codecparams->codec_id)
                {
                    HJFNLogw("warning, audio code id:{} change to :{}",
                             AVCodecIDToString(static_cast<AVCodecID>(prevCodecID)),
                             AVCodecIDToString(codecparams->codec_id));
                }
                AVCodecParameters* dupCodecParams = av_dup_codec_params(codecparams);
                if (!dupCodecParams) {
                    res = HJErrNewObj;
                    break;
                }
                checkAudioCodecParameters(mainfo, dupCodecParams);
                //
                m_extraABuffer = std::make_shared<HJBuffer>(dupCodecParams->extradata, dupCodecParams->extradata_size);
                //
                AVCodecParameters* storeCodecParams = av_dup_codec_params(dupCodecParams);
                if (!storeCodecParams) {
                    avcodec_parameters_free(&dupCodecParams);
                    res = HJErrNewObj;
                    break;
                }
                mainfo->setAVCodecParams(storeCodecParams);
                audioInfo->setAVCodecParams(dupCodecParams);
                mvf->setFlush();
                HJFNLogw("warning, audio media frame flush, switch, m_streamID:{}", m_streamID);
            }
            else if (m_extraABuffer) 
            {
                auto newExtraBuffer = hj_get_extradata_buffer(pkt);
                if (newExtraBuffer && !HJUtilitys::memoryCompare(m_extraABuffer->data(), m_extraABuffer->size(), newExtraBuffer->data(), newExtraBuffer->size()))
                {
                    m_extraABuffer = newExtraBuffer;
                    m_extraVBuffer = nullptr;
                    m_streamID++;
                    //
                    AVCodecParameters* dupCodecParams = av_dup_codec_params(codecparams);
                    if (!dupCodecParams) {
                        res = HJErrNewObj;
                        break;
                    }
                    if (dupCodecParams->extradata) {
                        av_freep(&dupCodecParams->extradata);
                    }
                    //
                    dupCodecParams->extradata = (uint8_t*)av_mallocz(newExtraBuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
                    if (!dupCodecParams->extradata) {
                        avcodec_parameters_free(&dupCodecParams);
                        res = HJErrMemAlloc;
                        break;
                    }
                    memcpy(dupCodecParams->extradata, newExtraBuffer->data(), newExtraBuffer->size());
                    dupCodecParams->extradata_size = (int)newExtraBuffer->size();
                    //
                    AVCodecParameters* storeCodecParams = av_dup_codec_params(dupCodecParams);
                    if (!storeCodecParams) {
                        avcodec_parameters_free(&dupCodecParams);
                        res = HJErrNewObj;
                        break;
                    }
                    mainfo->setAVCodecParams(storeCodecParams);
                    audioInfo->setAVCodecParams(dupCodecParams);
                    mvf->setFlush();
                    HJFNLogw("warning, audio media frame flush, compare, m_streamID:{}", m_streamID);
                }
            }
            int64_t dtsValue = (AV_NOPTS_VALUE != pkt->dts) ? pkt->dts : pkt->pts;
            int64_t dts = av_time_to_ms(dtsValue, st->time_base);
            if (HJ_NOTS_VALUE != m_lastAudioDTS) 
            {
                int64_t delta = dts - m_lastAudioDTS;
                if (!audioTrackSwitch && (dts < m_lastAudioDTS || delta > m_gapThreshold)) {
                    //m_extraVBuffer = nullptr;
                    m_streamID++;
                    AVCodecParameters* storeCodecParams = av_dup_codec_params(codecparams);
                    AVCodecParameters* frameCodecParams = av_dup_codec_params(codecparams);
                    if (!storeCodecParams || !frameCodecParams) {
                        if (storeCodecParams) {
                            avcodec_parameters_free(&storeCodecParams);
                        }
                        if (frameCodecParams) {
                            avcodec_parameters_free(&frameCodecParams);
                        }
                        res = HJErrNewObj;
                        break;
                    }
                    mainfo->setAVCodecParams(storeCodecParams);
                    audioInfo->setAVCodecParams(frameCodecParams);
                    mvf->setFlush();
                    HJFNLogw("warning, audio media frame flush, time skip, m_streamID:{}", m_streamID);
                }
            }
            if (audioTrackSwitch) {
                if (!mvf->isFlushFrame()) {
                    mvf->setFlush();
                }
                mvf->m_flag |= HJ_FRAME_FLAG_AUDIO_TRACK_SWITCH;
                m_switchingAudioTrackID = -1;
            }
            m_lastAudioDTS = dts;
            //
            m_afCnt++;
        } else {
            HJFNLogi("warning, not support or disable AVPacket type:{}",
                     static_cast<int>(codecparams->codec_type));
            continue;
        }
        pkt->time_base = st->time_base;
        if (AV_NOPTS_VALUE == pkt->dts && AV_NOPTS_VALUE != pkt->pts) {
            pkt->dts = pkt->pts - 1;
        }
        if (AV_NOPTS_VALUE == pkt->pts) {
            HJFNLogw("warning, pkt pts:{} invalid", pkt->pts);
        }
        if (m_timeOffsetEnable/*!m_lowDelay*/ && AV_NOPTS_VALUE != m_mediaInfo->m_startTime)
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
        mvf->m_streamIndex = m_streamID;
        mvf->setPTSDTS(pkt->pts, pkt->dts, av_rational_to_hj_rational(pkt->time_base));
        mvf->setDuration(pkt->duration, av_rational_to_hj_rational(pkt->time_base));
        mvf->setMTS(pkt->pts, pkt->dts, pkt->duration, av_rational_to_hj_rational(pkt->time_base));
        //
        mvf->setMPacket(pkt); pkt = NULL;

        if (mvf->getDuration() <= 2) {
            HJFNLogw("warning, frame duration too short:{}, info:{}", mvf->getDuration(), mvf->formatInfo());
        }
        //
        procSEI(mvf);
        //
        frame = std::move(mvf);
        //
#if HJ_HAVE_TRACKER
        frame->addTrajectory(HJakeTrajectory());
#endif
        HJNipInterval::Ptr nip = m_nipMuster->getNipById(frame->getType());
        if (frame && nip && nip->valid()) {
            HJFNLogi(frame->formatInfo());
        }
        break;
    }

    if (pkt) {
        av_packet_free(&pkt);
        pkt = NULL;
    }
    if (HJCurrentSteadyMS() - t0 > 5) {
        HJFNLogi("getFrame end, time:{}", HJCurrentSteadyMS() - t0);
    }
    
    return res;
}

void HJFFDemuxer::reset()
{
    HJBaseDemuxer::reset();
    m_gopFrames.clear();
    m_lastAudioDTS = HJ_NOTS_VALUE;
    m_pendingAudioTrackID.store(-1);
    m_switchingAudioTrackID = -1;
    
    int64_t startTime = (HJ_NOTS_VALUE != m_mediaUrl->getStartTime()) ? m_mediaUrl->getStartTime() : 0;
    if(startTime >= 0 && startTime < m_mediaInfo->getDuration()) {
        int res = seek(startTime);
        if(HJ_OK != res) {
            HJFNLoge("seek time:{},  error:{}", startTime , res);
        }
    }
    m_runState = HJRun_Ready;
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
        HJFNLoge("error, ic->nb_streams is zero");
		return HJErrFatal;
	}
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
//        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
    }
    
    //if (m_mediaUrl->isEnbleVideo()) {
        m_mediaInfo->m_vstIdx = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    //}
    //if (m_mediaUrl->isEnableAudio()) {
        m_mediaInfo->m_astIdx = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, m_mediaInfo->m_vstIdx, NULL, 0);
        m_mediaInfo->m_selectedAstIdx = m_mediaInfo->m_astIdx;
    //}
    //if (m_mediaUrl->isEnableSubtitle()) {
        m_mediaInfo->m_sstIdx = av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE, -1, (m_mediaInfo->m_astIdx >= 0) ? m_mediaInfo->m_astIdx : m_mediaInfo->m_vstIdx, NULL, 0);
    //}
    int64_t minStartTime = HJ_INT64_MAX;
    std::vector<int> nidIdxs;
    if (m_mediaInfo->m_vstIdx >= 0)
    {
        AVStream* st = ic->streams[m_mediaInfo->m_vstIdx];
        if (st) {
            st->discard = AVDISCARD_DEFAULT;
            AVCodecParameters* codecparams = st->codecpar;
            if (!codecparams) {
                HJFNLoge("error, video codec params is NULL");
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
            videoInfo->m_frameRate = HJ_CLIP(videoInfo->m_frameRate, 5, 100);
            videoInfo->m_TimeBase = HJMediaUtils::checkTimeBase({st->time_base.num, st->time_base.den});
            videoInfo->setTimeRange({{st->start_time, {st->time_base.num, st->time_base.den}}, {HJ_MAX(0,st->duration), {st->time_base.num, st->time_base.den}}});
            videoInfo->m_rotate = getRotation(st);
            //
            AVCodecParameters* storeParams = av_dup_codec_params(codecparams);
            if (!storeParams) {
                HJFNLoge("dup codec params error");
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
    bool audioTypeAdded = false;
    for (unsigned int i = 0; i < ic->nb_streams; ++i)
    {
        AVStream* st = ic->streams[i];
        if (!st || st->codecpar->codec_type != AVMEDIA_TYPE_AUDIO || !m_mediaUrl->isEnableAudio()) {
            continue;
        }
        if (st) {
            st->discard = AVDISCARD_DEFAULT;
            minStartTime = HJ_MIN(minStartTime, av_time_to_ms(st->start_time, st->time_base));
            m_mediaInfo->m_duration = HJ_MAX(m_mediaInfo->m_duration, av_time_to_ms(st->duration, st->time_base));
            //
            HJAudioInfo::Ptr audioInfo = makeAudioStreamInfo(ic, st, (int)i, true);
            if (!audioInfo) {
                return HJErrNewObj;
            }
            if (!audioTypeAdded) {
                nidIdxs.emplace_back(audioInfo->getType());
                audioTypeAdded = true;
            }
            m_mediaInfo->addAudioInfo(audioInfo);
            m_mediaInfo->addAudioTrackDisplayInfo(makeAudioTrackDisplayInfo(st, audioInfo, (int)i));
            if (i == (unsigned int)m_mediaInfo->getSelectedAudioTrackID()) {
                m_mediaInfo->replacePrimaryAudioInfo(audioInfo);
            }
        }
    }
    if (m_mediaInfo->getSelectedAudioTrackID() < 0 && !m_mediaInfo->getAudioInfos().empty()) {
        m_mediaInfo->setSelectedAudioTrackID(m_mediaInfo->getAudioInfos().front()->m_trackID);
    }
    m_mediaInfo->syncSelectedAudioStreamInfo();
    m_mediaInfo->syncSelectedAudioTrackDisplayInfo();
    m_mediaInfo->m_startTime = HJ_MAX(0, minStartTime);
    if (AV_NOPTS_VALUE != ic->start_time) {
        m_mediaInfo->m_startTime = HJ_US_TO_MS(ic->start_time);
    }
    if (ic->duration > 0) {
        m_mediaInfo->m_duration = HJ_US_TO_MS(ic->duration);
    }
	m_mediaInfo->setTimeRange({{m_mediaInfo->m_startTime, HJ_TIMEBASE_MS}, {m_mediaInfo->m_duration, HJ_TIMEBASE_MS}});
    m_nipMuster = std::make_shared<HJNipMuster>(nidIdxs);

    HJFNLogi("media info:{}", m_mediaInfo->formatInfo());

    return res;
}

void HJFFDemuxer::applyPendingAudioTrackSwitch()
{
    int trackID = m_pendingAudioTrackID.exchange(-1);
    if (trackID < 0 || !m_mediaInfo) {
        return;
    }
    if (!m_mediaInfo->setSelectedAudioTrackID(trackID)) {
        return;
    }
    m_switchingAudioTrackID = trackID;
    m_lastAudioDTS = HJ_NOTS_VALUE;
    m_extraABuffer = nullptr;
    HJFNLogi("apply audio track switch:{}", trackID);
}

bool HJFFDemuxer::isPendingAudioTrackSwitch(const int trackID) const
{
    return (m_switchingAudioTrackID >= 0 && m_switchingAudioTrackID == trackID);
}

HJAudioInfo::Ptr HJFFDemuxer::makeAudioStreamInfo(AVFormatContext* /*ic*/, AVStream* st, int trackID, bool storeCodecParams)
{
    if (!st || !st->codecpar) {
        return nullptr;
    }
    AVCodecParameters* codecparams = st->codecpar;
    HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
    audioInfo->setID(st->id);
    audioInfo->m_trackID = trackID;
    audioInfo->m_streamIndex = m_mediaUrl->getUrlHash();
    audioInfo->setCodecID(codecparams->codec_id);
    audioInfo->setChannels(&codecparams->ch_layout);
    audioInfo->m_samplesRate = codecparams->sample_rate;
    audioInfo->m_samplesPerFrame = codecparams->frame_size;
    audioInfo->m_blockAlign = codecparams->block_align;
    audioInfo->m_sampleFmt = codecparams->format;
    audioInfo->m_bytesPerSample = (codecparams->bits_per_coded_sample > 0) ? (codecparams->bits_per_coded_sample * audioInfo->m_channels / 8) : (2 * audioInfo->m_channels);
    audioInfo->setTimeBase({ st->time_base.num, st->time_base.den });
    audioInfo->setTimeRange({ {st->start_time, {st->time_base.num, st->time_base.den}}, {HJ_MAX(0, st->duration), {st->time_base.num, st->time_base.den}} });
    audioInfo->m_dataType = HJDATA_TYPE_ES;
    if (storeCodecParams) {
        AVCodecParameters* storeParams = av_dup_codec_params(codecparams);
        if (!storeParams) {
            return nullptr;
        }
        checkAudioCodecParameters(audioInfo, storeParams);
        audioInfo->setAVCodecParams(storeParams);
    }
    return audioInfo;
}

HJAudioTrackDisplayInfo::Ptr HJFFDemuxer::makeAudioTrackDisplayInfo(AVStream* st, const HJAudioInfo::Ptr& audioInfo, int trackID)
{
    if (!st) {
        return nullptr;
    }

    auto info = std::make_shared<HJAudioTrackDisplayInfo>();
    info->m_trackID = trackID;
    info->m_title = trimTrackLabel(getMetadataValue(st->metadata, "title"));
    info->m_language = trimTrackLabel(getMetadataValue(st->metadata, "language"));
    info->m_handlerName = trimTrackLabel(getMetadataValue(st->metadata, "handler_name"));
    info->m_isDefault = (st->disposition & AV_DISPOSITION_DEFAULT) != 0;
    info->m_isCommentary = (st->disposition & AV_DISPOSITION_COMMENT) != 0;
    if (audioInfo) {
        info->m_codecName = audioInfo->getCodecName();
        if (info->m_codecName.empty()) {
            info->m_codecName = avcodec_get_name((AVCodecID)audioInfo->getCodecID());
        }
        info->m_channels = audioInfo->m_channels;
        info->m_sampleRate = audioInfo->m_samplesRate;
    }
    info->m_isSelected = (m_mediaInfo && m_mediaInfo->getSelectedAudioTrackID() == trackID);
    info->m_displayName = makeTrackDisplayName(info->m_title, info->m_language, info->m_handlerName, info->m_isCommentary, trackID);
    return info;
}

AVDictionary* HJFFDemuxer::getFormatOptions()
{
    AVDictionary* fmtOpts = NULL;
    std::string thisStr = HJ2STR((size_t)this);
    av_dict_set(&fmtOpts, "opaque", thisStr.c_str(), 0);
    //
    //std::string blobUrl = HJUtilitys::concateString(HJUtilitys::concatenatePath("E:/movies/blob/", HJUtilitys::getTimeToString()), ".flv");
    //av_dict_set(&fmtOpts, "bloburl", blobUrl.c_str(), 0);
    auto blob_url = m_mediaUrl->getString("local_url");
    if (!blob_url.empty()) {
        av_dict_set(&fmtOpts, "blob_url", blob_url.c_str(), 0);
    }
    auto local_dir = m_mediaUrl->getString("local_dir");
    if (!local_dir.empty()) {
        av_dict_set(&fmtOpts, "local_dir", local_dir.c_str(), 0);
    }
    auto url_rid = m_mediaUrl->getString("url_rid");
    if (!url_rid.empty()) {
        av_dict_set(&fmtOpts, "url_rid", url_rid.c_str(), 0);
    }
    /**
	* rtmp - with timeout will bind socket timeout, cause more delay, so do not set timeout for rtmp
    */
    if (m_mediaUrl->getTimeout() >= 0 && !m_mediaUrl->isRTMP()) {
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
    av_dict_set_int(&fmtOpts, "streamtype", 2, 0);
    av_dict_set(&fmtOpts, "obj_name", getName().c_str(), 0);

    // http_persistent 默认就是 1，如果想显式设置：
    //av_dict_set(&fmtOpts, "http_multiple", "1", 0);
    //av_dict_set(&fmtOpts, "http_persistent", "1", 0);
    //av_dict_set(&fmtOpts, "multiple_requests", "1", 0);

    
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
    HJFNLogi("eof attrs:" + m_atrs->formatInfo());
    
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
#if 0
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
#endif

double HJFFDemuxer::getRotation(AVStream* st)
{
    double theta = 0.0;
    int32_t* displaymatrix = NULL;
    const AVPacketSideData* psd = av_packet_side_data_get(st->codecpar->coded_side_data,
        st->codecpar->nb_coded_side_data,
        AV_PKT_DATA_DISPLAYMATRIX);
    if (psd)
        displaymatrix = (int32_t*)psd->data;

    if (displaymatrix)
        theta = -round(av_display_rotation_get(displaymatrix));

    theta -= 360 * floor(theta / 360 + 0.9 / 360);

    if (fabs(theta - 90 * round(theta / 90)) > 2) {
        HJFLogi("Odd rotation angle.");
    }
    //av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
    //    "If you want to help, upload a sample "
    //    "of this file to https://streams.videolan.org/upload/ "
    //    "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");

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
            HJFNLogi("analyze protocols:{}", whitelist);
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

void HJFFDemuxer::procSEI(const HJMediaFrame::Ptr& mvf)
{
    auto seiNals = mvf->getSEINals();
#if 0
    if (seiNals && seiNals->size() > 0) {
        HJFLogi("sei nal size:{}", seiNals->size());

        std::vector<HJSEIData> userSEIDatas{};
        const auto& nals = seiNals->getDatas();
        for (const auto& nal : nals) {
            auto userDatas = HJSEIWrapper::parseSEINals(nal);
            for (const auto& userData : userDatas) {
                std::string userMsg = std::string(userData.data.begin(), userData.data.end());
                HJFNLogi("sei nal uuid:{}, msg:{}", userData.uuid, userMsg);
            }
            if (userDatas.size() > 0) {
                userSEIDatas.insert(userSEIDatas.end(), userDatas.begin(), userDatas.end());
            }
        }
    }
#endif
}

int HJFFDemuxer::checkAudioCodecParameters(const HJAudioInfo::Ptr& audioInfo, AVCodecParameters* codecParams)
{
    if ((!codecParams->extradata || codecParams->extradata_size <= 0))
    {
        HJFNLogw("audio extradata is empty. codec_id:{}, sampleRate:{}, channels:{}",
                 AVCodecIDToString(codecParams->codec_id), audioInfo->m_samplesRate, audioInfo->m_channels);
        if (AV_CODEC_ID_AAC == codecParams->codec_id) { // AAC specific extradata
            auto extraBuffer = HJMediaUtils::makeAACExtraData(audioInfo->m_samplesRate, audioInfo->m_channels);
            if (extraBuffer) {
                codecParams->extradata = (uint8_t*)av_mallocz(extraBuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
                if (codecParams->extradata) {
                    memcpy(codecParams->extradata, extraBuffer->data(), extraBuffer->size());
                    codecParams->extradata_size = extraBuffer->size();
                }
            }
        }
    }
    return HJ_OK;
}

int HJFFDemuxer::updateStatus()
{
    auto filemanager = HJDataSourceKit::getInstance()->getFileManager();
    if (!filemanager) {
        return HJErrNotAlready;
    }
    auto& url = m_mediaUrl->getUrl();
    auto rid = m_mediaUrl->getString("url_rid");

    return filemanager->incFileUseCount(rid, url);
}

int HJFFDemuxer::getSeekFlags()
{
    if (!m_mediaInfo) {
        return 0;
    }

    const bool hasVideo = m_mediaInfo->getVideoInfo() != nullptr;
    const bool hasAudio = m_mediaInfo->getAudioInfo() != nullptr;
    if (!hasVideo && hasAudio) {
        return AVSEEK_FLAG_ANY;
    }

    return 0;
}

//***********************************************************************************//
NS_HJ_END
