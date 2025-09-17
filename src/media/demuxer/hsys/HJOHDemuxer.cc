//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJOHDemuxer.h"
#include "HJFLog.h"
#include "HJGlobalSettings.h"
#include "HJTime.h"
#include "HJMediaUtils.h"
#include "HJUtilitys.h"
#include "HJFFUtils.h"
#include "multimedia/player_framework/native_avdemuxer.h"
#include "multimedia/player_framework/native_avsource.h"
#include "multimedia/player_framework/native_avcodec_base.h"
#include "multimedia/player_framework/native_avformat.h"
#include "multimedia/player_framework/native_avbuffer.h"
#include "multimedia/player_framework/native_averrors.h"

// 如果HarmonyOS SDK没有定义这些常量，我们自己定义
#ifndef AV_ERR_EOF
#define AV_ERR_EOF (-1)
#endif

NS_HJ_BEGIN
//***********************************************************************************//
HJOHDemuxer::HJOHDemuxer()
    : HJBaseDemuxer()
{

}

HJOHDemuxer::HJOHDemuxer(const HJMediaUrl::Ptr& mediaUrl)
    : HJBaseDemuxer(mediaUrl)
{

}

HJOHDemuxer::~HJOHDemuxer()
{
    done();
}

int HJOHDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
{
    int res = HJBaseDemuxer::init(mediaUrl);
    if (HJ_OK != res) {
        HJFNLoge("init error:{}", res);
        return res;
    }
    
    std::string url = m_mediaUrl->getUrl();
    if (url.empty()) {
        HJFNLoge("error, input url:{} invalid", url);
        return HJErrInvalidUrl;
    }
    
    int64_t t1 = 0;
    int64_t t0 = HJTime::getCurrentMillisecond();
    HJFNLogi("load stream url:{}, start time:{}, end time:{}, timeout:{}", 
        url, m_mediaUrl->getStartTime(), m_mediaUrl->getEndTime(), m_mediaUrl->getTimeout());
    
    do {
        // 创建AVSource (注意: OH_AVSource_CreateWithURI 需要非const char*)
        char* mutableUrl = const_cast<char*>(url.c_str());
        m_source = OH_AVSource_CreateWithURI(mutableUrl);
        if (!m_source) {
            HJFNLoge("OH_AVSource_CreateWithURI failed, error");
            res = HJErrFFNewIC;
            break;
        }
        
        m_demuxer = OH_AVDemuxer_CreateWithSource(m_source);
        if (!m_demuxer) {
            HJFNLoge("OH_AVDemuxer_CreateWithSource failed");
            res = HJErrFFNewIC;
            break;
        }
        
        // 设置回调函数 (注意：实际的回调结构可能会有所不同)
        // OH_AVDemuxerCallback callback = {0};
        // callback.onError = OnDemuxerError;
        // callback.onFormatChanged = OnDemuxerFormatChanged;
        // callback.userData = this;
        // 
        // ohRet = OH_AVDemuxer_SetCallback(m_demuxer, callback);
        // if (ohRet != AV_ERR_OK) {
        //     HJFNLogw("OH_AVDemuxer_SetCallback failed, error:{}", ohRet);
        // }
        
        t1 = HJTime::getCurrentMillisecond();
        
        // 解析媒体信息
        res = makeMediaInfo();
        if (HJ_OK != res) {
            break;
        }
        
        // 选择轨道
        res = selectTracks();
        if (HJ_OK != res) {
            break;
        }
        
        analyzeMediaFormat();
        
        m_runState = HJRun_Ready;
    } while (false);
    
    int64_t t2 = HJTime::getCurrentMillisecond();
    HJFNLogi("init load stream end, res:{}, time:{},{},{}, url:{}", 
        res, t1 - t0, t2 - t1, t2 - t0, url);
    
    return res;
}

int HJOHDemuxer::init() {
    return init(m_mediaUrl);
}

void HJOHDemuxer::done()
{
    HJBaseDemuxer::done();
    setQuit(true);
    
    if (m_sampleBuffer) {
        OH_AVBuffer_Destroy(m_sampleBuffer);
        m_sampleBuffer = nullptr;
    }
    
    if (m_demuxer) {
        OH_AVDemuxer_Destroy(m_demuxer);
        m_demuxer = nullptr;
    }
    
    if (m_source) {
        OH_AVSource_Destroy(m_source);
        m_source = nullptr;
    }
}

int HJOHDemuxer::seek(int64_t pos)
{
    HJBaseDemuxer::seek(pos);
    if (!m_mediaInfo || !m_demuxer) {
        return HJErrNotAlready;
    }
    
    int64_t timestamp = (HJ_NOTS_VALUE != m_mediaInfo->m_startTime) ? 
        (m_mediaInfo->m_startTime + pos) : pos;
    
    // 注意：需要使用正确的OH_AVSeekMode枚举值
    // 根据HarmonyOS SDK文档，可能的值有：0 (默认), 1 (精确), 2 (下一个关键帧)
    OH_AVErrCode ohRet = OH_AVDemuxer_SeekToTime(m_demuxer, HJ_MS_TO_US(timestamp), (OH_AVSeekMode)0);
    if (ohRet != AV_ERR_OK) {
        HJFNLoge("OH_AVDemuxer_SeekToTime failed, error:{}, seek time:{}", ohRet, timestamp);
        return HJErrFFSeek;
    }
    
    m_isEOF = false;
    HJFNLogi("seek end pos:{}, timestamp:{}", pos, timestamp);
    
    return HJ_OK;
}

int HJOHDemuxer::getFrame(HJMediaFrame::Ptr& frame)
{
    if (!m_demuxer) {
        return HJErrNotAlready;
    }
    
    if (m_isEOF) {
        frame = HJMediaFrame::makeEofFrame();
        return HJ_EOF;
    }
    
    if (!m_sampleBuffer) {
        m_sampleBuffer = OH_AVBuffer_Create(1024 * 1024); // 1MB buffer
        if (!m_sampleBuffer) {
            HJFNLoge("create sample buffer failed");
            return HJErrMemAlloc;
        }
    }
    
    int res = HJ_OK;
    
    // 由于HarmonyOS API需要指定轨道索引，我们需要轮流读取不同轨道
    std::vector<uint32_t> enabledTracks;
    if (m_videoTrackIndex >= 0 && m_mediaUrl->isEnbleVideo()) {
        enabledTracks.push_back(m_videoTrackIndex);
    }
    if (m_audioTrackIndex >= 0 && m_mediaUrl->isEnableAudio()) {
        enabledTracks.push_back(m_audioTrackIndex);
    }
    
    if (enabledTracks.empty()) {
        HJFNLoge("没有启用的轨道");
        return HJErrNotAlready;
    }
    
    // 逐个尝试读取每个启用的轨道
    for (uint32_t currentTrackIndex : enabledTracks) {
        OH_AVErrCode ohRet = OH_AVDemuxer_ReadSampleBuffer(m_demuxer, currentTrackIndex, m_sampleBuffer);
        if (ohRet != AV_ERR_OK) {
            if (ohRet == AV_ERR_EOF || ohRet == AV_ERR_NO_MEMORY) { // 使用定义的错误码
                // 该轨道已经结束，尝试下一个轨道
                continue;
            }
            HJFNLoge("OH_AVDemuxer_ReadSample failed for track {}, error:{}", currentTrackIndex, ohRet);
            continue;
        }
        
        OH_AVCodecBufferAttr bufferAttr;
        ohRet = OH_AVBuffer_GetBufferAttr(m_sampleBuffer, &bufferAttr);
        if (ohRet != AV_ERR_OK) {
            HJFNLoge("OH_AVBuffer_GetBufferAttr failed, error:{}", ohRet);
            continue;
        }
        
        // 使用currentTrackIndex来判断轨道类型
        uint32_t trackIndex = currentTrackIndex;
        
        HJMediaFrame::Ptr mvf = nullptr;
        bool isVideoFrame = false;
        bool isAudioFrame = false;
        
        // 根据轨道索引判断类型
        if (trackIndex == m_videoTrackIndex && m_mediaUrl->isEnbleVideo()) {
            isVideoFrame = true;
        } else if (trackIndex == m_audioTrackIndex && m_mediaUrl->isEnableAudio()) {
            isAudioFrame = true;
        } else {
            // 跳过未知轨道
            continue;
        }
        
        // 判断是视频还是音频轨道
        if (isVideoFrame) {
            HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo(m_mediaInfo->m_vstIdx);
            HJVideoInfo::Ptr mvinfo = m_mediaInfo->getVideoInfo();
            if (!mvinfo || !trackInfo) {
                res = HJErrESParse;
                HJFNLoge("error, get video info failed");
                break;
            }
            
            mvf = HJMediaFrame::makeVideoFrame();
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(mvf->m_info);
            
            videoInfo->m_trackID = m_mediaInfo->m_vstIdx;
            videoInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            videoInfo->setCodecID(mvinfo->getCodecID());
            videoInfo->m_width = mvinfo->m_width;
            videoInfo->m_height = mvinfo->m_height;
            videoInfo->m_frameRate = mvinfo->m_frameRate;
            videoInfo->m_TimeBase = mvinfo->m_TimeBase;
            videoInfo->m_dataType = HJDATA_TYPE_ES;
            
            // 检查是否为关键帧（注意：实际的标志位可能不同）
            // HarmonyOS 中的关键帧标志可能与 FFmpeg 不同  
            //AVCODEC_BUFFER_FLAGS_EOS
            if (bufferAttr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) { // 使用通用的关键帧标志
                mvf->setFrameType(HJFRAME_KEY);
                m_validVFlag = true;
            }
            
            mvf->setID(m_mediaInfo->m_vstIdx);
            mvf->setIndex(m_vfCnt);
            
            // 等待关键帧
            if (!m_validVFlag) {
                continue;
            }
            
            m_vfCnt++;
        } else if (isAudioFrame) {
            HJTrackInfo::Ptr trackInfo = m_atrs->getTrackInfo(m_mediaInfo->m_astIdx);
            HJAudioInfo::Ptr mainfo = m_mediaInfo->getAudioInfo();
            if (!mainfo || !trackInfo) {
                res = HJErrESParse;
                HJFNLoge("error, get audio info failed");
                break;
            }
            
            mvf = HJMediaFrame::makeAudioFrame();
            mvf->setFrameType(HJFRAME_KEY);
            mvf->setID(m_mediaInfo->m_astIdx);
            mvf->setIndex(m_afCnt);
            
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(mvf->m_info);
            audioInfo->m_trackID = m_mediaInfo->m_astIdx;
            audioInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            audioInfo->setCodecID(mainfo->getCodecID());
            audioInfo->m_channels = mainfo->m_channels;
            audioInfo->m_samplesRate = mainfo->m_samplesRate;
            audioInfo->m_samplesPerFrame = mainfo->m_samplesPerFrame;
            audioInfo->m_blockAlign = mainfo->m_blockAlign;
            audioInfo->m_sampleFmt = mainfo->m_sampleFmt;
            audioInfo->m_dataType = HJDATA_TYPE_ES;
            
            // 检查音频时间戳连续性
            int64_t dts = HJ_US_TO_MS(bufferAttr.pts);
            if (HJ_NOTS_VALUE != m_lastAudioDTS) {
                int64_t delta = dts - m_lastAudioDTS;
                if (dts < m_lastAudioDTS || delta > m_gapThreshold) {
                    m_streamID++;
                    mvf->setFlush();
                    HJFNLogw("warning, audio media frame flush, time skip, m_streamID:{}", m_streamID);
                }
            }
            m_lastAudioDTS = dts;
            
            m_afCnt++;
        } else {
            // 跳过未知轨道
            continue;
        }
        
        // 设置时间戳和数据
        HJRational timeBase = {1, 1000000}; // 微秒时间基
        mvf->setPTSDTS(bufferAttr.pts, bufferAttr.pts, timeBase);
        mvf->setDuration(bufferAttr.size, timeBase);
        mvf->setMTS(bufferAttr.pts, bufferAttr.pts, bufferAttr.size, timeBase);
        
        // 如果需要时间偏移
        if (m_timeOffsetEnable && HJ_NOTS_VALUE != m_mediaInfo->m_startTime) {
            int64_t offsetTime = 0;
            int64_t startTime = HJ_MS_TO_US(m_mediaInfo->m_startTime + offsetTime);
            if (HJ_NOTS_VALUE != bufferAttr.pts) {
                bufferAttr.pts -= startTime;
                mvf->setPTSDTS(bufferAttr.pts, bufferAttr.pts, timeBase);
                mvf->setMTS(bufferAttr.pts, bufferAttr.pts, bufferAttr.size, timeBase);
            }
        }
        
        mvf->m_streamIndex = m_streamID;
        
        // 复制数据
        uint8_t* bufferData = OH_AVBuffer_GetAddr(m_sampleBuffer);
        if (bufferData && bufferAttr.size > 0) {
            HJBuffer::Ptr frameBuffer = std::make_shared<HJBuffer>(bufferData + bufferAttr.offset, bufferAttr.size);
            mvf->setBuffer(frameBuffer);
        }
        
        frame = std::move(mvf);
        
#if HJ_HAVE_TRACKER
        frame->addTrajectory(HJakeTrajectory());
#endif
        
        HJNipInterval::Ptr nip = m_nipMuster->getNipById(frame->getType());
        if (frame && nip && nip->valid()) {
            HJFNLogi(frame->formatInfo());
        }
        return HJ_OK; // 成功获取帧，返回
    }
    
    // 所有轨道都到达末尾
    procEofFrame();
    frame = HJMediaFrame::makeEofFrame();
    m_isEOF = true;
    HJFNLogw("get frame eof, all track max duration:{}", getDuration());
    
    return HJ_EOF;
}

void HJOHDemuxer::reset()
{
    HJBaseDemuxer::reset();
    
    int64_t startTime = (HJ_NOTS_VALUE != m_mediaUrl->getStartTime()) ? m_mediaUrl->getStartTime() : 0;
    if(startTime >= 0 && startTime < m_mediaInfo->getDuration()) {
        int res = seek(startTime);
        if(HJ_OK != res) {
            HJFNLoge("seek time:{}, error:{}", startTime, res);
        }
    }
    return;
}

int HJOHDemuxer::makeMediaInfo()
{
    if (!m_source) {
        return HJErrNotAlready;
    }
    
    int res = HJ_OK;
    
    // 获取源格式信息
    OH_AVFormat* sourceFormat = OH_AVSource_GetSourceFormat(m_source);
    if (!sourceFormat) {
        HJFNLogw("获取源格式信息失败");
        return HJErrFatal;
    }
    
    // 从源格式中获取轨道数量
    int32_t trackCount = 0;
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount);
    if (trackCount <= 0) {
        OH_AVFormat_Destroy(sourceFormat);
        HJFNLoge("获取轨道数量失败，trackCount:{}", trackCount);
        return HJErrFatal;
    }
    
    int64_t duration = 0;
    if (sourceFormat) {
        OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration);
        m_mediaInfo->m_duration = HJ_US_TO_MS(duration);
        OH_AVFormat_Destroy(sourceFormat);
    }
    
    int64_t minStartTime = HJ_INT64_MAX;
    std::vector<int> nidIdxs;
    
    // 遍历所有轨道
    for (int32_t i = 0; i < trackCount; i++) {
        OH_AVFormat* trackFormat = OH_AVSource_GetTrackFormat(m_source, i);
        if (!trackFormat) {
            HJFNLogw("获取轨道 {} 格式信息失败", i);
            continue;
        }
        
        // 获取轨道类型
        int32_t trackType = -1;
        OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType);
        
        // 处理视频轨道
        if (trackType == MEDIA_TYPE_VID && m_mediaUrl->isEnbleVideo() && m_mediaInfo->m_vstIdx < 0) {
            m_mediaInfo->m_vstIdx = i;
            m_videoTrackIndex = i;
            
            OH_AVDemuxer_SelectTrackByID(m_demuxer, i);
            // 获取MIME类型
            const char* mimeType = nullptr;
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType);
            uint8_t *codecConfig = nullptr;
            size_t codecConfigLen = 0;
            OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &codecConfigLen);
            if (codecConfigLen > 0 && codecConfigLen < sizeof(codecConfig)) {
//                memcpy(info.codecConfig, codecConfig, info.codecConfigLen);
            }
            
            HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>();            videoInfo->m_trackID = i;
            videoInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            
            // 获取视频参数
            int32_t width = 0, height = 0;
            double frameRate = 0.0;
            int32_t rotation = 0;
            int64_t bitrate = 0;
            
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height);
            OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate);
            OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation);
            
            videoInfo->m_width = width;
            videoInfo->m_height = height;
            videoInfo->m_frameRate = frameRate;
            videoInfo->m_TimeBase = HJMediaUtils::checkTimeBase({1, 1000000}); // 微秒时间基
            
            // 根据MIME类型设置编解码器ID
            if (mimeType) {
                if(!strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_AVC)) {
                    videoInfo->setCodecID(AV_CODEC_ID_H264);
                } else if(!strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_HEVC)) {
                    videoInfo->setCodecID(AV_CODEC_ID_H265);
                }
            }
            
            videoInfo->setTimeRange({{0, {1, 1000}}, {m_mediaInfo->m_duration, {1, 1000}}});
            
            nidIdxs.emplace_back(videoInfo->getType());
            m_mediaInfo->addStreamInfo(std::move(videoInfo));
        }
        // 处理音频轨道
        else if (trackType == MEDIA_TYPE_AUD && m_mediaUrl->isEnableAudio() && m_mediaInfo->m_astIdx < 0) {
            m_mediaInfo->m_astIdx = i;
            m_audioTrackIndex = i;
            
            OH_AVDemuxer_SelectTrackByID(m_demuxer, i);
            // 获取MIME类型
            const char* mimeType = nullptr;
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType);
            
            uint8_t *codecConfig = nullptr;
            size_t codecConfigLen = 0;
            int32_t aacAdts = -1;
            OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &codecConfigLen);
            if (codecConfigLen > 0 && codecConfigLen < sizeof(codecConfig)) {
//                memcpy(info.codecConfig, codecConfig, info.codecConfigLen);
            }
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AAC_IS_ADTS, &aacAdts);
            
            HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
            audioInfo->m_trackID = i;
            audioInfo->m_streamIndex = m_mediaUrl->getUrlHash();
            
            // 获取音频参数
            int32_t audioSampleForamt = 0, sampleRate = 0, channelCount = 0;
            int64_t audioChannelLayout = 0;
            
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &audioSampleForamt);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount);
            OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &audioChannelLayout);
            OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate);
            
            audioInfo->m_samplesRate = sampleRate;
            audioInfo->m_channels = channelCount;
            audioInfo->setTimeBase({1, 1000000}); // 微秒时间基
            
            // 根据MIME类型设置编解码器ID
            if (mimeType) {
                if(!strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_AAC)) {
                    audioInfo->setCodecID(AV_CODEC_ID_AAC);
                } else if(!strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_OPUS)) {
                    audioInfo->setCodecID(AV_CODEC_ID_OPUS);
                }
            }
            
            audioInfo->setTimeRange({{0, {1, 1000}}, {m_mediaInfo->m_duration, {1, 1000}}});
            
            nidIdxs.emplace_back(audioInfo->getType());
            m_mediaInfo->addStreamInfo(std::move(audioInfo));
        }
        
        OH_AVFormat_Destroy(trackFormat);
    }
    
    m_mediaInfo->m_startTime = HJ_MAX(0, minStartTime == HJ_INT64_MAX ? 0 : minStartTime);
    m_mediaInfo->setTimeRange({{m_mediaInfo->m_startTime, HJ_TIMEBASE_MS}, {m_mediaInfo->m_duration, HJ_TIMEBASE_MS}});
    m_nipMuster = std::make_shared<HJNipMuster>(nidIdxs);
    
    HJFNLogi("media info:{}", m_mediaInfo->formatInfo());
    
    return res;
}

int HJOHDemuxer::selectTracks()
{
    if (!m_demuxer) {
        return HJErrNotAlready;
    }
    
    // 选择视频轨道
    if (m_videoTrackIndex >= 0) {
        OH_AVErrCode ohRet = OH_AVDemuxer_SelectTrackByID(m_demuxer, m_videoTrackIndex);
        if (ohRet != AV_ERR_OK) {
            HJFNLoge("OH_AVDemuxer_SelectTrackByID failed for video track {}, error:{}", m_videoTrackIndex, ohRet);
            return HJErrFFLoadUrl;
        }
        HJFNLogi("selected video track: {}", m_videoTrackIndex);
    }
    
    // 选择音频轨道
    if (m_audioTrackIndex >= 0) {
        OH_AVErrCode ohRet = OH_AVDemuxer_SelectTrackByID(m_demuxer, m_audioTrackIndex);
        if (ohRet != AV_ERR_OK) {
            HJFNLoge("OH_AVDemuxer_SelectTrackByID failed for audio track {}, error:{}", m_audioTrackIndex, ohRet);
            return HJErrFFLoadUrl;
        }
        HJFNLogi("selected audio track: {}", m_audioTrackIndex);
    }
    
    return HJ_OK;
}

HJMediaFrame::Ptr HJOHDemuxer::procEofFrame()
{
    HJMediaFrame::Ptr mvf = nullptr;
    
    int64_t minDelta = 0;
    int64_t maxDelta = 0;
    for (auto &it : m_atrs->m_trackInfos) {
        auto& tinfo = it.second;
        if(tinfo->m_startDelta > minDelta) {
            m_atrs->m_minHoleTrack = tinfo;
            minDelta = tinfo->m_startDelta;
        }
        if(tinfo->m_endDelta < maxDelta) {
            m_atrs->m_maxHoleTrack = tinfo;
            maxDelta = tinfo->m_endDelta;
        }
    }
    HJFNLogi("eof attrs:" + m_atrs->formatInfo());
    
    return mvf;
}

int64_t HJOHDemuxer::getDuration()
{
    if (m_mediaInfo) {
        int64_t duration = m_mediaInfo->getDuration();
        return HJ_MAX(duration, m_atrs->getDuration());
    } else if (m_mediaUrl) {
        return m_mediaUrl->getDuration();
    }
    return 0;
}

int HJOHDemuxer::analyzeMediaFormat()
{
    if (!m_source) {
        return HJErrNotAlready;
    }
    
    int res = HJ_OK;
    
    // 分析媒体格式，判断是否为直播流
    std::string url = m_mediaUrl->getUrl();
    if (url.find("rtmp") != std::string::npos ||
        url.find("rtsp") != std::string::npos ||
        url.find("http") != std::string::npos) {
        m_isLiving = true;
    }
    
    return res;
}

void HJOHDemuxer::OnDemuxerError(OH_AVDemuxer *demuxer, int32_t errorCode, void *userData)
{
    HJOHDemuxer* the = static_cast<HJOHDemuxer*>(userData);
    if (the) {
        the->priOnDemuxerError(demuxer, errorCode);
    }
}

void HJOHDemuxer::OnDemuxerFormatChanged(OH_AVDemuxer *demuxer, OH_AVFormat *format, void *userData)
{
    HJOHDemuxer* the = static_cast<HJOHDemuxer*>(userData);
    if (the) {
        the->priOnDemuxerFormatChanged(demuxer, format);
    }
}

void HJOHDemuxer::priOnDemuxerError(OH_AVDemuxer *demuxer, int32_t errorCode)
{
    HJFLoge("OH demuxer error: {}", errorCode);
}

void HJOHDemuxer::priOnDemuxerFormatChanged(OH_AVDemuxer *demuxer, OH_AVFormat *format)
{
    HJFLogi("OH demuxer format changed");
}

//***********************************************************************************//
NS_HJ_END