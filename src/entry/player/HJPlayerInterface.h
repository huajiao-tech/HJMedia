#pragma once

#include <string>
#include <functional>
#include "HJCommonInterface.h"
#include "HJPrioUtils.h"


using HJNAPIPlayerNotify = std::function<void(int i_type, const std::string& i_msgInfo)>;
using HJStatPlayerNotify = std::function<void(const std::string& i_name, int i_nType, const std::string& i_info)>;

typedef enum HJPlayerVideoCodecType
{
    HJPlayerVideoCodecType_SoftDefault  = 0,
    HJPlayerVideoCodecType_OHCODEC      = 1,
    HJPlayerVideoCodecType_VIDEOTOOLBOX = 2,
    HJPlayerVideoCodecType_MEDIACODEC   = 3,
} HJPlayerVideoCodecType;

typedef enum HJPlayerType
{
    HJPlayerType_UNKNOWN    = 0,
    HJPlayerType_LIVESTREAM = 1,
    HJPlayerType_VOD        = 2,
} HJPlayerType;

typedef struct HJPlayerInfo
{
    std::string m_url = "";
    int m_fps = 30;
    int m_videoCodecType = HJPlayerVideoCodecType_SoftDefault;
    HJPrioComSourceType m_sourceType = HJPrioComSourceType_SERIES;
    HJPlayerType m_playerType = HJPlayerType_LIVESTREAM;
    bool m_bSplitScreenMirror = false;
} HJPlayerInfo;

typedef enum HJPlayerNotifyType
{
    HJ_PLAYER_NOTIFY_NONE = 0,

    HJ_PLAYER_NOTIFY_VIDEO_FIRST_RENDER = 5,
    HJ_PLAYER_NOTIFY_AUDIO_FRAME = 6,
    HJ_PLAYER_NOTIFY_EOF = 7,

    HJ_PLAYER_NOTIFY_CLOSEDONE = 90,


    HJ_RENDER_NOTIFY_ERROR_DEFAULT = 100,
    HJ_RENDER_NOTIFY_ERROR_UPDATE = 101,
    HJ_RENDER_NOTIFY_ERROR_DRAW = 102,
    HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT = 103,
    HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT = 104,
    HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE = 105,
    HJ_RENDER_NOTIFY_NEED_SURFACE = 106,
    
    HJ_RENDER_NOTIFY_FACEU_ERROR = 107,
    HJ_RENDER_NOTIFY_FACEU_COMPLETE = 108,

} HJPlayerNotifyType;






