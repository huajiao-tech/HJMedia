export interface OpenPlayerInfo {
  url: string
  localDir: string
  rid: string
  fps: number
  repeats: number
  videoCodecType: HJPlayerVideoCodecType
  sourceType: HJPlayerSourceType
  playerType: HJPlayerType
  bSplitScreenMirror: boolean
  disableMFlag: number
  m_graphConfig: string
  m_enableSEIUUids: Map<string, boolean>
}

function serializeEnableSeiUuids(enable_sei_uuids: Map<string, boolean>): string {
  let uuid_entries: Array<string> = new Array<string>()
  enable_sei_uuids.forEach((value: boolean, key: string) => {
    uuid_entries.push(JSON.stringify(key) + ':' + (value ? 'true' : 'false'))
  })
  return '{' + uuid_entries.join(',') + '}'
}

export function serializeOpenPlayerInfo(open_player_info: OpenPlayerInfo): string {
  let json_parts: Array<string> = new Array<string>()
  json_parts.push('"url":' + JSON.stringify(open_player_info.url))
  json_parts.push('"localDir":' + JSON.stringify(open_player_info.localDir))
  json_parts.push('"rid":' + JSON.stringify(open_player_info.rid))
  json_parts.push('"fps":' + open_player_info.fps.toString())
  json_parts.push('"repeats":' + open_player_info.repeats.toString())
  json_parts.push('"videoCodecType":' + open_player_info.videoCodecType.toString())
  json_parts.push('"sourceType":' + open_player_info.sourceType.toString())
  json_parts.push('"playerType":' + open_player_info.playerType.toString())
  json_parts.push('"bSplitScreenMirror":' + (open_player_info.bSplitScreenMirror ? 'true' : 'false'))
  json_parts.push('"disableMFlag":' + open_player_info.disableMFlag.toString())
  json_parts.push('"m_graphConfig":' + JSON.stringify(open_player_info.m_graphConfig))
  json_parts.push('"m_enableSEIUUids":' + serializeEnableSeiUuids(open_player_info.m_enableSEIUUids))
  return '{' + json_parts.join(',') + '}'
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

  HJ_PLAYER_NOTIFY_DURATION = 50,
  HJ_PLAYER_NOTIFY_SEI_INFOS = 51,

  HJ_PLAYER_NOTIFY_CLOSEDONE = 90,
}

export enum HJPlayerType {
  HJPlayerType_UNKNOWN = 0,
  HJPlayerType_LIVESTREAM = 1,
  HJPlayerType_VOD = 2,
}

export interface HJPlayerSeiData {
  uuid: string
  data: ArrayBuffer
}

export interface HJPlayerSeiInfo {
  seiDatas: HJPlayerSeiData[]
  isKeyFrame: boolean
}

export enum HJPlayerMediaType
{
    HJPLAYER_MEDIA_TYPE_NONE       = 0x0000,
    HJPLAYER_MEDIA_TYPE_VIDEO      = 0x0001,
    HJPLAYER_MEDIA_TYPE_AUDIO      = 0x0002,
    HJPLAYER_MEDIA_TYPE_DATA       = 0x0004,
    HJPLAYER_MEDIA_TYPE_SUBTITLE   = 0x0008,
}
