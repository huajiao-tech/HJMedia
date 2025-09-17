//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJADecFFMpeg.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

#define HJ_HAVE_AUDIO_CONVERTER    1

NS_HJ_BEGIN
//***********************************************************************************//
HJADecFFMpeg::HJADecFFMpeg()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJADecFFMpeg)));
}

HJADecFFMpeg::~HJADecFFMpeg()
{
    done();
}

int HJADecFFMpeg::init(const HJStreamInfo::Ptr& info)
{
    int res = HJBaseCodec::init(info);
    if (HJ_OK != res) {
        HJFNLoge("base init error:{}", res);
        return res;
    }
    HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);
    AVCodecParameters* codecParam = audioInfo->getAVCodecParams();
    if (!codecParam) {
        HJFNLoge("can't find codec params error");
        return HJErrInvalidParams;
    }
    HJFNLogi("init entry, chanels:{}, sample rate:{}, sample fmt:{}", audioInfo->m_channels, audioInfo->m_samplesRate, audioInfo->m_sampleFmt);
    m_codecID = (AV_CODEC_ID_NONE != codecParam->codec_id) ? codecParam->codec_id : audioInfo->getCodecID();
    const AVCodec* codec = avcodec_find_decoder((AVCodecID)m_codecID);
    if (!codec) {
        HJFNLoge("can't find property codec error, codec id:{}", m_codecID);
        return HJErrFFCodec;
    }
    AVCodecContext* avctx = avcodec_alloc_context3(codec);
    if (!avctx) {
        HJFNLoge("alloc context error");
        return HJErrFFNewAVCtx;
    }
    res = avcodec_parameters_to_context(avctx, codecParam);
    if (HJ_OK != res) {
        HJFNLoge("codec parms from error");
        return res;
    }
    //
    m_timeBase = { 1, audioInfo->m_samplesRate };
    avctx->pkt_timebase = av_rational_from_hj_rational(m_timeBase);
    avctx->time_base = av_rational_from_hj_rational(m_timeBase);
    //avctx->thread_count = std::thread::hardware_concurrency();
    
    AVDictionary* opts = NULL;
    res = avcodec_open2(avctx, codec, &opts);
    if (res < HJ_OK) {
        HJFNLoge("codec open error");
        return res;
    }
    m_avctx = avctx;    
    //
    //buildConverters();

    m_runState = HJRun_Init;
    HJFNLogi("init end");
    
    return res;
}

int HJADecFFMpeg::getFrame(HJMediaFrame::Ptr& frame)
{
    if(!m_avctx) {
        return HJErrNotAlready;
    }
    //HJLogi("getFrame begin");
    if (m_fifo) {
        HJMediaFrame::Ptr mavf = m_fifo->getFrame();
        if (mavf) {
            frame = std::move(mavf);
            //
            //HJNipInterval::Ptr nip = m_nipMuster->getOutNip();
            //if (frame && nip && nip->valid()) {
            //    HJFNLogi(frame->formatInfo());
            //}
            HJFPERNLogi(frame->formatInfo());
            return HJ_OK;
        }
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
                frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
                m_runState = HJRun_Done;
                res = HJ_EOF;
            }
            break;
        } else if(res < HJ_OK) {
            HJFNLoge("receive frame error:{}", res);
            res = HJErrFFGetFrame;
            break;
        } else {
            HJMediaFrame::Ptr mvf = HJMediaFrame::makeAudioFrame();
            if (!mvf) {
                res = HJErrNewObj;
                break;
            }
            //
            avf->time_base = av_rational_from_hj_rational(m_timeBase); //avctx->time_base;
            avf->pts = avf->best_effort_timestamp;
            mvf->setPTSDTS(avf->best_effort_timestamp, avf->best_effort_timestamp, m_timeBase);
            mvf->setDuration(avf->duration, m_timeBase);
            if((/*avf->key_frame*/(avf->flags & AV_FRAME_FLAG_KEY) && avf->pict_type && (avf->pict_type == AV_PICTURE_TYPE_I))) {
                mvf->setFrameType(HJFRAME_KEY);
            }
            //
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(mvf->m_info);
//            audioInfo->setChannels(avctx->ch_layout.nb_channels, (HJHND)&avctx->ch_layout);
            audioInfo->setChannels(&avf->ch_layout);
            audioInfo->m_sampleFmt = avf->format;   //avctx->sample_fmt;
            audioInfo->m_bytesPerSample = av_get_bytes_per_sample((AVSampleFormat)audioInfo->m_sampleFmt);
            audioInfo->m_samplesRate = avf->sample_rate;
            audioInfo->m_sampleCnt = avf->nb_samples;
            audioInfo->m_samplesPerFrame = avctx->frame_size;
            audioInfo->m_blockAlign = avctx->block_align;
            audioInfo->m_dataType = HJDATA_TYPE_RS;
            //
            mvf->setMFrame(avf); 
            avf = NULL;
            //
            HJStreamInfo::Ptr outAInfo = getOutInfo();
            if (outAInfo)
            {
                HJAudioInfo::Ptr cvtAInfo = std::dynamic_pointer_cast<HJAudioInfo>(outAInfo);
#if HJ_HAVE_AUDIO_CONVERTER
                //if (!m_converter && (HJ_SAMPLE_RATE_DEFAULT != audioInfo->m_samplesRate || HJ_CHANNEL_DEFAULT != audioInfo->m_channels))
                if (!m_converter && (cvtAInfo->m_samplesRate != audioInfo->m_samplesRate || cvtAInfo->m_channels != audioInfo->m_channels || cvtAInfo->m_sampleFmt != audioInfo->m_sampleFmt))
                {
                    //HJAudioInfo::Ptr cvtAInfo = std::dynamic_pointer_cast<HJAudioInfo>(audioInfo->dup());
                    //cvtAInfo->setDefaultSampleRate();
                    //cvtAInfo->setDefaultChannels();
                    m_converter = std::make_shared<HJAudioConverter>(cvtAInfo);
                    if (!m_converter) {
                        res = HJErrNewObj;
                        break;
                    }
                }
#else
//                if (!m_adopter && (HJ_SAMPLE_RATE_DEFAULT != audioInfo->m_samplesRate || HJ_CHANNEL_DEFAULT != audioInfo->m_channels))
                if (!m_adopter && (cvtAInfo->m_samplesRate != audioInfo->m_samplesRate || cvtAInfo->m_channels != audioInfo->m_channels))
                {
//                    HJAudioInfo::Ptr cvtAInfo = std::dynamic_pointer_cast<HJAudioInfo>(audioInfo->dup());
//                    cvtAInfo->setDefaultSampleRate();
//                    cvtAInfo->setDefaultChannels();
                    m_adopter = std::make_shared<HJAudioAdopter>(cvtAInfo);
                    if (!m_adopter) {
                        return HJErrNewObj;
                    }
                }
#endif //HJ_HAVE_AUDIO_CONVERTER
                if (m_converter) {
                    mvf = m_converter->convert(std::move(mvf));
                }
                if (m_adopter) {
                    mvf = m_adopter->convert(std::move(mvf));
                }
                if (!mvf) {
                    HJFNLoge("error, audio convert result");
                    break;
                }
                const HJAudioInfo::Ptr mavfAInfo = mvf->getAudioInfo();
                if (!m_fifo && cvtAInfo->m_samplesPerFrame != mavfAInfo->m_sampleCnt)
                {
                    m_fifo = std::make_shared<HJAudioFifo>(mavfAInfo->m_channels, mavfAInfo->m_sampleFmt, mavfAInfo->m_samplesRate);
                    res = m_fifo->init(cvtAInfo->m_samplesPerFrame);
                    if (HJ_OK != res) {
                        HJFNLoge("audio fifo init error");
                        break;
                    }
                }
                if (m_fifo) 
                {
                    res = m_fifo->addFrame(std::move(mvf));
                    if (HJ_OK != res) {
                        HJFNLoge("audio fifo add frame error");
                        break;
                    }
                    mvf = m_fifo->getFrame();
                }
            } //outAInfo

            if (mvf) 
            {
                if (m_isChanged) {
                    mvf->setFlush();
                    m_isChanged = false;
                }
                frame = std::move(mvf);
                //
                //HJNipInterval::Ptr nip = m_nipMuster->getOutNip();
                //if (frame && nip && nip->valid()) {
                //    HJFNLogi(frame->formatInfo());
                //}
                if (frame) {
                    HJFPERNLogi(frame->formatInfo());
                }
            }
        }
    } while (false);

    if (avf) {
        av_frame_free(&avf);
    }

    return res;
}

int HJADecFFMpeg::run(const HJMediaFrame::Ptr frame)
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
        //
        AVPacket* pkt = NULL;
        if(mvf && (HJFRAME_EOF != mvf->getFrameType()))
        {
            mvf->setAVTimeBase(m_timeBase);
            pkt = mvf->getAVPacket();
        } else {
            nullPkt = hj_make_null_packet();
            pkt = nullPkt;
        }
        if (!pkt) {
            HJFNLoge("run get avpacket error");
            return HJErrInvalidParams;
        }
        //HJNipInterval::Ptr nip = m_nipMuster->getInNip();
        //if (mvf && nip && nip->valid()) {
        //    HJFNLogi(mvf->formatInfo());
        //}
        if (mvf) {
            HJFPERNLogi(mvf->formatInfo());
        }
        //
        res = avcodec_send_packet(avctx, pkt);
        if (AVERROR(EAGAIN) == res) {
            return HJ_WOULD_BLOCK;
        } else if(AVERROR_EOF == res) {
            m_runState = HJRun_Stop;
            return HJ_EOF;
        } else if (res < HJ_OK){
            HJFNLoge("auido decoder send packet error:" + HJ2STR(res));
            return HJErrFatal;
        }
    } while (false);

    if(nullPkt) {
        av_packet_free(&nullPkt);
    }
    return res;
}

int HJADecFFMpeg::flush()
{
    if (m_fifo) {
        m_fifo->reset();
    }
    if(m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_flush_buffers(avctx);
    }
    m_runState = HJRun_Ready;
    return HJ_OK;
}

bool HJADecFFMpeg::checkFrame(const HJMediaFrame::Ptr frame)
{
    if (HJRun_Init == m_runState) {
        return false;
    }
    if (frame->isFlushFrame()) 
    {
        //HJFLogi("flush frame");
        const auto& inInfo = frame->getAudioInfo();
        if (!inInfo) {
            HJFNLogw("waning, audio info in null");
            return false;
        }
        const auto otherCodecParam = inInfo->getCodecParams();
        const auto codecParam = m_info->getCodecParams();
        if (!codecParam || !codecParam->isEqual(otherCodecParam)) {
            m_info->setCodecParams(otherCodecParam);
            HJFNLogi("set flush codec params");
            return true;
        }
        //const auto& inInfo = frame->getAudioInfo();
        //AVCodecParameters* codecParam = (AVCodecParameters*)inInfo->getAVCodecParams();
        //if (codecParam && (codecParam != m_info->getAVCodecParams())) {
        //    m_info->setAVCodecParams(av_dup_codec_params(codecParam));
        //    HJFLogi("set flush codec params");
        //    return true;
        //}
    }
    return false;
}

int HJADecFFMpeg::reset()
{
    HJFNLogi("entry");
    done();
    return init(m_info);
}

void HJADecFFMpeg::done()
{
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_free_context(&avctx);
        m_avctx = NULL;
    }
    m_converter = nullptr;
    m_adopter = nullptr;
    m_fifo = nullptr;
}

int HJADecFFMpeg::buildConverters()
{
 //   HJAudioInfo::Ptr outAInfo = std::make_shared<HJAudioInfo>();
	//outAInfo->m_sampleFmt = AV_SAMPLE_FMT_FLTP;  //AV_SAMPLE_FMT_S16;
 //   outAInfo->setChannels(2);
 //   addOutInfo(outAInfo);

    auto it = m_outInfos.begin();
    if (it != m_outInfos.end()) {
        HJAudioInfo::Ptr ainfo = std::dynamic_pointer_cast<HJAudioInfo>(it->second);
        if (!ainfo) {
            return HJErrNotAlready;
        }
		m_converter = std::make_shared<HJAudioConverter>(ainfo);
		if (!m_converter) {
			return HJErrNewObj;
		}
        //m_adopter = std::make_shared<HJAudioAdopter>(ainfo);
        //if (!m_adopter) {
        //    return HJErrNewObj;
        //}
    }
    return HJ_OK;
}

NS_HJ_END

