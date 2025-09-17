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