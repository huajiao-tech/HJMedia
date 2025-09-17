//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJInterleavedMuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJInterleavedMuxer::HJInterleavedMuxer()
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJInterleavedMuxer)));
}

HJInterleavedMuxer::~HJInterleavedMuxer()
{
    done();
}

int HJInterleavedMuxer::init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts)
{
    int res = HJ_OK;
    do
    {
        m_muxer = std::make_shared<HJFFMuxer>();
        res = m_muxer->init(mediaInfo);
        if (HJ_OK != res) {
            break;
        }
        m_interweaver = std::make_shared<HJAVInterweaver>();
        res = m_interweaver->init(mediaInfo);
        if (HJ_OK != res) {
            break;
        }
    } while (false);

    return res;
}

int HJInterleavedMuxer::init(const std::string url, int mediaTypes, HJOptions::Ptr opts)
{
    int res = HJ_OK;
    do
    {
        m_muxer = std::make_shared<HJFFMuxer>();
        res = m_muxer->init(url, mediaTypes, opts);
        if (HJ_OK != res) {
            break;
        }
        m_mediaInfo = HJCreates<HJMediaInfo>();
        //
        m_interweaver = std::make_shared<HJAVInterweaver>();
        res = m_interweaver->init(m_mediaInfo);
        if (HJ_OK != res) {
            break;
        }
    } while (false);

    return res;
}

int HJInterleavedMuxer::addFrame(const HJMediaFrame::Ptr& frame)
{
    if (!m_muxer) {
        HJErrNotAlready;
    }
    //HJLogi("entry");
    int res = m_interweaver->addFrame(frame);
    if (HJ_OK != res) {
        return res;
    }
    auto mvf = m_interweaver->getFrame();
    while (mvf) {
        res = m_muxer->addFrame(std::move(mvf));
        if (HJ_OK != res) {
            break;
        }
        mvf = m_interweaver->getFrame();
    }
    //HJLogi("end");

    return res;
}

int HJInterleavedMuxer::writeFrame(const HJMediaFrame::Ptr& frame)
{
    if (!m_muxer) {
        HJErrNotAlready;
    }
    //HJLogi("entry");
    int res = m_interweaver->addFrame(frame);
    if (HJ_OK != res) {
        return res;
    }
    auto mvf = m_interweaver->getFrame();
    while (mvf) {
        res = m_muxer->writeFrame(std::move(mvf));
        if (HJ_OK != res) {
            break;
        }
        mvf = m_interweaver->getFrame();
    }
    //HJLogi("end");

    return res;
}

void HJInterleavedMuxer::done()
{
    if (m_muxer) {
        m_muxer->done();
        m_muxer = nullptr;
    }
    m_interweaver = nullptr;
}

NS_HJ_END
