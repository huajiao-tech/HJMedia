#include "HJGraphComPusher.h"
#include "HJFLog.h"
//#include "HJFFHeaders.h"
#include "HJComVideoEnc.h"
#include "HJComInterleave.h"
#include "HJComAudioCapture.h"
#include "HJComAudioFrameCombine.h"
#include "HJComAudioEnc.h"
#include "HJComRTMP.h"
#include "HJComMuxer.h"

#define USE_INTERLEAVE 1

NS_HJ_BEGIN

HJGraphComPusher::HJGraphComPusher()
{
    HJ_SetInsName(HJGraphComPusher);
}
HJGraphComPusher::~HJGraphComPusher()
{
    HJFLogi("{} ~HJGraphComPusher enter", m_insName);
}
int HJGraphComPusher::priConnect()
{
    int i_err = 0;
    do
    {
//
#if USE_INTERLEAVE
        ComConnect(m_videoEnc, m_interleave)
#else
        ComConnect(m_videoEnc, m_rtmp)
#endif
        ComConnect(m_audioCapture, m_audioFrameCombine)
        ComConnect(m_audioFrameCombine, m_audioEnc)

#if USE_INTERLEAVE
        ComConnect(m_audioEnc, m_interleave)
        // only use once
        ComConnect(m_interleave, m_rtmp)
#else
        ComConnect(m_audioEnc, m_rtmp)
#endif

    } while (false);
    return i_err;
}

void HJGraphComPusher::setMute(bool i_mute)
{
    // thread safe must be protected
    async([this, i_mute]()
    {
        if (m_audioCapture)
        {
            m_audioCapture->setMute(i_mute);
        }
        return 0;
    });
}
int HJGraphComPusher::openRecorder(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        sync([this, i_param]()
        { 
            int i_err = 0;
            do 
            {
                HJComMuxer::Ptr muxer = HJComMuxer::Create();
                //muxer->setInsName("HJComMuxer");
                //muxer->setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO);
                muxer->setNotify(nullptr);
                
                ComConnect(m_interleave, muxer);   
                addCom(muxer);
                HJBaseGraphCom::resetComsExecutor();
                
//                HJBuffer::Ptr videoHeader = nullptr;
//                if (m_videoEnc)
//                {
//                     videoHeader = m_videoEnc->getHeader();
//                }    
//                HJBuffer::Ptr audioHeader = nullptr;
//                if (m_audioEnc)
//                {
//                     audioHeader = m_audioEnc->getHeader();
//                }        
//                if (!m_videoEnc || !m_audioEnc)
//                {
//                    HJFLoge("header is not avaliabe, after fixme");
//                    return -1;
//                }    
//                //AVCodecParameters ff header is conflict with FDKAAC header, so after muxer create AVCodecParameters              
//                (*i_param)[HJ_CatchName(videoHeader)] = (HJBuffer::Ptr)videoHeader;
//                (*i_param)[HJ_CatchName(audioHeader)] = (HJBuffer::Ptr)audioHeader;
                i_err = muxer->init(i_param);
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

void HJGraphComPusher::closeRecorder()
{
    sync([this]()
    { 
        int i_err = 0;
        HJComMuxer::Ptr muxer = nullptr;
        for (auto it = m_comQueue.begin(); it != m_comQueue.end(); it++)              
        {
            muxer = std::dynamic_pointer_cast<HJComMuxer>(*it);
            if (muxer)
            {
                break;
            }    
        }
        
        if (muxer)
        {
            HJBaseCom::replace(muxer, nullptr, nullptr);
            removeCom(muxer);
            muxer = nullptr;
            
            HJBaseGraphCom::resetComsExecutor();
        }
        return i_err;
    });
}
void HJGraphComPusher::removeInterleave()
{
    sync([this]()
    {
       if (m_interleave)
       {
            HJFLogi("removeInterleave enter");
            HJBaseCom::replace(m_interleave, nullptr, [this]()
            {
               while (true)
               {
                    HJComMediaFrame::Ptr frame = m_interleave->drain();
                    if (!frame)
                    {
                        break;
                    } 
                    if (m_rtmp)
                    {
                        HJFLogi("removeInterleave rtmp send frame");
                        m_rtmp->send(frame, SEND_TO_OWN);
                    }    
               }
               return 0;
            });
            removeCom(m_interleave);
            m_interleave = nullptr;
            HJFLogi("removeInterleave resetComsExecutor enter");
           
            HJBaseGraphCom::resetComsExecutor();
            
            HJFLogi("removeInterleave resetComsExecutor end");
       }
       return 0;
    });
    HJFLogi("removeInterleave all end");
}
int HJGraphComPusher::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseGraphComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        async([this, i_param]()
        {
            int i_err = 0;
            do
            {
                m_videoEnc = HJComVideoEnc::Create();
                //m_videoEnc->setInsName("HJComVideoEnc");
                //m_videoEnc->setFilterType(HJCOM_FILTER_TYPE_VIDEO);
                m_videoEnc->setNotify(nullptr);
                addCom(m_videoEnc);

                m_audioCapture = HJComAudioCapture::Create();
                //m_audioCapture->setInsName("HJComAudioCapture");
                //m_audioCapture->setFilterType(HJCOM_FILTER_TYPE_AUDIO);
                m_audioCapture->setNotify(nullptr);
                addCom(m_audioCapture);

                m_audioFrameCombine = HJComAudioFrameCombine::Create();
                //m_audioFrameCombine->setInsName("HJComAudioFrameCombine");
                //m_audioFrameCombine->setFilterType(HJCOM_FILTER_TYPE_AUDIO);
                m_audioFrameCombine->setNotify(nullptr);
                addCom(m_audioFrameCombine);

                m_audioEnc = HJComAudioEnc::Create();
                //m_audioEnc->setInsName("HJComAudioEnc");
                //m_audioEnc->setFilterType(HJCOM_FILTER_TYPE_AUDIO);
                m_audioEnc->setNotify(nullptr);
                addCom(m_audioEnc);

                m_interleave = HJComInterleave::Create();
                //m_interleave->setInsName("HJComInterleave");
                //m_interleave->setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJCOM_FILTER_TYPE_AUDIO);
                m_interleave->setNotify(nullptr);
                addCom(m_interleave);

                m_rtmp = HJComRTMP::Create();
                //m_rtmp->setInsName("HJComRTMP");
                //m_rtmp->setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJCOM_FILTER_TYPE_AUDIO);
                m_rtmp->setNotify(nullptr);
                addCom(m_rtmp);

                i_err = priConnect();
                if (i_err < 0)
                {
                    break;
                }

                HJBaseGraphCom::resetComsExecutor();
                
                // first target init, then source init
                i_err = m_rtmp->init(i_param);
                if (i_err < 0)
                {
                    break;
                }

                i_err = m_interleave->init(i_param);
                if (i_err < 0)
                {
                    break;
                }

                i_err = m_videoEnc->init(i_param);
                if (i_err < 0)
                {
                    break;
                }
                i_err = m_audioEnc->init(i_param);
                if (i_err < 0)
                {
                    break;
                }
                i_err = m_audioFrameCombine->init(i_param);
                if (i_err < 0)
                {
                    break;
                }
                i_err = m_audioCapture->init(i_param);
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
void HJGraphComPusher::done()
{
    HJFLogi("{} done enter", m_insName);
    
    sync([this]()
    {
        HJBaseGraphCom::doneAllCom();
        return 0;
    });
    
    HJBaseGraphComAsyncThread::done();
    HJFLogi("{} done end-------------", m_insName);
}
NS_HJ_END