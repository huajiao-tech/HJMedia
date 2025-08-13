//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBaseDemuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
const MTS MTS::MTS_ZERO = MTS(0, 0);
const MTS MTS::MTS_NOPTS = MTS(HJ_NOTS_VALUE, HJ_NOTS_VALUE);
const MTS MTS::MTS_MAX = MTS(HJ_INT64_MAX, HJ_INT64_MAX);
//
const std::string HJBaseDemuxer::KEY_WORLDS_URLBUFFER = "HJ_URL_BUFFER";
//
HJBaseDemuxer::HJBaseDemuxer(const HJMediaUrl::Ptr& mediaUrl)
    : m_mediaUrl(mediaUrl)
{

}

HJBaseDemuxer::~HJBaseDemuxer()
{

}

int HJBaseDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
{
    m_mediaUrl = mediaUrl;
    m_mediaInfo = std::make_shared<HJMediaInfo>();
    if (m_name.empty()) {
        m_name = HJ2STR(mediaUrl->getUrlHash());
    }
    m_atrs = std::make_shared<HJSourceAttribute>();
    
    return HJ_OK;
}

int HJBaseDemuxer::init(const HJMediaUrlVector& mediaUrls)
{
    m_mediaInfo = std::make_shared<HJMediaInfo>();
    if (m_name.empty()) {
        m_name = HJ2STR(mediaUrls[0]->getUrlHash());
    }
    m_atrs = std::make_shared<HJSourceAttribute>();

    return HJ_OK;
}

void HJBaseDemuxer::done()
{
    HJFLogi("entry, url:{}", m_mediaUrl ? m_mediaUrl->getUrl() : "");
    HJBaseDemuxer::reset();
    m_runState = HJRun_Done;
}

int HJBaseDemuxer::seek(int64_t pos)
{
    HJBaseDemuxer::reset();

    return HJ_OK;
}

void HJBaseDemuxer::reset()
{
    m_vfCnt = 0;
    m_afCnt = 0;
    m_validVFlag = false;
    
    if (m_atrs) {
        m_atrs->reset();
    }
    m_extraVBuffer = nullptr;
    m_extraABuffer = nullptr;

    return;
}

NS_HJ_END
