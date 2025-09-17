#include "HJComAudioEncoder.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJComAudioEncoder::HJComAudioEncoder()
{
    HJ_SetInsName(HJComAudioEncoder);  
    setFilterType(HJCOM_FILTER_TYPE_AUDIO);
}
HJComAudioEncoder::~HJComAudioEncoder()
{
    
}

int HJComAudioEncoder::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJBaseComSync::init(i_param);
        if (i_err < 0)
        {
            break;
        }    
        if (i_param->contains(HJ_CatchName(HJAudioInfo::Ptr)))
        {
            m_audioInfo = i_param->getValue<HJAudioInfo::Ptr>(HJ_CatchName(HJAudioInfo::Ptr));
        }
        if (!m_audioInfo)
        {
            i_err = -1;
            break;
        }
        m_encoder = std::make_shared<HJAEncFDKAAC>();
        if (!m_encoder)
        {
            i_err = -1;
            break;
        }    
        
        i_err = m_encoder->init(m_audioInfo);
        if (i_err < 0)
        {
            break;
        }
        
//        if (i_param->contains("HORTMPPusherInfo"))
//        {
//            m_rtmpInfo = i_param->getValue<HORTMPPusherInfo>("HORTMPPusherInfo");
//        }
////        if (i_param->contains("HOAudioCaptureInfo"))
////        {
////            m_audioInfo = i_param->getValue<HOAudioCaptureInfo>("HOAudioCaptureInfo");
////        }
//        
//        m_audioEncoder = HOAudioEncoder::Create();
//
//        HOAudioEncoderInfo encoderInfo;
//        encoderInfo.m_bitrate = m_rtmpInfo.audio.bitrate;
//        encoderInfo.m_channels = m_rtmpInfo.audio.channels;
//        encoderInfo.m_type = "LC";
//        encoderInfo.m_bitspersample = 16;
//        encoderInfo.m_sampleRate = m_rtmpInfo.audio.sample_rate;
//
//        i_err = m_audioEncoder->open(encoderInfo, [this](unsigned char* i_data, int nSize, int64_t i_timeStamp, int i_nFlag)
//        {
//            SLComMediaFrame::Ptr frame = SLComMediaFrame::Create();
//            
//            int nFlag = 0;
//            nFlag |= SL::MEDIA_AUDIO;
//            if (i_nFlag & flag_aac_header)
//            {
//                nFlag |= SL::MEDIA_DATA_HEADER;
//                nFlag |= SL::MEDIA_DATA_KEY; 
//            } 
//            else
//            {
//                nFlag |= SL::MEDIA_DATA_KEY;
//                nFlag |= SL::MEDIA_DATA_NORMAL;
//            }
//            
//            frame->SetPts(i_timeStamp);
//            frame->SetDts(i_timeStamp);
//            frame->SetBuffer(SL::HJSPBuffer::create(nSize, i_data));
//            
//            frame->SetFlag(nFlag);
//            int ret = send(frame);
//            if (ret < 0)
//            {
//                JFLoge("send error ret:{}", ret);
//            }    
//        });	
//
//        if (i_err < 0)
//        {
//            JFLoge("audio encoder error i_err:{}", i_err);
//            break;
//        }        
//    
////        m_audioCapture = HOAudioCapture::Create();
////		i_err = m_audioCapture->open(m_audioInfo, [this](unsigned char* i_data, int nSize, int64_t i_timeStamp, int i_nFlag)
////			{
////				if (m_audioEncoder)
////				{
////                    m_audioEncoder->write(i_data, nSize, i_timeStamp);
////				}
////			});	       
        
    } while (false);
    return i_err;
}

int HJComAudioEncoder::doSend(HJComMediaFrame::Ptr i_frame)
{
    int i_err = 0;
    do 
    {
        if (m_encoder)
        {
            if (i_frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
            {
                HJMediaFrame::Ptr iFrame = i_frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));
                i_err = m_encoder->run(iFrame);
                if (i_err < 0)
                {
                    break;
                }  
            }
            
            while (true)
            {
                HJMediaFrame::Ptr hjFrame = nullptr;
                i_err = m_encoder->getFrame(hjFrame);
                if (i_err != HJ_OK)
                {
                    break;
                }    
                HJComMediaFrame::Ptr frame = HJComMediaFrame::Create();
                frame->SetFlag(MEDIA_AUDIO);
                (*frame)[HJ_CatchName(HJMediaFrame::Ptr)] = hjFrame;
                i_err = send(frame);
                if (i_err < 0)
                {
                    HJFLoge("send error ret:{}", i_err);
                }
            }
        }    
//        if (m_audioEncoder)
//        {
//            i_err = m_audioEncoder->write(i_frame->GetBuffer()->getBuf(), i_frame->GetBuffer()->getSize(), i_frame->GetDts());
//            if (i_err < 9)
//            {
//                break;    
//            }
//        } 
    } while (false);
    return i_err;
}
void HJComAudioEncoder::done() 
{
    HJBaseComSync::done();
}

NS_HJ_END