import { State, Store, Action } from '../../interface/Store';
import { AppStorageV2 } from '@kit.ArkUI';
import { PusherConfigPreset, pusherPreset } from '../model/PusherPreviewModel';
import { AudioCodecType, VideoCodecType } from 'hjpusher';
import {
  PusherOption, PusherPresetEnum, PusherPresetSetting
} from '../model/SettingsModel';

@ObservedV2
export class SettingsState implements State {
  @Trace videoBitrate: number = 2200;
  @Trace isAdaptiveBitrateEnabled: boolean = false;
  @Trace pusherVideoSettings: PusherPresetSetting[] = []
  @Trace pusherAudioSettings: PusherPresetSetting[] = []

  getVideoPusherSetting(option: PusherPresetEnum) {
    return this.pusherVideoSettings.find(item => item.type === option)?.value
  }

  getAudioPusherSetting(option: PusherPresetEnum) {
    return this.pusherAudioSettings.find(item => item.type === option)?.value
  }

  get width() {
    return this.getVideoPusherSetting(PusherPresetEnum.RESOLUTION) ?? 1080
  }

  get height() {
    if (this.getVideoPusherSetting(PusherPresetEnum.RESOLUTION) == 1080) {
      return 1280
    }
    return 1920
  }
}

export class VideoBitrateAction implements Action<SettingsState> {
  payload: number;

  constructor(payload: number) {
    this.payload = payload;
  }
}

export class AdaptiveBitrateAction implements Action<SettingsState> {
  payload: boolean;

  constructor(payload: boolean) {
    this.payload = payload;
  }
}

export class ResetSettingsAction implements Action<SettingsState> {}

export class SaveSettingsAction implements Action<SettingsState> {}

export class SettingsStore implements Store<SettingsState> {
  state = new SettingsState();
  getState = () => this.state;

  aboutToAppear() {
    this.resetSettings()
  }

  dispatch(action: Action<SettingsState>): void {
    if (action instanceof VideoBitrateAction) {
      this.state.videoBitrate = action.payload;
    } else if (action instanceof AdaptiveBitrateAction) {
      this.state.isAdaptiveBitrateEnabled = action.payload;
    } else if (action instanceof ResetSettingsAction) {
      this.resetSettings();
    } else if (action instanceof SaveSettingsAction) {
      this.saveSettings();
    }
  }

  private resetSettings() {
    this.state.pusherVideoSettings = [
      new PusherPresetSetting(PusherPresetEnum.RESOLUTION, "分辨率", [
        new PusherOption("720p", 720),
        new PusherOption("1080p", 1080),
      ], 1080),
      new PusherPresetSetting(PusherPresetEnum.ENCODING, "编码方式", [
        new PusherOption("H.264", 264),
        new PusherOption("H.265", 265),
      ], 265),
      new PusherPresetSetting(PusherPresetEnum.FRAMERATE, "视频帧率", [
        new PusherOption("15Hz", 15),
        new PusherOption("30Hz", 30),
      ], 30)
    ]
    this.state.pusherAudioSettings = [
      new PusherPresetSetting(PusherPresetEnum.AUDIO_BITRATE, "音频码率", [
        new PusherOption("48kbps", 48),
        new PusherOption("64kbps", 64),
        new PusherOption("128kbps", 128),
        new PusherOption("164kbps", 164),
      ], 164),
      new PusherPresetSetting(PusherPresetEnum.SAMPLE_RATE, "音频采样率", [
        new PusherOption("22.05KHz", 22.5),
        new PusherOption("44.1KHz", 44.1),
        new PusherOption("48KHz", 48),
      ], 48)
    ]
    this.state.videoBitrate = 2200;
    this.state.isAdaptiveBitrateEnabled = false;
  }

  private saveSettings() {
    const config = AppStorageV2.connect(PusherConfigPreset, () => pusherPreset)!
    config.videoConfig.codecID = this.state.getVideoPusherSetting(PusherPresetEnum.ENCODING) == 264 ? VideoCodecType.HJCodecH264 : VideoCodecType.HJVCodecH265
    config.videoConfig.width = this.state.width
    config.videoConfig.height = this.state.height
    config.videoConfig.bitrate = this.state.videoBitrate * 1024
    config.videoConfig.frameRate = this.state.getVideoPusherSetting(PusherPresetEnum.FRAMERATE) ?? 30

    config.audioConfig.bitrate = (this.state.getAudioPusherSetting(PusherPresetEnum.AUDIO_BITRATE) ?? 164) * 1024
    config.audioConfig.samplesRate = (this.state.getAudioPusherSetting(PusherPresetEnum.SAMPLE_RATE) ?? 48) * 1000
  }
}