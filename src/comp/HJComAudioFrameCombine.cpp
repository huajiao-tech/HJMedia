#include "HJComAudioFrameCombine.h"
#include "HJLog.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJComAudioFrameCombine::HJComAudioFrameCombine()
{
    HJ_SetInsName(HJComAudioFrameCombine);   
    setFilterType(HJCOM_FILTER_TYPE_AUDIO);
}
HJComAudioFrameCombine::~HJComAudioFrameCombine()
{
    
}

int HJComAudioFrameCombine::init(HJBaseParam::Ptr i_param)
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
        
        m_audiofifo = std::make_shared<HJAudioFifo>(m_audioInfo->m_channels, m_audioInfo->m_sampleFmt, m_audioInfo->m_samplesRate);
        if (!m_audiofifo)
        {
            i_err = -1;
            break;
        }    
        i_err = m_audiofifo->init(1024);
        if (i_err < 0)
        {
            HJFLoge("audio fifo init error i_err:{}", i_err);
            return i_err;
        }
        
    } while (false);
    return i_err;
}

int HJComAudioFrameCombine::doSend(HJComMediaFrame::Ptr i_frame)
{
    int i_err = 0;
    do 
    {
        if (m_audiofifo)
        {
            if (i_frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
            {
                HJMediaFrame::Ptr iFrame = i_frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));
                iFrame->setAVTimeBase(HJTimeBase{1, m_audioInfo->m_samplesRate});
                i_err = m_audiofifo->addFrame(std::move(iFrame));
                if (i_err < 0)
                {
                    break;
                }  
            }
            HJMediaFrame::Ptr hjFrame = m_audiofifo->getFrame();
            if (hjFrame)
            {
                HJComMediaFrame::Ptr frame = HJComMediaFrame::Create();
                frame->SetFlag(MEDIA_AUDIO);
                (*frame)[HJ_CatchName(HJMediaFrame::Ptr)] = hjFrame;
                i_err = send(frame);
                if (i_err < 0)
                {
                    break;
                }    
            }
        }    
    } while (false);
    return i_err;
}

void HJComAudioFrameCombine::done() 
{
    HJBaseComSync::done();
}

NS_HJ_END