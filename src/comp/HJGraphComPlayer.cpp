#include "HJGraphComPlayer.h"
#include "HJFLog.h"
#include "HJComDemuxer.h"
#include "HJMediaInfo.h"
#include "HJComVideoDecoder.h"
#include "HJComVideoRender.h"
#include "HJMediaUtils.h"

#if defined(HarmonyOS)
    #include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

HJGraphComPlayer::HJGraphComPlayer()
{
    HJ_SetInsName(HJGraphComPlayer);
}
HJGraphComPlayer::~HJGraphComPlayer()
{
    HJFLogi("{} ~ enter", m_insName);
}

int HJGraphComPlayer::priReady(HJMediaInfo::Ptr i_mediaInfo)
{
    int i_err = HJ_OK;
    do
    {
        int mediaType = i_mediaInfo->getMediaTypes();
        if (mediaType & HJMEDIA_TYPE_VIDEO)
        {
            m_videoDecoder = HJComVideoDecoder::Create();
            m_videoDecoder->setNotify(nullptr);          
            addCom(m_videoDecoder);

            m_videoRender = HJComVideoRender::Create();
            m_videoRender->setNotify([this](HJBaseNotifyInfo::Ptr i_info)
                {
                    if (m_notify)
                    {
                        m_notify(i_info);
                    }
                    
                });
            addCom(m_videoRender);

            ComConnect(m_demuxer, m_videoDecoder);
            ComConnect(m_videoDecoder, m_videoRender);

            HJBaseParam::Ptr renderParam = HJBaseParam::Create();
            (*renderParam)["HJDeviceType"] = (int)m_HJDeviceType;
            if (m_HJDeviceType == HJDEVICE_TYPE_NONE)
            {
#if defined(HarmonyOS)
                HJ_CatchMapSetVal(renderParam, HJOGRenderWindowBridge::Ptr, m_bridge);
#endif
            }
            
            if (m_softBridge) //fixme
            {
#if defined(HarmonyOS)
                HJ_CatchMapPlainSetVal(renderParam, HJOGRenderWindowBridge::Ptr, "HJOGRenderWindowBridgeSoft", m_softBridge);
#endif
            }
            
            m_videoRender->setMediaQueueMaxSize(HJComVideoRender::s_maxMediaSize);
            i_err = m_videoRender->init(renderParam);
            if (i_err < 0)
            {
                break;
            }

            HJBaseParam::Ptr decParam = HJBaseParam::Create();
            (*decParam)[HJ_CatchName(HJStreamInfo::Ptr)] = (HJStreamInfo::Ptr)i_mediaInfo->getStreamInfo(HJMEDIA_TYPE_VIDEO);
            (*decParam)["HJDeviceType"] = (int)m_HJDeviceType;
            
            if (m_HJDeviceType == HJDEVICE_TYPE_OHCODEC)
            {
#if defined(HarmonyOS)
                HJ_CatchMapSetVal(decParam, HJOGRenderWindowBridge::Ptr, m_bridge);
#endif
            }
            m_videoDecoder->setMediaQueueMaxSize(HJComVideoDecoder::s_maxMediaSize);
            i_err = m_videoDecoder->init(decParam);
            if (i_err < 0)
            {
                break;
            }
        }
        if (mediaType & HJMEDIA_TYPE_AUDIO)
        {

        }


        if (i_err == HJ_OK)
        {
            HJFLogi("priReady end, demuxer start");
            m_demuxer->start();
        }

    } while (false);
    return i_err;
}
int HJGraphComPlayer::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseGraphComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        if (!i_param)
        {
            break;
        }    
        if (i_param->contains(HJ_CatchName(HJBaseNotify)))
        {
            m_notify = i_param->getValue<HJBaseNotify>(HJ_CatchName(HJBaseNotify));
        }
#if defined(HarmonyOS)
        HJ_CatchMapGetVal(i_param, HJOGRenderWindowBridge::Ptr, m_bridge);
        {
            //fixme lfs
            HJ_CatchMapPlainGetVal(i_param, HJOGRenderWindowBridge::Ptr, "HJOGRenderWindowBridgeSoft", m_softBridge);
        }
#endif
        
        
        if (i_param->contains("HJDeviceType"))
        {
            m_HJDeviceType = i_param->getValInt("HJDeviceType");
        }
        
        async([this, i_param]()
        {
            int i_err = 0;
            do
            {
                m_demuxer = HJComDemuxer::Create();
                m_demuxer->setNotify([this](HJBaseNotifyInfo::Ptr i_info)
                {
                    if (i_info->getType() == HJCOMPLAYER_DEMUX_READY)
                    {
                        if (i_info->contains(HJ_CatchName(HJMediaInfo::Ptr)))
                        {
                            HJMediaInfo::Ptr mediaInfo = i_info->getValue<HJMediaInfo::Ptr>(HJ_CatchName(HJMediaInfo::Ptr));
                            priReady(mediaInfo);
                        }
                    }
                });
                addCom(m_demuxer);
                m_demuxer->setMediaQueueMaxSize(HJComDemuxer::s_maxMediaSize);
                i_err = m_demuxer->init(i_param);
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
void HJGraphComPlayer::done()
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