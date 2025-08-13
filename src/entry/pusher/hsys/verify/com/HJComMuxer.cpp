#include "HJComMuxer.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

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
//                HJMediaInfo::Ptr mediaInfo = std::make_shared<HJMediaInfo>();//HJMEDIA_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO);
//                HJAudioInfo::Ptr audioPtr = nullptr;
//                HJVideoInfo::Ptr videoPtr = nullptr;
//                if (i_param->contains(HJ_CatchName(HJAudioInfo::Ptr)))
//                {
//                    audioPtr = i_param->getValue<HJAudioInfo::Ptr>(HJ_CatchName(HJAudioInfo::Ptr));
//                    
//                    if (i_param->contains(HJ_CatchName(audioHeader)))
//                    {
//                        HJBuffer::Ptr audioHeader = i_param->getValue<HJBuffer::Ptr>(HJ_CatchName(audioHeader));
//                        AVCodecParameters *codecParams = avcodec_parameters_alloc();  
//                        
//                        codecParams->codec_type = AVMEDIA_TYPE_AUDIO;
//                        codecParams->codec_id = (AVCodecID)audioPtr->getCodecID();
//                        
//                        codecParams->sample_rate = audioPtr->getSampleRate();
//                        codecParams->ch_layout = *audioPtr->getAVChannelLayout();
// 
//                        codecParams->extradata = (uint8_t *)av_mallocz(audioHeader->size() + AV_INPUT_BUFFER_PADDING_SIZE);
//                        memcpy(codecParams->extradata, audioHeader->data(), audioHeader->size());
//                        codecParams->extradata_size = (int)audioHeader->size();
//                        audioPtr->setAVCodecParams(codecParams);
//                    }    
//                    mediaInfo->addStreamInfo(audioPtr);
//                }
//                if (i_param->contains(HJ_CatchName(HJVideoInfo::Ptr)))
//                {
//                    videoPtr = i_param->getValue<HJVideoInfo::Ptr>(HJ_CatchName(HJVideoInfo::Ptr));
//                    if (i_param->contains(HJ_CatchName(videoHeader)))
//                    {
//                        HJBuffer::Ptr videoHeader = i_param->getValue<HJBuffer::Ptr>(HJ_CatchName(videoHeader));
//                        AVCodecParameters *codecParams = avcodec_parameters_alloc();
//                                                
//                        codecParams->codec_type = AVMEDIA_TYPE_VIDEO;
//                        codecParams->codec_id = (AVCodecID)videoPtr->getCodecID();
//                        
//                        codecParams->width = videoPtr->m_width;
//                        codecParams->height = videoPtr->m_height;
//
//                        codecParams->extradata = (uint8_t *)av_mallocz(videoHeader->size() + AV_INPUT_BUFFER_PADDING_SIZE);
//                        memcpy(codecParams->extradata, videoHeader->data(), videoHeader->size());
//                        codecParams->extradata_size = (int)videoHeader->size();
//                        videoPtr->setAVCodecParams(codecParams);
//                    } 
//                    mediaInfo->addStreamInfo(videoPtr);
//                }
//                if (i_param->contains(HJ_CatchName(HJMediaUrl::Ptr)))
//                {
//                    mediaInfo->setMediaUrl(i_param->getValue<HJMediaUrl::Ptr>(HJ_CatchName(HJMediaUrl::Ptr)));
//                }
                
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
                    m_startTime = std::min(m_firstVideoTime, m_firstAudioTime);
                    m_state = HJComMuxerState_Run;
                    HJFLogi("{} muxer state to run startTime:{} ", m_insName, m_startTime); 
                }
            }

            if (m_state >= HJComMuxerState_Ready)
            {
                //HJFLogi("{} muxer pushback ", m_insName); 
                m_mediaFrameDeque.push_back(i_frame);
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