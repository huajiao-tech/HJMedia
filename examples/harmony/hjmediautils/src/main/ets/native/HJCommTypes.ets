export enum LogMode {
  CONSOLE = 1 << 1,
  FILE = 1 << 2
}

export enum SetWindowState {
  TARGET_CREATE = 0,
  TARGET_CHANGE = 1,
  TARGET_DESTROY = 2
}

export interface SetWindowInfo {
  classStyle: string
  insName: string
  surfaceId: string
  width: number
  height: number
  state: SetWindowState
}

export interface MediaStateInfo {
  uid: number
  device: string
  sn: string
}

export interface HJOptionDir {
  name: string
  max: number
}

export interface HJPlayerContextInitConfig {
  valid: boolean
  logDir: string
  logLevel: number
  logMode: number
  maxSize: number
  maxFiles: number
  medias_dir: string
  medias_cache_max: number
  other_dirs_options: HJOptionDir[]
  download_retry_max: number
}

export interface HJPlayerDownloadInfo {
  url: string
  dir: string
  rid: string
  preCacheSize: number
  priority: number
}

export enum HJRenderNotifyType
{
  HJ_RENDER_NOTIFY_ERROR_DEFAULT = 100,
  HJ_RENDER_NOTIFY_ERROR_UPDATE = 101,
  HJ_RENDER_NOTIFY_ERROR_DRAW = 102,
  HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT = 103,
  HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT = 104,
  HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE = 105,
  HJ_RENDER_NOTIFY_NEED_SURFACE = 106,

  HJ_RENDER_NOTIFY_FACEU_ERROR = 107,
  HJ_RENDER_NOTIFY_FACEU_COMPLETE = 108,

  HJ_RENDER_NOTIFY_NodeCreateSuccess  = 109,
  HJ_RENDER_NOTIFY_NodeCreateFailed   = 110,

  HJ_RENDER_NOTIFY_NodeConnectSuccess = 111,
  HJ_RENDER_NOTIFY_NodeConnectFailed  = 112,

  HJ_RENDER_NOTIFY_NodeDisconnectSuccess = 113,
  HJ_RENDER_NOTIFY_NodeDisconnectFailed  = 114,

  HJ_RENDER_NOTIFY_NodeDeleteSuccess     = 115,
  HJ_RENDER_NOTIFY_NodeDeleteFailed      = 116,

  HJ_RENDER_NOTIFY_NodeEnableSuccess     = 117,
  HJ_RENDER_NOTIFY_NodeEnableFailed      = 118,

  HJ_RENDER_NOTIFY_NodeLinkInfoChangeSuccess = 119,
  HJ_RENDER_NOTIFY_NodeLinkInfoChangeFailed  = 120,

  HJ_RENDER_NOTIFY_CACHE_START = 200,
  HJ_RENDER_NOTIFY_CACHE_PROGRESS = 201,
  HJ_RENDER_NOTIFY_CACHE_COMPLETED = 202,
  HJ_RENDER_NOTIFY_CACHE_FAILED = 203,
}