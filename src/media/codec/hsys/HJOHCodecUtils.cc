//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJOHCodecUtils.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "multimedia/player_framework/native_avcodec_base.h"
#include "multimedia/player_framework/native_avbuffer.h"
NS_HJ_BEGIN
//***********************************************************************************//
HJOHAVBufferInfo::HJOHAVBufferInfo(OH_AVBuffer* buffer, uint32_t index)
    : m_buffer(buffer)
{
    setID(index);
    m_attr = std::make_shared<OH_AVCodecBufferAttr>();
    m_attr->pts = 0;
    m_attr->size = 0;
    m_attr->offset = 0;
    m_attr->flags = AVCODEC_BUFFER_FLAGS_NONE;
    if(m_buffer) {
        OH_AVBuffer_GetBufferAttr(m_buffer, &(*m_attr));
    }
}

/**
 * freeFunc
 * OH_VideoDecoder_RenderOutputBuffer
 * OH_VideoDecoder_FreeOutputBuffer
 * OH_VideoEncoder_FreeOutputBuffer
 * OH_AudioCodec_FreeOutputBuffer
 */
HJOHAVBufferInfo::HJOHAVBufferInfo(OH_AVCodec* codec, OH_AVBuffer* buffer, uint32_t index, HJOHCodecFreeAVBuffer freeFunc)
    : m_codec(codec)
    , m_buffer(buffer)
    , m_freeFunc(freeFunc)
{
    setID(index);
    m_attr = std::make_shared<OH_AVCodecBufferAttr>();
    m_attr->pts = 0;
    m_attr->size = 0;
    m_attr->offset = 0;
    m_attr->flags = AVCODEC_BUFFER_FLAGS_NONE;
    if(m_buffer) {
        OH_AVBuffer_GetBufferAttr(m_buffer, &(*m_attr));
    }
}

HJOHAVBufferInfo::~HJOHAVBufferInfo()
{
    if(m_codec && m_buffer && m_freeFunc) {
        int res = m_freeFunc(m_codec, m_id);
        // if(HJMEDIA_TYPE_VIDEO == m_type) {
        //     res = m_render ? OH_VideoDecoder_RenderOutputBuffer(m_codec, m_id) : OH_VideoDecoder_FreeOutputBuffer(m_codec, m_id);
        //     //OH_VideoEncoder_FreeOutputBuffer(m_codec, m_id);
        // } else {
        //     res = OH_AudioCodec_FreeOutputBuffer(m_codec, m_id);
        // }
        if(AV_ERR_OK != res) {
            HJFLoge("free buffer failed:{}, m_id:{}, m_type:{}, m_render:{}", res, m_id);
        }
    }
}

int HJOHAVBufferInfo::setBufferAttr(uint8_t* data, int size, int64_t pts, uint32_t flags)
{
    if(!m_buffer) {
        HJFLogw("warning: buffer is null");
        return HJErrNotAlready;
    }
    m_attr->pts = pts;
    m_attr->size = size;
    m_attr->offset = 0;
    m_attr->flags = flags;
    int res = OH_AVBuffer_SetBufferAttr(m_buffer, &(*m_attr));
    if(AV_ERR_OK != res) {
        HJFLoge("setBufferAttr failed:{}", res);
        return HJErrCodecBuffer;
    }
    if(data) 
    {
        auto capacity = OH_AVBuffer_GetCapacity(m_buffer);
        auto dst = OH_AVBuffer_GetAddr(m_buffer);
        if(!dst || capacity < size) {
            HJFLoge("error, get buffer addr failed, capacity:{}, size:{}", capacity, size);
            return HJErrCodecBuffer;
        }
        memcpy(dst, data, size);
    }
    HJFPERNLogi("set buffer attr, size:{}, pts:{}, flags:{}", size, pts, flags);

    return HJ_OK;
}

//***********************************************************************************//
// --- Public Method Implementations ---
const char* HJOHCodecUtils::mapCodecIdToMime(int codecId) {
    switch (codecId) {
        case AV_CODEC_ID_AAC:
            return OH_AVCODEC_MIMETYPE_AUDIO_AAC; //"audio/mp4a-latm";
        case AV_CODEC_ID_OPUS:
            return OH_AVCODEC_MIMETYPE_AUDIO_OPUS; //"audio/opus";
        case AV_CODEC_ID_MP3:
            return OH_AVCODEC_MIMETYPE_AUDIO_MPEG; //"audio/mpeg";
        case AV_CODEC_ID_VORBIS:
            return OH_AVCODEC_MIMETYPE_AUDIO_VORBIS; //"audio/vorbis";
        case AV_CODEC_ID_FLAC:
            return OH_AVCODEC_MIMETYPE_AUDIO_FLAC; //"audio/flac";
        default:
            return NULL;
    }
}

const int HJOHCodecUtils::mapAVSampleFormatToOH(int format)
{
    switch (format)
    {
    case AV_SAMPLE_FMT_U8:
        return SAMPLE_U8;
    case AV_SAMPLE_FMT_S16:
        return SAMPLE_S16LE;
    case AV_SAMPLE_FMT_S32:
        return SAMPLE_S32LE;
//    case AV_SAMPLE_FMT_FLT:
//        return SAMPLE_FLT;
//    case AV_SAMPLE_FMT_DBL:
//        return SAMPLE_DBL;
    case AV_SAMPLE_FMT_U8P:
        return SAMPLE_U8P;
    case AV_SAMPLE_FMT_S16P:
        return SAMPLE_S16P;
    case AV_SAMPLE_FMT_S32P:
        return SAMPLE_S32P;
//    case AV_SAMPLE_FMT_FLTP:
//        return SAMPLE_FLTP;
    default:
        return INVALID_WIDTH;
    }
}

NS_HJ_END