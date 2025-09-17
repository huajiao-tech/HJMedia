import { AudioCodecType, AudioConfig, PusherConfig, VideoCodecType, VideoConfig } from "hjpusher"

@ObservedV2
export class VideoStateData {
  name: string
  @Trace value: number
  @Trace unit: string = ''

  constructor(name: string, value: number, unit: string) {
    this.name = name
    this.value = value
    this.unit = unit
  }
}

@ObservedV2
export class PusherSetting {
  type: string = ""
  layout: string = ""
  primaryIcon: ResourceStr
  secondaryIcon: ResourceStr
  @Trace value: boolean = false

  constructor(type: string, layout: string, primaryIcon: ResourceStr, secondaryIcon: ResourceStr, value: boolean) {
    this.type = type
    this.layout = layout
    this.primaryIcon = primaryIcon
    this.secondaryIcon = secondaryIcon
    this.value = value
  }
}

export class PusherConfigPreset implements PusherConfig {
  videoConfig: VideoConfig
  audioConfig: AudioConfig
  url: string
  isLandScape: boolean = false
  isScreenOn: boolean = false

  constructor(videoConfig: VideoConfig, audioConfig: AudioConfig, url: string) {
    this.videoConfig = videoConfig;
    this.audioConfig = audioConfig;
    this.url = url;
  }
}

export const pusherPreset = new PusherConfigPreset(
  {
    codecID: VideoCodecType.HJVCodecH265,
    width: 720,
    height: 1280,
    bitrate: 2 * 1024 * 1024,
    frameRate: 30,
    gopSize: 60
  },
  {
    codecID: AudioCodecType.HJCodecAAC,
    bitrate: 164 * 1000,
    sampleFmt: 1,
    samplesRate: 48000,
    channels: 2
  },
  ""
)

@ObservedV2
export class PreviewAndPusherSelector {
  @Trace currentIndex: number = 0
}