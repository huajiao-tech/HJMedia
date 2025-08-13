import { AudioCodecType, LogMode, PreviewInfo, HJPusher, RecorderInfo, VideoCodecType } from 'hjpusher';
import { cameraService } from '../../camera/service/CameraService';
import { State, Store, Action } from '../../interface/Store';
import DateTimeUtil from '../../util/DateTimeUtils';
import { applySaveVideo } from '../../util/RecorderVideoUtils';
import { pusherPreset, PusherConfigPreset, PusherSetting, VideoStateData } from '../model/PusherPreviewModel';
import { AppStorageV2 } from '@kit.ArkUI';
import { WindowUtils } from '../../util/WindowUtils';
import { ToastUtils } from '../../util/ToastUtils';

@ObservedV2
export class PusherPreviewState implements State {
  @Trace videoStateData: VideoStateData[] = []
  @Trace playState: boolean = false
  @Trace live_time: number = 0
  @Trace hour: number = 0;
  path: string = ""
  src: string = ""
  ext: string = "mp4"
  pusherSettings: PusherSetting[] = [
    new PusherSetting("microphone", "left", $r("app.media.microphone_1_icon"), $r("app.media.microphone_0_icon"),  false),
    new PusherSetting("recorder", "left", $r("app.media.video_recorder_1_icon"), $r("app.media.video_recorder_0_icon"),  false),
    new PusherSetting("camera", "right", $r("app.media.camera_front"), $r("app.media.camera_rear"),  false),
    new PusherSetting("gift", "right", $r("app.media.gift_icon"), $r("app.media.gift_icon"),  false),
  ]
  textTimerController: TextTimerController = new TextTimerController()
  hjPusher: HJPusher = new HJPusher()
  config: PusherConfigPreset = AppStorageV2.connect(PusherConfigPreset, () => pusherPreset)!
}

export class MicrophoneAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class RecorderActionPayLoad {
  index: number
  recordUrl: string

  constructor(index: number, recordUrl: string) {
    this.index = index
    this.recordUrl = recordUrl
  }
}

export class RecorderAction implements Action<PusherPreviewState> {
  payload: RecorderActionPayLoad

  constructor(payload: RecorderActionPayLoad) {
    this.payload = payload
  }
}

export class PusherAction implements Action<PusherPreviewState> {
}

export class CameraAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class GiftAction implements Action<PusherPreviewState> {
  payload: string

  constructor(payload: string) {
    this.payload = payload
  }
}

export class TimerAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class PusherPreviewStore implements Store<PusherPreviewState> {
  state = new PusherPreviewState()
  getState = () => this.state

  aboutToAppear() {
    if (this.state.config.isScreenOn) {
      WindowUtils.setWindowKeepScreenOn(true)
    }
    let logLevel = 2;
    let logMode = LogMode.CONSOLE | LogMode.FILE;
    let logPath = '/data/storage/el2/base/haps/entry/files/';
    let bLog = true;
    let logSize = 1024 * 1024 * 5;
    let logCount = 5;
    HJPusher.contextInit(bLog, logPath, logLevel, logMode, logSize, logCount);

    this.state.hjPusher.createPusher();
    const previewInfo = new PreviewInfo();
    previewInfo.realWidth = this.state.config.videoConfig.width
    previewInfo.realHeight = this.state.config.videoConfig.height
    let previewSurfaceId: bigint = this.state.hjPusher.openPreview(previewInfo, (str: string) => {
      // JSON.parse(str) as PusherStateCall;
      console.info("### receivecallbak #type is ", str);
    });
    cameraService.initCamera(getContext() as Context);
    cameraService.bindSurfaceId(previewSurfaceId.toString());
    cameraService.startPreview(0)
  }

  aboutToDisappear() {
    if (this.state.config.isScreenOn) {
      WindowUtils.setWindowKeepScreenOn(false)
    }
    cameraService.releaseCamera()
    this.state.hjPusher.closePusher()
    this.state.hjPusher.closePreview()
    this.state.hjPusher.destroyPusher()
  }

  dispatch(action: Action<PusherPreviewState>): void {
    if (action instanceof MicrophoneAction) {
      const index = action.payload
      this.state.pusherSettings[index].value = !this.state.pusherSettings[index].value
      this.state.hjPusher.setMute(this.state.pusherSettings[index].value)
    } else if (action instanceof PusherAction) {
      this.state.playState = !this.state.playState
      if (this.state.playState) {
        this.openPusher()
      } else {
        this.closePusher()
      }
    } else if (action instanceof CameraAction) {
      const index = action.payload
      this.state.pusherSettings[index].value = !this.state.pusherSettings[index].value
      cameraService.switchCamera(this.state.pusherSettings[index].value ? 1 : 0)
    } else if (action instanceof GiftAction) {
      this.state.hjPusher.openPngSeq(action.payload)
    } else if (action instanceof TimerAction) {
      if (action.payload != 0 && action.payload % 3600 == 0) {
        this.state.hour++
      }
      this.state.live_time = action.payload
    } else if (action instanceof RecorderAction) {
      this.recorder(action)
    }
  }

  private async recorder(action: RecorderAction) {
    const index = action.payload.index
    this.state.pusherSettings[index].value = !this.state.pusherSettings[index].value
    if (this.state.pusherSettings[index].value) { // 开始录制
      const context = getContext()
      this.state.path = context.tempDir // temp目录每次应用启动时清空
      const DATETIME = new DateTimeUtil()
      this.state.src = `${DATETIME.getDate()}_${DATETIME.getTime()}`
      const url = this.state.path + "/" + this.state.src + "." + this.state.ext
      const recorderInfo = new RecorderInfo()
      recorderInfo.recordUrl = url
      this.state.hjPusher.openRecorder(recorderInfo)
    } else {
      this.state.hjPusher.closeRecorder()
      const res = await applySaveVideo(getContext(), this.state.path, this.state.src, this.state.ext)
      if (res) {
        ToastUtils.showToast("视频已保存到本地")
      } else {
        ToastUtils.showToast("保存失败")
      }
    }
  }

  private openPusher() {
    this.state.hjPusher.openPusher(this.state.config)
    this.startTimer();
  }

  private closePusher(){
    this.state.hjPusher.closePusher()
    this.stopTimer();
  }

  private startTimer() {
    this.state.textTimerController.start();
  }

  private stopTimer() {
    this.state.textTimerController.reset();
  }
}
