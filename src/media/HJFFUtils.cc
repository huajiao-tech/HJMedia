//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFFUtils.h"
#include "HJFLog.h"

//***********************************************************************************//
AVCodecParameters* av_dup_codec_params(const AVCodecParameters* src)
{
    if (!src) {
        return NULL;
    }
    AVCodecParameters* dst = avcodec_parameters_alloc();
    if (!dst) {
        HJLoge("alloc codec params error");
        return NULL;
    }
    int res = avcodec_parameters_copy(dst, src);
    if (res < HJ_OK) {
        HJLoge("copy video param error");
        avcodec_parameters_free(&dst);
        return NULL;
    }
    return dst;
}

bool av_codec_params_compare(const AVCodecParameters* params1, const AVCodecParameters* params2)
{
    if (params1 == params2) {
        return true;
    }
    if (params1->extradata_size != params2->extradata_size) {
        return false;
    }
    bool same = (0 == memcmp(params1->extradata, params2->extradata, params1->extradata_size));
    return same;
}

AVCodecParameters* av_dup_codec_params_from_avcodec(const AVCodecContext* avctx)
{
    AVCodecParameters* codecpar = avcodec_parameters_alloc();
    if (!codecpar) {
        HJLoge("alloc codec params error");
        return nullptr;
    }
    int res = avcodec_parameters_from_context(codecpar, avctx);
    if (res < HJ_OK) {
        HJLoge("codec store avcodec params" + HJ2STR(res));
        avcodec_parameters_free(&codecpar);
        return nullptr;
    }
    return codecpar;
}

AVChannelLayout* av_dup_channel_layout(const AVChannelLayout* src)
{
    AVChannelLayout* dst = (AVChannelLayout*)av_mallocz(sizeof(AVChannelLayout));
    int res = av_channel_layout_copy(dst, src);
    if (res < HJ_OK) {
        av_channel_layout_uninit(dst);
        av_freep(&dst);
        return NULL;
    }
    return dst;
}

AVChannelLayout* av_make_channel_layout_default(const int channels)
{
    AVChannelLayout* ch_layout = (AVChannelLayout*)av_mallocz(sizeof(AVChannelLayout));
    av_channel_layout_default(ch_layout, channels);
    return ch_layout;
}

AVPixelFormat av_pixel_format_from_hwtype(const AVHWDeviceType hwtype)
{
    static const std::unordered_map<AVHWDeviceType, AVPixelFormat> g_HWDevicePixelFormat = {
        {AV_HWDEVICE_TYPE_NONE, AV_PIX_FMT_YUV420P},
        {AV_HWDEVICE_TYPE_VDPAU, AV_PIX_FMT_VDPAU},
        {AV_HWDEVICE_TYPE_CUDA, AV_PIX_FMT_CUDA},
        {AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI},
        {AV_HWDEVICE_TYPE_DXVA2, AV_PIX_FMT_DXVA2_VLD},
        {AV_HWDEVICE_TYPE_QSV, AV_PIX_FMT_QSV},
        {AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX},
        {AV_HWDEVICE_TYPE_D3D11VA, AV_PIX_FMT_D3D11VA_VLD},
        {AV_HWDEVICE_TYPE_DRM, AV_PIX_FMT_DRM_PRIME},
        {AV_HWDEVICE_TYPE_OPENCL, AV_PIX_FMT_OPENCL},
        {AV_HWDEVICE_TYPE_MEDIACODEC, AV_PIX_FMT_MEDIACODEC},
        {AV_HWDEVICE_TYPE_VULKAN, AV_PIX_FMT_VULKAN},
    };
    auto it = g_HWDevicePixelFormat.find(hwtype);
    return (it != g_HWDevicePixelFormat.end()) ? it->second : AV_PIX_FMT_YUV420P;
}

AVPacket* hj_make_null_packet()
{
    AVPacket* nullPkt = av_packet_alloc();
    nullPkt->data = NULL;
    nullPkt->size = 0;
    nullPkt->dts = 0;
    nullPkt->pts = 0;
    return nullPkt;
}

AVPacket* hj_avpacket_with_extradata(AVPacket* pkt, uint8_t* csd0, int csd0_size, uint8_t* csd1, int csd1_size)
{
    if (!pkt) {
        return NULL;
    }
    size_t buf_size = 0;
    if (csd0) {
        buf_size += csd0_size;
    }
    if (csd1) {
        buf_size += csd1_size;
    }
    buf_size += pkt->size;
    //
    AVBufferRef* avbuf = NULL;
    int ret = av_buffer_realloc(&avbuf, (size_t)(buf_size + AV_INPUT_BUFFER_PADDING_SIZE));
    if (ret < 0) {
        return NULL;
    }
    memset(avbuf->data + buf_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    size_t rd_size = 0;
    if (csd0) {
        memcpy(avbuf->data + rd_size, csd0, csd0_size);
        rd_size += csd0_size;
    }
    if (csd1) {
        memcpy(avbuf->data + rd_size, csd1, csd1_size);
        rd_size += csd1_size;
    }
    memcpy(avbuf->data + rd_size, pkt->data, pkt->size);
    //free
    av_buffer_unref(&pkt->buf);
    pkt->buf = avbuf;
    pkt->data = avbuf->data;
    pkt->size = (int)buf_size;

    return pkt;
}

AVPacket* hj_avpacket_with_newdata(AVPacket* pkt, uint8_t* data, int size)
{
    AVBufferRef* avbuf = NULL;
    int ret = av_buffer_realloc(&avbuf, (size_t)(size + AV_INPUT_BUFFER_PADDING_SIZE));
    if (ret < 0) {
        return NULL;
    }
    memcpy(avbuf->data, data, size);
    memset(avbuf->data + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    av_buffer_unref(&pkt->buf);
    pkt->buf = avbuf;
    pkt->data = avbuf->data;
    pkt->size = (int)size;

    return pkt;
}

AVFrame* hj_dup_audio_frame(AVFrame* src, int dstChannels, const AVChannelLayout* layout)
{
    AVFrame* dst = av_frame_alloc();
    if (!dst) {
        return NULL;
    }
    int res = HJ_OK;
    do
    {
        dst->time_base = src->time_base;
        dst->pts = src->pts;
        dst->best_effort_timestamp = src->best_effort_timestamp;
        dst->nb_samples = src->nb_samples;
        dst->format = src->format;
        //        dst->channels = dstChannels ? dstChannels : src->channels;
        //        dst->channel_layout = av_get_default_channel_layout(dst->channels);
        dst->sample_rate = src->sample_rate;
        if (dstChannels) {
            av_channel_layout_default(&dst->ch_layout, dstChannels);
        }
        else if (layout) {
            res = av_channel_layout_copy(&dst->ch_layout, layout);
        }
        else {
            res = av_channel_layout_copy(&dst->ch_layout, &src->ch_layout);
        }
        if (HJ_OK != res) {
            break;
        }
        res = av_frame_get_buffer(dst, 0);
        if (res < HJ_OK) {
            break;
        }
    } while (false);

    if (HJ_OK != res && dst) {
        av_frame_free(&dst);
        dst = NULL;
    }
    return dst;
}

AVFrame* hj_make_silence_audio_frame(int channel, int sampleRate, enum AVSampleFormat sampleFmt, int nbSamples, const AVChannelLayout* layout)
{
    AVFrame* dst = av_frame_alloc();
    if (!dst) {
        return NULL;
    }
    int res = HJ_OK;
    do
    {
        dst->time_base = { 1, sampleRate };
        dst->pts = 0;
        dst->best_effort_timestamp = 0;
        dst->nb_samples = nbSamples;
        dst->format = sampleFmt;
        //		dst->channels = channel;
        //		dst->channel_layout = av_get_default_channel_layout(dst->channels);
        dst->sample_rate = sampleRate;
        if (channel) {
            av_channel_layout_default(&dst->ch_layout, channel);
        }
        else if (layout) {
            res = av_channel_layout_copy(&dst->ch_layout, layout);
        }
        if (HJ_OK != res) {
            break;
        }
        res = av_frame_get_buffer(dst, 0);
        if (res < HJ_OK) {
            break;
        }
        switch (dst->format)
        {
        case AV_SAMPLE_FMT_U8P:
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_FLTP:
        case AV_SAMPLE_FMT_DBLP:
        case AV_SAMPLE_FMT_S64P: {
            for (int i = 0; i < dst->ch_layout.nb_channels; i++) {
                memset(dst->data[i], 0, dst->linesize[0]);
            }
            break;
        }
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_S64: {
            memset(dst->data[0], 0, dst->linesize[0]);
            break;
        }
        default:
            break;
        }
    } while (false);

    if (HJ_OK != res && dst) {
        av_frame_free(&dst);
        dst = NULL;
    }
    return dst;
}
AVFrame* hj_make_avframe_fromy_yuv420p(int width, int height, uint8_t* y_data, uint8_t* u_data, uint8_t* v_data)
{
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error: Could not allocate AVFrame.\n");
        return nullptr;
    }                                                                            
    frame->width = width;
    frame->height = height;
    frame->format = AV_PIX_FMT_YUV420P;
                                                                  
    int ret = av_frame_get_buffer(frame, 0);                       
    if (ret < 0) {
        fprintf(stderr, "Error: Could not allocate buffer for AVFrame.\n");
        av_frame_free(&frame);
        return nullptr;
    }
                       
    const int y_plane_size = width * height;
    memcpy(frame->data[0], y_data, y_plane_size);
                                                      
    const int chroma_width = width / 2;
    const int chroma_height = height / 2;
    const int uv_plane_size = chroma_width * chroma_height;
                                      
    memcpy(frame->data[1], u_data, uv_plane_size);                      
    memcpy(frame->data[2], v_data, uv_plane_size);

    frame->linesize[0] = width;
    frame->linesize[1] = frame->linesize[2] = chroma_width;

    return frame;
}
AVFrame* hj_make_avframe(int width, int height, enum AVPixelFormat format)
{
    AVFrame* avf = av_frame_alloc();
    if (!avf) {
        HJLoge("error, alloc avframe failed");
        return NULL;;
    }
    avf->width = width;
    avf->height = height;
    avf->format = format;
    int res = av_frame_get_buffer(avf, 0);
    if (res < 0) {
        av_frame_free(&avf);
        HJFLoge("error, avframe get buffer failed:{}, width:{}, height:{}, format:{}", res, width, height, format);
        return NULL;
    }
    //    res = av_frame_make_writable(avf);
    //    if (res < HJ_OK){
    //        break;
    //    }
    return avf;
}

bool hj_has_protocol(char* protoName)
{
    const URLProtocol** protocols = ffurl_get_protocols(NULL, NULL);
    if (!protocols)
        return false;
    for (int i = 0; protocols[i]; i++)
    {
        const URLProtocol* up = protocols[i];
        if (!strcmp(protoName, up->name))
        {
            av_freep(&protocols);
            return true;
        }
    }
    av_freep(&protocols);
    return false;

}

NS_HJ_BEGIN
//***********************************************************************************//
AVHWDeviceType hj_device_type_by_codec_name(const std::string& codecName)
{
    if (std::string::npos != codecName.find("vaapi")) {
        return AV_HWDEVICE_TYPE_VAAPI;
    }
    else if (std::string::npos != codecName.find("vadpu")) {
        return AV_HWDEVICE_TYPE_VDPAU;
    }
    else if (std::string::npos != codecName.find("qsv")) {
        return AV_HWDEVICE_TYPE_QSV;
    }
    else if (std::string::npos != codecName.find("videotoolbox")) {
        return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
    }
    else if (std::string::npos != codecName.find("mediacodec")) {
        return AV_HWDEVICE_TYPE_MEDIACODEC;
    }
    return AV_HWDEVICE_TYPE_NONE;
}

std::string hj_hwcodec_name_by_codec_id(AVCodecID codecID, AVHWDeviceType deviceType, bool isEnc)
{
    std::string codecName = "";
    static const std::unordered_map<AVHWDeviceType, std::string> g_h264HWCodecName = {
        {AV_HWDEVICE_TYPE_VDPAU, "h264_vdpau"},
        //{AV_HWDEVICE_TYPE_CUDA, "h264_cuvid"},      //h264_cuvid, h264_nvenc, h264_nvdec
        {AV_HWDEVICE_TYPE_VAAPI, "h264_vaapi"},
        {AV_HWDEVICE_TYPE_DXVA2, "h264_dxva2"},
        {AV_HWDEVICE_TYPE_QSV, "h264_qsv"},
        {AV_HWDEVICE_TYPE_VIDEOTOOLBOX, "h264_videotoolbox"},
        {AV_HWDEVICE_TYPE_MEDIACODEC, "h264_mediacodec"},
        {AV_HWDEVICE_TYPE_VULKAN, "h264_vulkan"}
    };
    //    AV_HWDEVICE_TYPE_D3D11VA,
    //    AV_HWDEVICE_TYPE_DRM,
    //    AV_HWDEVICE_TYPE_OPENCL,
    static const std::unordered_map<AVHWDeviceType, std::string> g_h265HWCodecName = {
        {AV_HWDEVICE_TYPE_VDPAU, "hevc_vdpau"},
        //{AV_HWDEVICE_TYPE_CUDA, "hevc_cuvid"},      //hevc_cuvid, hevc_nvenc, hevc_nvdec
        {AV_HWDEVICE_TYPE_VAAPI, "hevc_vaapi"},
        {AV_HWDEVICE_TYPE_DXVA2, "hevc_dxva2"},
        {AV_HWDEVICE_TYPE_QSV, "hevc_qsv"},
        {AV_HWDEVICE_TYPE_VIDEOTOOLBOX, "hevc_videotoolbox"},
        {AV_HWDEVICE_TYPE_MEDIACODEC, "hevc_mediacodec"},
        {AV_HWDEVICE_TYPE_VULKAN, "hevc_vulkan"}
    };
    switch (codecID) {
    case AV_CODEC_ID_H264: {
        if (AV_HWDEVICE_TYPE_CUDA == deviceType) {
            codecName = isEnc ? "h264_nvenc" : "h264_cuvid";
        }
        else {
            auto it = g_h264HWCodecName.find(deviceType);
            codecName = (it != g_h264HWCodecName.end()) ? it->second : "";
        }
        break;
    }
    case AV_CODEC_ID_H265: {
        if (AV_HWDEVICE_TYPE_CUDA == deviceType) {
            codecName = isEnc ? "hevc_nvenc" : "hevc_cuvid";
        }
        else {
            auto it = g_h265HWCodecName.find(deviceType);
            codecName = (it != g_h265HWCodecName.end()) ? it->second : "";
        }
        break;
    }
    default: {
        HJLogw("not support error");
        break;
    }
    }
    return codecName;
}

const AVCodec* hj_find_av_decoder(AVCodecID codecID, AVHWDeviceType deviceType)
{
    const AVCodec* codec = avcodec_find_decoder(codecID);
    if (!codec) {
        return NULL;
    }
    AVPixelFormat hwPixFmt = hj_hw_pixel_format(codec, deviceType);
    if (AV_PIX_FMT_NONE == hwPixFmt)
    {
        std::string codecName = hj_hwcodec_name_by_codec_id(codecID, deviceType, false);
        if (codecName.empty()) {
            return NULL;
        }
        codec = avcodec_find_decoder_by_name(codecName.c_str());
    }
    //fixme lfs
    //const AVCodec* codec = avcodec_find_decoder_by_name("h264_cuvid");
    return codec;
}

const AVCodec* hj_find_av_decoder_by_name(AVCodecID codecID, AVHWDeviceType deviceType)
{
    std::string codecName = hj_hwcodec_name_by_codec_id(codecID, deviceType, false);
    if (codecName.empty()) {
        return NULL;
    }
    return avcodec_find_decoder_by_name(codecName.c_str());
}

const AVCodec* hj_find_av_encoder(AVCodecID codecID, AVHWDeviceType deviceType)
{
    std::string codecName = hj_hwcodec_name_by_codec_id(codecID, deviceType, true);
    if (codecName.empty()) {
        return NULL;
    }
    return avcodec_find_encoder_by_name(codecName.c_str());
}

AVPixelFormat hj_hw_pixel_format(const AVCodec* codec, AVHWDeviceType deviceType)
{
    AVPixelFormat hwPixFmt = AV_PIX_FMT_NONE;
    if (!codec) {
        return hwPixFmt;
    }
    for (int i = 0;; i++) {
        const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
        if (!config) {
            //HJLogw("Decoder:" + HJ2SSTR(codec->name) + " does not support device type:" + HJ2SSTR(av_hwdevice_get_type_name(deviceType)));
            break;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == deviceType) {
            hwPixFmt = config->pix_fmt;
            break;
        }
    }
    return hwPixFmt;
}

const AVRational av_rational_from_hj_rational(const HJRational& ratio)
{
    AVRational res = { ratio.num, ratio.den };
    return res;
}

const HJRational av_rational_to_hj_rational(const AVRational& ratio)
{
    HJRational res = { ratio.num, ratio.den };
    return res;
}

AVHWDeviceType hj_device_type_map(const HJDeviceType type)
{
    return (AVHWDeviceType)type;
}

AVCodecID av_codec_id_from_hjid(const HJCodecID codecId)
{
    AVCodecID avId = AV_CODEC_ID_NONE;
    switch (codecId) {
    case HJ_CODEC_ID_H264:
        avId = AV_CODEC_ID_H264;
        break;
    case HJ_CODEC_ID_H265:
        avId = AV_CODEC_ID_HEVC;
        break;
    case HJ_CODEC_ID_AAC:
        avId = AV_CODEC_ID_AAC;
        break;
    case HJ_CODEC_ID_OPUS:
        avId = AV_CODEC_ID_OPUS;
        break;
    default: avId = AV_CODEC_ID_NONE; break;
    }
    return avId;
}

AVSampleFormat av_sample_format_from_hjfmt(const HJSampleFormat fmt)
{
    AVSampleFormat avFmt = AV_SAMPLE_FMT_NONE;
    switch (fmt) {
    case HJ_SAMPLE_FMT_U8:
        avFmt = AV_SAMPLE_FMT_U8;
        break;
    case HJ_SAMPLE_FMT_S16:
        avFmt = AV_SAMPLE_FMT_S16;
        break;
    case HJ_SAMPLE_FMT_S32:
        avFmt = AV_SAMPLE_FMT_S32;
        break;
    case HJ_SAMPLE_FMT_FLT:
        avFmt = AV_SAMPLE_FMT_FLT;
        break;
    case HJ_SAMPLE_FMT_DBL:
        avFmt = AV_SAMPLE_FMT_DBL;
        break;
    case HJ_SAMPLE_FMT_U8P:
        avFmt = AV_SAMPLE_FMT_U8P;
        break;
    case HJ_SAMPLE_FMT_S16P:
        avFmt = AV_SAMPLE_FMT_S16P;
        break;
    case HJ_SAMPLE_FMT_S32P:
        avFmt = AV_SAMPLE_FMT_S32P;
        break;
    case HJ_SAMPLE_FMT_FLTP:
        avFmt = AV_SAMPLE_FMT_FLTP;
        break;
    case HJ_SAMPLE_FMT_DBLP:
        avFmt = AV_SAMPLE_FMT_DBLP;
        break;
    case HJ_SAMPLE_FMT_S64:
        avFmt = AV_SAMPLE_FMT_S64;
        break;
    case HJ_SAMPLE_FMT_S64P:
        avFmt = AV_SAMPLE_FMT_S64P;
        break;
    case HJ_SAMPLE_FMT_NB:
        avFmt = AV_SAMPLE_FMT_NB;
        break;
    default: avFmt = AV_SAMPLE_FMT_NONE; break;
    }
    return avFmt;
}

int hj_get_bits_per_pixel(int fmt)
{
    return av_get_bits_per_pixel(av_pix_fmt_desc_get((enum AVPixelFormat)fmt));
}

HJ::HJBuffer::Ptr hj_get_extradata_buffer(AVPacket* pkt)
{
    if (!pkt) {
        return nullptr;
    }
    size_t extradata_size = 0;
    uint8_t* extradata = av_packet_get_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA, &extradata_size);
    if (!extradata || !extradata_size) {
        return nullptr;
    }
    return std::make_shared<HJ::HJBuffer>(extradata, extradata_size);
}

std::string HJ_AVErr2Str(int err)
{
    char buf[1024];
    av_make_error_string(buf, 1024, err);
    return buf;
}

static const std::map<int, int> g_AVERR_MTERR_MAPS = {
    {AVERROR(ENOSPC),       HJErrENOSPC},
    {AVERROR(EINVAL),       HJErrEINVAL},
    {AVERROR(ETIME),        HJErrETIME},
    {AVERROR(ETIMEDOUT),    HJErrTimeOut},
};
int AVErr2HJErr(int err, int default2)
{
    int ret = default2;
    auto it = g_AVERR_MTERR_MAPS.find(err);
    if (it != g_AVERR_MTERR_MAPS.end()) {
        ret = it->second;
    }
    return ret;
}

std::string HJ_AVSampleFMTName(int fmt)
{
    return av_get_sample_fmt_name((enum AVSampleFormat)fmt);
}

std::string HJ_AVPixelFMTName(int fmt)
{
    return av_get_pix_fmt_name((enum AVPixelFormat)fmt);
}

std::string HJ_AVPictTypeName(int type) {
    char name = (AV_PICTURE_TYPE_NONE != type) ? av_get_picture_type_char((enum AVPictureType)type) : 'N';
    std::string str(1, name);
    return str;
}

int hj_get_avframe_slice(uint8_t** slice, AVFrame* frame, HJ::HJRectf rect, bool reset)
{
    const AVPixFmtDescriptor* pixFmtDesc = av_pix_fmt_desc_get((enum AVPixelFormat)frame->format);
    for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
        AVBufferRef* plane = av_frame_get_plane_buffer(frame, i);
        if (plane)
        {
            int offset = 0;
            if (!i) {
                if (reset) {
                    memset(frame->data[i], 16, (size_t)frame->linesize[i] * frame->height);
                }
                offset = (int)rect.x + (int)rect.y * frame->linesize[i];
            }
            else {
                if (reset) {
                    memset(frame->data[i], 128, (size_t)frame->linesize[i] * (frame->height >> pixFmtDesc->log2_chroma_h));
                }
                int chroma_w = ((int)rect.x) >> pixFmtDesc->log2_chroma_w;
                int chroma_h = ((int)rect.y) >> pixFmtDesc->log2_chroma_h;
                offset = chroma_w + chroma_h * frame->linesize[i];
            }
            slice[i] = frame->data[i] + offset;
        }
        else {
            slice[i] = NULL;
        }
    }
    return HJ_OK;
}

HJEnumToStringFuncImplBegin(AVCodecID)
{
    AV_CODEC_ID_PNG, "PNG"
},
{ AV_CODEC_ID_H264, "h264" },
{ AV_CODEC_ID_HEVC, "h265" },
{ AV_CODEC_ID_AV1, "av1" },
{ AV_CODEC_ID_VVC, "VVC" },
{ AV_CODEC_ID_MP3, "mp3" },
{ AV_CODEC_ID_AAC, "AAC" },
{ AV_CODEC_ID_SPEEX, "speex" },
{ AV_CODEC_ID_OPUS, "opus" },
HJEnumToStringFuncImplEnd(AVCodecID);


HJUriComponents hj_url_split(const std::string& uri)
{
    int port = 0;
    char hostname[1024], hoststr[1024], proto[10];
    char auth[1024], proxyauth[1024] = "";
    char path[MAX_URL_SIZE];

    av_url_split(proto, sizeof(proto), auth, sizeof(auth),
        hostname, sizeof(hostname), &port,
        path, sizeof(path), uri.c_str());

    return HJUriComponents{ proto, auth, hostname, port, path };
}

#define HJ_ADTS_HEADER_SIZE 7
HJBuffer::Ptr hj_make_adts_header(size_t dataSize, const HJ_AMPEG_ID mpegID/* = HJ_AMPEG_2*/, const HJAACProfileType profile/* = HJ_AAPROFILE_LC*/, const int samplerate/* = 48000*/, const int channels/* = 2*/)
{
	PutBitContext pb;
	HJBuffer::Ptr adtsHeader = std::make_shared<HJBuffer>(HJ_ADTS_HEADER_SIZE);
	int adtsLen = dataSize + HJ_ADTS_HEADER_SIZE;

	init_put_bits(&pb, adtsHeader->data(), HJ_ADTS_HEADER_SIZE);

	/* adts_fixed_header */
	put_bits(&pb, 12, 0xfff);   /* syncword */
	put_bits(&pb, 1, mpegID);	/* ID : mpeg4 - 0, mpeg2 - 1 */
	put_bits(&pb, 2, 0);        /* layer */
	put_bits(&pb, 1, 1);        /* protection_absent */
	put_bits(&pb, 2, profile);  /* profile_objecttype */
	auto srType = HJMediaUtils::HJAACSamleRate2Type(samplerate);
	if (HJ_AACSR_NONE == srType) {
		return nullptr;
	}
	put_bits(&pb, 4, srType);  //sample_rate_index
	put_bits(&pb, 1, 0);        /* private_bit */
	put_bits(&pb, 3, channels); /* channel_configuration */
	put_bits(&pb, 1, 0);        /* original_copy */
	put_bits(&pb, 1, 0);        /* home */

	/* adts_variable_header */
	put_bits(&pb, 1, 0);        /* copyright_identification_bit */
	put_bits(&pb, 1, 0);        /* copyright_identification_start */
	put_bits(&pb, 13, adtsLen); /* aac_frame_length */
	put_bits(&pb, 11, 0x7ff);   /* adts_buffer_fullness */
	put_bits(&pb, 2, 0);        /* number_of_raw_data_blocks_in_frame */

	flush_put_bits(&pb);
	//
	adtsHeader->setSize(HJ_ADTS_HEADER_SIZE);

	return adtsHeader;
}

std::string hj_replace_fasthttp(const std::string& url) {
    if ((url.compare(0, 8, "https://") == 0) && hj_has_protocol("fasthttps")) {
        return "fasthttps://" + url.substr(8);
    }
    else if ((url.compare(0, 7, "http://") == 0) && hj_has_protocol("fasthttp")) {
        return "fasthttp://" + url.substr(7);
    }
    return url;
}

NS_HJ_END