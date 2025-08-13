//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJVEncFFMpeg.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
static int set_hwframe_ctx(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, HJVideoInfo::Ptr videoInfo, bool swfFlag)
{
    AVBufferRef* hw_frames_ref = NULL;
    AVHWFramesContext* frames_ctx = NULL;
    int err = 0;

    auto it = videoInfo->find(HJVideoInfo::STORE_KEY_HWFramesCtx);
    if (it != videoInfo->end()) {
        hw_frames_ref = std::any_cast<AVBufferRef *>(it->second);
    }
    if (hw_frames_ref) {
        ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    } else {
        hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx);
        if (!hw_frames_ref) {
            HJLoge("Failed to create HW frame context. error");
            return HJErrNewObj;
        }
        frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
        frames_ctx->format    = ctx->pix_fmt;
        frames_ctx->sw_format = AV_PIX_FMT_NV12;
        frames_ctx->width     = videoInfo->m_width;
        frames_ctx->height    = videoInfo->m_height;
        if(swfFlag) {
            frames_ctx->initial_pool_size = 20;
        }
        if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
            //HJLoge("Failed to initialize VAAPI frame context. error");
            av_buffer_unref(&hw_frames_ref);
            return err;
        }
        ctx->hw_frames_ctx = hw_frames_ref;
    }
    if (!ctx->hw_frames_ctx){
        err = AVERROR(ENOMEM);
        return err;
    }
    return err;
}
//***********************************************************************************//
HJVEncFFMpeg::HJVEncFFMpeg()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJVEncFFMpeg)));
    m_codecID = AV_CODEC_ID_H264;
}

HJVEncFFMpeg::~HJVEncFFMpeg()
{
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_free_context(&avctx);
        m_avctx = NULL;
    }
}

int HJVEncFFMpeg::init(const HJStreamInfo::Ptr& info)
{
    HJLogi("init begin");
    int res = HJBaseCodec::init(info);
    if (HJ_OK != res) {
        return res;
    }
    HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
    if (AV_CODEC_ID_NONE != videoInfo->m_codecID) {
        m_codecID = videoInfo->m_codecID;
    }
    m_codecName = videoInfo->m_codecName;
    //
    const AVCodec* codec = NULL;
    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_NONE;
    HJHWDevice::Ptr hwDevice = videoInfo->getHWDevice();
    if (hwDevice) 
    {
        AVBufferRef* hw_device_ctx = (AVBufferRef*)hwDevice->getHWDeviceCtx();
        hwType = hj_device_type_map(hwDevice->getDeviceType());
        if (!hw_device_ctx || AV_HWDEVICE_TYPE_NONE == hwType) {
            HJLoge("hw device error or Device type is not supported");
            return HJErrHWDevice;
        }
        codec = hj_find_av_encoder((AVCodecID)m_codecID, hwType);
    } else if(!m_codecName.empty()) {
        codec = avcodec_find_encoder_by_name(m_codecName.c_str());
        m_codecID = codec->id;
    } else {
        codec = avcodec_find_encoder((AVCodecID)m_codecID);
    }
    if (!codec) {
        HJLoge("can't find codec:" + HJ2STR(m_codecID) + " error");
        return HJErrFFCodec;
    }

    AVCodecContext* avctx = avcodec_alloc_context3(codec);
    if (!avctx) {
        return HJErrFFNewAVCtx;
    }
    m_avctx = avctx;
    //
    avctx->bit_rate = videoInfo->m_bitrate;
    avctx->rc_max_rate = avctx->bit_rate * 1.3;
//    avctx->rc_max_available_vbv_use = avctx->rc_max_rate;
    avctx->width = videoInfo->m_width;
    avctx->height = videoInfo->m_height;
    avctx->pix_fmt = hwDevice ? av_pixel_format_from_hwtype(hwType) : (AVPixelFormat)videoInfo->m_pixFmt;
    //
	//m_timeBase = { 1, videoInfo->m_frameRate };
    m_timeBase = HJMediaUtils::checkTimeBase(videoInfo->m_TimeBase);//HJ_TIMEBASE_MS;
	avctx->time_base = av_rational_from_hj_rational(m_timeBase);
    avctx->pkt_timebase = avctx->time_base;
    avctx->framerate = { videoInfo->m_frameRate, 1 };
    avctx->gop_size = (videoInfo->m_gopSize > 0) ? videoInfo->m_gopSize : videoInfo->m_frameRate;
    avctx->sample_aspect_ratio = { 0, 1 };
    avctx->max_b_frames = videoInfo->m_maxBFrames;
    avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    //
    if(m_info->haveStorage("swfflag")) {
        m_swfFlag = m_info->getStorageValue<bool>("swfflag");
    }
    if (hwDevice) {
        m_hwFrameCtx = videoInfo->getHWFramesCtx();
        if (!m_hwFrameCtx) {
            int poolSize = m_swfFlag ? 20 : 0;
            m_hwFrameCtx = std::make_shared<HJHWFrameCtx>(hwDevice, avctx->pix_fmt, AV_PIX_FMT_NV12, videoInfo->m_width, videoInfo->m_height, poolSize);
        }
        if (!m_hwFrameCtx) {
            res = HJErrHWFrameCtx;
            HJLoge("hw frame ctx error");
            return res;
        }
        avctx->hw_frames_ctx = av_buffer_ref((AVBufferRef*)m_hwFrameCtx->getHWFrameRef());

        //res = set_hwframe_ctx(avctx, hw_device_ctx, videoInfo, m_swfFlag);
        //if (res < HJ_OK) {
        //    HJLoge("hw frame ctx error");
        //    return res;
        //}
    } else {
        //avctx->me_range = 16;
        //avctx->max_qdiff = 4;
        //avctx->qmin = 1;
        //avctx->qmax = 69;
        //avctx->qcompress = 0.6f;
        //
        if (AV_CODEC_ID_MJPEG == m_codecID) {
            avctx->color_range = AVCOL_RANGE_JPEG;
            avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            avctx->qmin = 1;
            avctx->mb_lmin = avctx->qmin * FF_QP2LAMBDA;
            avctx->mb_lmax = avctx->qmax * FF_QP2LAMBDA;
            if (m_info->haveStorage("quality")) {
                avctx->flags |= AV_CODEC_FLAG_QSCALE;
                float quality = m_info->getStorageValue<float>("quality");
                float qscaleFactor = 1.0f - HJ_CLIP(quality, 0.0f, 1.0f);
                avctx->global_quality = (int)(avctx->qmin + (avctx->qmax - avctx->qmin) * qscaleFactor) * FF_QP2LAMBDA;
            }
        }
    }
    AVDictionary* opts = NULL;
    if (m_lowDelay) {
        avctx->gop_size = 2;
        avctx->max_b_frames = 0;
        avctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
        av_opt_set(avctx->priv_data, "preset", "superfast", 0);
        av_opt_set(avctx->priv_data, "tune", "zerolatency", 0);
        //
        av_dict_set(&opts, "flags", "low_delay", 0);
        av_dict_set(&opts, "threads", "1", 0);
    } else 
    {
        if(m_info->haveStorage("threads")) {
            avctx->thread_count = m_info->getStorageValue<int>("threads");
            HJLogi("threads set " + HJ2STR(avctx->thread_count));
        } else {
            HJLogi("threads set auto ");
            av_dict_set(&opts, "threads", "auto", 0);
        }
        if (!hwDevice) {
            av_dict_set(&opts, "rc-lookahead", HJ2STR(videoInfo->m_lookahead).c_str(), 0);
            //only soft encode, after modify
            if (m_info->haveStorage("profile"))
            {
                std::string profile = m_info->getStorageValue<std::string>("profile");
                HJLogi("profile set " + profile);
                av_opt_set(avctx->priv_data, "profile", profile.c_str(), 0);
            }

            if (m_info->haveStorage("preset"))
            {
                std::string preset = m_info->getStorageValue<std::string>("preset");
                HJLogi("preset set " + preset);
                av_opt_set(avctx->priv_data, "preset", preset.c_str(), 0);
            }

            //CS by lfs
            if (AV_CODEC_ID_H264 == m_codecID)
            {
                if (m_info->haveStorage("rc"))
                {
                    float vbvFactor = 1.f;
                    if (m_info->haveStorage("vbvFactor"))
                    {
                        vbvFactor = m_info->getStorageValue<float>("vbvFactor");
                    }

                    std::string rc = m_info->getStorageValue<std::string>("rc");
                    HJLogi("rc set " + rc);
                    if (rc == "ABR")
                    {
                        //default
                    }
                    else if (rc == "ABR+VBV")
                    {
                        avctx->rc_buffer_size = avctx->bit_rate * vbvFactor;
                        avctx->rc_max_rate = avctx->bit_rate * vbvFactor;
                    }
                    else if (rc == "CBR")
                    {
                        avctx->rc_buffer_size = avctx->bit_rate * vbvFactor;
                        avctx->rc_max_rate = avctx->bit_rate * vbvFactor;
                        av_opt_set(avctx->priv_data, "nal-hrd", "cbr", 0);  //nal-hrd=X264_NAL_HRD_CBR
                    }
                    HJLogi("vbv factor " + HJ2STR(vbvFactor) + " bufsize " + HJ2STR(avctx->rc_buffer_size) + " maxrate " + HJ2STR(avctx->rc_max_rate));
                }
            }
            //av_opt_set(avctx->priv_data, "x264-params", "preset=veryfast:profile=high:level=3.1:keyint=60:ref=3:b-pyramid=normal:weightp=1:vbv-bufsize=5200:vbv-maxrate=5200:rc-lookahead=15:bframes=3:force-cfr=1", 0);
        } else {
            if (m_info->haveValue("realtime")) {
                av_opt_set(avctx->priv_data, "realtime", HJ2STR(m_info->getValue<bool>("realtime")).c_str(), 0);
            }
        }
		if (HJQuality_HQ == m_quality) {
            av_opt_set(avctx->priv_data, "preset", "p6", 0);
            av_opt_set(avctx->priv_data, "tune", "ll", 0);
            av_opt_set(avctx->priv_data, "profile", "high", 0);
            av_opt_set(avctx->priv_data, "rc", "vbr", 0);
            av_opt_set(avctx->priv_data, "rc-lookahead", "20", 0);
        }
    }
    res = avcodec_open2(avctx, codec, &opts);
    if (res < HJ_OK) {
        HJLoge("avcodec open error:" + HJ2STR(res));
        return res;
    }
    m_timeBase = HJMediaUtils::checkTimeBase({ avctx->time_base.num, avctx->time_base.den });

    AVCodecParameters* codecParams = av_dup_codec_params_from_avcodec(avctx);
    if (!codecParams) {
        HJLoge("codec params dup error");
        return res;
    }
    m_info->setAVCodecParams(codecParams);
    //
    if (AV_CODEC_ID_H264 == m_codecID || AV_CODEC_ID_HEVC == m_codecID) {
        m_parser = std::make_shared<HJBSFParser>();
        res = m_parser->init("", codecParams);
        if (HJ_OK != res) {
            HJLoge("bsf parser init error");
            return res;
        }
        m_parser->setAVCHeader(true);

        //m_seiParser = std::make_shared<HJBSFParser>();
        //HJOptions::Ptr opts = std::make_shared<HJOptions>();
        //(*opts)["sei_user_data"] = HJUtils::generateUUID() + "+" + "hello";
        //res = m_seiParser->init("h264_metadata", codecParams, opts);
        //if (HJ_OK != res) {
        //    HJLoge("bsf parser init error");
        //    return res;
        //}
        //m_seiParser->setAVCHeader(true);
    }
    //
    m_runState = HJRun_Init;
    HJLogi("init end");
    
    return res;
}

int HJVEncFFMpeg::getFrame(HJMediaFrame::Ptr& frame)
{
    if (!m_avctx) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        return HJErrNewObj;
    }
    do {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        res = avcodec_receive_packet(avctx, pkt);
        if (AVERROR(EAGAIN) == res) {
            res = HJ_WOULD_BLOCK;
            break;
        } else if(AVERROR_EOF == res) {
            frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_VIDEO);
            m_runState = HJRun_Done;
            res = HJ_EOF;
            break;
        } else if(res < HJ_OK) {
            HJLoge("receive frame error:" + HJ2String(res));
            res = HJErrFFGetFrame;
            break;
        } else
        {
            HJMediaFrame::Ptr mvf = HJMediaFrame::makeVideoFrame();
            if (!mvf) {
                res = HJErrNewObj;
                break;
            }
            pkt->time_base = av_rational_from_hj_rational(m_timeBase);
            pkt->duration = HJMediaUtils::calcFrameDuration(m_timeBase, { avctx->framerate.num, avctx->framerate.den });
            if (m_parser && (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) &&
                (pkt->flags & AV_PKT_FLAG_KEY))
            {
                uint8_t* csd0_data = NULL;
                uint8_t* csd1_data = NULL;
                int csd0_size = 0;
                int csd1_size = 0;
                HJBuffer::Ptr csd0 = m_parser->getCSD0();
                if(csd0) {
                    csd0_data = csd0->data();
                    csd0_size = (int)csd0->size();
                }
                HJBuffer::Ptr csd1 = m_parser->getCSD1();
                if (csd1) {
                    csd1_data = csd1->data();
                    csd1_size = (int)csd1->size();
                }
                pkt = hj_avpacket_with_extradata(pkt, csd0_data, csd0_size, csd1_data, csd1_size);
            }
            //if (m_seiParser) {
            //    HJBuffer::Ptr pktbuffer = std::make_shared<HJBuffer>(pkt->data, pkt->size);
            //    HJBuffer::Ptr outpktbuffer = m_seiParser->filter(pktbuffer);
            //    if (outpktbuffer) {
            //        pkt = hj_avpacket_with_newdata(pkt, outpktbuffer->data(), outpktbuffer->size());
            //    }
            //}
            //
            mvf->setPTSDTS(pkt->pts, pkt->dts, m_timeBase);
            mvf->setDuration(pkt->duration, av_rational_to_hj_rational(pkt->time_base));
            //
            if (pkt->flags & AV_PKT_FLAG_KEY) {
                mvf->setFrameType(HJFRAME_KEY);
            }

            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(mvf->m_info);
            videoInfo->clone(m_info);
            videoInfo->m_dataType = HJDATA_TYPE_ES;
            //
            mvf->setMPacket(pkt);
            pkt = NULL;
            //
            frame = std::move(mvf);
            //
#if HJ_HAVE_TRACKER
            auto tracker = getTracker(HJ2STR(frame->getPTS()));
            if (tracker) {
                tracker->addTrajectory(HJakeTrajectory());
                frame->setTracker(tracker);
            }
#endif
            HJNipInterval::Ptr nip = m_nipMuster->getOutNip();
            if (frame && nip && nip->valid()) {
                HJLogi("name:" + getName() + ", " + frame->formatInfo());
            }
        }
        
    } while (false);
    
    if (pkt) {
        av_packet_free(&pkt);
        pkt = NULL;
    }
    
    return res;
}

int HJVEncFFMpeg::run(const HJMediaFrame::Ptr frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    if(!m_avctx)
    {
        res = init(frame->getInfo());
        if (HJ_OK != res) {
            HJLoge("init in run error");
            return res;
        }
    }
    AVFrame* avf = NULL;
    AVFrame* hw_avf = NULL;
    AVCodecContext* avctx = (AVCodecContext*)m_avctx;
    do {
        m_runState = HJRun_Running;
        HJMediaFrame::Ptr mvf = frame;
        if(mvf && (HJFRAME_EOF != mvf->getFrameType()))
        {
            avf = (AVFrame *)mvf->getAVFrame();
            if (!avf) {
                HJLogi("have no AVFrame");
                res = HJErrInvalidParams;
                break;
            }
            mvf->setAVTimeBase(m_timeBase);
            if (m_swfFlag)
            {
                hw_avf = av_frame_alloc();
                if (!hw_avf) {
                    return HJErrNewObj;
                }
                res = av_hwframe_get_buffer(avctx->hw_frames_ctx, hw_avf, 0);
                if (res < HJ_OK || !hw_avf || !hw_avf->hw_frames_ctx) {
                    HJLoge("get hw frame buffer error");
                    return HJErrNewObj;
                }
                res = av_hwframe_transfer_data(hw_avf, avf, 0);
                if (res < HJ_OK) {
                    HJLoge("hwframe transfer data error");
                    return res;
                }
                hw_avf->key_frame = avf->key_frame;
                hw_avf->pict_type = avf->pict_type;
                hw_avf->pts = avf->pts;
                hw_avf->time_base = avf->time_base;
                hw_avf->best_effort_timestamp = avf->best_effort_timestamp;
                //
                avf = hw_avf;
            }
            if (avctx->global_quality > 0) {
                avf->quality = avctx->global_quality;
            }
#if HJ_HAVE_TRACKER
            auto tracker = mvf->getTracker()->dup();
            if (tracker) {
                tracker->addTrajectory(HJakeTrajectory());
                addTracker(HJ2STR(mvf->getPTS()), tracker);
                //m_trackers[HJ2STR(mvf->getPTS())] = mvf->getTracker()->dup();
            }
#endif
            HJNipInterval::Ptr nip = m_nipMuster->getInNip();
            if (frame && nip && nip->valid()) {
                HJLogi("name:" + getName() + ", " + mvf->formatInfo());
            }
		} else {
			//HJLogi("video encoder send frame pts:" + HJ2STR(frame->getPTS()) + ", dts:" + HJ2STR(frame->getDTS()));
		}
        //
        res = avcodec_send_frame(avctx, avf);
        if (AVERROR(EAGAIN) == res) {
            return HJ_WOULD_BLOCK;
        } else if(AVERROR_EOF == res) {
            m_runState = HJRun_Stop;
            return HJ_EOF;
        } else if (res < HJ_OK){
            HJLoge("error, video enc send packet");
            return HJErrFatal;
        }
    } while (false);
    
    if (hw_avf) {
        av_frame_free(&hw_avf);
    }
    
    return HJ_OK;
}

int HJVEncFFMpeg::flush()
{
    if(m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_flush_buffers(avctx);
    }
    m_runState = HJRun_Init;
    return HJ_OK;
}

/**
 * x264-params 
 * preset=medium:profile=high:level=3.1:keyint=60:ref=3:b-pyramid=normal:weightp=1:vbv-bufsize=1.3*bitrate:vbv-maxrate=1.3*bitrate:rc-lookahead=15:bframes=3
 * preset=veryfast:profile=high:level=3.1:keyint=60:ref=3:b-pyramid=normal:weightp=1:vbv-bufsize=1.3*bitrate:vbv-maxrate=1.3*bitrate:rc-lookahead=15:bframes=3 
 */
std::string HJVEncFFMpeg::formatX264Params(const std::string preset, int framerate, int bitrate, int bframes, int ref)
{
    return HJFMT("preset={}:profile=high:level=3.1:keyint={}:ref={}:b-pyramid=normal:weightp=1:vbv-bufsize={}:vbv-maxrate={}:rc-lookahead={}:bframes={}",
        preset, 2*framerate, ref, (size_t)(1.3 * bitrate), (size_t)(1.3 * bitrate), (size_t)(0.5 * framerate + 0.5), bframes);
}

/**
 * 
 */
std::string HJVEncFFMpeg::formatX265Params(const std::string preset, int framerate, int bitrate, int bframes, int ref)
{
    return "";
}

NS_HJ_END
