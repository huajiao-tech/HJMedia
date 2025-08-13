//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBaseMuxer.h"

NS_HJ_BEGIN
const std::string HJBaseMuxer::KEY_KEY_FRAME_CHECK = "key_frame_check";
//***********************************************************************************//
HJBaseMuxer::HJBaseMuxer()
{

}
HJBaseMuxer::~HJBaseMuxer()
{

}

int HJBaseMuxer::init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts)
{
    m_mediaInfo = mediaInfo;
    m_opts = opts;
    if (m_mediaInfo) {
        m_mediaTypes = m_mediaInfo->getMediaTypes();
        const auto& mediaUrl = m_mediaInfo->getMediaUrl();
        if (mediaUrl) {
            m_url = mediaUrl->getUrl();
        }
    }
    if (m_opts) {
        if (m_opts->haveValue(KEY_KEY_FRAME_CHECK)) {
            m_keyFrameCheck = m_opts->getBool(KEY_KEY_FRAME_CHECK);
        }
    }

    return HJ_OK;
}

int HJBaseMuxer::init(const std::string url, int mediaTypes, HJOptions::Ptr opts)
{
    m_url = url;
    m_mediaTypes = mediaTypes;
    m_opts = opts;
    if (m_opts) {
        if (m_opts->haveValue(KEY_KEY_FRAME_CHECK)) {
            m_keyFrameCheck = m_opts->getBool(KEY_KEY_FRAME_CHECK);
        }
    }

    return HJ_OK;
}


void HJBaseMuxer::done()
{

}

NS_HJ_END
