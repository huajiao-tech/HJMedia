//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJFFHeaders.h"
#if defined( __cplusplus )
#include "HJUtilitys.h"
#include "HJMediaUtils.h"
#endif //__cplusplus

typedef enum AVHWDeviceType AVHWDeviceType;
typedef enum AVPixelFormat AVPixelFormat;
typedef enum AVCodecID AVCodecID;
typedef struct AVChannelLayout AVChannelLayout;
typedef struct AVCodecParameters AVCodecParameters;
typedef enum AVSampleFormat AVSampleFormat;
//***********************************************************************************//
AVCodecParameters* av_dup_codec_params(const AVCodecParameters* src);
bool av_codec_params_compare(const AVCodecParameters* params1, const AVCodecParameters* params2);
AVCodecParameters* av_dup_codec_params_from_avcodec(const AVCodecContext* avctx);
AVChannelLayout* av_dup_channel_layout(const AVChannelLayout* src);
AVChannelLayout* av_make_channel_layout_default(const int channels);
AVPixelFormat av_pixel_format_from_hwtype(const AVHWDeviceType hwtype);

static inline int64_t av_time_to_time(int64_t t, AVRational s, AVRational d) {
    if (AV_NOPTS_VALUE != t && t) {
        return av_cmp_q(s, d) ? av_rescale_q_rnd(t, s, d, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) : t;
    }
    return t;
}
static inline int64_t av_time_to_ms(int64_t t, AVRational s) {
    AVRational d = { 1, 1000 };
    return av_time_to_time(t, s, d);
}
static inline int64_t av_ms_to_time(int64_t ms, AVRational d) {
    AVRational s = { 1, 1000 };
    return av_time_to_time(ms, s, d);
}
AVPacket* hj_make_null_packet(void);
AVPacket* hj_avpacket_with_extradata(AVPacket* pkt, uint8_t* csd0, int csd0_size, uint8_t* csd1, int csd1_size);
AVPacket* hj_avpacket_with_newdata(AVPacket* pkt, uint8_t* data, int size);
AVFrame* hj_dup_audio_frame(AVFrame* src, int dstChannels, const AVChannelLayout* layout);
AVFrame* hj_make_silence_audio_frame(int channel, int sampleRate, enum AVSampleFormat sampleFmt, int nbSamples, const AVChannelLayout* layout);
AVFrame* hj_make_avframe(int width, int height, enum AVPixelFormat format);

bool hj_has_protocol(char* protoName);

#if defined( __cplusplus )
NS_HJ_BEGIN
//***********************************************************************************//
AVHWDeviceType hj_device_type_by_codec_name(const std::string& codecName);
std::string hj_hwcodec_name_by_codec_id(AVCodecID codecID, AVHWDeviceType deviceType, bool isEnc = false);
const AVCodec* hj_find_av_decoder(AVCodecID codecID, AVHWDeviceType deviceType);
const AVCodec* hj_find_av_decoder_by_name(AVCodecID codecID, AVHWDeviceType deviceType);
const AVCodec* hj_find_av_encoder(AVCodecID codecID, AVHWDeviceType deviceType);
AVPixelFormat hj_hw_pixel_format(const AVCodec* codec, AVHWDeviceType deviceType);
const AVRational av_rational_from_hj_rational(const HJRational& ratio);
const HJRational av_rational_to_hj_rational(const AVRational& ratio);

AVHWDeviceType hj_device_type_map(const HJDeviceType type);
AVCodecID av_codec_id_from_hjid(const HJCodecID codecId);
AVSampleFormat av_sample_format_from_hjfmt(const HJSampleFormat fmt);
int hj_get_bits_per_pixel(int fmt);

HJBuffer::Ptr hj_get_extradata_buffer(AVPacket* pkt);

std::string HJ_AVErr2Str(int err);
int AVErr2HJErr(int err, int default2 = HJErrFatal);
std::string HJ_AVSampleFMTName(int fmt);
std::string HJ_AVPixelFMTName(int fmt);
std::string HJ_AVPictTypeName(int type);

int hj_get_avframe_slice(uint8_t** slice, AVFrame* frame, HJRectf rect, bool reset);
//
HJEnumToStringFuncDecl(AVCodecID);

typedef struct HJUriComponents
{
    std::string proto;
    std::string auth;
    std::string hostname;
    int port;
    std::string path;
} HJUriComponents;

HJUriComponents hj_url_split(const std::string& uri);

HJBuffer::Ptr hj_make_adts_header(size_t dataSize, const HJ_AMPEG_ID mpegID = HJ_AMPEG_2, const HJAACProfileType profile = HJ_AAPROFILE_LC, const int samplerate = 48000, const int channels = 2);

std::string hj_replace_fasthttp(const std::string& url);

//***********************************************************************************//
NS_HJ_END
#endif //__cplusplus