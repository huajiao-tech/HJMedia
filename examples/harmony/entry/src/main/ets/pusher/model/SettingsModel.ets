export enum PusherPresetEnum {
  RESOLUTION = "resolution",
  ENCODING = "encoding",
  FRAMERATE = "frameRate",

  AUDIO_BITRATE = "audioBitrate",
  SAMPLE_RATE = "sampleRate"
}

@ObservedV2
export class PusherPresetSetting {
  type: PusherPresetEnum
  title: string
  options: PusherOption[]
  @Trace value: number

  constructor(type: PusherPresetEnum, title: string, options: PusherOption[], value: number) {
    this.type = type
    this.title = title
    this.options = options
    this.value = value
  }
}

export class PusherOption {
  option: string
  value: number

  constructor(option: string, value: number) {
    this.option = option
    this.value = value
  }
}