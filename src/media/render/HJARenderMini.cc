//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJARenderMini.h"
//namespace ffmpeg {
#   include "HJFFHeaders.h"
//}
#include "HJFLog.h"
#if defined(HJ_HAVE_ARENDER_MINI)
    #if defined(HJ_OS_DARWIN)
    #   define MA_NO_RUNTIME_LINKING
    #endif
    #define MINIAUDIO_IMPLEMENTATION
    #include "miniaudio.h"
#endif

NS_HJ_BEGIN
//***********************************************************************************//
HJARenderMini::HJARenderMini()
{

}
HJARenderMini::~HJARenderMini()
{
    HJLogi("entry");
    stop();
#if defined(HJ_HAVE_ARENDER_MINI)
    if (m_adev) {
        ma_device_uninit(m_adev);
        free(m_adev);
        m_adev = NULL;
    }
    if (m_adevCfg) {
        free(m_adevCfg);
        m_adevCfg = NULL;
    }
    if (m_actx) {
        ma_context_uninit(m_actx);
        free(m_actx);
        m_actx = NULL;
    }
#endif
    m_converter = nullptr;
    m_fifoProcessor = nullptr;
    //m_fifo = nullptr;
    //m_frameStors = nullptr;

    HJLogi("end");
}

int HJARenderMini::init(const HJAudioInfo::Ptr& info, HJARenderBase::ARCallback cb/* = nullptr*/)
{
    HJLogi("entry");
    int res = HJARenderBase::init(info, cb);
    if (HJ_OK != res) {
        return res;
    }
    m_outInfo = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
    //m_outInfo->m_sampleFmt = info->m_sampleFmt;
#if defined(HJ_HAVE_ARENDER_MINI)
    m_adevCfg = (ma_device_config *)malloc(sizeof(*m_adevCfg));
    if (!m_adevCfg) {
        HJLoge("error, alloc ma device config failed");
        return HJErrNewObj;
    }
    ma_format fmt = ma_format_s16;
    switch (info->m_sampleFmt)
    {
        case AV_SAMPLE_FMT_S16: {
            m_outInfo->m_sampleFmt = AV_SAMPLE_FMT_S16;
            fmt = ma_format_s16;
            break;
        }
        case AV_SAMPLE_FMT_S32: {
            m_outInfo->m_sampleFmt = AV_SAMPLE_FMT_S32;
            fmt = ma_format_s32;
            break;
        }
        case AV_SAMPLE_FMT_FLT: {
            m_outInfo->m_sampleFmt = AV_SAMPLE_FMT_FLT;
            fmt = ma_format_f32;
            break;
        }
        default: {
            m_outInfo->m_sampleFmt = AV_SAMPLE_FMT_S16;
            fmt = ma_format_s16;
            break;
        }
    }
    m_outInfo->m_bytesPerSample = av_samples_get_buffer_size(NULL, m_outInfo->m_channels, 1, (enum AVSampleFormat)m_outInfo->m_sampleFmt, 1);
    //
    m_actx = (ma_context*)malloc(sizeof(ma_context));
    if (!m_actx) {
        return HJErrNewObj;
    }
    if (ma_context_init(NULL, 0, NULL, m_actx) != MA_SUCCESS) {
        res = HJErrAContext;
        HJLoge("error, ma context init failed");
        return res;
    }
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(m_actx, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        res = HJErrAContext;
        HJLoge("error, ma context get device failed");
        return res;
    }
    for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
        HJFLogi("{} - {}", iDevice, pPlaybackInfos[iDevice].name);
    }

    *m_adevCfg = ma_device_config_init(ma_device_type_playback);
    m_adevCfg->playback.format = fmt;
    m_adevCfg->playback.channels = info->m_channels;
    m_adevCfg->sampleRate = info->m_samplesRate;
    m_adevCfg->periodSizeInFrames = info->m_samplesPerFrame;
    m_adevCfg->dataCallback = outAudioCallback;
    m_adevCfg->pUserData = (void *)this;

    m_adev = (ma_device*)malloc(sizeof(*m_adev));
    if (!m_adev) {
        return HJErrNewObj;
    }
    if (ma_device_init(m_actx, m_adevCfg, m_adev) != MA_SUCCESS) {
        HJLoge("error, Failed to open playback device");
        return HJErrCreateADevice;
    }
    //m_fifo = std::make_shared<HJPCMFifo>(m_outInfo->m_sampleFmt, m_outInfo->m_channels);
#endif
    //m_frameStors = std::make_shared<HJMediaStorage>(HJ_DEFAULT_STORAGE_CAPACITY);
    m_nipMuster = std::make_shared<HJNipMuster>();

    m_runState = HJRun_Init;
    HJLogi("end");

    return res;
}

void HJARenderMini::outAudioCallback(ma_device* dev, void* output, const void* input, unsigned int cnt)
{
    if (!dev || !output || cnt <= 0) {
        return;
    }
#if defined(HJ_HAVE_ARENDER_MINI)
    HJARenderMini* render = (HJARenderMini*)dev->pUserData;
    if (render)
    {
        HJ_AUTO_LOCK(render->m_mutex);
        if (!render->m_fifoProcessor) {
            return;
        }
        //
        //HJLogi("entry, callback cnt:" + HJ2STR(cnt));
        HJMediaFrame::Ptr mavf = render->m_fifoProcessor->getFrame(); //render->m_frameStors->pop();
        if (mavf && mavf->getAVFrame())
        {
            //HJLogi("202407221140 entry, callback need audio samples cnt:" + HJ2STR(cnt) + ", mavf info:" + mavf->formatInfo());
            if (render->m_callback) {
                render->m_callback(mavf);
            }

            AVFrame* avf = (AVFrame *)mavf->getAVFrame();
            if (avf) {
                memcpy(output, avf->data[0], avf->linesize[0]);
            }
        }
        else {
            ma_silence_pcm_frames(output, cnt, dev->playback.format, dev->playback.channels);
            //memset(output, 0, (size_t)(cnt * render->m_outInfo->m_bytesPerSample));
            HJLogw("202407221140 warning, set audio kernel silence data 0, need samples cnt:" + HJ2STR(cnt));
        }
    }
#endif
    return;
}

int HJARenderMini::start()
{
    if (!m_adev) {
        return HJErrNotAlready;
    }
    HJLogi("entry");
    int res = HJARenderBase::start();
    if (HJ_OK != res) {
        return res;
    }
    m_runState = HJRun_Ready;
#if defined(HJ_HAVE_ARENDER_MINI)
    ma_result ret = ma_device_start(m_adev);
    if (MA_SUCCESS != ret) {
        HJFLoge("error, ma device start failed, ret:{}", ret);
        res = HJErrFatal;
    }
#endif
    HJLogi("end");

    return res;
}

int HJARenderMini::pause()
{
    if (!m_adev) {
        return HJErrNotAlready;
    }
    HJLogi("pause entry");
    int res = HJARenderBase::pause();
    if (HJ_OK != res) {
        return res;
    }
    m_runState = HJRun_Start;
#if defined(HJ_HAVE_ARENDER_MINI)
    ma_result ret = ma_device_stop(m_adev);
    if (MA_SUCCESS != ret) {
        HJFLoge("error, ma device stop failed, ret:{}", ret);
        res = HJErrFatal;
    }
#endif
    return HJ_OK;
}

//int HJARenderMini::resume()
//{
//    HJLogi("resume entry");
//    return start();
//}

int HJARenderMini::reset()
{
    if (!m_adev) {
        return HJErrNotAlready;
    }
    HJLogi("HJARenderMini reset entry");
    int res = HJARenderBase::reset();
    if (HJ_OK != res) {
        return res;
    }
#if defined(HJ_HAVE_ARENDER_MINI)
    //ma_result ret = ma_device_stop(m_adev);
    //if (MA_SUCCESS != ret) {
    //    HJFLoge("error, ma device stop failed, ret:{}", ret);
    //    res = HJErrFatal;
    //}
    if (m_fifoProcessor) {
        m_fifoProcessor->reset();
    }
    //m_frameStors->clear();
    //
    ma_result ret = ma_device_start(m_adev);
    if (MA_SUCCESS != ret) {
        HJFLoge("error, ma device start failed, ret:{}", ret);
        res = HJErrFatal;
    }
#endif
    return res;
}

int HJARenderMini::stop()
{
    if (!m_adev) {
        return HJErrNotAlready;
    }
    HJLogi("stop entry");
    int res = HJARenderBase::stop();
    if (HJ_OK != res) {
        return res;
    }
#if defined(HJ_HAVE_ARENDER_MINI)
    ma_result ret = ma_device_stop(m_adev);
    if (MA_SUCCESS != ret) {
        HJFLoge("error, ma device stop failed, ret:{}", ret);
        res = HJErrFatal;
    }
#endif
    m_runState = HJRun_Stop;
    return HJ_OK;
}

int HJARenderMini::write(const HJMediaFrame::Ptr rawFrame)
{
    if (!m_adev) {
        return HJErrNotAlready;
    }
    //HJLogi("entry");

    int res = HJ_OK;
    HJ_AUTO_LOCK(m_mutex);
    do
    {
        if (m_fifoProcessor && m_fifoProcessor->getSize() >= HJ_DEFAULT_STORAGE_CAPACITY * m_fifoProcessor->getSpeed()) {
            return HJ_WOULD_BLOCK;
        }
        HJMediaFrame::Ptr mavf = rawFrame;
        HJNipInterval::Ptr nip = m_nipMuster->getInNip();
        if (mavf && nip && nip->valid()) {
            HJFLogi("entry, frame info:{}, frame storage:{}", mavf->formatInfo(), m_fifoProcessor ? m_fifoProcessor->getSize() : 0);
        }
        const HJAudioInfo::Ptr audioInfo = mavf->getAudioInfo();
        //
        if (!m_converter && (*m_outInfo != *audioInfo)) {
            m_converter = std::make_shared<HJAudioConverter>(m_outInfo);
            if (!m_converter) {
                res = HJErrNewObj;
                break;
            }
        } 
        if (m_converter) {
            mavf = m_converter->convert(std::move(mavf));
        }
        if (!m_fifoProcessor) {
            m_fifoProcessor = std::make_shared<HJAFifoProcessor>(m_outInfo->m_samplesPerFrame);
            m_fifoProcessor->setSpeed(m_speed);
            res = m_fifoProcessor->init(m_outInfo);
            if (HJ_OK != res) {
                HJFLoge("error, fifo processor creat failed:{}", res);
                break;
            }
        }
        if (m_fifoProcessor->getSpeed() != m_speed) {
            m_fifoProcessor->setSpeed(m_speed);
        }
        res = m_fifoProcessor->addFrame(std::move(mavf));
        if (HJ_OK != res) {
            HJFLoge("error, fifo processor add frame failed:{}", res);
            break;
        }
        //
        //if (!m_audioProcessor) {
        //    m_audioProcessor = std::make_shared<HJAudioProcessor>();
        //    m_audioProcessor->setSpeed(2.0f);
        //}
        //if (m_audioProcessor)
        //{
        //    res = m_audioProcessor->addFrame(mavf);
        //    if (HJ_OK != res) {
        //        HJLoge("error, audio processor add frame failed");
        //        break;
        //    }
        //    HJMediaFrame::Ptr outFrame = nullptr;
        //    res = m_audioProcessor->getFrame(outFrame);
        //    if (HJ_OK != res) {
        //        HJLoge("error, audio processor get frame failed");
        //        break;
        //    }
        //    if (!outFrame) {
        //        break;
        //    }
        //    mavf = std::move(outFrame);
        //}
        //
        //const HJAudioInfo::Ptr mavfAInfo = mavf->getAudioInfo();
        //if (!m_fifo && m_outInfo->m_samplesPerFrame != mavfAInfo->m_sampleCnt)
        //{
        //    m_fifo = std::make_shared<HJAudioFifo>(mavfAInfo->m_channels, mavfAInfo->m_sampleFmt, mavfAInfo->m_samplesRate);
        //    res = m_fifo->init(m_outInfo->m_samplesPerFrame);
        //    if (HJ_OK != res) {
        //        HJLoge("audio fifo init error");
        //        break;
        //    }
        //}
        //if (m_fifo)
        //{
        //    res = m_fifo->addFrame(std::move(mavf));
        //    if (HJ_OK != res) {
        //        HJLoge("audio fifo add frame error");
        //        break;
        //    }
        //    while (m_fifo->getSize() > 0) {
        //        mavf = m_fifo->getFrame();
        //        if (!mavf) {
        //            break;
        //        }
        //        //HJLogi("push frame stors, frame info:" + mavf->formatInfo() + ", frame storage:" + HJ2STR(m_frameStors->size() + 1));
        //        m_frameStors->push(std::move(mavf));
        //    }
        //}
        //else {
        //    //HJLogi("push frame stors, frame info:" + mavf->formatInfo() + ", frame storage:" + HJ2STR(m_frameStors->size() + 1));
        //    m_frameStors->push(std::move(mavf));
        //}
    } while (false);

    return res;
}

void HJARenderMini::setVolume(const float volume)
{
    HJARenderBase::setVolume(volume);
#if defined(HJ_HAVE_ARENDER_MINI)
    ma_result ret = ma_device_set_master_volume(m_adev, m_volume);
    if (MA_SUCCESS != ret) {
        HJFLoge("error, ma device set volume failed, ret:{}", ret);
    }
#endif
    return;
}

NS_HJ_END
