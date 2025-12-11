//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJVDecFFMpeg.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

#define HJ_HAVE_VideoConvert   1

NS_HJ_BEGIN
//***********************************************************************************//
static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    enum AVPixelFormat hw_pix_fmt = *(enum AVPixelFormat *)ctx->opaque;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }
    HJLoge("error, Failed to get HW surface format.");
    return AV_PIX_FMT_NONE;
}
//***********************************************************************************//
HJVDecFFMpeg::HJVDecFFMpeg()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJVDecFFMpeg)));
    m_hwPixFmt = AV_PIX_FMT_NONE;
}

HJVDecFFMpeg::~HJVDecFFMpeg()
{
    done();
}

static AVPixelFormat get_mediacodec_format(AVCodecContext* avctx, const enum AVPixelFormat* pix_fmts)
{
	while (*pix_fmts != AV_PIX_FMT_NONE)
	{
		if (*pix_fmts == AV_PIX_FMT_MEDIACODEC)
		{
			return AV_PIX_FMT_MEDIACODEC;
		}

		pix_fmts++;
	}
	return AV_PIX_FMT_NONE;
}
/**
 * in: hw_device_ctx
 */
int HJVDecFFMpeg::init(const HJStreamInfo::Ptr& info)
{
    HJFNLogi("init entry");
    int res = HJBaseCodec::init(info);
    if (HJ_OK != res) {
        return res;
    }
    HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
    m_bMediacodecDec = videoInfo->haveValue("mediacodec_surface");
    //
    AVCodecParameters* codecParam = (AVCodecParameters*)videoInfo->getAVCodecParams();
    if (!codecParam) {
        HJFNLoge("can't find codec params error");
        return HJErrInvalidParams;
    }
    m_codecID = (AV_CODEC_ID_NONE != codecParam->codec_id) ? codecParam->codec_id : videoInfo->getCodecID();
    //
    const AVCodec* codec = NULL;
    AVBufferRef* hw_device_ctx = NULL;
    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_NONE;
    HJHWDevice::Ptr hwDevice = videoInfo->getHWDevice();
    if (hwDevice) 
    {
        hw_device_ctx = (AVBufferRef*)hwDevice->getHWDeviceCtx();
        hwType = hj_device_type_map(hwDevice->getDeviceType());
        if (!hw_device_ctx || AV_HWDEVICE_TYPE_NONE == hwType) {
            HJFNLoge("hw device error or Device type is not supported");
            return HJErrHWDevice;
        }
        codec = hj_find_av_decoder(codecParam->codec_id, hwType);
    } else
    {
        codec = avcodec_find_decoder((AVCodecID)m_codecID);
    }
    if (!codec) {
        HJFNLoge("can't find property codec error, codec id:{}", codecParam->codec_id);
        return HJErrFFCodec;
    }

    AVCodecContext* avctx = avcodec_alloc_context3(codec);
    if (!avctx) {
        HJFNLoge("alloc context error");
        return HJErrFFNewAVCtx;
    }
    m_avctx = avctx;
    //
    res = avcodec_parameters_to_context(avctx, codecParam);
    if (HJ_OK != res) {
        HJFNLoge("codec parms from error");
        return res;
    }    
    //
	//m_timeBase = { 1, videoInfo->m_frameRate };
    m_timeBase = HJMediaUtils::checkTimeBase(videoInfo->m_TimeBase);  //HJ_TIMEBASE_MS;
	avctx->time_base = av_rational_from_hj_rational(m_timeBase);
    avctx->pkt_timebase = avctx->time_base;
    if (hwDevice) 
    {
        if (!m_bMediacodecDec) {
            avctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        }
        m_hwPixFmt = hj_hw_pixel_format(codec, hwType);
        if (AV_PIX_FMT_NONE == m_hwPixFmt) {
            HJFNLoge("error, hw pixel format is none");
            return HJErrFFCodec;
        }
        avctx->opaque = &m_hwPixFmt;
        avctx->get_format = get_hw_format;
    }

    if (m_bMediacodecDec)
    {
        m_MediaCodecContext = (void*)av_mediacodec_alloc_context();
        if (NULL == m_MediaCodecContext)
        {
            return HJErrFFCodec;
        }
        void* surface = videoInfo->getValue<void*>("mediacodec_surface"); //std::any_cast<void *>((*videoInfo)["mediacodec_surface"]);
        res = av_mediacodec_default_init(avctx, (AVMediaCodecContext*)m_MediaCodecContext, surface);
        if (res < 0)
        {
            return res;
        }
    }

    AVDictionary* opts = NULL;
    //CS by lfs
	//if (AV_HWDEVICE_TYPE_CUDA == hwType)
	//{
	//	av_dict_set_int(&opts, "surfaces", 1, 0);
	//	avctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
	//}
    if (m_lowDelay) {
        av_dict_set(&opts, "flags", "low_delay", 0);
        av_dict_set(&opts, "threads", "1", 0);
        std::string codeName = codec->name;
        if (std::string::npos != codeName.find("cuvid")) {
            av_dict_set_int(&opts, "surfaces", 1, 0);
        }
    } else {
        if(m_info->haveStorage("threads")) {
            avctx->thread_count = m_info->getStorageValue<int>("threads");
        } else {
            av_dict_set(&opts, "threads", "auto", 0);
        }
    }
    res = avcodec_open2(avctx, codec, &opts);
    if (res < HJ_OK) {
        HJLoge("codec open error");
        return res;
    }
    m_timeBase = HJMediaUtils::checkTimeBase({avctx->time_base.num, avctx->time_base.den});
    //
    //buildConverters();
    flush();
    //
#if HJ_HAVE_CHECK_XTRADATA
    if (AV_CODEC_ID_H264 == m_codecID || AV_CODEC_ID_HEVC == m_codecID) {
        m_parser = std::make_shared<HJH2645Parser>();
        res = m_parser->init(codecParam);
        if (HJ_OK != res) {
            m_parser = nullptr;
            HJFNLoge("parser open error");
            return res;
        }
    }
#endif

    m_runState = HJRun_Init;
    HJFNLogi("init end");
    
    return res;
}

int HJVDecFFMpeg::getFrame(HJMediaFrame::Ptr& frame)
{
    if(!m_avctx) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    AVFrame* avf = av_frame_alloc();
    if (!avf) {
        HJFNLoge("error, alloc avframe");
        return HJErrNewObj;
    }
    do {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        res = avcodec_receive_frame(avctx, avf);
        if (AVERROR(EAGAIN) == res) {
            res = HJ_WOULD_BLOCK;
            break;
        } else if(AVERROR_EOF == res) {
            if (m_storeFrame) {
                m_isChanged = true;
                reset();
                run(std::move(m_storeFrame));
                res = getFrame(frame);
            } else {
                frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_VIDEO);
                m_runState = HJRun_Done;
                res = HJ_EOF;
            }
            break;
        } else if(res < HJ_OK) {
            HJFNLoge("receive frame error:" + HJ2String(res));
            res = HJErrFFGetFrame;
            break;
        } else
        {
            if (m_swfFlag && (m_hwPixFmt == avf->format))
            {
                AVFrame* sw_avf = av_frame_alloc();
                res = av_hwframe_transfer_data(sw_avf, avf, 0);
                if (res < HJ_OK) {
                    break;
                }
                //sw_avf->key_frame = avf->key_frame;
                sw_avf->flags = avf->flags;
                sw_avf->pict_type = avf->pict_type;
                sw_avf->pts = avf->pts;
                sw_avf->best_effort_timestamp = avf->best_effort_timestamp;
                //
                av_frame_free(&avf);
                avf = sw_avf;
            }

            HJMediaFrame::Ptr mvf = HJMediaFrame::makeVideoFrame();
            if (!mvf) {
                res = HJErrNewObj;
                break;
            }
            avf->time_base = av_rational_from_hj_rational(m_timeBase);
            avf->pts = avf->best_effort_timestamp;
			mvf->setPTSDTS(avf->best_effort_timestamp, avf->best_effort_timestamp, m_timeBase);
            mvf->setDuration(avf->duration, m_timeBase);
			//mvf->m_pts = av_time_to_ms(avf->best_effort_timestamp, avf->time_base);
			//mvf->m_dts = av_time_to_ms(avf->best_effort_timestamp, avf->time_base);
            if((/*avf->key_frame*/(avf->flags & AV_FRAME_FLAG_KEY) && avf->pict_type && (avf->pict_type == AV_PICTURE_TYPE_I))) {
                mvf->setFrameType(HJFRAME_KEY);
            }
            //
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(mvf->m_info);
            videoInfo->m_codecID = avctx->codec_id;
            videoInfo->m_codecName = avctx->codec->name;
            videoInfo->m_width = avf->width;
            videoInfo->m_height = avf->height;
            videoInfo->m_stride[0] = avf->linesize[0];
            videoInfo->m_stride[1] = avf->linesize[1];
            videoInfo->m_stride[2] = avf->linesize[2];
            videoInfo->m_stride[3] = avf->linesize[3];
            videoInfo->m_pixFmt = avf->format; //avctx->pix_fmt;
            videoInfo->m_rotate = std::dynamic_pointer_cast<HJVideoInfo>(m_info)->m_rotate;
            if (avctx->framerate.den) {
                videoInfo->m_frameRate = avctx->framerate.num / (float)avctx->framerate.den + 0.5f;
            }
            videoInfo->m_dataType = HJDATA_TYPE_RS;

            if (!m_swfFlag && avctx->hw_frames_ctx) {
                (*videoInfo)[HJVideoInfo::STORE_KEY_HWFramesCtx] = std::make_shared<HJHWFrameCtx>(avctx->hw_frames_ctx); //av_buffer_ref(avctx->hw_frames_ctx);
            }
            mvf->setMFrame(avf); 
            avf = NULL;
            //
            for (auto& converter : m_videoConverters) {
                auto outMvf = converter->convert(mvf);
                if (outMvf) {
                    mvf->setAuxMFrame(outMvf->getMFrame());
                    HJFNLogi("convert scale");
                }
            }
            //output
            if (mvf)
            {
                if (m_isChanged) {
                    mvf->setFlush();
                    m_isChanged = false;
                }
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
                    HJFNLogi(frame->formatInfo());
                }
            }
        }
    } while (false);

    if (avf) {
        av_frame_free(&avf);
    }

    return res;
}

int HJVDecFFMpeg::run(const HJMediaFrame::Ptr frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    if(!m_avctx)
    {
        res = init(frame->getInfo());
        if (HJ_OK != res) {
            HJFNLoge("init in run error");
            return res;
        }
    }
    AVPacket* nullPkt = NULL;
    AVCodecContext* avctx = (AVCodecContext*)m_avctx;
    do {
        HJMediaFrame::Ptr mvf = frame->deepDup();
#if HJ_HAVE_CHECK_XTRADATA
        bool invalid = checkFrame(mvf);
        if (invalid) {
            m_storeFrame = std::move(mvf);
        }
#endif
        m_runState = HJRun_Running;
        AVPacket* pkt = NULL;
        if (mvf && (HJFRAME_EOF != mvf->getFrameType()))
        {
            mvf->setAVTimeBase(m_timeBase);
            pkt = mvf->getAVPacket();
        }
        else {
            nullPkt = hj_make_null_packet();
            pkt = nullPkt;
        }
        if (!pkt) {
            HJFNLoge("run get avpacket error");
            return HJErrInvalidParams;
        }
        if (AV_NOPTS_VALUE == pkt->dts || AV_NOPTS_VALUE == pkt->pts) {
            HJFNLogw("checkFrame dts pts invalid before");
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
        if (mvf && nip && nip->valid()) {
            HJFNLogi(mvf->formatInfo());
        }
        //
        res = avcodec_send_packet(avctx, pkt);
        if (AVERROR(EAGAIN) == res) {
            return HJ_WOULD_BLOCK;
        }
        else if (AVERROR_EOF == res) {
            m_runState = HJRun_Stop;
            return HJ_EOF;
        }
        else if (res < HJ_OK) {
            HJFNLoge("error, video dec send packet failed:{}", res);
            return HJErrFatal;
        }
    } while (false);

    if (nullPkt) {
        av_packet_free(&nullPkt);
    }
    return res;
}

int HJVDecFFMpeg::flush()
{
    HJFNLogi("flush entry");
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_flush_buffers(avctx);
    }
    m_runState = HJRun_Ready;
    HJFNLogi("flush end");

    return HJ_OK;
}

bool HJVDecFFMpeg::checkFrame(const HJMediaFrame::Ptr frame)
{
    if (HJRun_Init == m_runState) {
        return false;
    }
    if (frame->isFlushFrame())
    {
        //HJFLogi("flush frame");
        const auto& inInfo = frame->getVideoInfo();
        if (!inInfo) {
            HJFNLogw("waning, video info in null");
            return false;
        }
        const auto otherCodecParam = inInfo->getCodecParams();
        const auto codecParam = m_info->getCodecParams();
        if (!codecParam || !codecParam->isEqual(otherCodecParam)) {
            m_info->setCodecParams(otherCodecParam);
            HJFNLogi("set flush codec params");
            return true;
        }
        //AVCodecParameters* inCodecParam = (AVCodecParameters*)inInfo->getAVCodecParams();
        //AVCodecParameters* codecParam = (AVCodecParameters*)m_info->getAVCodecParams();
        //if (!codecParam || !av_codec_params_compare(inCodecParam, codecParam)) {
        //    m_info->setAVCodecParams(av_dup_codec_params(inCodecParam));
        //    HJFLogi("set flush codec params");s
        //    return true;
        //}
    }
    bool invalid = false;
    do {
        if (!m_parser || (HJFRAME_KEY != frame->getFrameType())) {
            break;
        }
        const AVPacket* pkt = frame->getAVPacket();
        if (!pkt) {
            break;
        }
        if (AV_NOPTS_VALUE == pkt->dts || AV_NOPTS_VALUE == pkt->pts) {
            HJFNLogw("checkFrame dts pts invalid");
        }
//        uint64_t tt0 = HJUtils::getCurrentMillisecond();
        int ret = m_parser->parse(pkt);
        if (HJ_OK != ret) {
            break;
        }
        
        if (m_parser->getReboot())
        {
            HJSizei size = m_parser->getVSize();
            HJFNLogi("key video frame new size:{}, {}", size.w, size.h);
            
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
            const auto vcodecParam = videoInfo->getCodecParams();
            //AVCodecParameters* codecParam = (AVCodecParameters *)videoInfo->getAVCodecParams();
            if (!vcodecParam) {
                return false;
            }
            const auto newCodecParma = vcodecParam->dup();
            AVCodecParameters* avcp = newCodecParma->getAVCodecParameters();
            if (!avcp) {
                return false;
            }
            const HJBuffer::Ptr exbuffer = m_parser->getExBuffer();
            if (!exbuffer || exbuffer->size() <= 0) {
                return false;
            }
            avcp->profile = m_parser->getProfile();
            avcp->level = m_parser->getLevel();
            avcp->width = size.w;
            avcp->height = size.h;
            if (avcp->extradata) {
                av_freep(&avcp->extradata);
            }
            //
            avcp->extradata = (uint8_t *)av_mallocz(exbuffer->size() + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(avcp->extradata, exbuffer->data(), exbuffer->size());
            avcp->extradata_size = (int)exbuffer->size();
            //
            m_info->setCodecParams(newCodecParma);

            invalid = true;
            HJFNLogi("reboot codec params");
        }
    } while (false);
    
    return invalid;
}

int HJVDecFFMpeg::reset()
{
    HJFNLogi("entry");
    done();
    return init(m_info);
}

void HJVDecFFMpeg::done()
{
    if (m_parser) {
        m_parser->done();
        m_parser = nullptr;
    }
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        if (m_bMediacodecDec)
        {
            //avcodec can use surface, after DeleteGlobalRef surface 
            //avcodec_close(avctx);
            av_mediacodec_default_free(avctx);
        }
        avcodec_free_context(&avctx);
        m_avctx = NULL;
    }
    return;
}

int HJVDecFFMpeg::buildConverters()
{
    //HJVideoInfo::Ptr outVInfo = std::make_shared<HJVideoInfo>();
    //outVInfo->m_width = 360;
    //outVInfo->m_height = 640;
    //addOutInfo(outVInfo);

    for (auto& it : m_outInfos)
    {
        HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(it.second);
        auto converter = std::make_shared<HJVideoConverter>(videoInfo);
        m_videoConverters.push_back(converter);
    }
    return HJ_OK;
}

//***********************************************************************************//
HJVDecFFMpegPlus::HJVDecFFMpegPlus()
    : HJVDecFFMpeg()
{
    m_popFrontFrame = true;
}

int HJVDecFFMpegPlus::getFrame(HJMediaFrame::Ptr& frame)
{
    int res = HJVDecFFMpeg::getFrame(frame);
    if (res < HJ_OK) {
        return res;
    }
    if (m_popFrontFrame && m_firstFrame && !frame)
    {
        HJFNLogi("pop front frame entry");
        int retryCount = 0;
        do
        {
            res = HJVDecFFMpeg::run(m_firstFrame);
            if (HJ_OK != res) {
                break;
            }
            res = HJVDecFFMpeg::getFrame(frame);
            if (res < HJ_OK) {
                break;
            }
            retryCount++;
        } while (!frame && retryCount < 16);
        //
        m_firstFrame = nullptr;
        m_popFrontFrame = false;

        if (frame) {
            HJFNLogi("pop front frame end:{}, retryCount:{}", frame->formatInfo(), retryCount);
        }
    }
    //check
    if (frame)
    {
        if (frame->isKeyFrame() && m_checkFirstFrameFlag)
        {
            auto dts = frame->getDTS();
            if (dts <= m_firstFrameDTS) {
                HJFNLogi("discard frame:{}", frame->formatInfo());
                frame = nullptr;
            }
            if (HJ_NOTS_VALUE == m_firstFrameDTS) {
                m_firstFrameDTS = dts;
            }
        } else {
            m_checkFirstFrameFlag = false;
        }
    } 
    //if (frame) {
    //    HJFNLogi("frame:{}", frame->formatInfo());
    //}
    
    return res;
}

int HJVDecFFMpegPlus::run(const HJMediaFrame::Ptr frame)
{
    //HJFNLogi("frame:{}", frame->formatInfo());
    if (m_popFrontFrame && !m_firstFrame) {
        m_firstFrame = frame;
    }
    return HJVDecFFMpeg::run(frame);
}

NS_HJ_END

