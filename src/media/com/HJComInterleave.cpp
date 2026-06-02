#include "HJComInterleave.h"
#include "HJFLog.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN

HJComInterleave::HJComInterleave()
{
    HJ_SetInsName(HJComInterleave);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJCOM_FILTER_TYPE_AUDIO);
}
HJComInterleave::~HJComInterleave()
{
    
}

int HJComInterleave::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJBaseComSync::init(i_param);
        if (i_err < 0)
        {
            break;
        }   
    } while (false);
    return i_err;
}

int HJComInterleave::doSend(HJComMediaFrame::Ptr i_frame)
{
    int i_err = 0;
    do 
    {
        HJ_LOCK(m_inter_mutex);
        
        if (i_frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
        {
            HJMediaFrame::Ptr frame = i_frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));

            i_frame->SetDts(frame->getDTS());
            i_frame->SetPts(frame->getPTS());

            if (frame->isVideo())
            {
                m_videoQueue.push_back(i_frame);  
                if (m_firstVideoTime == INT64_MIN)
                {
                    m_firstVideoTime = frame->getDTS();
                }    
            }
            else if (frame->isAudio())
			{
			    m_audioQueue.push_back(i_frame);
                if (m_firstAudioTime == INT64_MIN)
                {
                    m_firstAudioTime = frame->getDTS();
                }
            }
		}

		if (!m_bStarted)
		{
			if ((m_firstVideoTime != INT64_MIN) && (m_firstAudioTime != INT64_MIN))
			{
				// m_startTime = std::min(m_firstVideoTime, m_firstAudioTime);
				// HJFLogi("{} first startTime:{}", m_insName, m_startTime);
                HJFLogi("{} first startTime:", m_insName);
				m_bStarted = true;
			}
		}
		if (!m_bStarted)
		{
			break;
		}

        priTryDrain();
        
    } while (false);
    return i_err;
}

int HJComInterleave::priTryDrain()
{
    int i_err = 0;
    do 
    {
        if (!m_audioQueue.isEmpty() && !m_videoQueue.isEmpty())
        {
            int64_t curTime = m_videoQueue.getFirst()->GetDts();
            i_err = priTryDrainDetail(m_audioQueue, curTime);
            if (i_err < 0)
            {
                break;
            }
            if (!m_audioQueue.isEmpty())
            {
                curTime = m_audioQueue.getFirst()->GetDts();
                i_err = priTryDrainDetail(m_videoQueue, curTime);
                if (i_err < 0)
                {
                    break;    
                }
            }
        }
    } while (false);
    return i_err;
}

int HJComInterleave::priTryDrainDetail(HJDataDequeNoLock<HJComMediaFrame::Ptr> &i_refDeque, int64_t i_curTime)
{
    int i_err = 0;
	while (!i_refDeque.isEmpty() && (i_refDeque.getFirst()->GetDts() <= i_curTime))
	{
		HJComMediaFrame::Ptr frame = i_refDeque.pop();
        // frame->SetDts(frame->GetDts() - m_startTime);
        // frame->SetPts(frame->GetPts() - m_startTime);
        //HJFLogi("HJComInterleave frame:{} pts:{} dts:{}", frame->IsVideo()?"video":"audio", frame->GetPts(), frame->GetDts());
        i_err = send(frame);
        if (i_err < 0)
        {
            HJFLoge("{} send error", m_insName);
            break;    
        }
	}  
    return i_err;
}
void HJComInterleave::done() 
{
    HJBaseComSync::done();
}

HJComMediaFrame::Ptr HJComInterleave::drain()
{
    if (!m_audioQueue.isEmpty())
    {   
        return m_audioQueue.pop();
    }    
    else if (!m_videoQueue.isEmpty())
    {
        return m_videoQueue.pop();
    }        
    return nullptr;
}


NS_HJ_END