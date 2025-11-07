export class PreviewInfo {
  realWidth: number = 0;
  realHeight: number = 0;
  previewFps: number = 30;
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
  videoIsROIEnc: boolean;
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
  HJ_PUSHER_NOTIFY_NONE = 0,
  HJ_PUSHER_NOTIFY_CONNECT_SUCCESS = 1,
  HJ_PUSHER_NOTIFY_CONNECT_FAILED = 2,
  HJ_PUSHER_NOTIFY_CONNECT_CLOSED = 3,
  HJ_PUSHER_NOTIFY_CONNECT_RETRY = 4,
  HJ_PUSHER_NOTIFY_ERROR = 5,
  HJ_PUSHER_NOTIFY_DROP_FRAME = 6,
  HJ_PUSHER_NOTIFY_AUTOADJUST_BITRATE = 7,
  HJ_PUSHER_NOTIFY_LIVE_INFO = 8,

  HJ_PUSHER_NOTIFY_ERROR_MUXER_INIT = 11,
  HJ_PUSHER_NOTIFY_ERROR_MUXER_WRITEFRAME = 12,
  HJ_PUSHER_NOTIFY_ERROR_CODEC_RUN = 13,
  HJ_PUSHER_NOTIFY_ERROR_CODEC_GETFRAME = 14,
  HJ_PUSHER_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME = 15,
  HJ_PUSHER_NOTIFY_ERROR_CAPTURER_GETFRAME = 16,

  HJ_RENDER_NOTIFY_ERROR_DEFAULT = 100,
  HJ_RENDER_NOTIFY_ERROR_UPDATE = 101,
  HJ_RENDER_NOTIFY_ERROR_DRAW = 102,
  HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT = 103,
  HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT = 104,
  HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE = 105,

  HJ_RENDER_NOTIFY_FACEU_ERROR = 107,
  HJ_RENDER_NOTIFY_FACEU_COMPLETE = 108,
}
