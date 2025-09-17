//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaFrame.h"
#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJException.h"

NS_HJ_BEGIN
//***********************************************************************************//
const std::string HJMediaFrame::KEY_WORLDS = HJ_TYPE_NAME(HJMediaFrame);
const std::string HJMediaFrame::STORE_KEY_HJAVPACKET = HJ_TYPE_NAME(HJAVPacket);
const std::string HJMediaFrame::STORE_KEY_HJAVFRAME = HJ_TYPE_NAME(HJAVFrame);
const std::string HJMediaFrame::STORE_KEY_MEDIAINFO = HJ_TYPE_NAME(HJMediaInfo);
const std::string HJMediaFrame::STORE_KEY_STREAMINFO = HJ_TYPE_NAME(HJStreamInfo);
const std::string HJMediaFrame::STORE_KEY_SEIINFO = "SEI_INFO";
const std::string HJMediaFrame::STORE_KEY_ROIINFO = "ROI_INFO";
const std::string HJMediaFrame::STORE_KEY_VID_BITRATE = "vid_bitrate";
const std::string HJMediaFrame::STORE_KEY_EXTRADATAS = "HJ_EXTRA_DATAS";

//const std::string HJMediaFrame::makeAVFrameStoreKey(int width, int height, int fmt)
//{
//    std::string key = HJMediaFrame::STORE_KEY_HJAVFRAME + std::string("_") + HJ2STR(width) + std::string("_") + HJ2STR(height) + std::string("_") + HJ2STR(fmt);
//    return key;
//}
//
const std::string HJMediaFrame::frameType2Name(const int& type)
{
    static const std::unordered_map<HJFrameType, std::string> g_frameTypeNames = {
        {HJFRAME_NORMAL, "normal"},
        {HJFRAME_KEY, "key"},
        {HJFRAME_EOF, "eof"},
        {HJFRAME_FLUSH, "flush"}
    };
    auto it = g_frameTypeNames.find((HJFrameType)HJFRAME_MAINTYPE(type));
    return (it != g_frameTypeNames.end()) ? it->second : "";
}

const std::string HJMediaFrame::vesfType2Name(const HJVESFrameType& type)
{
    static const std::unordered_map<HJVESFrameType, std::string> g_vesFrameTypeNames = {
        {HJ_VESF_NONE, "none"},
        {HJ_VESF_IDR, "IDR"},
        {HJ_VESF_I, "I"},
        {HJ_VESF_P, "P"},
        {HJ_VESF_BRef, "BRef"},
        {HJ_VESF_B, "B"},
    };        
    auto it = g_vesFrameTypeNames.find(type);
    return (it != g_vesFrameTypeNames.end()) ? it->second : "";
}

//
HJMediaFrame::HJMediaFrame(const HJStreamInfo::Ptr& info/* = nullptr*/)
    : m_info(info)
{
    m_tracker = std::make_shared<HJTracker>();
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJMediaFrame)));
}

HJMediaFrame::~HJMediaFrame()
{
    m_tracker = nullptr;
    //removeAVPacket();
    //removeAVFrame();
    //removeAuxAVFrame();
}

int HJMediaFrame::setData(uint8_t* data, size_t size)
{
    if (!data || size <= 0) {
        return HJErrInvalidParams;
    }
    if (!m_Buffer) {
        m_Buffer = std::make_shared<HJBuffer>(data, size);
    } else {
        m_Buffer->setData(data, size);
    }
    if (m_info) {
        m_info->m_codecID = HJDATA_TYPE_ES;
    }
    return HJ_OK;
}

int HJMediaFrame::appendData(uint8_t* data, size_t size)
{
    if (!data || size <= 0) {
        return HJErrInvalidParams;
    }
    if (!m_Buffer) {
        m_Buffer = std::make_shared<HJBuffer>(data, size);
    } else {
        m_Buffer->appendData(data, size);
    }
    if (m_info) {
        m_info->m_codecID = HJDATA_TYPE_ES;
    }
    return HJ_OK;
}

const int64_t HJMediaFrame::getDTSUS()
{
    AVFrame* avf = (AVFrame *)getAVFrame();
    if (avf) {
        return av_rescale_q(avf->best_effort_timestamp, avf->time_base, { HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den });
    }
    AVPacket* pkt = (AVPacket*)getAVPacket();
    if (pkt) {
        return av_rescale_q(pkt->dts, pkt->time_base, { HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den });
    }
    return 0;
}

const int64_t HJMediaFrame::getPTSUS()
{
    AVFrame* avf = (AVFrame*)getAVFrame();
    if (avf) {
        return av_rescale_q(avf->best_effort_timestamp, avf->time_base, { HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den });
    }
    AVPacket* pkt = (AVPacket*)getAVPacket();
    if (pkt) {
        return av_rescale_q(pkt->pts, pkt->time_base, { HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den });
    }
    return 0;
}

/**
 * pts��dts
 * time base ����
 */
void HJMediaFrame::setPTSDTS(const int64_t pts, const int64_t dts, const HJTimeBase tb)
{
    AVRational src_tb = av_rational_from_hj_rational(tb);
	m_dts = av_time_to_ms(dts, src_tb);
	m_pts = av_time_to_ms(pts, src_tb);
    //m_duration = m_timeBase.den ? (HJ_MS_PER_SEC * m_timeBase.num / m_timeBase.den) : 66;
	//AVFrame, auxiliary frame
    auto allmfs = getALLMFrames();
    for (const auto& mf : allmfs) {
        mf->setTimeStamp(pts, dts, tb);
    }
    auto mpkt = getMPacket();
    if (mpkt) {
        mpkt->setTimeStamp(pts, dts, tb);
    }
    return;
}

/**
 * pts��dts 
 * �޸� AVFrame, AVPacket��time base
 */
void HJMediaFrame::setAVTimeBase(const HJTimeBase& timeBase)
{
    AVRational dst_tb = av_rational_from_hj_rational(timeBase);
    //AVFrame, auxiliary frame
    auto allmfs = getALLMFrames();
    for (const auto& mf : allmfs) {
        mf->setTimeBase(timeBase);
    }
    auto mpkt = getMPacket();
    if (mpkt) {
        mpkt->setTimeBase(timeBase);
    }
    return;
}

void HJMediaFrame::setDuration(const int64_t duration, const HJRational timeBase)
{
    m_duration = av_rescale_q(duration, { timeBase.num, timeBase.den }, { m_timeBase.num, m_timeBase.den });
}

void HJMediaFrame::setMTS(const int64_t pts, const int64_t dts, const int64_t duration, const HJTimeBase tb)
{
    int64_t pts1 = av_time_to_time(pts, {tb.num, tb.den}, {HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den});
    int64_t dts1 = av_time_to_time(dts, {tb.num, tb.den}, {HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den});
    int64_t duration1 = av_time_to_time(duration, {tb.num, tb.den}, {HJ_TIMEBASE_US.num, HJ_TIMEBASE_US.den});
    m_mts = HJMediaTime(pts1, dts1, duration1, HJ_TIMEBASE_US);
    return;
}

void HJMediaFrame::setMPacket(AVPacket* pkt)
{
    if (!pkt) {
        return;
    }
    (*this)[STORE_KEY_HJAVPACKET] = std::make_shared<HJAVPacket>(pkt);
    return;
}

void HJMediaFrame::setMPacket(const HJAVPacket::Ptr& mpkt)
{
    (*this)[STORE_KEY_HJAVPACKET] = mpkt;
}

HJAVPacket::Ptr HJMediaFrame::getMPacket()
{
    HJAVPacket::Ptr pkt = nullptr;
	auto it = find(STORE_KEY_HJAVPACKET);
	if (it != end()) {
        pkt = std::any_cast<HJAVPacket::Ptr>(it->second);
	}
	return pkt;
}

AVPacket* HJMediaFrame::getAVPacket()
{
    const auto mpkt = getMPacket();
    if(mpkt) {
        return mpkt->getAVPacket();
    }
    return NULL;
}

void HJMediaFrame::setMFrame(AVFrame* avf)
{
    if (!avf) {
        return;
    }
    (*this)[STORE_KEY_HJAVFRAME] = std::make_shared<HJAVFrame>(avf);
    return;
}

void HJMediaFrame::setMFrame(const HJAVFrame::Ptr& mavf)
{
    (*this)[STORE_KEY_HJAVFRAME] = mavf;
}

HJAVFrame::Ptr HJMediaFrame::getMFrame()
{
    HJAVFrame::Ptr frame = nullptr;
	auto it = find(STORE_KEY_HJAVFRAME);
	if (it != end()) {
        frame = std::any_cast<HJAVFrame::Ptr>(it->second);
	}
	return frame;
}

AVFrame* HJMediaFrame::getAVFrame()
{
    const auto mf = getMFrame();
    if (mf) {
        return mf->getAVFrame();
    }
    return NULL;
}

void HJMediaFrame::setAuxMFrame(const HJAVFrame::Ptr& mavf)
{
    if (!mavf) {
        return;
    }
    std::string key = mavf->getKey();
    if (!key.empty()) {
        (*this)[key] = mavf;          //auxiliary frame
    }
    return;
}

HJAVFrameVector HJMediaFrame::getAuxMFrames()
{
    HJAVFrameVector auxMFrames;
    for (auto it = begin(); it != end(); it++) {
        std::string key = it->first;
        if ((key != HJMediaFrame::STORE_KEY_HJAVFRAME) &&
            HJUtilitys::containWith(key, HJMediaFrame::STORE_KEY_HJAVFRAME)) {
            HJAVFrame::Ptr mcf = std::any_cast<HJAVFrame::Ptr>(it->second);
            auxMFrames.emplace_back(mcf);
        }
    }
    return auxMFrames;
}

HJAVFrameVector HJMediaFrame::getALLMFrames()
{
    HJAVFrameVector allFrames;
    for (auto it = begin(); it != end(); it++) {
        std::string key = it->first;
        if (HJUtilitys::containWith(key, HJMediaFrame::STORE_KEY_HJAVFRAME)) {
            HJAVFrame::Ptr mcf = std::any_cast<HJAVFrame::Ptr>(it->second);
            allFrames.emplace_back(mcf);
        }
    }
    return allFrames;
}

HJHND HJMediaFrame::makeAVFrame()
{
    AVFrame* avf = (AVFrame *)getAVFrame();
    if(avf) {
        return avf;
    }
//    if (!m_Buffer) {
//        return NULL;
//    }
    avf = av_frame_alloc();
    if (!avf) {
        return NULL;
    }
    int res = HJ_OK;
    do {
        const HJMediaType mediaType = getType();
        if (HJMEDIA_TYPE_AUDIO == mediaType)
        {
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);
            AVChannelLayout* ch_layout = (AVChannelLayout *)audioInfo->getAVChannelLayout();
            if (!ch_layout) {
                res = HJErrNotAlready;
                break;
            }
            avf->time_base = {1, audioInfo->m_samplesRate};
			avf->pts = av_rescale_q(m_pts, av_rational_from_hj_rational(m_timeBase), avf->time_base);
            avf->nb_samples = audioInfo->m_sampleCnt;
            avf->format = audioInfo->m_sampleFmt;
//            avf->channels = audioInfo->m_channels;
//            avf->channel_layout = audioInfo->m_channelLayout;
            res = av_channel_layout_copy(&avf->ch_layout, ch_layout);
            if (res < HJ_OK){
                break;
            }
            avf->sample_rate = audioInfo->m_samplesRate;
            res = av_frame_get_buffer(avf, 0);
            if (res < HJ_OK){
                break;
            }
//            res = av_frame_make_writable(avf);
//            if (res < HJ_OK){
//                break;
//            }
            if (m_Buffer) {
                memcpy(avf->data[0], m_Buffer->data(), m_Buffer->size());
            }
        } else if (HJMEDIA_TYPE_VIDEO == mediaType)
        {
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(m_info);
            avf->width = videoInfo->m_width;
            avf->height = videoInfo->m_height;
            avf->format = videoInfo->m_pixFmt;
            int res = av_frame_get_buffer(avf, 0);
            if (res < 0) {
                HJLoge("error, avframe get buffer failed, res:" + HJ2STR(res));
                break;
            }
//            av_frame_make_writable(avf);
        }
        setMFrame(avf);
    } while (false);
    
    if (HJ_OK != res && avf) {
        av_frame_free(&avf);
        avf = NULL;
    }
    return avf;
}

HJMediaFrame::Ptr HJMediaFrame::makeVideoFrame(const HJVideoInfo::Ptr& info)
{
    HJMediaFrame::Ptr frame = nullptr;
    if(info) {
//        HJVideoInfo::Ptr mInfo = std::dynamic_pointer_cast<HJ::HJVideoInfo>(info->dup());
        frame = std::make_shared<HJMediaFrame>(info);
    } else {
        frame = std::make_shared<HJMediaFrame>();
        frame->m_info = std::make_shared<HJVideoInfo>();
    }
    return frame;
}

HJMediaFrame::Ptr HJMediaFrame::makeAudioFrame(const HJAudioInfo::Ptr& info)
{
    HJMediaFrame::Ptr frame = nullptr;
    if(info) {
//        HJAudioInfo::Ptr mInfo = std::dynamic_pointer_cast<HJ::HJAudioInfo>(info->dup());
        frame = std::make_shared<HJMediaFrame>(info);
    } else {
        frame = std::make_shared<HJMediaFrame>();
        frame->m_info = std::make_shared<HJAudioInfo>();
    }
    return frame;
}

HJMediaFrame::Ptr HJMediaFrame::makeSilenceAudioFrame(const HJAudioInfo::Ptr info/* = nullptr*/)
{
	HJMediaFrame::Ptr maf = std::make_shared<HJMediaFrame>();
    maf->m_info = info ? info : std::make_shared<HJAudioInfo>();
    maf->setFrameType(HJFRAME_KEY);
    
    HJAudioInfo::Ptr ainfo = maf->getAudioInfo();
    AVFrame* avf = hj_make_silence_audio_frame(ainfo->m_channels, ainfo->m_samplesRate, (enum AVSampleFormat)ainfo->m_sampleFmt, ainfo->m_sampleCnt, (AVChannelLayout*)ainfo->getAVChannelLayout());
    if (!avf){
        return nullptr;
    }
    maf->setMFrame(avf);

    return maf;
}

HJMediaFrame::Ptr HJMediaFrame::makeSilenceAudioFrame(int channel, int sampleRate, int sampleFmt, int nbSamples)
{
    HJAudioInfo::Ptr ainfo = std::make_shared<HJAudioInfo>();
    ainfo->m_channels = channel;
    ainfo->m_samplesRate = sampleRate;
    ainfo->m_sampleFmt = sampleFmt;
    ainfo->m_bytesPerSample = av_get_bytes_per_sample((AVSampleFormat)ainfo->m_sampleFmt);
    ainfo->m_sampleCnt = nbSamples;
    ainfo->m_samplesPerFrame = ainfo->m_sampleCnt;

    return makeSilenceAudioFrame(ainfo);
}

HJMediaFrame::Ptr HJMediaFrame::makeDefaultSilenceAudioFrame()
{
    return makeSilenceAudioFrame(HJ_CHANNEL_DEFAULT, HJ_SAMPLE_RATE_DEFAULT, HJ_SAMPLE_FORMAT_DEFAULT, HJ_FRAME_SAMPLES_DEFAULT);
}

HJMediaFrame::Ptr HJMediaFrame::makeEofFrame(const HJMediaType type/* = HJMEDIA_TYPE_NONE*/)
{
    HJMediaFrame::Ptr frame = std::make_shared<HJMediaFrame>();
    if(HJMEDIA_TYPE_VIDEO == type) {
        frame->m_info = std::make_shared<HJVideoInfo>();
    } else if (HJMEDIA_TYPE_AUDIO == type) {
        frame->m_info = std::make_shared<HJAudioInfo>();
    } else {
        frame->m_info = std::make_shared<HJStreamInfo>();
    }
    frame->m_frameType = HJFRAME_EOF;
    return frame;
}

HJMediaFrame::Ptr HJMediaFrame::makeFlushFrame(const int streamIndex, const HJMediaInfo::Ptr info, int flag)
{
    HJMediaFrame::Ptr frame = std::make_shared<HJMediaFrame>();
    frame->m_frameType = HJFRAME_FLUSH;
    frame->m_streamIndex = streamIndex;
    frame->m_flag = flag;
    if (info) {
        frame->addStorage(STORE_KEY_MEDIAINFO, info);
    }
    return frame;
}
HJCodecParameters::Ptr HJMediaFrame::makeHJAVCodecParam(const HJStreamInfo::Ptr &i_info, HJBuffer::Ptr i_extradata)  
{
    AVCodecParameters *codecParams = avcodec_parameters_alloc();

    if (i_extradata)
    {
        codecParams->extradata = (uint8_t *)av_mallocz(i_extradata->size() + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(codecParams->extradata, i_extradata->data(), i_extradata->size());
        codecParams->extradata_size = (int)i_extradata->size();
    }
    codecParams->codec_tag = 0;
    codecParams->bits_per_coded_sample = 0;
    codecParams->bits_per_raw_sample = 0;
    codecParams->codec_id = (enum AVCodecID)i_info->getCodecID();

    HJMediaType type = i_info->getType();
    if (type == HJMEDIA_TYPE_AUDIO)
    {
        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(i_info);
        if (audioInfo)
        {
            codecParams->codec_type = AVMEDIA_TYPE_AUDIO;
            codecParams->bit_rate = audioInfo->m_bitrate;
            codecParams->format = audioInfo->m_sampleFmt;
            codecParams->sample_rate = audioInfo->m_samplesRate;
            codecParams->ch_layout = *audioInfo->getAVChannelLayout();
        }
    }
    else if (type == HJMEDIA_TYPE_VIDEO)
    {
        HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(i_info);
        if (videoInfo)
        {
            codecParams->codec_type = AVMEDIA_TYPE_VIDEO;
            codecParams->format = videoInfo->m_pixFmt;
            codecParams->bit_rate = videoInfo->m_bitrate;
            codecParams->width = videoInfo->m_width;
            codecParams->height = videoInfo->m_height;
            codecParams->color_range = (enum AVColorRange)videoInfo->m_colorRange;              // AVCOL_RANGE_UNSPECIFIED;
            codecParams->color_primaries = (enum AVColorPrimaries)videoInfo->m_colorPrimaries;  // AVCOL_PRI_UNSPECIFIED;
            codecParams->color_trc = (enum AVColorTransferCharacteristic)videoInfo->m_colorTrc; // AVCOL_TRC_UNSPECIFIED;
            codecParams->color_space = (enum AVColorSpace)videoInfo->m_colorSpace;              // AVCOL_SPC_UNSPECIFIED;
            codecParams->chroma_location = (enum AVChromaLocation)videoInfo->m_chromaLocation;  // AVCHROMA_LOC_UNSPECIFIED;
        }
    }
    const auto params = std::make_shared<HJCodecParameters>(codecParams);
    return params;
}
int HJMediaFrame::makeAVCodecParam(const HJStreamInfo::Ptr &i_info, HJBuffer::Ptr i_extradata)
{
    int i_err = HJ_OK;
    do
    {
        HJCodecParameters::Ptr params = makeHJAVCodecParam(i_info, i_extradata);
        i_info->setCodecParams(params);
    } while (false);
    return i_err;
}
HJMediaFrame::Ptr HJMediaFrame::makeMediaFrameAsAVFrame(const HJStreamInfo::Ptr &i_info, uint8_t *i_data, int i_size, bool i_bKey, int64_t i_pts, const HJRational& i_ratio)
{
    HJMediaFrame::Ptr frame = nullptr;
    do
    {
        i_info->m_dataType = HJDATA_TYPE_RS;
        HJMediaType type = i_info->getType();
        if (type == HJMEDIA_TYPE_AUDIO)
        {
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(i_info);
            
            if (audioInfo->m_sampleFmt != AV_SAMPLE_FMT_S16)
            {
                HJFLoge("audio format not support, now only support s16");
                frame = nullptr;
                break;
            }

            int bytes_per_sample = av_get_bytes_per_sample((enum AVSampleFormat)audioInfo->m_sampleFmt);
            int sample_count = i_size / (bytes_per_sample * audioInfo->m_channels);
             
            audioInfo->m_sampleCnt = sample_count;

            frame = HJMediaFrame::makeAudioFrame(audioInfo);
            AVFrame* avf = hj_make_silence_audio_frame(audioInfo->m_channels, audioInfo->m_samplesRate, (enum AVSampleFormat)audioInfo->m_sampleFmt, audioInfo->m_sampleCnt, (AVChannelLayout*)audioInfo->getAVChannelLayout());
            if (!avf)
            {
                frame = nullptr;
                break;
            }
            avf->time_base = av_rational_from_hj_rational(i_ratio);
            avf->pts = avf->best_effort_timestamp = i_pts; 
            frame->setDuration(avf->duration, i_ratio);
            
            memcpy(avf->data[0], i_data, i_size);        
            avf->nb_samples = sample_count;
            
            frame->setTimeBase(i_ratio);
            frame->setPTSDTS(i_pts, i_pts, i_ratio);
            if (i_bKey)
            {
                frame->setFrameType(frame->getFrameType() | HJFRAME_KEY);
            }
            frame->setMFrame(avf);
        }
        else if (type == HJMEDIA_TYPE_VIDEO)
        {
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(i_info);
            frame = HJMediaFrame::makeVideoFrame(videoInfo);  
            HJFLoge("video format not support, now only support s16");
            frame = nullptr;
            break;
        }
    } while (false);
    return frame;
}
HJMediaFrame::Ptr HJMediaFrame::makeMediaFrameAsAVPacket(const HJStreamInfo::Ptr &i_info, uint8_t *i_data, int i_size, bool i_bKey, int64_t i_pts, int64_t i_dts, const HJRational& i_ratio)
{
    HJMediaFrame::Ptr frame = nullptr;
    do
    {
        AVPacket *pkt = av_packet_alloc();
        if (!pkt)
        {
            break;
        }
        HJMediaType type = i_info->getType();
        if (type == HJMEDIA_TYPE_AUDIO)
        {
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(i_info);
          
            frame = HJMediaFrame::makeAudioFrame(audioInfo);
            pkt->duration =  HJMediaUtils::calcFrameDuration(i_ratio, {audioInfo->m_sampleCnt, 1});
        }
        else if (type == HJMEDIA_TYPE_VIDEO)
        {
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(i_info);
            frame = HJMediaFrame::makeVideoFrame(videoInfo);  
            pkt->duration = HJMediaUtils::calcFrameDuration(i_ratio, { videoInfo->m_frameRate, 1});
        }

        i_info->m_dataType = HJDATA_TYPE_ES;

        pkt->flags = i_bKey ? AV_PKT_FLAG_KEY : 0;
        pkt->stream_index = (int)i_info->m_streamIndex;   
        pkt->time_base = av_rational_from_hj_rational(i_ratio);
        pkt->pts = i_pts;
        pkt->dts = i_dts;
        pkt->data = i_data;
        pkt->size = i_size;

        int ret = av_packet_make_writable(pkt);
        if (ret < HJ_OK)
        {
            frame = nullptr;
            break;
        }
        if (i_bKey)
        {
            frame->setFrameType(frame->getFrameType() | HJFRAME_KEY);
        }
        
        frame->setTimeBase(i_ratio);
        frame->setPTSDTS(pkt->pts, pkt->dts, i_ratio);
        frame->setDuration(pkt->duration, av_rational_to_hj_rational(pkt->time_base));
        frame->setMPacket(pkt);
        pkt = nullptr;
    } while (false);
    return frame;    
}   
int HJMediaFrame::getDataFromAVFrame(const HJMediaFrame::Ptr &i_frame, uint8_t *&o_data, int &o_size)
{
    int i_err = HJ_OK;
    do
    {
        HJMediaType mediaType = i_frame->getType();
        auto info = i_frame->getInfo();
        if (mediaType == HJMEDIA_TYPE_AUDIO)
        {
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(info);
            if (audioInfo->m_sampleFmt != AV_SAMPLE_FMT_S16)
            {
                i_err = HJErrNotSupport;
                HJFLoge("audio format not support, now only support s16");
                break;
            }
            
            AVFrame *avFrame = i_frame->getAVFrame();
            if (avFrame)
            {
                o_data = avFrame->data[0];
                o_size = av_samples_get_buffer_size(NULL, audioInfo->getChannels(), audioInfo->getSampleCnt(), ( enum AVSampleFormat)audioInfo->m_sampleFmt, 1);  // align = 1, not use align, get the precise size
            }
        }
        else if (mediaType == HJMEDIA_TYPE_VIDEO)
        {
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(info);
            HJFLoge("video format not support, now only support s16");
            i_err = HJErrNotSupport;
            break;
        }
    } while (false);
    return i_err;
}
void HJMediaFrame::setCodecIdAAC(int &o_codecId)
{
    o_codecId = AV_CODEC_ID_AAC;
}
bool HJMediaFrame::isCodecMatchAAC(int i_codecId)
{
    return i_codecId == AV_CODEC_ID_AAC;
}

HJMediaFrame::Ptr HJMediaFrame::makeFlushFrame(const int streamIndex, const HJStreamInfo::Ptr info, int flag)
{
    HJMediaFrame::Ptr frame = std::make_shared<HJMediaFrame>();
    frame->m_frameType = HJFRAME_FLUSH;
    frame->m_streamIndex = streamIndex;
    frame->m_flag = flag;
    frame->m_info = info;
    return frame;
}

uint8_t* HJMediaFrame::getPixelBuffer()
{
    const auto mxf = getMFrame();
    if (!mxf) {
        return NULL;
    }
    return mxf->getPixelBuffer();
}

const std::string HJMediaFrame::formatInfo(bool trace)
{
    if (!m_info) {
        return "";
    }
    std::string fmtStr = "type:" + HJStreamInfo::type2String(m_info->getType()) + ", pts:" + HJ2STR(m_pts) + ", dts:" + HJ2STR(m_dts) + ", time base:" + HJ2STR(m_timeBase.num) + "/" + HJ2STR(m_timeBase.den) + ", stream id:" + HJ2STR(m_streamIndex);
//    fmtStr += ", mpts:" + HJ2STR(m_mts.getPTSMS()) + ", mdts:" + HJ2STR(m_mts.getDTSMS()) + ", duration:" + HJ2STR(m_mts.getDurationMS());
    AVFrame* avf = (AVFrame *)getAVFrame();
    if (avf) {
        fmtStr += ", AVFrame pts:" + HJ2STR(avf->pts) + ", dts:" + HJ2STR(avf->best_effort_timestamp) + ", time base:" + HJ2STR(avf->time_base.num) + "/" + HJ2STR(avf->time_base.den);
        fmtStr += ", idx:" + HJ2STR(av_rescale_q(avf->pts, avf->time_base, { 1, 30 }));
        if (isVideo()) {
            fmtStr += ", pict type:" + HJ_AVPictTypeName(avf->pict_type);
        }
    }
    AVPacket* pkt = (AVPacket*)getAVPacket();
    if (pkt) {
        fmtStr += ", AVPacket pts:" + HJ2STR(pkt->pts) + ", dts:" + HJ2STR(pkt->dts) + ", time base:" + HJ2STR(pkt->time_base.num) + "/" + HJ2STR(pkt->time_base.den);
        fmtStr += ", idx:" + HJ2STR(av_rescale_q(pkt->pts, pkt->time_base, { 1, 30 })) + ", size:" + HJ2STR(pkt->size);
    }
    fmtStr += ", " + getFrameTypeStr();
#if HJ_HAVE_TRACKER
    if (trace && m_tracker) {
        fmtStr += ", " + m_tracker->formatInfo();
    }
#endif
    return fmtStr;
}

void HJMediaFrame::addTrajectory(const HJTrajectory::Ptr& trajectory){
    m_tracker->addTrajectory(trajectory);
}

void HJMediaFrame::dupTracker(const HJMediaFrame::Ptr& other)
{

}

void HJMediaFrame::clone(const HJMediaFrame::Ptr& other)
{
    m_index = other->m_index;
    m_pts = other->m_pts;
    m_dts = other->m_dts;
    m_duration = other->m_duration;
    m_timeBase = other->m_timeBase;
    m_mts = other->m_mts;
    m_extraTS = other->m_extraTS;
    m_speed = other->m_speed;
    m_frameType = other->m_frameType;
    m_vesfType = other->m_vesfType;
    //m_Buffer = other->m_Buffer;
    m_info = other->m_info;
    //
    m_trackIndex = other->m_trackIndex;
    m_streamIndex = other->m_streamIndex;
    m_flag = other->m_flag;
    m_bufferPos = other->m_bufferPos;
    //m_tracker = other->m_tracker;
    //
    HJKeyStorage::clone(other);
}

HJMediaFrame::Ptr HJMediaFrame::dup() {
    HJMediaFrame::Ptr mavf = std::make_shared<HJMediaFrame>();
    mavf->clone(sharedFrom(this));
    //
    mavf->m_info = m_info->dup();
    return mavf;
}

HJMediaFrame::Ptr HJMediaFrame::deepDup() {
    auto mavf = dup();
    if (!mavf) {
        return nullptr;
    }
    auto mxf = getMFrame();
    if (mxf) {
        auto newMxf = mxf->dup();
        mavf->setMFrame(newMxf);
    }
    auto mpkt = getMPacket();
    if (mpkt) {
        auto newMPkt = mpkt->dup();
        mavf->setMPacket(newMPkt);
    }
    return mavf;
}

//***********************************************************************************//
HJAVFrame::HJAVFrame()
{
	AVFrame* avf = av_frame_alloc();
	if (!avf) {
		return;
	}
    m_avf = avf;
}

HJAVFrame::HJAVFrame(AVFrame* avf)
    : m_avf(avf)
{
    if(m_avf) {
        m_datas[0] = m_avf->data[0];
        m_datas[1] = m_avf->data[1];
        m_datas[2] = m_avf->data[2];
        m_datas[3] = m_avf->data[3];
    }
}

HJAVFrame::~HJAVFrame()
{
    AVFrame* avf = (AVFrame*)m_avf;
    if (avf) {
        av_frame_free(&avf);
        m_avf = NULL;
    }
}

const std::string HJAVFrame::getKey()
{
    if (m_avf) {
        return HJFMT("{}_{}_{}_{}", HJ_TYPE_NAME(HJAVFrame), m_avf->width, m_avf->height, m_avf->format);
    }
    return "";
}

uint8_t* HJAVFrame::getPixelBuffer()
{
    if (!m_avf) {
        return NULL;
    }
    if (AV_PIX_FMT_VIDEOTOOLBOX != m_avf->format) {
        return NULL;
    }
    return m_avf->data[3];
}

void HJAVFrame::setTimeBase(const HJTimeBase& timeBase)
{
    if (!m_avf) {
        return;
    }
    AVRational dst_tb = { timeBase.num, timeBase.den };     //av_rational_from_hj_rational(timeBase);
    if (av_cmp_q(dst_tb, m_avf->time_base)) {
        m_avf->pts = m_avf->best_effort_timestamp = av_rescale_q(m_avf->best_effort_timestamp, m_avf->time_base, dst_tb);
        m_avf->time_base = dst_tb;
    }
    return;
}

void HJAVFrame::setTimeStamp(const int64_t pts, const int64_t dts, const HJTimeBase tb)
{
    if (!m_avf) {
        return;
    }
    AVRational src_tb = { tb.num, tb.den }; //av_rational_from_hj_rational(tb);
    if (0 == m_avf->time_base.num && 1 == m_avf->time_base.den) {
        m_avf->pts = m_avf->best_effort_timestamp = pts;
        m_avf->time_base = src_tb;
    } else {
        m_avf->pts = m_avf->best_effort_timestamp = av_rescale_q(pts, src_tb, m_avf->time_base);
    }
    return;
}

HJAVFrame::Ptr HJAVFrame::dup()
{
    if (!m_avf) {
        return nullptr;
    }
    AVFrame* avf = av_frame_clone(m_avf);
    if (!avf) {
        return nullptr;
    }
    return std::make_shared<HJAVFrame>(avf);
}

//***********************************************************************************//
HJAVPacket::HJAVPacket()
{
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        return;
    }
    m_pkt = pkt;
}

HJAVPacket::HJAVPacket(AVPacket* pkt)
    : m_pkt(pkt)
{

}

HJAVPacket::~HJAVPacket()
{
    AVPacket* pkt = (AVPacket*)m_pkt;
    if (pkt){
        av_packet_free(&pkt);
        m_pkt = NULL;
    }
}

void HJAVPacket::setTimeBase(const HJTimeBase& timeBase)
{
    if (!m_pkt) {
        return;
    }
    AVRational dst_tb = { timeBase.num, timeBase.den };     //av_rational_from_hj_rational(timeBase);
    if (av_cmp_q(dst_tb, m_pkt->time_base)) {
        av_packet_rescale_ts(m_pkt, m_pkt->time_base, dst_tb);
        m_pkt->time_base = dst_tb;
    }
    return;
}

void HJAVPacket::setTimeStamp(const int64_t pts, const int64_t dts, const HJTimeBase tb)
{
    if (!m_pkt) {
        return;
    }
    AVRational src_tb = { tb.num, tb.den }; //av_rational_from_hj_rational(tb);
    m_pkt->pts = pts;
    m_pkt->dts = dts;
    if (0 == m_pkt->time_base.num && 1 == m_pkt->time_base.den) {
        m_pkt->time_base = src_tb;
    } else {
        av_packet_rescale_ts(m_pkt, src_tb, m_pkt->time_base);
    }
    return;
}

HJAVPacket::Ptr HJAVPacket::dup()
{
    if (!m_pkt) {
        return nullptr;
    }
    if (AV_NOPTS_VALUE == m_pkt->dts || AV_NOPTS_VALUE == m_pkt->pts) {
        HJFNLogw("checkFrame dts pts invalid before");
    }
    AVPacket* pkt = av_packet_clone(m_pkt);
    if (!pkt) {
        return nullptr;
    }
    if (AV_NOPTS_VALUE == m_pkt->dts || AV_NOPTS_VALUE == m_pkt->pts) {
        HJFNLogw("checkFrame dts pts invalid end");
    }
    return std::make_shared<HJAVPacket>(pkt);
}

//***********************************************************************************//
const std::string HJAVBuffer::KEY_WORLDS = HJ_TYPE_NAME(HJAVBuffer);
HJAVBuffer::HJAVBuffer(const HJAVBuffer::Ptr& other)
{
    m_size = 0;
    m_capacity = other->getSize();
    m_data = (uint8_t*)av_mallocz(m_capacity);
    if (!m_data) {
        HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "av alloc failed");
        return;
    }
    write(other->getData(), other->getSize());
}
HJAVBuffer::HJAVBuffer(uint8_t* data, const size_t size)
{
    m_capacity = m_size = size;
    m_data = (uint8_t*)av_mallocz(m_capacity);
    if (!m_data) {
        HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "av alloc failed");
        return;
    }
    memcpy(m_data, data, m_size);
}

HJAVBuffer::HJAVBuffer(size_t capacity)
{
    m_size = 0;
    m_capacity = capacity;
    m_data = (uint8_t*)av_mallocz(m_capacity);
    if (!m_data) {
        HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "av alloc failed");
        return;
    }
    //HJFLogi("HJAVBuffer new data:{}", (size_t)m_data);
}

HJAVBuffer::~HJAVBuffer()
{
    //HJFLogi("HJAVBuffer free data:{}", (size_t)m_data);
    if (m_data) {
        av_freep(&m_data);
        m_data = NULL;
    }
}

void HJAVBuffer::appendData(const uint8_t* data, size_t size)
{
    size_t totalSize = m_size + size;
    if (m_capacity < totalSize) {
        m_capacity = totalSize * 3 / 2;
        m_data = (uint8_t *)av_realloc(m_data, m_capacity);
    }
    memcpy(m_data + m_size, data, size);
    m_size = totalSize;
    return;
}

//***********************************************************************************//
HJMediaStorage::HJMediaStorage(size_t capacity/* = 5*/, const std::string& name/* = ""*/)
    : HJObject(name)
    , m_capacity(capacity)
{
    
}

HJMediaStorage::~HJMediaStorage()
{
    m_mediaFrames.clear();
    m_durations.clear();
}

int HJMediaStorage::push(HJMediaFrame::Ptr&& frame)
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.size() >= m_capacity) {
        return HJErrFull;
    }
    //if (frame->isEofFrame() && !m_mediaFrames.empty() && m_mediaFrames.front()->isEofFrame()) {
    //    HJLogw("store aready have eof frame, lost input warning");
    //    return HJ_OK;
    //}
    if (m_hasEofFrame) {
        HJLogw("warning, store aready have eof frame, lost input frame");
        return HJ_OK;
    }
    if (frame->isEofFrame()) {
        m_hasEofFrame = true;
    }
    if (HJ_INT64_MIN == m_nextDTS) {
        m_nextDTS = frame->m_dts;
    }
    m_curDTS = frame->m_dts;
    //
    addDuration(frame);

    m_mediaFrames.emplace_back(std::move(frame));
    m_inCnt++;
    
    return HJ_OK;
}

int HJMediaStorage::pushFresh(HJMediaFrame::Ptr&& frame)
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.size() >= m_capacity) {
        HJMediaFrame::Ptr frame = m_mediaFrames.front();
        m_nextDTS = frame->m_dts + frame->m_duration;
        subDuration(frame);
        //
        m_mediaFrames.pop_front();
        m_outCnt++;
    }
    //if (frame->isEofFrame() && !m_mediaFrames.empty() && m_mediaFrames.front()->isEofFrame()) {
    //    HJLogw("store aready have eof frame, lost input warning");
    //    return HJ_OK;
    //}
    if (m_hasEofFrame) {
        HJLogw("warning, store aready have eof frame, lost input frame");
        return HJ_OK;
    }
    if (frame->isEofFrame()) {
        m_hasEofFrame = true;
    }
    if (HJ_INT64_MIN == m_nextDTS) {
        m_nextDTS = frame->m_dts;
    }
    m_curDTS = frame->m_dts;
    //
    addDuration(frame);

    m_mediaFrames.emplace_back(std::move(frame));
    m_inCnt++;

    return HJ_OK;
}

HJMediaFrame::Ptr HJMediaStorage::pop()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return nullptr;
    }
    HJMediaFrame::Ptr frame = m_mediaFrames.front();
    m_nextDTS = frame->m_dts + frame->m_duration;
    subDuration(frame);
    //
    m_mediaFrames.pop_front();
    m_outCnt++;

    return frame;
}

HJMediaFrame::Ptr HJMediaStorage::next()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return nullptr;
    }
    return m_mediaFrames.front();
}

HJMediaFrame::Ptr HJMediaStorage::last()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return nullptr;
    }
    return m_mediaFrames.back();
}

bool HJMediaStorage::isFull()
{
    HJ_AUTO_LOCK(m_mutex);
    bool res = false;
    switch (m_storeType)
    {
    case HJ::HJMediaStorage::StoreType::NONE: {
        if (m_mediaFrames.size() >= m_capacity) {
            res = true;
        }
        break;
    }
    case HJ::HJMediaStorage::StoreType::TIME: {
        if (getMinDuration() >= m_timeCapacity) {
            res = true;
        }
        //int64_t totalDuration = 0;
        //for (auto mavf : m_mediaFrames) {
        //    totalDuration += mavf->getDuration();
        //}
        //if (totalDuration >= m_timeCapacity) {
        //    res = true;
        //}
        break;
    }
    default:
        break;
    }
    return res;
}

void HJMediaStorage::clear()
{
    HJ_AUTO_LOCK(m_mutex);
    m_mediaFrames.clear();
    m_inCnt = 0;
    m_outCnt = 0;
    m_curDTS = { HJ_INT64_MIN };
    m_nextDTS = { HJ_INT64_MIN };
    m_durations.clear();
    m_hasEofFrame = false;
    m_mts = HJUtilTime::HJTimeNOTS;
}

size_t HJMediaStorage::size() {
    HJ_AUTO_LOCK(m_mutex);
    return m_mediaFrames.size();
}

const size_t HJMediaStorage::getTotalInCnt()
{
    HJ_AUTO_LOCK(m_mutex);
    return m_inCnt;
}

const size_t HJMediaStorage::getTotalOutCnt()
{
    HJ_AUTO_LOCK(m_mutex);
    return m_outCnt;
}

int64_t HJMediaStorage::getTopDTS()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return HJ_INT64_MIN;
    }
    return m_mediaFrames.front()->getDTS();
}
int64_t HJMediaStorage::getTopPTS()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return HJ_INT64_MIN;
    }
    return m_mediaFrames.front()->getPTS();
}

int64_t HJMediaStorage::getLastDTS()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return HJ_INT64_MIN;
    }
    return m_mediaFrames.back()->getDTS();
}

int64_t HJMediaStorage::getDTSDuration()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return HJ_INT64_MIN;
    }
    HJMediaFrame::Ptr first = m_mediaFrames.front();
    HJMediaFrame::Ptr end = m_mediaFrames.back();

    return end->getDTS() + end->getDuration() - first->getPTS();
}

int64_t HJMediaStorage::getDuration()
{
    HJ_AUTO_LOCK(m_mutex);
    int64_t totalDuration = 0;
    for (auto mavf : m_mediaFrames) {
        totalDuration += mavf->getDuration();
    }
    return totalDuration;
}

bool HJMediaStorage::isEOF()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_mediaFrames.empty()) {
        return false;
    }
    HJMediaFrame::Ptr frame = m_mediaFrames.front();
    if (frame && (HJFRAME_EOF == frame->getFrameType())) {
        return true;
    }
    return false;
}

//***********************************************************************************//
NS_HJ_END
