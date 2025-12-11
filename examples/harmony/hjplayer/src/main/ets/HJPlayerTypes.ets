export interface OpenPlayerInfo {
  url: string
  fps: number
  videoCodecType: HJPlayerVideoCodecType
  sourceType: HJPlayerSourceType
  playerType: HJPlayerType
  bSplitScreenMirror: boolean
  disableMFlag: number
}

export enum HJPlayerVideoCodecType {
  HJPlayerVideoCodecType_SoftDefault = 0,
  HJPlayerVideoCodecType_OHCODEC = 1,
  HJPlayerVideoCodecType_VIDEOTOOLBOX = 2,
  HJPlayerVideoCodecType_MEDIACODEC = 3,
}


export enum HJPlayerSourceType {
  HJPlayerSourceType_UNKNOWN = 0,
  HJPlayerSourceType_Bridge = 1,
  HJPlayerSourceType_SPLITSCREEN = 2,
  HJPlayerSourceType_SERIES = 3,
}

export enum HJPlayerNotifyType {
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
}

export enum HJPlayerType {
  HJPlayerType_UNKNOWN = 0,
  HJPlayerType_LIVESTREAM = 1,
  HJPlayerType_VOD = 2,
}

export enum HJPlayerMediaType
{
    HJPLAYER_MEDIA_TYPE_NONE       = 0x0000,
    HJPLAYER_MEDIA_TYPE_VIDEO      = 0x0001,
    HJPLAYER_MEDIA_TYPE_AUDIO      = 0x0002,
    HJPLAYER_MEDIA_TYPE_DATA       = 0x0004,
    HJPLAYER_MEDIA_TYPE_SUBTITLE   = 0x0008,
}