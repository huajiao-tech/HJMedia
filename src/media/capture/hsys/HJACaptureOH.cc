#include "HJACaptureOH.h"
#include "HJFLog.h"
#include "HJTime.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN

HJACaptureOH::HJACaptureOH()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJACaptureOH)));
	m_timeBase = HJ_TIMEBASE_MS;
}
HJACaptureOH::~HJACaptureOH()
{
    if (m_capturer)
    {
        OH_AudioCapturer_Stop(m_capturer);
        OH_AudioCapturer_Release(m_capturer);
        m_capturer = nullptr;
    }
        
    if (m_builder != nullptr)
    {
        OH_AudioStreamBuilder_Destroy(m_builder);
        m_builder = nullptr;
    }
    m_outputQueue.clear();
}

int HJACaptureOH::OnReadData(unsigned  char *data, int data_size) 
{
    int i_err = HJ_OK;
    do
    {
        if (m_bMute)
        {
            if (data)
            {
                memset(data, 0, data_size);
            }    
        }
        
        HJAudioInfo::Ptr info = std::make_shared<HJAudioInfo>();
	    info->clone(m_info);
        info->m_sampleFmt = (int)AV_SAMPLE_FMT_S16;


        HJMediaFrame::Ptr frame = HJMediaFrame::makeMediaFrameAsAVFrame(info, data, data_size, true, HJCurrentSteadyMS(), m_timeBase);
        if (frame)
        {
            m_outputQueue.push_back_move(frame);

            if (m_newBufferCb) {
                m_newBufferCb();
            }
        }
        else
        {
            i_err = HJErrNotSupport;
            HJFLoge("not support");
            break;
        }
    } while (false);
    return i_err;
}
int32_t HJACaptureOH::OnReadDataStatic(OH_AudioCapturer *capturer, 
                                       void *userData, 
                                       void *buffer, 
                                       int32_t bufferLen) {
    if (userData == nullptr) {
        return -1;
    }

    HJACaptureOH* instance = static_cast<HJACaptureOH*>(userData);
    if (instance) 
    {
        return instance->OnReadData((unsigned char *)buffer, bufferLen);
    }
    return -1;
} 
void HJACaptureOH::setMute(bool i_bMute)
{
    m_bMute = i_bMute;
}
int HJACaptureOH::init(const HJStreamInfo::Ptr& info)
{
    int i_err = HJ_OK;
    do 
    {
        HJFLogi("init enter");
        i_err = HJBaseCapture::init(info);
	    if (HJ_OK != i_err)
	    {
		    break;
	    }
        
        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);
        HJFLogi("init begin channels:{} samplerate:{}", audioInfo->getChannels(), audioInfo->getChannels());
        i_err = OH_AudioStreamBuilder_Create(&m_builder, AUDIOSTREAM_TYPE_CAPTURER);
        if (i_err != AUDIOSTREAM_SUCCESS) {
            HJFLoge("create error");
            i_err = HJErrCodecCreate;
            break;
        }
        i_err = OH_AudioStreamBuilder_SetSamplingRate(m_builder, audioInfo->getSampleRate());
        if (i_err != AUDIOSTREAM_SUCCESS)
        {
            HJFLoge("SetSamplingRate error");
            i_err = HJErrParseData;
            break;
        }    
        i_err = OH_AudioStreamBuilder_SetChannelCount(m_builder, audioInfo->getChannels());
        if (i_err != AUDIOSTREAM_SUCCESS) 
        {
            HJFLoge("SetChannelCount error");
            break;
        }    
        i_err = OH_AudioStreamBuilder_SetSampleFormat(m_builder, AUDIOSTREAM_SAMPLE_S16LE);
        if (i_err != AUDIOSTREAM_SUCCESS)
        {    
            HJFLoge("SetSampleFormat error");
            break;
        }
        i_err = OH_AudioStreamBuilder_SetEncodingType(m_builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
        if (i_err != AUDIOSTREAM_SUCCESS)
        {    
            HJFLoge("SetEncodingType error");
            break;
        }
        i_err = OH_AudioStreamBuilder_SetCapturerInfo(m_builder, AUDIOSTREAM_SOURCE_TYPE_MIC);
        if (i_err != AUDIOSTREAM_SUCCESS)
        {    
            HJFLoge("SetCapturerInfo error");
            break;
        }
        
        i_err = OH_AudioStreamBuilder_SetLatencyMode(m_builder, m_lowDelay ? AUDIOSTREAM_LATENCY_MODE_FAST : AUDIOSTREAM_LATENCY_MODE_NORMAL); // 设置低延迟模式
        if (i_err != AUDIOSTREAM_SUCCESS)
        {    
            HJFLoge("SetLatencyMode error"); 
            break;
        }
    
        OH_AudioCapturer_Callbacks callbacks;
        callbacks.OH_AudioCapturer_OnReadData = &HJACaptureOH::OnReadDataStatic;
        callbacks.OH_AudioCapturer_OnStreamEvent = nullptr;
        callbacks.OH_AudioCapturer_OnInterruptEvent = nullptr;
        callbacks.OH_AudioCapturer_OnError = nullptr;
    
        i_err = OH_AudioStreamBuilder_SetCapturerCallback(m_builder, callbacks, this);
        if (i_err != AUDIOSTREAM_SUCCESS) 
        {    
            HJFLoge("SetCapturerCallback error");  
            break;
        }

        i_err = OH_AudioStreamBuilder_GenerateCapturer(m_builder, &m_capturer);
        if (i_err != AUDIOSTREAM_SUCCESS) 
        {    
            HJFLoge("GenerateCapture error");  
            break;
        }
        
        i_err = OH_AudioCapturer_Start(m_capturer);
        if (i_err != AUDIOSTREAM_SUCCESS) 
        {    
            HJFLoge("OH_AudioCapturer_Start error");   
            break;
        }

        m_newBufferCb = info->getValue<HJRunnable>("newBufferCb");
    } while (false);
    HJFLogi("init end ret:{}", i_err);
    return i_err;    
}
    
int HJACaptureOH::getFrame(HJMediaFrame::Ptr& frame)
{
    int i_err = HJ_OK;
    do 
    {
        if (m_outputQueue.isEmpty())
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
        
        frame = m_outputQueue.pop();

    } while (false);
    return i_err;
}

NS_HJ_END