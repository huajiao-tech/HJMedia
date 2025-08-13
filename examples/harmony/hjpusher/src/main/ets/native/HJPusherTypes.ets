export enum LogMode {
  CONSOLE = 1 << 1,
  FILE = 1 << 2
}

export class PreviewInfo {
  realWidth: number = 0;
  realHeight: number = 0;
  previewFps: number = 30;
}

export interface SetWindowInfo {
  surfaceId: string
  width: number
  height: number
  state: SetWindowState
}

export interface PusherConfig {
  videoConfig: VideoConfig;
  audioConfig: AudioConfig;
  url: string;
}

export interface VideoConfig {
  codecID: VideoCodecType;
  width: number;
  height: number;
  bitrate: number;
  frameRate: number;
  gopSize: number;
}

export interface AudioConfig {
  codecID: AudioCodecType;
  bitrate: number;
  sampleFmt: number;
  samplesRate: number;
  channels: number;
}

export enum VideoCodecType {
  HJCodecUnknown = -1,
  HJCodecH264 = 27,
  HJVCodecH265 = 173,
}

export enum AudioCodecType {
  HJCodecUnknown = -1,
  HJCodecAAC = 86018,
}

export class RecorderInfo {
  recordUrl: string = "";
}

export enum PusherStateType {
  EVENT_UNKNOWN = 0, // 非法事件
  EVENT_CONNECTED, // 成功连接到服务器
  EVENT_CONNECT_FAILED, // 连接失败
  EVENT_DISCONNECTED, // 连接断开
  EVENT_SN_ARRIVAL, // 调度成功
  EVENT_SN_FAILED = 5, // 调度失败
  EVENT_STREAM_CONNECT_FAIL, // 开流失败
  EVENT_STREAM_CONNECT, // 开流成功
  EVENT_STREAM_FINISH, // 主播推流结束
  EVENT_WRITELOCAL_FAILED, // 本地录制失败
  EVENT_DROP_FRAME = 10, // 丢帧，内部事件
  EVENT_NEED_RESCHEDULING, // 需要重新调度
  EVENT_NOT_HEALTHY_TRANS, // 推流效果较差
  EVENT_RECONNECTING_START, // 开始重连
  EVENT_START_CHANNEL_OK, // 申请嘉宾成功
  EVENT_STOP_CHANNEL_OK = 15, // 取消嘉宾成功
  EVENT_START_CHANNEL_FAILED, // 申请嘉宾失败
  EVENT_START_PREVIEW, // 开始预览
  EVENT_CHANNEL_ADD, // 增加一个嘉宾
  EVENT_CHANNEL_DELETE, // 减少一个嘉宾
  EVENT_CHANNEL_KICK = 20, // 踢掉一个嘉宾
  EVENT_SET_CHANNEL_AUDIO, // 关闭或打开一个嘉宾的声音
  EVENT_SET_CHANNEL_VIDEO, // 关闭或打开一个嘉宾的画面
  EVENT_SET_CHANNEL_NUM, // 设置嘉宾最大数量
  EVENT_APPLY_FOR_CHANNEL, // 申请一个嘉宾
  EVENT_USE_HWENCODER = 100, // 使用硬编码
  EVENT_AUTOADJUST_BITRATE, // 自适应码率
  EVENT_IO_INTERRUPTED, // 取消RTMP重连，返回失败
  EVENT_DATA_RECEIVE = 999 // 内部事件
}

export type PusherStateCall = (type: PusherStateType, ret: number, msgInfo: string) => void;

export enum SetWindowState {
  TARGET_CREATE = 0,
  TARGET_CHANGE = 1,
  TARGET_DESTROY = 2
}