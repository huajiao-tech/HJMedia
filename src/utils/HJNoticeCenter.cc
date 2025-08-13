//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJNoticeCenter.h"

NS_HJ_BEGIN
//***********************************************************************************//
//HJ_INSTANCE_IMP(HJNoticeCenter)
HJ_INSTANCE(HJNoticeCenter)

namespace HJBroadcast {
    const std::string EVENT_PLAYER_NOTIFY = "event_player_notify";
    const std::string EVENT_PLAYER_SEEKINFO = "event_player_seekinfo";
    const std::string EVENT_PLAYER_MEDIAFRAME = "event_player_mediaframe";
    const std::string EVENT_PLAYER_SOURCEFRAME = "event_player_sourceframe";
};

//***********************************************************************************//
HJNoticeHandler::HJNoticeHandler()
{
    m_noticeCenter = std::make_shared<HJNoticeCenter>();
}

HJNoticeHandler::~HJNoticeHandler()
{
    m_noticeCenter = nullptr;
}

//***********************************************************************************//
NS_HJ_END
