#include "HJComAudioCapture.h"
#include "HJLog.h"
#include "HJSPBuffer.h"
#include "capture/HJBaseCapture.h"
#include <sys/socket.h>
#include "HJFLog.h"
NS_HJ_BEGIN

HJComAudioCapture::HJComAudioCapture()
{
    HJ_SetInsName(HJComAudioCapture);      
    setFilterType(HJCOM_FILTER_TYPE_AUDIO);
}
HJComAudioCapture::~HJComAudioCapture()
{
}

int HJComAudioCapture::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseComAsyncThread::init(i_param);
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
        async([this]()
        {
            int i_err = 0;
            do
            {
                m_capture = std::dynamic_pointer_cast<HJACaptureOH>(HJBaseCapture::createCapture(HJMEDIA_TYPE_AUDIO, HJCAPTURE_TYPE_OH));
                if (!m_capture)
                {
                    i_err = -1;
                    break;
                }    
                i_err = m_capture->init(m_audioInfo);
                if (i_err < 0)
                {
                    break;
                }
            } while(false);
            return i_err; 
        });

        //
        //        m_audioCapture = HOAudioCapture::Create();
        //		i_err = m_audioCapture->open(m_audioInfo, [this](unsigned char* i_data, int nSize, int64_t i_timeStamp, int i_nFlag)
        //			{
        //				SLComMediaFrame::Ptr frame = SLComMediaFrame::Create();
        //
        //                int nFlag = 0;
        //                nFlag |= SL::MEDIA_AUDIO;
        //
        //
        //                frame->SetPts(i_timeStamp);
        //                frame->SetDts(i_timeStamp);
        //                frame->SetBuffer(SL::HJSPBuffer::create(nSize, i_data));
        //
        //                frame->SetFlag(nFlag);
        //
        //                int ret = send(frame, SEND_TO_OWN);
        //                if (ret < 0)
        //                {
        //                    JFLoge("send error ret:{}", ret);
        //                }
        //			});

    } while (false);
    return i_err;
}

void HJComAudioCapture::setMute(bool i_bMute)
{
    if (m_capture)
    {
        m_capture->setMute(i_bMute);
    }    
}
int HJComAudioCapture::run()
{
    int i_err = 0;
    do
    {
        if (m_capture)
        {
            HJMediaFrame::Ptr hjFrame = nullptr;
            i_err = m_capture->getFrame(hjFrame);
            if (i_err == HJ_WOULD_BLOCK)
            {
                setTimeout(10);
            }
            else if (i_err == HJ_OK)
            {
                HJComMediaFrame::Ptr frame = HJComMediaFrame::Create();
                frame->SetFlag(MEDIA_AUDIO);
                (*frame)[HJ_CatchName(HJMediaFrame::Ptr)] = hjFrame;
                i_err = send(frame);
                //HJFLogi("{} send frame", i_err);
                if (i_err < 0)
                {
                    HJFLoge("send error ret:{}", i_err);
                }

                setTimeout(0);
            }
            else if (i_err < 0)
            {
                HJFLoge("{} error", m_insName);
            }
        }
    } while (false);
    return i_err;
}
void HJComAudioCapture::done()
{
    HJBaseComAsyncThread::done();
}

NS_HJ_END