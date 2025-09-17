//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJGraphDec.h"
#include "HJTicker.h"
#include "HJContext.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJGraphDec::HJGraphDec(const HJEnvironment::Ptr& env)
    : HJGraphBase(env)
{
    setName(HJMakeGlobalName("graph_dec"));
}

HJGraphDec::~HJGraphDec()
{
    done();
}

int HJGraphDec::init(const HJMediaUrl::Ptr& mediaUrl)
{
    if (!mediaUrl) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do
    {
        m_mediaUrl = mediaUrl;
        //
        m_demuxerExecutor = HJExecutorByName("HJDemuxer");    //HJCreateExecutor(HJFMT("{}_{}", getName(), "demuxer"));
        m_decodeExecutor = HJExecutorByName("HJDecode");  //HJCreateExecutor(HJFMT("{}_{}", getName(), "decode"));
        HJNodeDemuxer::Ptr demuxer = std::make_shared<HJNodeDemuxer>(m_env, m_demuxerExecutor);
        res = demuxer->init(mediaUrl);
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("demuxer init error, url" + mediaUrl->getUrl())));
            break;
        }
        m_demuxer = demuxer;
        m_runState = HJRun_Init;
    } while (false);

    return res;
}

int HJGraphDec::init(const HJMediaUrlVector& mediaUrls)
{
    if(mediaUrls.size() <= 0) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do
    {
        m_mediaUrls = mediaUrls;
        m_mediaUrl = m_mediaUrls[0];
        //
        m_demuxerExecutor = HJExecutorByName("HJDemuxer");    //HJCreateExecutor(HJFMT("{}_{}", getName(), "demuxer"));
        m_decodeExecutor = HJExecutorByName("HJDecode");  //HJCreateExecutor(HJFMT("{}_{}", getName(), "decode"));
        HJNodeDemuxer::Ptr demuxer = std::make_shared<HJNodeDemuxer>(m_env, m_demuxerExecutor);
        res = demuxer->init(m_mediaUrls);
        if (HJ_OK != res) {
            notify(HJMakeNotification(HJNotify_Error, res, std::string("demuxer init error, url" + m_mediaUrl->getUrl())));
            break;
        }
        m_demuxer = demuxer;
        m_runState = HJRun_Init;
    } while (false);

    return res;
}

int HJGraphDec::buildGraph()
{
    int res = HJ_OK;
    do {
        if (!m_demuxer || m_runState < HJRun_Init) {
            res = HJErrNotAlready;
            break;
        }
        HJMediaInfo::Ptr mediaInfo = m_demuxer->getBestMediaInfo();
        if (!mediaInfo || mediaInfo->getStreamInfos().size() <= 0) {
            HJLoge("media info error");
            res = HJErrNotMInfo;
            break;
        }
        HJLogi("start begin, build graph");
        const auto& streamInfos = mediaInfo->getStreamInfos();
        for (auto it = streamInfos.begin(); it != streamInfos.end(); it++)
        {
            HJMediaType mediaType = (HJMediaType)(*it)->getType();
            switch (mediaType)
            {
                case HJMEDIA_TYPE_VIDEO:
                {
                    if (m_mediaUrl->getDisableMFlag() & HJMEDIA_TYPE_VIDEO) {
                        break;
                    }
                    auto devAttri = m_env->getDevAttri();
                    HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(*it);
                    if (HJDEVICE_TYPE_NONE != devAttri.m_devType) {
                        videoInfo->setHWDevice(m_env->getHWDevice(devAttri.m_devType, devAttri.m_devID));
                    }
                    (*videoInfo)["threads"] = (int)HJ_MIN(m_env->getVDecThreads(), (int)std::thread::hardware_concurrency());
                    //
                    m_videoDecoder = std::make_shared<HJNodeVDecoder>(m_env, m_decodeExecutor);
                    res = m_videoDecoder->init(videoInfo, devAttri.m_devType);
                    if (HJ_OK != res) {
                        HJLoge("video decoder init error:" + HJ2String(res));
                        break;
                    }
                    //estimate capacity
                    int capacity = HJ_MAX(HJ_FRAMERATE_DEFALUT, HJ_MS_TO_SEC(m_nodeTimeCapacity) * videoInfo->m_frameRate);
                    m_demuxer->connect(m_videoDecoder, capacity);
                    break;
                }
                case HJMEDIA_TYPE_AUDIO:
                {
                    if (m_mediaUrl->getDisableMFlag() & HJMEDIA_TYPE_AUDIO) {
                        break;
                    }
                    HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(*it);
                    m_audioDecoder = std::make_shared<HJNodeADecoder>(m_env, m_decodeExecutor);
                    res = m_audioDecoder->init(audioInfo);
                    if (HJ_OK != res) {
                        HJLoge("audio decoder init error:" + HJ2String(res));
                        break;
                    }
                    int capacity = HJ_MAX(HJ_ADEC_STOREAGE_CAPACITY, HJ_MS_TO_SEC(m_nodeTimeCapacity) * audioInfo->m_samplesRate / audioInfo->m_samplesPerFrame);
                    m_demuxer->connect(m_audioDecoder, capacity);
                    break;
                }
                default: {
                    HJLogw("warning, not support media info, mediaType:" + HJ2String(mediaType));
                    break;
                }
            } //switch (mediaType)
            if (HJ_OK != res) {
                break;
            }
        }//for
        if (HJ_OK != res) {
            break;
        }
        m_runState = HJRun_Ready;
    } while (false);
    
    return res;
}

int HJGraphDec::start()
{
    int res = HJ_OK;
    do
    {
        if (!m_demuxer || m_runState < HJRun_Init) {
            res = HJErrNotAlready;
            break;
        }
        res = m_demuxer->start();
        if (HJ_OK != res) {
            HJLoge("error, demuxer start failed:" + HJ2STR(res));
            break;
        }
    } while (false);
    return res;
}

int HJGraphDec::play()
{
    int res = HJ_OK;
    do
    {
        if (!m_demuxer || m_runState < HJRun_Ready) {
            res = HJErrNotAlready;
            break;
        }
        res = m_demuxer->play();
        if (HJ_OK != res) {
            HJLoge("error, demuxer play failed:" + HJ2STR(res));
            break;
        }
    } while (false);

    return res;
}

int HJGraphDec::pause()
{
    int res = HJ_OK;
    do
    {
        if (!m_demuxer || m_runState < HJRun_Ready) {
            res = HJErrNotAlready;
            break;
        }
        res = m_demuxer->pause();
        if (HJ_OK != res) {
            HJLoge("error, demuxer pause failed:" + HJ2STR(res));
            break;
        }
    } while (false);

    return res;
}

//int HJGraphDec::resume()
//{
//    int res = HJ_OK;
//    do
//    {
//        if (!m_demuxer || m_runState < HJRun_Ready) {
//            res = HJErrNotAlready;
//            break;
//        }
//        res = m_demuxer->resume();
//        if (HJ_OK != res) {
//            HJLogi("demuxer start error:" + HJ2STR(res));
//            break;
//        }
//    } while (false);
//
//    return res;
//}

//int HJGraphDec::flush()
//{
//    int res = HJ_OK;
//    
//    return res;
//}

void HJGraphDec::done()
{
    if (!m_demuxer) {
        return;
    }
    HJLogi("entry");
    if (m_demuxer) {
        m_demuxer->done();
        m_demuxer = nullptr;
    }
    if (m_audioDecoder) {
        m_audioDecoder->done();
        m_audioDecoder = nullptr;
    }
    if (m_videoDecoder) {
        m_videoDecoder->done();
        m_videoDecoder = nullptr;
    }
    m_demuxerExecutor = nullptr;
    m_decodeExecutor = nullptr;
    HJLogi("end");
}

int HJGraphDec::seek(const HJSeekInfo::Ptr info)
{
    int res = HJ_OK;
    do
    {
        if (!m_demuxer || m_runState < HJRun_Ready) {
            res = HJErrNotAlready;
            break;
        }
        res = m_demuxer->seek(info);
        if (HJ_OK != res) {
            HJLoge("erro, demuxer seek failed:" + HJ2STR(res));
            break;
        }
    } while (false);

    return res;
}

int HJGraphDec::connectVOut(const HJMediaVNode::Ptr& node) {
    if (!m_videoDecoder) {
        return HJErrNotAlready;
    }
    int capacity = HJ_VREND_STOREAGE_CAPACITY;
    return m_videoDecoder->connect(node, capacity);
}

int HJGraphDec::connectAOut(const HJMediaVNode::Ptr& node) {
    if (!m_audioDecoder) {
        return HJErrNotAlready;
    }
    int capacity = HJ_AREND_STOREAGE_CAPACITY;
    return m_audioDecoder->connect(node, capacity);
}

NS_HJ_END
