//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJOHADecoder.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "multimedia/player_framework/native_avcodec_audiocodec.h"
#include "multimedia/player_framework/native_avformat.h"
#include "multimedia/player_framework/native_avbuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJOHADecoder::HJOHADecoder() 
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJOHADecoder)));
}

HJOHADecoder::~HJOHADecoder() 
{
    done();
}

int HJOHADecoder::init(const HJStreamInfo::Ptr& info) 
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
    const char* mime = HJOHCodecUtils::mapCodecIdToMime(m_codecID);
    if (!mime) {
        HJFNLoge("error, Unsupported codec ID: {}", m_codecID);
        return HJErrNotSupport;
    }

    m_decoder = OH_AudioCodec_CreateByMime(mime, false);
    if (!m_decoder) {
        HJFNLoge("Failed to create OH audio decoder for mime: {}", mime);
        return HJErrNewObj;
    }
    audioInfo->m_sampleFmt = AV_SAMPLE_FMT_S16;
    int sampleFormatOH =  HJOHCodecUtils::mapAVSampleFormatToOH(audioInfo->m_sampleFmt);
    if (sampleFormatOH == INVALID_WIDTH) {
        HJFNLoge("Unsupported sample format: {}", audioInfo->m_sampleFmt);
        return HJErrNotSupport;
    }
    int audioChannelLayout = 0;
    OH_AVFormat* format = OH_AVFormat_Create();
    if (!format) {
        HJFNLoge("error, Failed to create OH format");
        return HJErrNewObj;
    }
    
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, audioInfo->m_channels);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, audioInfo->m_samplesRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormatOH);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, audioChannelLayout);

    if (codecParam->extradata && codecParam->extradata_size > 0) {
        HJFNLogi("extradata size: {}, data:{:02x},{:02x}", codecParam->extradata_size, codecParam->extradata[0], codecParam->extradata[1]);
        OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, codecParam->extradata, codecParam->extradata_size);
    }

    res = OH_AudioCodec_Configure(m_decoder, format);
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_Configure failed with error: {}", res);
        if(format) {
            OH_AVFormat_Destroy(format);
            format = nullptr;
        }
        done();
        return HJErrCodecConfig;
    }
    if(format) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    m_userData = std::make_shared<HJOHACodecUserData>(HJSharedFromThis(), getName());
    OH_AVCodecCallback callbacks = {&OnError, &OnStreamChanged, &OnNeedInputBuffer, &OnNewOutputBuffer};
    res = OH_AudioCodec_RegisterCallback(m_decoder, callbacks, m_userData.get());
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_RegisterCallback failed with error: {}", res);
        done();
        return HJErrCodecCallback;
    }

    res = OH_AudioCodec_Prepare(m_decoder);
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_Prepare failed with error: {}", res);
        done();
        return HJErrCodecPrepare;
    }

    res = OH_AudioCodec_Start(m_decoder);
    if (res != AV_ERR_OK) {
        HJFNLoge("OH_AudioCodec_Start failed with error: {}", res);
        done();
        return HJErrCodecStart;
    }
    
    m_timeBase = {1, AV_TIME_BASE/*audioInfo->m_samplesRate*/};
    m_runState = HJRun_Ready;
    HJFNLogi("init end");

    return HJ_OK;
}

int HJOHADecoder::getFrame(HJMediaFrame::Ptr& frame) 
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
            //
            HJFPERNLogi(frame->formatInfo());
        }
    } while (false);
    
    return res;
}

int HJOHADecoder::run(const HJMediaFrame::Ptr frame) 
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    if (!m_decoder) {
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
    // HJFNLogi("entry");
    do { 
        HJMediaFrame::Ptr mvf = frame;//->deepDup();
// #if HJ_HAVE_CHECK_XTRADATA
//         if (checkFrame(mvf)) {
//             m_storeFrame = std::move(mvf);
//         }
// #endif
        m_runState = HJRun_Running;
        //
        auto inBuffer = m_userData->getInputBuffer();
        if (!inBuffer) {
            HJFNLoge("run get input buffer error");
            return HJErrCodecBuffer;
        }
        if(mvf && !mvf->isEofFrame())
        {
            AVPacket* pkt = mvf->getAVPacket();
            if (!pkt) {
                HJFNLoge("run get avpacket error");
                return HJErrInvalidParams;
            }

            inBuffer->setBufferAttr(pkt->data, pkt->size, mvf->getPTSUS());
        } else {
            HJFNLogi("run eof frame");
            inBuffer->setBufferAttr(nullptr, 0, 0, AVCODEC_BUFFER_FLAGS_EOS);
        }
        if (mvf) {
            HJFPERNLogi(mvf->formatInfo());
        }
    
        res = OH_AudioCodec_PushInputBuffer(m_decoder, inBuffer->getID());
        if (res != AV_ERR_OK) {
            HJFNLoge("OH_AudioCodec_PushInputBuffer failed with error: {}, index:{}", res, inBuffer->getID());
            res = HJErrCodecDecode;
        }
    } while (false);
    // HJFNLogi("end");

    return HJ_OK;
}

int HJOHADecoder::flush() {
    if (m_decoder) {
        OH_AudioCodec_Flush(m_decoder);
    }
    if(m_userData) {
        m_userData->clear();
    }
    m_runState = HJRun_Ready;

    return HJ_OK;
}

bool HJOHADecoder::checkFrame(const HJMediaFrame::Ptr frame) 
{
    if (m_runState == HJRun_Init || !frame || !frame->isFlushFrame()) {
        return false;
    }
    
    const auto& inInfo = frame->getAudioInfo();
    if (!inInfo) {
        HJFNLogw("warning, audio info in flush frame is null");
        return false;
    }

    const auto otherCodecParam = inInfo->getCodecParams();
    const auto codecParam = m_info->getCodecParams();
    if (!codecParam || !codecParam->isEqual(otherCodecParam)) {
        m_info->setCodecParams(otherCodecParam);
        HJFNLogi("set flush codec params");
        return true;
    }
    return false;
}

int HJOHADecoder::reset() {
    HJFNLogi("entry");
    done();
    return init(m_info);
}

void HJOHADecoder::done() {
    if (m_decoder) {
        OH_AudioCodec_Flush(m_decoder);
        OH_AudioCodec_Stop(m_decoder);
    }
    if(m_userData) {
        m_userData->clear();
        m_userData = nullptr;
    }
    if(m_decoder) {
        OH_AudioCodec_Destroy(m_decoder);
        m_decoder = nullptr;
    }
    m_runState = HJRun_Done;
}


// --- Callback Implementations ---
void HJOHADecoder::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData) {
    HJFLoge("OHADecoder Error Callback, code: {}", errorCode);
    // auto* ctx = static_cast<HJOHACodecUserData*>(userData);
    // if (!ctx) {
    //     HJFLoge("error, ctx is null, index: {}", index);
    //     return;
    // }
    // auto self = std::dynamic_pointer_cast<HJOHADecoder>(ctx->getParent());
    // if (!self) {
    //     HJFLoge("error, self is null, index: {}", index);
    //     return;
    // }

    return;
}

void HJOHADecoder::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
    HJFLogi("OHADecoder Format Changed Callback");
    // auto* ctx = static_cast<HJOHACodecUserData*>(userData);
    // if (!ctx) {
    //     HJFLoge("error, ctx is null, index: {}", index);
    //     return;
    // }
    // auto self = std::dynamic_pointer_cast<HJOHADecoder>(ctx->getParent());
    // if (!self) {
    //     HJFLoge("error, self is null, index: {}", index);
    //     return;
    // }

    return;
}

void HJOHADecoder::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
    // HJFLogi("OHADecoder input Buffer Callback entry");
    auto* ctx = static_cast<HJOHACodecUserData*>(userData);
    if (!ctx) {
        HJFLoge("error, ctx is null, index: {}", index);
        return;
    }
    auto self = std::dynamic_pointer_cast<HJOHADecoder>(ctx->getParent());
    if (!self) {
        HJFLoge("error, self is null, index: {}", index);
        return;
    }
    
    auto obj = std::make_shared<HJOHAVBufferInfo>(buffer, index);
    obj->setName(self->getName());
    ctx->setInputBuffer(obj);

    // HJFLogi("OHADecoder input Buffer Callback end");
    return;
}

void HJOHADecoder::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
    // HJFLogi("OHADecoder out Buffer Callback entry");s
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
        auto self = std::dynamic_pointer_cast<HJOHADecoder>(ctx->getParent());
        if (!self) {
            HJFLoge("error, self is null, index: {}", index);
            return;
        }
        auto audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(self->getInfo());
        auto mavf = HJOHADecoder::makeFrame(buffer, audioInfo);
        if(mavf) {
            ctx->setOutputFrames(mavf);
            // HJFLogi("media frame:{}", mavf->formatInfo());
        }
    } while (false);
    OH_AudioCodec_FreeOutputBuffer(codec, index);
    // HJFLogi("OHADecoder out Buffer Callback end");
    
    return;
}

HJMediaFrame::Ptr HJOHADecoder::makeFrame(OH_AVBuffer *buffer, const HJAudioInfo::Ptr& info)
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
            HJFLogi("OHADecoder received EOS on output");
            mavf = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
        } else {
            auto dataPtr = OH_AVBuffer_GetAddr(buffer);
            if (!dataPtr || attr.size <= 0) {
                HJFLoge("OH_AVBuffer_GetAddr returned null or size is zero");
                break;
            }
            auto audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
            audioInfo->m_sampleFmt = AV_SAMPLE_FMT_S16;
            int64_t pts = av_time_to_time(attr.pts, {1, AV_TIME_BASE}, {1, audioInfo->m_samplesRate});
            mavf = HJMediaFrame::makeAudioFrameWithSample(audioInfo, dataPtr, attr.size, pts, pts, {1, audioInfo->m_samplesRate});
            // mavf = HJMediaFrame::makeMediaFrameAsAVFrame(audioInfo, dataPtr, attr.size, true, attr.pts, {1, AV_TIME_BASE});
        }
        // HJFNLogi("make frame:{}", mavf->formatInfo());
    } while (false);
    
    return mavf;
}

NS_HJ_END
