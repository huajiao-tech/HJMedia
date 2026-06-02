#include "HJComMuxer.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"
#include "HJFFMuxer.h"

NS_HJ_BEGIN

HJComMuxer::HJComMuxer()
{
    HJ_SetInsName(HJComMuxer);      
    setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO);
}
HJComMuxer::~HJComMuxer()
{
}

int HJComMuxer::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        async([this, i_param]()
        {
            int i_err = 0;
            do
            {
                HJFLogi("{} init enter", m_insName);
                m_muxer = std::make_shared<HJFFMuxer>();
                if (!m_muxer)
                {
                    i_err = -1;
                    break;
                }                
                std::string url = "";
                if (i_param->contains("localRecordUrl"))
                {
                    url = i_param->getValue<std::string>("localRecordUrl");
                }    
                
                i_err = m_muxer->init(url, HJMEDIA_TYPE_AV);
                HJFLogi("{} muxer init ret:{} ", m_insName, i_err); 
                if (i_err < 0)
                {
                    break;
                }
            } while (false);
            return i_err; 
        });

    } while (false);
    return i_err;
}

int HJComMuxer::doSend(HJComMediaFrame::Ptr i_frame)
{
    int i_err = 0;
    do
    {
        if (i_frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
        {
            HJMediaFrame::Ptr iFrame = i_frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));
            if (iFrame->isVideo() && iFrame->isKeyFrame())
            {
                m_state = HJComMuxerState_Ready;
            }

            if (m_state == HJComMuxerState_Ready)
            {
                if (iFrame->isVideo())
                {
                    if (m_firstVideoTime == INT64_MIN)
                    {
                        m_firstVideoTime = iFrame->getDTS();
                    }
                }
                else if (iFrame->isAudio())
                {
                    if (m_firstAudioTime == INT64_MIN)
                    {
                        m_firstAudioTime = iFrame->getDTS();
                    }
                }
                if ((m_firstVideoTime != INT64_MIN) && (m_firstAudioTime != INT64_MIN))
                {
                    m_startTime = (std::min)(m_firstVideoTime, m_firstAudioTime);
                    m_state = HJComMuxerState_Run;
                    HJFLogi("{} muxer state to run startTime:{} ", m_insName, m_startTime); 
                }
            }

            if (m_state >= HJComMuxerState_Ready)
            {
                //HJFLogi("{} muxer pushback ", m_insName); 
                m_mediaFrameDeque.push_back(i_frame);
                HJBaseComAsyncThread::signal();
            }
        }

    } while (false);
    return i_err;
}
int HJComMuxer::run()
{
    int i_err = 0;
    do
    {
        if (!m_muxer)
        {
            return HJ_WOULD_BLOCK;
        }    
        if (m_state != HJComMuxerState_Run)
        {
            return HJ_WOULD_BLOCK;
        }

        while (!m_mediaFrameDeque.isEmpty())
        {
            HJComMediaFrame::Ptr frame = m_mediaFrameDeque.pop();
            if (frame)
            {
                if (frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
                {
                    HJMediaFrame::Ptr iFrame = frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));

                    //deepdup frame, RTMP and record must not be conflict, modify pts and dts; 
                    HJMediaFrame::Ptr muxFrame = iFrame->deepDup();
                    if (muxFrame->m_timeBase.isMatch(HJ_TIMEBASE_MS))
                    {
                        muxFrame->setDTS(muxFrame->getDTS() - m_startTime);
                        muxFrame->setPTS(muxFrame->getPTS() - m_startTime);
                        AVPacket *pkt = (AVPacket *)muxFrame->getAVPacket();
                        if (!pkt)
                        {
                            return HJErrInvalidParams;
                        }
                        pkt->dts -= m_startTime;
                        pkt->pts -= m_startTime;
                    }
                    else
                    {
                        i_err = -1;
                        HJFLoge("timebase is not ms, not support, fixme after support");
                        break;
                    }

                    //HJFLogi("muxer addFrame {} type:{} pts:{}", m_insName, iFrame->getType() == HJMEDIA_TYPE_VIDEO ? "video":"audio", iFrame->getPTS());
                    i_err = m_muxer->writeFrame(muxFrame);
                    // HJFLogi("{} doneAllCom addFrame i_err:{}", m_insName, i_err);
                    if (i_err < 0)
                    {
                        break;
                    }
                }
            }
        }
        
    } while (false);
    //HJFLogi("{} doneAllCom run end i_err:{}", m_insName, i_err);
    return i_err;
}

void HJComMuxer::done()
{
    HJFLogi("{} HJComMuxer done done enter", m_insName);
    
    if (m_muxer)
    {
        HJFLogi("{}HJComMuxer done muxer done enter", m_insName);
        m_muxer->done();
        HJFLogi("{} HJComMuxer done muxer done end", m_insName);
        m_muxer = nullptr;
    }
    
//    sync([this]()
//    {
//        HJFLogi("{} HJComMuxer sync done enter", m_insName);
//        if (m_muxer)
//        {
//            HJFLogi("{}HJComMuxer done muxer done enter", m_insName);
//            m_muxer->done();
//            HJFLogi("{} HJComMuxer done muxer done end", m_insName);
//            m_muxer = nullptr;
//        }
//        return 0;
//    });
    HJFLogi("{} HJComMuxer done muxer HJBaseComAsyncThread done enter", m_insName);
    HJBaseComAsyncThread::done();
    HJFLogi("{} HJComMuxer done  muxer HJBaseComAsyncThread done end", m_insName);
}

NS_HJ_END