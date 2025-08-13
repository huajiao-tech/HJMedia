//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAEncFFMpeg.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return HJ_OK;
        p++;
    }
    return HJErrNotSupport;
}

static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec, AVChannelLayout *dst)
{
    const AVChannelLayout *p, *best_ch_layout;
    int best_nb_channels   = 0;

    if (!codec->ch_layouts) {
#if defined(HJ_OS_WIN32)
        dst->order = (AVChannelOrder)AV_CHANNEL_ORDER_NATIVE;
        dst->nb_channels = 2;
        dst->u.mask = AV_CH_LAYOUT_STEREO;
        return HJ_OK;
#else
        AVChannelLayout ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
        return av_channel_layout_copy(dst, &ch_layout);
#endif
    }

    p = codec->ch_layouts;
    while (p->nb_channels) {
        int nb_channels = p->nb_channels;

        if (nb_channels > best_nb_channels) {
            best_ch_layout   = p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return av_channel_layout_copy(dst, best_ch_layout);
}
//***********************************************************************************//
HJAEncFFMpeg::HJAEncFFMpeg()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJAEncFFMpeg)));
    m_codecID = AV_CODEC_ID_AAC;
}

HJAEncFFMpeg::~HJAEncFFMpeg()
{
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_free_context(&avctx);
        m_avctx = NULL;
    }
}

int HJAEncFFMpeg::init(const HJStreamInfo::Ptr& info)
{
    HJLogi("init begin");
    int res = HJBaseCodec::init(info);
    if (HJ_OK != res) {
        return res;
    }
    HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);
    //
    if (AV_CODEC_ID_NONE != audioInfo->m_codecID) {
        m_codecID = audioInfo->m_codecID;
    } else {
        audioInfo->m_codecID = m_codecID;
    }
    const AVCodec* codec = avcodec_find_encoder((AVCodecID)m_codecID);
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
    avctx->sample_fmt = (AVSampleFormat)audioInfo->m_sampleFmt;  //AV_SAMPLE_FMT_S16
//    avctx->channels = audioInfo->m_channels;
//    avctx->channel_layout = audioInfo->m_channelLayout;
    HJChannelLayout::Ptr chLayout = audioInfo->getChannelLayout();
    if (chLayout) {
        av_channel_layout_copy(&avctx->ch_layout, (const AVChannelLayout *)chLayout->getAVCHLayout());
    } else {
        av_channel_layout_default(&avctx->ch_layout, audioInfo->m_channels);
    }
    avctx->bit_rate = audioInfo->m_bitrate;
    //
    res = check_sample_fmt(codec, avctx->sample_fmt);
    if (res < HJ_OK) {
        HJLoge("sample format error");
        return res;
    }
    avctx->sample_rate = audioInfo->m_samplesRate;
    if (!avctx->sample_rate) {
        avctx->sample_rate = select_sample_rate(codec);
    }
    res = select_channel_layout(codec, &avctx->ch_layout);
    if (res < HJ_OK) {
        HJLoge("select channel layout error");
        return res;
    }
	m_timeBase = { 1, audioInfo->m_samplesRate };
	avctx->time_base = av_rational_from_hj_rational(m_timeBase);
    avctx->pkt_timebase = avctx->time_base;
	//if ((*output_format_context)->oformat->flags & AVFMT_GLOBALHEADER)
	avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    res = avcodec_open2(avctx, codec, NULL);
    if (res < HJ_OK) {
        HJLoge("avcodec open error:" + HJ2STR(res));
        return res;
    }
    m_info->setAVCodecParams(av_dup_codec_params_from_avcodec(avctx));
    //
    m_runState = HJRun_Init;
    HJLogi("init end");

    return res;
}

int HJAEncFFMpeg::getFrame(HJMediaFrame::Ptr& frame)
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
            frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
            m_runState = HJRun_Done;
            res = HJ_EOF;
            break;
        } else if(res < HJ_OK) {
            HJLoge("receive frame error:" + HJ2String(res));
            res = HJErrFFGetFrame;
            break;
        } else
        {
            HJMediaFrame::Ptr mvf = HJMediaFrame::makeAudioFrame();
            if (!mvf) {
                res = HJErrNewObj;
                break;
            }
            pkt->time_base = av_rational_from_hj_rational(m_timeBase);
            mvf->setPTSDTS(pkt->pts, pkt->dts, m_timeBase);
            //mvf->m_dts = av_time_to_ms(pkt->dts, avctx->time_base);
            //mvf->m_pts = av_time_to_ms(pkt->pts, avctx->time_base);
            //mvf->m_duration = avctx->sample_rate ? (HJ_MS_PER_SEC * pkt->duration) / avctx->sample_rate : 23;
            mvf->m_duration = avctx->sample_rate ?  HJ_SEC_TO_MS(pkt->duration / (double)avctx->sample_rate) : 23;
            //
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(mvf->m_info);
            audioInfo->clone(m_info);
            audioInfo->m_dataType = HJDATA_TYPE_ES;
            //
            mvf->setMPacket(pkt);
            pkt = NULL;
            //
            frame = std::move(mvf);

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

int HJAEncFFMpeg::run(const HJMediaFrame::Ptr frame)
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
    AVCodecContext* avctx = (AVCodecContext*)m_avctx;
    do {
        m_runState = HJRun_Running;
        if(frame && (HJFRAME_EOF != frame->getFrameType()))
        {
            avf = frame->getAVFrame();
            if (!avf) {
                avf = (AVFrame*)frame->makeAVFrame();
            }
            if (!avf) {
                HJLogi("have no AVFrame");
                res = HJErrInvalidParams;
                break;
            }
            frame->setAVTimeBase(m_timeBase);
            //
            HJNipInterval::Ptr nip = m_nipMuster->getInNip();
            if (frame && nip && nip->valid()) {
                HJLogi("name:" + getName() + ", " + frame->formatInfo());
            }
        } else {
            //HJLogi("audio encoder send frame pts:" + HJ2STR(frame->getPTS()) + ", dts:" + HJ2STR(frame->getDTS()));
        }
        //
        res = avcodec_send_frame(avctx, avf);
        if (AVERROR(EAGAIN) == res) {
            return HJ_WOULD_BLOCK;
        } else if(AVERROR_EOF == res) {
            m_runState = HJRun_Stop;
            return HJ_EOF;
        } else if (res < HJ_OK){
            HJLoge("error, audio encoder send frame");
            return HJErrFatal;
        }
        m_frameCounter++;
    } while (false);
    
    return HJ_OK;
}

int HJAEncFFMpeg::flush()
{
    if(m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_flush_buffers(avctx);
    }
    m_runState = HJRun_Init;
    return HJ_OK;
}

NS_HJ_END
