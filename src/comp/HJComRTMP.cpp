#include "HJComRTMP.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

HJComRTMP::HJComRTMP()
{
    HJ_SetInsName(HJComRTMP);    
    setFilterType(HJCOM_FILTER_TYPE_VIDEO | HJCOM_FILTER_TYPE_AUDIO);
}
HJComRTMP::~HJComRTMP()
{
}

int HJComRTMP::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        m_muxer = std::make_shared<HJRTMPMuxer>();
        if (!m_muxer)
        {
            i_err = -1;
            break;
        }

        //init tcp io, so another thread
        async([this, i_param]()
        {
            int i_err = 0;
            do
            {
                std::string strUrl = "";
                HJMediaInfo::Ptr mediaInfo = std::make_shared<HJMediaInfo>();//HJMEDIA_TYPE_VIDEO | HJMEDIA_TYPE_AUDIO);
                if (i_param->contains(HJ_CatchName(HJAudioInfo::Ptr)))
                {
                    mediaInfo->addStreamInfo(i_param->getValue<HJAudioInfo::Ptr>(HJ_CatchName(HJAudioInfo::Ptr)));
                }
                if (i_param->contains(HJ_CatchName(HJVideoInfo::Ptr)))
                {
                    mediaInfo->addStreamInfo(i_param->getValue<HJVideoInfo::Ptr>(HJ_CatchName(HJVideoInfo::Ptr)));
                }
                if (i_param->contains(HJ_CatchName(HJMediaUrl::Ptr)))
                {
                    mediaInfo->setMediaUrl(i_param->getValue<HJMediaUrl::Ptr>(HJ_CatchName(HJMediaUrl::Ptr)));
                    auto mediaUrl = i_param->getValue<HJMediaUrl::Ptr>(HJ_CatchName(HJMediaUrl::Ptr));
                    strUrl = mediaUrl->getUrl();
                }
                // i_err = m_muxer->init(mediaInfo);
                int mediaTypes = HJMEDIA_TYPE_AV;
                // auto options = std::make_shared<HJOptions>();
                // (*options)["HJRTMPMuxer::STORE_KEY_LOCALURL"] = "E:/MMGroup/dumps/test.flv";
                i_err = m_muxer->init(strUrl, mediaTypes);
                if (i_err < 0)
                {
                    break;
                }
            } while (false);
            return i_err; 
        });

        //        m_rtmpPusher = MD::MDRtmp::Create();
        //        if (i_param->contains("HORTMPPusherInfo"))
        //        {
        //            m_info = i_param->getValue<HORTMPPusherInfo>("HORTMPPusherInfo");
        //        }
        //
        //        MD::MDRtmpInfo mediaInfo;
        //        mediaInfo.m_rtmpUrl = m_info.rtmp_url;
        //
        //        mediaInfo.m_socketBufferSize = 2 * 1024 * 1024;
        //        mediaInfo.m_retryMax = 9;
        //        mediaInfo.m_retryMaxTime = 180;
        //        mediaInfo.video.m_width = m_info.video.width;
        //        mediaInfo.video.m_height = m_info.video.height;
        //        mediaInfo.video.m_bitrateKbps = m_info.video.bitrate / 1024;
        //        mediaInfo.video.m_fps = m_info.video.fps;
        //        mediaInfo.video.codec = m_info.video.codec;
        //
        //        mediaInfo.audio.m_sample_rate = m_info.audio.sample_rate;
        //        mediaInfo.audio.m_channels = m_info.audio.channels;
        //        mediaInfo.audio.m_bitrateKbps = m_info.audio.bitrate;
        //
        //        i_err = m_rtmpPusher->init(mediaInfo, [this](int i_flag)
        //        {
        //
        //        });
        //        if (i_err < 0)
        //        {
        //            break;
        //        }
        //        if (i_err < 0)
        //        {
        //            JFLoge("video encoder open error:{}", i_err);
        //            break;
        //        }
    } while (false);
    return i_err;
}
void HJComRTMP::priStat(const HJComMediaFrame::Ptr &i_frame)
{
    HJ_LOCK(m_stat_mutex);
    if (i_frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
    {
        HJMediaFrame::Ptr iFrame = i_frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));
        AVPacket *packet = iFrame->getAVPacket();
        if (packet)
        {
            bool bKey = packet->flags == AV_PKT_FLAG_KEY;
            int size = packet->size;
            int64_t pts = packet->pts;
            int64_t dts = packet->dts;
            bool isVideo = iFrame->isVideo(); 
            if (isVideo)
            {
                m_vvDiffDts = dts - m_videoCacheDts;
                m_avDiffDts = dts - m_audioCacheDts;
                m_videoCacheDts = dts;
            }
            else
            {
                m_aaDiffDts = dts - m_audioCacheDts;
                m_avDiffDts = dts - m_videoCacheDts;
                m_audioCacheDts = dts;
            }
            
            m_regularLog.proc([this, packet, isVideo, size, pts, dts, bKey]()
            {
                HJFLogi("{} type:{} size:{} pts:{} dts:{} isKey:{} vvdiff:{} aadiff:{} avdiff:{}", m_insName, isVideo ? "video" : "audio", size, pts, dts, bKey, m_vvDiffDts, m_aaDiffDts, m_avDiffDts);
            });
        }
    }
}

int HJComRTMP::run()
{
    int i_err = 0;
    do
    {
        while (!m_mediaFrameDeque.isEmpty())
        {
            HJComMediaFrame::Ptr frame = m_mediaFrameDeque.pop();
            if (frame)
            {
                if (frame->contains(HJ_CatchName(HJMediaFrame::Ptr)))
                {
                    HJMediaFrame::Ptr iFrame = frame->getValue<HJMediaFrame::Ptr>(HJ_CatchName(HJMediaFrame::Ptr));
                    iFrame->setExtraTS(HJCurrentSteadyMS());
                    //HJFLogi("{} type:{} pts:{}", m_insName, iFrame->getType() == HJMEDIA_TYPE_VIDEO ? "video":"audio", iFrame->getPTS());
                    i_err = m_muxer->writeFrame(iFrame);
                    if (i_err < 0)
                    {
                        break;
                    }
                }
            }
        }
    } while (false);
    return i_err;
}
void HJComRTMP::join()
{
    HJBaseComAsyncThread::join();
}
void HJComRTMP::done()
{
    if (m_muxer)
    {
        m_muxer->setQuit();
        m_muxer->done();
    }
    HJBaseComAsyncThread::done();
}

NS_HJ_END