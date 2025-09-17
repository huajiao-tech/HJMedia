//***********************************************************************************//
// HJMedia FRAMEWORK SOURCE;
// AUTHOR:
// CREATE TIME:
//***********************************************************************************//
#include "HJAEncFDKAAC.h"
#include "HJFLog.h"
// #include "HJFFUtils.h"

NS_HJ_BEGIN

int HJAEncFDKAAC::s_bytesPerSample = 2;

HJAEncFDKAAC::HJAEncFDKAAC()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJAEncFDKAAC)));

    m_timeBase = HJ_TIMEBASE_MS;

    HJMediaFrame::setCodecIdAAC(m_codecID); // m_codecID = AV_CODEC_ID_AAC;
}
HJAEncFDKAAC::~HJAEncFDKAAC()
{
    if (m_aacHandle)
    {
        aacEncClose(&m_aacHandle);
        m_aacHandle = nullptr;
    }
}

int HJAEncFDKAAC::init(const HJStreamInfo::Ptr &info)
{
    int i_err = HJ_OK;
    do
    {
        HJFNLogi("init begin");
        i_err = HJBaseCodec::init(info);
        if (HJ_OK != i_err)
        {
            HJFNLoge("init error");
            return i_err;
        }
        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);
        if (!HJMediaFrame::isCodecMatchAAC(audioInfo->m_codecID))
        {
            HJFNLoge("isCodecMatchAAC error");
            i_err = HJErrInvalidParams;
            break;
        }
        HJFNLogi("init begin channels:{} samplerate:{}", audioInfo->getChannels(), audioInfo->getSampleRate());
        i_err = aacEncOpen(&m_aacHandle, 0, audioInfo->getChannels());
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncOpen error:{}", i_err);
            i_err = HJErrCodecCreate;
            break;
        }
        AUDIO_OBJECT_TYPE type = AOT_AAC_LC;

        m_samplePerFrame = (type == AOT_AAC_LC) ? 1024 : 2048;

        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_AOT, type);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam aot error:{}", i_err);
            i_err = HJErrCodecInit;
            break;
        }
        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_SAMPLERATE, audioInfo->getSampleRate());
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam samplerate error:{}", i_err);
            i_err = HJErrCodecInit;
            break;
        }

        CHANNEL_MODE mode;
        switch (audioInfo->getChannels())
        {
        case 1:
            mode = MODE_1;
            break;
        case 2:
            mode = MODE_2;
            break;
        case 3:
            mode = MODE_1_2;
            break;
        case 4:
            mode = MODE_1_2_1;
            break;
        case 5:
            mode = MODE_1_2_2;
            break;
        case 6:
            mode = MODE_1_2_2_1;
            break;
        default:
            HJFNLoge("channel is not support:{}", audioInfo->getChannels());
            i_err = HJErrCodecInit;
            break;
        }

        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_CHANNELMODE, mode);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam channelmode error:{}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }
        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_CHANNELORDER, 0);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam AACENC_CHANNELORDER error {}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }
        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_BITRATEMODE, 0);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam AACENC_BITRATEMODE error {}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }
        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_BITRATE, audioInfo->getBitrate());
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam AACENC_BITRATE error {}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }
        
        //- 0: raw access units
        //- 1: ADIF bitstream format
        //- 2: ADTS bitstream format
        UINT tansMuxVal = m_bADTS ? 2 : 0;
         HJFNLogi("aac use adts {}", m_bADTS);
        i_err = aacEncoder_SetParam(m_aacHandle, AACENC_TRANSMUX, tansMuxVal);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncoder_SetParam AACENC_TRANSMUX error {}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }

        i_err = aacEncEncode(m_aacHandle, NULL, NULL, NULL, NULL);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncEncode error {}", i_err);
            i_err = HJErrCodecInit;
            break;
        }

        AACENC_InfoStruct info;
        memset(&info, 0, sizeof(info));
        i_err = aacEncInfo(m_aacHandle, &info);
        if (i_err != AACENC_OK)
        {
            HJFNLoge("aacEncInfo error {}", i_err);
            i_err = HJErrCodecInit;
            break;
        }
        if (0 == info.confSize)
        {
            HJFNLoge("info.confSize error {}");
            i_err = HJErrCodecInit;
            break;
        }
        
        for (int i = 0; i < info.confSize; i++)
        {
            HJFNLogi("header i:{} size:{} value:{}", i, info.confSize, info.confBuf[i]);
        }    
        
        m_headerBuffer = std::make_shared<HJBuffer>(info.confBuf, info.confSize);
        m_keyCodecParams = HJMediaFrame::makeHJAVCodecParam(m_info, m_headerBuffer);

        m_inElemSize = s_bytesPerSample;

        m_outSize = sizeof(m_outBuffer);
        m_outElemSize = sizeof(unsigned char);

    } while (false);
    HJFNLogi("init end ret:{}", i_err);
    return i_err;
}

int HJAEncFDKAAC::getFrame(HJMediaFrame::Ptr &frame)
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
int HJAEncFDKAAC::run(const HJMediaFrame::Ptr i_frame)
{
    int i_err = HJ_OK;
    do
    {
        AACENC_BufDesc inBuf{0};
        AACENC_BufDesc outBuf{0};
        AACENC_InArgs inArgs{0};
        AACENC_OutArgs outArgs{0};

        HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_info);

        unsigned char *data = nullptr;
        i_err = HJMediaFrame::getDataFromAVFrame(i_frame, data, m_inSize);
        if (i_err < 0)
        {
            HJFNLoge("getDataFromAVFrame error {}", i_err);
            break;
        }

        // m_inSize = i_nSize;
        inBuf.bufferIdentifiers = &m_inIdentifier;
        inBuf.bufSizes = &m_inSize;
        inBuf.bufElSizes = &m_inElemSize;

        inBuf.bufs = (void **)&data;
        inBuf.numBufs = 1;

        void *out_ptr = m_outBuffer;
        outBuf.bufferIdentifiers = &m_outIdentifier;
        outBuf.bufSizes = &m_outSize;
        outBuf.bufElSizes = &m_outElemSize;
        outBuf.bufs = &out_ptr;
        outBuf.numBufs = 1;

        inArgs.numInSamples = m_inSize / m_inElemSize; // multiple of input channels

        int64_t t0 = HJCurrentSteadyMS();
        i_err = aacEncEncode(m_aacHandle, &inBuf, &outBuf, &inArgs, &outArgs);
        if (i_err == AACENC_OK)
        {
            m_statIdx++;
            int64_t diff = HJCurrentSteadyMS() - t0;
            m_statSum += diff;
            HJFNLogd("aac Encoder time idx:{} curDiff:{} avgDiff:{}", m_statIdx, diff, (m_statSum / m_statIdx));
            HJAudioInfo::Ptr info = std::make_shared<HJAudioInfo>();
            info->clone(m_info);
            info->setSampleCnt(m_samplePerFrame);
            info->setCodecParams(m_keyCodecParams);            

            HJMediaFrame::Ptr frame = HJMediaFrame::makeMediaFrameAsAVPacket(info, m_outBuffer, outArgs.numOutBytes, true, i_frame->getPTS(), i_frame->getDTS(), m_timeBase);
            m_outputQueue.push_back_move(frame);
        }
        //fixme after proc
        else if (i_err == AACENC_ENCODE_EOF)
        {
            HJFNLogi("eof");
            break;
        }
        else
        {
            i_err = HJErrCodecEncode;
            break;
        }
    } while (false);
    return i_err;
}

NS_HJ_END
