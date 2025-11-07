//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJOHAEncoder.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "multimedia/player_framework/native_avcodec_audiocodec.h"
#include "multimedia/player_framework/native_avformat.h"
#include "multimedia/player_framework/native_avbuffer.h"
#include "multimedia/native_audio_channel_layout.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJOHAEncoder::HJOHAEncoder() 
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJOHAEncoder)));
    m_codecID = AV_CODEC_ID_AAC;
}

HJOHAEncoder::~HJOHAEncoder() 
{
    done();
}

int HJOHAEncoder::init(const HJStreamInfo::Ptr& info) 
{
    int res = HJBaseCodec::init(info);
    if (HJ_OK != res) {
        HJFNLoge("base init error:{}", res);
        return res;
    }
    HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);
    if (!audioInfo) {
        HJFNLoge("audio info is null");
        return HJErrInvalidParams;
    }
    if (AV_CODEC_ID_NONE != audioInfo->m_codecID) {
        m_codecID = audioInfo->m_codecID;
    } else {
        audioInfo->m_codecID = m_codecID;
    }
    HJFNLogi("init entry, channels:{}, sample rate:{}, sample fmt:{}", audioInfo->m_channels, audioInfo->m_samplesRate, audioInfo->m_sampleFmt);
    const char* mime = HJOHCodecUtils::mapCodecIdToMime(m_codecID);
    if (!mime) {
        HJFNLoge("error, Unsupported codec ID: {}", m_codecID);
        return HJErrNotSupport;
    }
    m_encoder = OH_AudioCodec_CreateByMime(mime, true);
    if (!m_encoder) {
        HJFNLoge("Failed to create OH audio encoder for mime: {}", mime);
        return HJErrNewObj;
    }
    
    // Configure encoder parameters
    OH_AVFormat* format = OH_AVFormat_Create();
    if (!format) {
        HJFNLoge("error, Failed to create OH format");
        return HJErrNewObj;
    }
    
    // Set encoding parameters
    audioInfo->m_sampleFmt = AV_SAMPLE_FMT_S16;
    audioInfo->m_bitrate = 64000;
    int sampleFormatOH = HJOHCodecUtils::mapAVSampleFormatToOH(audioInfo->m_sampleFmt);
    if (sampleFormatOH == -1) {
        HJFNLoge("Unsupported sample format: {}", audioInfo->m_sampleFmt);
        OH_AVFormat_Destroy(format);
        return HJErrNotSupport;
    }
    int audioChannelLayout = (1 == audioInfo->m_channels) ? OH_AudioChannelLayout::CH_LAYOUT_MONO : OH_AudioChannelLayout::CH_LAYOUT_STEREO;
    int audioMaxInputSize = audioInfo->m_samplesPerFrame * audioInfo->m_bytesPerSample;
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, audioInfo->m_channels);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, audioInfo->m_samplesRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormatOH);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, audioChannelLayout);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, audioInfo->m_bitrate > 0 ? audioInfo->m_bitrate : 64000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, audioMaxInputSize);

    res = OH_AudioCodec_Configure(m_encoder, format);
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_Configure failed with error: {}", res);
        OH_AVFormat_Destroy(format);
        done();
        return HJErrCodecConfig;
    }
    OH_AVFormat_Destroy(format); format = nullptr;
    
    m_userData = std::make_shared<HJOHACodecUserData>(HJSharedFromThis(), getName());
    OH_AVCodecCallback callbacks = {&OnError, &OnStreamChanged, &OnNeedInputBuffer, &OnNewOutputBuffer};
    res = OH_AudioCodec_RegisterCallback(m_encoder, callbacks, m_userData.get());
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_RegisterCallback failed with error: {}", res);
        done();
        return HJErrCodecCallback;
    }

    res = OH_AudioCodec_Prepare(m_encoder);
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_Prepare failed with error: {}", res);
        done();
        return HJErrCodecPrepare;
    }

    res = OH_AudioCodec_Start(m_encoder);
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_Start failed with error: {}", res);
        done();
        return HJErrCodecStart;
    }
    
    m_timeBase = {1, AV_TIME_BASE};
    m_runState = HJRun_Ready;
    m_isFirstFrame = true;
    HJFNLogi("init end");

    return HJ_OK;
}

int HJOHAEncoder::getFrame(HJMediaFrame::Ptr& frame) 
{
    if (!m_userData) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    do
    {
        if(HJRun_EOF == m_runState) {
            frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
            res = HJ_EOF;
            break;
        }
        auto mavf = m_userData->getOutputFrame();
        if(!mavf) {
            res = HJ_WOULD_BLOCK;
            break;
        } else if (mavf->isEofFrame()) {
            m_runState = HJRun_EOF;
            res = HJ_EOF;
            break;
        }
        if(mavf) {
            frame = std::move(mavf);
            HJFNLogi(frame->formatInfo());
        }
    } while (false);
    
    return res;
}

int HJOHAEncoder::run(const HJMediaFrame::Ptr frame) 
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    if (!m_encoder) {
        int res = init(frame->getInfo());
        if (HJ_OK != res) {
            HJFNLoge("init in run error");
            return res;
        }
    }
    if(m_userData->inputQueueSize() <= 0) {
        HJFNLogi("input queue is empty, wait for input buffer");
        return HJ_WOULD_BLOCK;
    }
    
    int res = HJ_OK;
    do { 
        HJMediaFrame::Ptr mvf = frame;  //->deepDup();
        m_runState = HJRun_Running;
        
        auto inBuffer = m_userData->getInputBuffer();
        if (!inBuffer) {
            HJFNLoge("run get input buffer error");
            return HJErrCodecBuffer;
        }
        int64_t pts = HJ_NOPTS_VALUE;
        if(mvf && !mvf->isEofFrame())
        {
            AVFrame* avf = frame->getAVFrame();
            if (!avf) {
                HJFNLogi("have no AVFrame");
                res = HJErrInvalidParams;
                break;
            }
            uint32_t flags = AVCODEC_BUFFER_FLAGS_NONE;
            if (m_isFirstFrame) {
                flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                m_isFirstFrame = false;
            }
            pts = mvf->getPTSUS();
            if(HJ_NOPTS_VALUE == m_firstTime) {
                m_firstTime = pts;
            }
            pts -= m_firstTime;

            inBuffer->setBufferAttr(avf->data[0], avf->linesize[0], pts, flags);
        } else {
            HJFNLogi("run eof frame");
            inBuffer->setBufferAttr(nullptr, 0, 0, AVCODEC_BUFFER_FLAGS_EOS);
        } 
        if (mvf) {
            HJFNLogi("frame:{}, m_firstTime:{}, pts:{}", mvf->formatInfo(), m_firstTime, pts);
        }
    
        res = OH_AudioCodec_PushInputBuffer(m_encoder, inBuffer->getID());
        if (res != AV_ERR_OK) {
            HJFNLoge("OH_AudioCodec_PushInputBuffer failed with error: {}, index:{}", res, inBuffer->getID());
            res = HJErrCodecEncode;
        }
    } while (false);

    return HJ_OK;
}

int HJOHAEncoder::flush() {
    if (m_encoder) {
        OH_AudioCodec_Flush(m_encoder);
    }
    if(m_userData) {
        m_userData->clear();
    }
    m_runState = HJRun_Ready;
    m_isFirstFrame = true;

    return HJ_OK;
}
void HJOHAEncoder::done() {
    if (m_encoder) {
        OH_AudioCodec_Flush(m_encoder);
        OH_AudioCodec_Stop(m_encoder);
    }
    if(m_userData) {
        m_userData->clear();
        m_userData = nullptr;
    }
    if(m_encoder) {
        OH_AudioCodec_Destroy(m_encoder);
        m_encoder = nullptr;
    }
    m_runState = HJRun_Done;
    m_isFirstFrame = true;
}

// --- Callback Implementations ---
void HJOHAEncoder::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData) {
    HJFLoge("OHAEncoder Error Callback, code: {}", errorCode);
    return;
}

void HJOHAEncoder::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
    HJFLogi("OHAEncoder Format Changed Callback");
    return;
}

void HJOHAEncoder::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
    auto* ctx = static_cast<HJOHACodecUserData*>(userData);
    if (!ctx) {
        HJFLoge("error, ctx is null, index: {}", index);
        return;
    }
    auto self = std::dynamic_pointer_cast<HJOHAEncoder>(ctx->getParent());
    if (!self) {
        HJFLoge("error, self is null, index: {}", index);
        return;
    }
    
    auto obj = std::make_shared<HJOHAVBufferInfo>(buffer, index);
    obj->setName(self->getName());
    ctx->setInputBuffer(obj);
    
    return;
}

void HJOHAEncoder::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
    if (!codec || !buffer) {
        HJFLoge("error, buffer is null, index: {}", index);
        return;
    }
    do
    {
        auto* ctx = static_cast<HJOHACodecUserData*>(userData);
        if (!ctx) {
            HJFLoge("error, ctx is null, index: {}", index);
            return;
        }
        auto self = std::dynamic_pointer_cast<HJOHAEncoder>(ctx->getParent());
        if (!self) {
            HJFLoge("error, self is null, index: {}", index);
            return;
        }
        auto audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(self->getInfo());
        auto firstTime = self->getFirstTime();
        auto firstOutTime = self->getFirstOutTime();
        auto mavf = HJOHAEncoder::makeFrame(buffer, audioInfo, firstTime, firstOutTime);
        if(mavf) {
            ctx->setOutputFrames(mavf);
        }
        self->setFirstOutTime(firstOutTime);
    } while (false);
    OH_AudioCodec_FreeOutputBuffer(codec, index);
    
    return;
}

HJMediaFrame::Ptr HJOHAEncoder::makeFrame(OH_AVBuffer *buffer, const HJAudioInfo::Ptr& info, const int64_t firstTime, int64_t& outTime)
{
    if(!buffer || !info) {
        HJFLoge("error, buffer or info is null");
        return nullptr;
    }

    int res = HJ_OK;
    HJMediaFrame::Ptr mavf = nullptr;
    do {
        OH_AVCodecBufferAttr attr;
        res = OH_AVBuffer_GetBufferAttr(buffer, &attr);
        if(AV_ERR_OK != res) {
            HJFLoge("OH_AVBuffer_GetBufferAttr failed:{}", res);
            break;
        }

        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            HJFLogi("OHAEncoder received EOS on output");
            mavf = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
        } else {
            auto dataPtr = OH_AVBuffer_GetAddr(buffer);
            if (!dataPtr || attr.size <= 0) {
                HJFLoge("OH_AVBuffer_GetAddr returned null or size is zero");
                break;
            }
            
            // Create encoded audio frame (compressed data packet)
            auto audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
            audioInfo->m_dataType = HJDATA_TYPE_ES;
            int64_t pts = attr.pts;
            if(HJ_NOPTS_VALUE == outTime) {
                outTime = pts;
            }
            pts -= outTime;
             if(HJ_NOPTS_VALUE != firstTime) {
                 pts += firstTime;
             }
            pts = av_time_to_time(pts, {1, AV_TIME_BASE}, {1, audioInfo->m_samplesRate});
            mavf = HJMediaFrame::makeMediaFrameAsAVPacket(audioInfo, dataPtr, attr.size, (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) != 0, pts, pts, {1, audioInfo->m_samplesRate});
        }
    } while (false);
    
    return mavf;
}

NS_HJ_END