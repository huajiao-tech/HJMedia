#include "HJComDemuxer.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"
#include "HJFFDemuxerEx.h"
#include "HJComMediaFrameCvt.h"

NS_HJ_BEGIN

int HJComDemuxer::s_maxMediaSize = 200;

HJComDemuxer::HJComDemuxer()
{
    HJ_SetInsName(HJComDemuxer);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO);
}
HJComDemuxer::~HJComDemuxer()
{
}
void HJComDemuxer::start()
{
    m_bStart = true;
}
int HJComDemuxer::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        
        m_baseDemuxer = std::make_shared<HJFFDemuxerEx>();
        if (!m_baseDemuxer)
        {
            i_err = -1;
            break;
        }

        m_threadPool = HJThreadPool::Create();
        m_threadPool->setBeginFunc([this, i_param]()
        {
            int i_err = 0;
            do
            {
                HJMediaUrl::Ptr mediaUrl = nullptr;
                if (i_param)
                {
                    mediaUrl = i_param->getValue<HJMediaUrl::Ptr>(HJ_CatchName(HJMediaUrl::Ptr));
                }
                if (!mediaUrl)
                {
                    i_err = -1;
                    break;
                }

                //HJ_CvtBaseToDerived(HJFFDemuxerEx, m_baseDemuxer);
                i_err = m_baseDemuxer->init(mediaUrl);
                if (i_err < 0)
                {
                    break;
                }

                if (m_notify)
                {
                    HJBaseNotifyInfo::Ptr info = HJBaseNotifyInfo::Create(HJCOMPLAYER_DEMUX_READY);
                    (*info)[HJ_CatchName(HJMediaInfo::Ptr)] = (HJMediaInfo::Ptr)m_baseDemuxer->getMediaInfo();
                    m_notify(info);
                }
            } while (false);
            return i_err;
        });
        m_threadPool->setRunFunc([this]() 
        {
            int i_err = 0;
            do
            {
                if (!m_bStart)
                {
                    break;
                }
                if (m_bEof)
                {
                    break;
                }

                if (isMediaQueueFull())
                {
                    break;
                }

                HJMediaFrame::Ptr kframe;
                int i_err = m_baseDemuxer->getFrame(kframe);
                if (i_err < 0)
                {
                    break;
                }
                if (i_err == HJ_EOF)
                {
                    HJMediaFrame::Ptr vidFrame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_VIDEO);
                    HJComMediaFrame::Ptr vFrame = HJComMediaFrameCvt::getComFrame(MEDIA_VIDEO, vidFrame);
                    send(vFrame, SEND_TO_OWN);
                    
                    HJMediaFrame::Ptr audFrame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
                    HJComMediaFrame::Ptr aFrame = HJComMediaFrameCvt::getComFrame(MEDIA_AUDIO, audFrame);
                    send(aFrame, SEND_TO_OWN);                    
                    
                    m_bEof = true;
                }
                if (i_err == HJ_OK)
                {
                    int flag = kframe->isVideo() ? MEDIA_VIDEO : (kframe->isAudio() ? MEDIA_AUDIO : 0);
                    HJComMediaFrame::Ptr frame = HJComMediaFrameCvt::getComFrame(flag, kframe);             
                    i_err = send(frame, SEND_TO_OWN);
                    
//                    if (kframe->isVideo())
//                    {
//                        AVPacket* pk = kframe->getAVPacket();
//                        HJFLogi("testIsMatch demxuer pts:{}, dts:{} size:{} isKey:{} mediaSize:{} maxMediaSize:{}", kframe->getPTS(),kframe->getDTS(), pk->size, kframe->isKeyFrame(), getMediaQueueSize(), getMediaQueueMaxSize());
//                    }
                    
                    m_threadPool->setTimeout(0);
                }

            } while (false);
            return i_err;
        });
        m_threadPool->setEndFunc([this]()
        {
            return 0;
        });
        i_err = m_threadPool->start();
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

int HJComDemuxer::run()
{
    int i_err = 0;
    do
    {
        if (!m_bStart)
        {
            break;
        }

        if (isNextFull(getSharedFrom(this)))
        {
            break;
        }

        if (m_mediaFrameDeque.isEmpty())
        {
            break;
        }

        HJComMediaFrame::Ptr frame = m_mediaFrameDeque.pop();
        i_err = send(frame);
        if (i_err < 0)
        {
            break;
        }
        setTimeout(0);

    } while (false);
    return i_err;
}
void HJComDemuxer::join() 
{
    if (m_baseDemuxer)
    {
        m_baseDemuxer->setQuit(true);
        m_baseDemuxer->done();
    }
    HJBaseComAsyncThread::join();
}
void HJComDemuxer::done()
{
    HJFLogi("{} HJComDemuxer done enter", m_insName);
    HJBaseComAsyncThread::done();
    HJFLogi("{} HJComDemuxer done end", m_insName);
}

NS_HJ_END