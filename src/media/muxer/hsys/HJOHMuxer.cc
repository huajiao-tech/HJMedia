//***********************************************************************************//
//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJOHMuxer.h"
#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJUtilitys.h"
#include "HJTime.h"
#include "multimedia/player_framework/native_avmuxer.h"
#include "multimedia/player_framework/native_avformat.h"
#include "multimedia/player_framework/native_avbuffer.h"
#include "multimedia/player_framework/native_avcodec_base.h"
#include "multimedia/player_framework/native_averrors.h"
#include <fcntl.h>
#include <unistd.h>

#define HJ_MUXER_DELAY_INIT   1

NS_HJ_BEGIN
//***********************************************************************************//
HJOHMuxer::HJOHMuxer()
{
}

HJOHMuxer::HJOHMuxer(HJListener listener)
{

}

HJOHMuxer::~HJOHMuxer()
{
    destroyMuxer();
}

int HJOHMuxer::init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts)
{
    int res = HJBaseMuxer::init(mediaInfo, opts);
    if (HJ_OK != res) {
        return res;
    }
#if !defined(HJ_MUXER_DELAY_INIT)
    res = createMuxer();
    if (HJ_OK != res) {
        destroyMuxer();
    }
#endif
    return res;
}

int HJOHMuxer::init(const std::string url, int mediaTypes, HJOptions::Ptr opts)
{
    int res = HJBaseMuxer::init(url, mediaTypes, opts);
    if (HJ_OK != res) {
        return res;
    }
    if (m_url.empty()) {
        HJFNLoge("error, invalid url:{}", m_url);
        return HJErrInvalidParams;
    }
    m_mediaInfo = HJCreates<HJMediaInfo>();

    return res;
}

int HJOHMuxer::gatherMediaInfo(const HJMediaFrame::Ptr& frame)
{
    auto mtype = frame->getType();
    if (mtype & m_mediaTypes)
    {
        auto mediaInfo = m_mediaInfo->getStreamInfo(mtype);
        if (!mediaInfo) {
            m_mediaInfo->addStreamInfo(frame->getInfo());
            HJFNLogi("add stream info:{}, key frame info:{}", mtype, frame->formatInfo());
        }
    }
    else {
        HJFNLoge("deliver not support media type:{}", mtype);
        return HJErrNotSupport;
    }
    return HJ_OK;
}

int64_t HJOHMuxer::waitStartDTSOffset(HJMediaFrame::Ptr frame)
{
    HJFNLogi("entry, frame info:{}", frame->formatInfo());
    m_framesQueue.push_back(frame);
    //
    int64_t startDTSOffset = HJ_NOTS_VALUE;
    if ((m_mediaTypes & HJMEDIA_TYPE_VIDEO) && (m_mediaTypes & HJMEDIA_TYPE_AUDIO))
    {
        HJMediaFrame::Ptr videoFrame = nullptr;
        HJMediaFrame::Ptr audioFrame = nullptr;
        for (const auto& mavf : m_framesQueue) {
            if (mavf->isVideo() && !videoFrame) {
                videoFrame = mavf;
            }
            else if (mavf->isAudio() && !audioFrame) {
                audioFrame = mavf;
            }
            if (videoFrame && audioFrame) {
                startDTSOffset = HJ_MIN(videoFrame->m_dts, audioFrame->m_dts);
                break;
            }
        }
    }
    else
    {
        startDTSOffset = frame->m_dts;
    }
    return startDTSOffset;
}

int HJOHMuxer::createMuxer()
{
    int res = HJ_OK;
    if (m_url.empty()) {
        HJFNLoge("error, invalid url");
        return HJErrInvalidParams;
    }
    HJFNLogi("init begin, url:{}", m_url);
    
    do {
        // 确定输出格式
        OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
        if (HJUtilitys::endWith(m_url, ".m4a") || HJUtilitys::endWith(m_url, ".aac")) {
            outputFormat = AV_OUTPUT_FORMAT_M4A;
        }
        
        // 打开输出文件
        m_outputFd = open(m_url.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (m_outputFd < 0) {
            HJFNLoge("error, Could not open output file:{}", m_url);
            res = HJErrIOOpen;
            break;
        }
        
        // 创建muxer
        m_muxer = OH_AVMuxer_Create(m_outputFd, outputFormat);
        if (!m_muxer) {
            HJFNLoge("OH_AVMuxer_Create failed, url:{}", m_url);
            res = HJErrFFNewIC;
            break;
        }
        
        // 设置错误回调
        // 注意：实际的回调结构可能会有所不同，这里按照可能的API设计
        // OH_AVMuxerCallback callback = {0};
        // callback.onError = OnMuxerError;
        // callback.userData = this;
        // OH_AVMuxer_SetCallback(m_muxer, callback);
        
        std::vector<int> nidIdxs;
        
        // 添加轨道
        res = m_mediaInfo->forEachInfo([&](const HJStreamInfo::Ptr& info) -> int {
            int ret = HJ_OK;
            HJMediaType mediaType = info->getType();
            if (HJMEDIA_TYPE_VIDEO != mediaType &&
                HJMEDIA_TYPE_AUDIO != mediaType &&
                HJMEDIA_TYPE_SUBTITLE != mediaType) {
                return HJ_OK;
            }
            
            HJTrackHolder::Ptr holder = std::make_shared<HJTrackHolder>();
            m_tracks.emplace(mediaType, holder);
            
            // 获取编码器参数
            const HJStreamInfo::Ptr encInfo = info;
            if (!info->getAVCodecParams()) {
                HJBaseCodec::Ptr encoder = getEncoder(mediaType);
                if (!encoder) {
                    return HJErrCodecCreate;
                }
//                encInfo = encoder->getInfo();
                holder->m_codec = encoder;
            }
            
            if (!encInfo || !encInfo->getAVCodecParams()) {
                HJFNLoge("need codec params error");
                return HJErrFatal;
            }
            
            ret = addTrack(encInfo);
            if (ret != HJ_OK) {
                return ret;
            }
            
            nidIdxs.emplace_back(info->getType());
            return ret;
        });
        
        if (res < HJ_OK) {
            HJFNLogi("m_mediaInfo->forEachInfo error:{}", res);
            break;
        }
        
        m_nipMuster = std::make_shared<HJNipMuster>(nidIdxs);
    } while (false);
    
    HJFNLogi("init end, res:{}", res);
    return res;
}

void HJOHMuxer::destroyMuxer()
{
    setQuit(true);
    
    if (m_muxer) {
        OH_AVMuxer_Destroy(m_muxer);
        m_muxer = nullptr;
    }
    
    if (m_outputFd >= 0) {
        close(m_outputFd);
        m_outputFd = -1;
    }
}

/**
* dts > pst
* interleave
* 
*/
int HJOHMuxer::addFrame(const HJMediaFrame::Ptr& frame)
{
    return procFrame(frame, false);
}

int HJOHMuxer::writeFrame(const HJMediaFrame::Ptr& frame)
{
    return procFrame(frame, true);
}

void HJOHMuxer::done()
{
    if (m_muxer && m_muxerStarted) {
        OH_AVMuxer_Stop(m_muxer);
        m_muxerStarted = false;
    }
    HJFNLogi("done frame cnt video:{}, audio:{}, total:{}", m_videoFrameCnt, m_audioFrameCnt, m_totalFrameCnt);
}

int HJOHMuxer::procFrame(const HJMediaFrame::Ptr& frame, bool isDup)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do {
        if (m_mediaTypes != m_mediaInfo->getMediaTypes()) 
        {
            if (m_keyFrameCheck)
            {
                if (frame->isKeyFrame()) {
                    m_firstKeyFrame = true;
                }
                if (!m_firstKeyFrame) {
                    HJFNLogw("warning, before first key frame, drop frame:{} ", frame->formatInfo());
                    return HJ_MORE_DATA;
                }
            }
            res = gatherMediaInfo(frame);
            if (HJ_OK != res) {
                break;
            }
        }
        if (HJ_NOTS_VALUE == m_startDTSOffset) {
            m_startDTSOffset = waitStartDTSOffset(frame);
            res = HJ_MORE_DATA;
            break;
        }
        if (m_framesQueue.size() > 0) {
            for (const auto& mavf : m_framesQueue) {
                res = deliver(mavf, isDup);
                if (HJ_OK != res) {
                    HJFNLoge("error, deliver frame info:{}", mavf->formatInfo());
                    break;
                }
            }
            m_framesQueue.clear();
        }

        res = deliver(frame, isDup);
    } while (false);

    return res;
}

int HJOHMuxer::deliver(const HJMediaFrame::Ptr& frame, bool isDup)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do {
#if defined(HJ_MUXER_DELAY_INIT)
        if (!m_muxer)
        {
            res = createMuxer();
            if (HJ_OK != res) {
                destroyMuxer();
                break;
            }
        }
#endif //HJ_MUXER_DELAY_INIT
        if (!m_muxer) {
            res = HJErrNotAlready;
            HJFNLoge("error, muxer is null");
            break;
        }
        
        // 启动muxer
        if (!m_muxerStarted) {
            OH_AVErrCode ohRet = OH_AVMuxer_Start(m_muxer);
            if (ohRet != AV_ERR_OK) {
                HJFNLoge("OH_AVMuxer_Start failed, error:{}", ohRet);
                res = HJErrFFNewIC;
                break;
            }
            m_muxerStarted = true;
        }
        
        HJMediaFrame::Ptr mavf = nullptr;
        if (isDup) {
            mavf = frame->deepDup();
            if (!mavf) {
                res = HJErrNotAlready;
                HJFNLoge("error, dup frame failed");
                break;
            }
        }
        else {
            mavf = frame;
        }
        
        if(m_timestampZero) {
            mavf->setPTSDTS(mavf->m_pts - m_startDTSOffset, mavf->m_dts - m_startDTSOffset);
        }

        HJMediaType mediaType = mavf->getType();
        int32_t trackId = -1;
        
        if (mediaType == HJMEDIA_TYPE_VIDEO) {
            trackId = m_videoTrackId;
            
            if ((mavf->m_dts < m_lastVideoDTS) && (m_lastVideoDTS != HJ_INT64_MIN)) {
                HJFNLoge("error, muxer video input dts:{} < last dts:{}", mavf->m_dts, m_lastVideoDTS);
                return HJErrMuxerDTS;
            }
            if (mavf->m_pts < mavf->m_dts) {
                mavf->m_pts = mavf->m_dts + 1;
                HJFNLogw("muxer write video warning, pts:{} < dts:{}", mavf->m_pts, mavf->m_dts);
            }
            m_lastVideoDTS = mavf->m_dts;
            m_videoFrameCnt++;
        }
        else if (mediaType == HJMEDIA_TYPE_AUDIO) {
            trackId = m_audioTrackId;
            
            if ((mavf->m_dts < m_lastAudioDTS) && (m_lastAudioDTS != HJ_INT64_MIN)) {
                HJFNLoge("error, muxer audio input dts:{} < last dts:{}", mavf->m_dts, m_lastAudioDTS);
                return HJErrMuxerDTS;
            }
            if (mavf->m_pts < mavf->m_dts) {
                mavf->m_pts = mavf->m_dts + 1;
                HJFNLogw("muxer write audio warning, pts:{} < dts:{}", mavf->m_pts, mavf->m_dts);
            }
            m_lastAudioDTS = mavf->m_dts;
            m_audioFrameCnt++;
        }
        
        if (trackId < 0) {
            HJFNLoge("can't find property track for media type:{}", mediaType);
            return HJ_OK;
        }

        if (mavf && (HJFRAME_EOF != mavf->getFrameType())) {
            res = writeSampleBuffer(mavf, trackId);
            if (res == HJ_OK) {
                m_totalFrameCnt++;
            }
        }
        
        // 日志输出
        HJNipInterval::Ptr nip = m_nipMuster->getNipById(mediaType);
        if (mavf && nip && nip->valid()) {
            HJFNLogi(mavf->formatInfo());
        }
        
    } while (false);

    if (res < HJ_OK) {
        HJFNLoge("muxer write frame error:{}", res);
    }

    return res;
}

int HJOHMuxer::addTrack(const HJStreamInfo::Ptr& info)
{
    if (!m_muxer || !info) {
        return HJErrInvalidParams;
    }
    
    OH_AVFormat* format = createFormat(info);
    if (!format) {
        HJFNLoge("create format failed");
        return HJErrFatal;
    }
    
    int32_t trackId = -1;
    OH_AVErrCode ohRet = OH_AVMuxer_AddTrack(m_muxer, &trackId, format);
    OH_AVFormat_Destroy(format);
    
    if (ohRet != AV_ERR_OK) {
        HJFNLoge("OH_AVMuxer_AddTrack failed, error:{}", ohRet);
        return HJErrFFNewIC;
    }
    
    // 存储轨道ID
    HJMediaType mediaType = info->getType();
    if (mediaType == HJMEDIA_TYPE_VIDEO) {
        m_videoTrackId = trackId;
    } else if (mediaType == HJMEDIA_TYPE_AUDIO) {
        m_audioTrackId = trackId;
    }
    
    HJFNLogi("add track success, type:{}, id:{}", mediaType, trackId);
    return HJ_OK;
}

OH_AVFormat* HJOHMuxer::createFormat(const HJStreamInfo::Ptr& info)
{
    if (!info) {
        return nullptr;
    }
    
    HJMediaType mediaType = info->getType();
    OH_AVFormat* format = nullptr;
    
    if (mediaType == HJMEDIA_TYPE_VIDEO) {
        HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(info);
        if (!videoInfo) {
            return nullptr;
        }
        
        // 确定MIME类型
        std::string mimeType = "video/mp4v-es"; // 默认
        AVCodecParameters* codecParams = info->getAVCodecParams();
        if (codecParams) {
            switch (codecParams->codec_id) {
                case AV_CODEC_ID_H264:
                    mimeType = "video/avc";
                    break;
                case AV_CODEC_ID_HEVC:
                    mimeType = "video/hevc";
                    break;
                default:
                    mimeType = "video/avc"; // 默认使用H264
                    break;
            }
        }
        
        format = OH_AVFormat_CreateVideoFormat(mimeType.c_str(), videoInfo->m_width, videoInfo->m_height);
        if (format) {
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, videoInfo->m_width);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, videoInfo->m_height);
            OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, videoInfo->m_frameRate);
            OH_AVFormat_SetStringValue(format, OH_MD_KEY_CODEC_MIME, mimeType.c_str());
            
            if (codecParams) {
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, codecParams->bit_rate);
            }
        }
    }
    else if (mediaType == HJMEDIA_TYPE_AUDIO) {
        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(info);
        if (!audioInfo) {
            return nullptr;
        }
        
        // 确定MIME类型
        std::string mimeType = "audio/mp4a-latm"; // 默认AAC
        AVCodecParameters* codecParams = info->getAVCodecParams();
        if (codecParams) {
            switch (codecParams->codec_id) {
                case AV_CODEC_ID_AAC:
                    mimeType = "audio/mp4a-latm";
                    break;
                case AV_CODEC_ID_MP3:
                    mimeType = "audio/mpeg";
                    break;
                default:
                    mimeType = "audio/mp4a-latm"; // 默认使用AAC
                    break;
            }
        }
        
        format = OH_AVFormat_CreateAudioFormat(mimeType.c_str(), audioInfo->m_samplesRate, audioInfo->m_channels);
        if (format) {
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, audioInfo->m_channels);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, audioInfo->m_samplesRate);
            OH_AVFormat_SetStringValue(format, OH_MD_KEY_CODEC_MIME, mimeType.c_str());
            
            if (codecParams) {
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, codecParams->bit_rate);
            }
        }
    }
    
    return format;
}

int HJOHMuxer::writeSampleBuffer(const HJMediaFrame::Ptr& frame, int32_t trackId)
{
    if (!frame || !m_muxer || trackId < 0) {
        return HJErrInvalidParams;
    }
    
    // 获取帧数据
    const uint8_t* data = NULL; //frame->getData();
    size_t dataSize = 0; //frame->getSize();
    if (!data || dataSize == 0) {
        HJFNLogw("empty frame data");
        return HJ_OK;
    }
    
    // 创建OH_AVBuffer
    OH_AVBuffer* buffer = OH_AVBuffer_Create(dataSize);
    if (!buffer) {
        HJFNLoge("create buffer failed");
        return HJErrMemAlloc;
    }
    
    // 复制数据
    uint8_t* bufferData = OH_AVBuffer_GetAddr(buffer);
    if (!bufferData) {
        OH_AVBuffer_Destroy(buffer);
        HJFNLoge("get buffer addr failed");
        return HJErrMemAlloc;
    }
    
    memcpy(bufferData, data, dataSize);
    
    // 设置buffer属性
    OH_AVCodecBufferAttr attr;
    memset(&attr, 0, sizeof(attr));
    attr.pts = HJ_MS_TO_US(frame->m_pts);
    attr.size = dataSize;
    attr.offset = 0;
    attr.flags = 0;
    
    if (frame->isKeyFrame()) {
        attr.flags |= AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    }
    
    OH_AVErrCode ohRet = OH_AVBuffer_SetBufferAttr(buffer, &attr);
    if (ohRet != AV_ERR_OK) {
        OH_AVBuffer_Destroy(buffer);
        HJFNLoge("set buffer attr failed, error:{}", ohRet);
        return HJErrFatal;
    }
    
    // 写入sample
    ohRet = OH_AVMuxer_WriteSampleBuffer(m_muxer, trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    
    if (ohRet != AV_ERR_OK) {
        HJFNLoge("write sample buffer failed, error:{}", ohRet);
        return HJErrFatal;
    }
    
    return HJ_OK;
}

HJBaseCodec::Ptr HJOHMuxer::getEncoder(const HJMediaType mediaType)
{
    int res = HJ_OK;
    HJTrackHolder::Ptr holder = nullptr;
    HJBaseCodec::Ptr encoder = nullptr;
    auto it = m_tracks.find(mediaType);
    if (it != m_tracks.end()) {
        holder = it->second;
        encoder = holder->m_codec;
    }
    if (!holder) {
        return nullptr;
    }
    if (!encoder)
    {
        const HJStreamInfo::Ptr info = m_mediaInfo->getStreamInfo(mediaType);
        if (!info) {
            return nullptr;
        }
        encoder = HJBaseCodec::createEncoder(mediaType);
        if (!encoder) {
            return nullptr;
        }
        res = encoder->init(info);
        if (HJ_OK != res) {
            return nullptr;
        }
        holder->m_codec = encoder;
    }
    return encoder;
}

void HJOHMuxer::OnMuxerError(OH_AVMuxer *muxer, int32_t errorCode, void *userData)
{
    HJOHMuxer* hjMuxer = static_cast<HJOHMuxer*>(userData);
    if (hjMuxer) {
        hjMuxer->priOnMuxerError(muxer, errorCode);
    }
}

void HJOHMuxer::priOnMuxerError(OH_AVMuxer *muxer, int32_t errorCode)
{
    HJFNLoge("muxer error callback, error code:{}", errorCode);
    // 可以在这里处理错误回调
}

NS_HJ_END