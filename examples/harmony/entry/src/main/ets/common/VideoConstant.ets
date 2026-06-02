export interface PusherLiveInfo {
  kbps: number
  fps: number
  delay: number
}

export interface HJDownloadNotify {
  type: number
  rid: string
  totalSize: number
  validSize: number
  status: number
}