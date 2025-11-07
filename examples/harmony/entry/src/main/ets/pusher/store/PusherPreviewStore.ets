import { AudioCodecType, LogMode, PreviewInfo, HJPusher, RecorderInfo, VideoCodecType, SetWindowState } from 'hjpusher';
import { cameraService } from '../../camera/service/CameraService';
import { State, Store, Action } from '../../interface/Store';
import DateTimeUtil from '../../util/DateTimeUtils';
import { applySaveVideo } from '../../util/RecorderVideoUtils';
//import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from 'hjmediautils'

import { HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet } from '../../../../../../hjpusher/src/main/ets/HJFaceDetectMgr'

import {
  pusherPreset,
  PusherConfigPreset,
  PusherSetting,
  VideoStateData,
  PreviewAndPusherSelector
} from '../model/PusherPreviewModel';
import { AppStorageV2, window } from '@kit.ArkUI';
import { WindowUtils } from '../../util/WindowUtils';
import { ToastUtils } from '../../util/ToastUtils';
import { PusherLiveInfo } from '../../common/VideoConstant';
import {
  CameraAction,
  DoubleScreenAction,
  FaceuAction,
  FaceDetectAction,
  SpeechRecognizerAction,
  GiftAction,
  MicrophoneAction,
  PusherAction,
  RecorderAction,
  SelectorAction,
  TimerAction
} from './action/PusherPreviewAction';
import { NavStackUtil } from '../../util/NavStackUitl';
import { ImageReceiverService } from '../../common/ImageReceiverService';

@ObservedV2
export class PusherPreviewState implements State {
  @Trace kbps: VideoStateData = new VideoStateData("实时码率", 0, 'kbps')
  @Trace fps: VideoStateData = new VideoStateData("实时帧率", 0, 'fps')
  @Trace delay: VideoStateData = new VideoStateData("实时延迟", 0, 'ms')
  @Trace videoFormat: VideoStateData = new VideoStateData("视频编码格式", 0, 'H265')
  @Trace audioFormat: VideoStateData = new VideoStateData("音频编码格式", 0, 'AAC')
  @Trace playState: boolean = false
  @Trace live_time: number = 0
  @Trace hour: number = 0;
  @Trace bFaceProtected: boolean = false
  @Trace doubleScreen: boolean = false
  @Trace bFaceu: boolean = false
  @Trace bFaceDetect: boolean = false
  @Trace bSpeechRecognizer: boolean = false;
  @Trace selectorX: number | string = "50%"
  @Trace selectorY: number | string = 0
  path: string = ""
  src: string = ""
  ext: string = "mp4"
  pusherSettings: PusherSetting[] = [
    new PusherSetting("microphone", "left", $r("app.media.microphone_1_icon"), $r("app.media.microphone_0_icon"),  false),
    new PusherSetting("recorder", "left", $r("app.media.video_recorder_1_icon"), $r("app.media.video_recorder_0_icon"),  false),
    new PusherSetting("doubleScreen", "left", $r("app.media.double_screen_icon"), $r("app.media.double_screen_icon"),  false),
    new PusherSetting("camera", "right", $r("app.media.camera_front"), $r("app.media.camera_rear"),  true),
    new PusherSetting("facedetect", "right", $r("app.media.facedetect_close"), $r("app.media.facedetect_open"),  false),
    new PusherSetting("faceu", "right", $r("app.media.faceu_close"), $r("app.media.faceu_open"),  false),
    new PusherSetting("speechRecognizer", "right", $r("app.media.speechRecognizer"), $r("app.media.speechRecognizer"),  false),
    new PusherSetting("gift_self", "right", $r("app.media.gift_icon_1"), $r("app.media.gift_icon_1"),  false),
  ]
  textTimerController: TextTimerController = new TextTimerController()
  hjPusher: HJPusher = new HJPusher()
  config: PusherConfigPreset = AppStorageV2.connect(PusherConfigPreset, () => pusherPreset)!
  previewAndPusherSelector: PreviewAndPusherSelector = new PreviewAndPusherSelector()
  enableFaceDetector: boolean = false
  imageReceiverService: ImageReceiverService = new ImageReceiverService(getContext() as Context)

  get pusherLiveInfo() {
    return [this.videoFormat, this.audioFormat, this.kbps, this.fps, this.delay]
  }

  @Monitor("doubleScreen")
  doubleScreenChange() {
    this.hjPusher.setDoubleScreen(this.doubleScreen)
  }
}

export class PusherPreviewStore implements Store<PusherPreviewState> {
  state = new PusherPreviewState()
  getState = () => this.state

  // private m_faceDetectRet : number = HJFaceDetectMgrRet.unknown;
  private m_faceDetectMgr : HJFaceDetectMgr = new HJFaceDetectMgr();

  aboutToAppear() {
    this.state.videoFormat.unit = this.state.config.videoConfig.codecID == VideoCodecType.HJCodecH264 ? "H264" : "H265"
    if (!this.state.config.isLandScape) {
      this.state.selectorX = 0
      this.state.selectorY = "50%"
    }
    this.state.doubleScreen = NavStackUtil.getParams<boolean>()
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
      const strJson: ESObject = JSON.parse(str)
      console.info("### receivecallbak #type is ", strJson.type, strJson.msgInfo);
      if (strJson.type == 8) {
        const liveInfo = JSON.parse(strJson.msgInfo) as PusherLiveInfo
        this.state.kbps.value = liveInfo.kbps
        this.state.fps.value = liveInfo.fps
        this.state.delay.value = liveInfo.delay
      }
    });

    this.state.hjPusher.setFaceProtected(false);

    this.state.hjPusher.setVideoEncQuantOffset(this.state.config.roiQuantOffset);

    cameraService.initCamera(getContext() as Context);
    cameraService.bindSurfaceId(previewSurfaceId.toString());
    cameraService.startPreview(this.state.pusherSettings[3].value ? 1 : 0)

    if (this.state.enableFaceDetector) {
      this.state.imageReceiverService.getReceivingSurfaceId().then((surfaceId) => {
        this.state.hjPusher.setWindow({
          surfaceId: surfaceId,
          width: this.state.config.videoConfig.width,
          height: this.state.config.videoConfig.height,
          state: SetWindowState.TARGET_CREATE
        })
      })
    }
  }

  aboutToDisappear() {
    if (this.state.config.isScreenOn) {
      WindowUtils.setWindowKeepScreenOn(false)
    }
    cameraService.releaseCamera()
    this.state.hjPusher.closePusher()
    this.state.hjPusher.closePreview()
    this.state.hjPusher.destroyPusher()
    if (this.state.enableFaceDetector) {
      this.state.imageReceiverService.release()
    }
  }

  onPageShow(): void {
    WindowUtils.setWindowLayoutFullScreen(true)
    if (this.state.config.isLandScape) {
      WindowUtils.setOrientation(window.Orientation.LANDSCAPE)
    }
  }

  onPageHide(): void {
    WindowUtils.setWindowLayoutFullScreen(false)
    if (this.state.config.isLandScape) {
      WindowUtils.setOrientation(window.Orientation.UNSPECIFIED)
    }
  }

  async faceDetectCtrl(index : number)
  {
      if (this.state.bFaceu)
      {
          ToastUtils.showToast("检测到faceu仍在打开状态，请先关闭")
          return;
      }
      if (!this.state.bFaceDetect)  //open
      {
          this.m_faceDetectMgr.registerFaceInfoCb((width:number, height:number, faceInfo:string)=>
          {
              this.state.hjPusher.setFaceInfo(width, height, faceInfo);
          });
          this.m_faceDetectMgr.registerNativeSourceOpenCb(() =>
          {
              this.state.hjPusher.nativeSourceOpen(true);
              return 0;
          });
          this.m_faceDetectMgr.registerNativeSourceAcquireCb((): HJNativeSourceData | null =>
          {
              return this.state.hjPusher.nativeSourceAcquire();
          });
          this.m_faceDetectMgr.registerNativeSourceCloseCb(()=>
          {
              this.state.hjPusher.nativeSourceClose();
          });
          let faceDetectRet: number = await this.m_faceDetectMgr.openFaceDetect();

          if (faceDetectRet != HJFaceDetectMgrRet.ok)
          {
              await this.m_faceDetectMgr.closeFaceDetect();
              console.log("openFaceDetect error");
              ToastUtils.showToast("打开人脸检测失败")
          }
          else
          {
              this.state.bFaceDetect = true;
              ToastUtils.showToast("打开人脸检测成功")
              this.switchIcon(index);
          }
      }
      else //close
      {
          await this.m_faceDetectMgr.closeFaceDetect();
          this.state.bFaceDetect = false;
          this.switchIcon(index);
      }
  }

  faceutCtrl(index: number, url: string)
  {
      if (!this.state.bFaceDetect)
      {
          ToastUtils.showToast("人脸检测模块未打开，请打开")
          return;
      }
      if (!this.state.bFaceu)  //open
      {
            this.state.hjPusher.openFaceu(url);
            this.state.bFaceu = true;
            this.switchIcon(index);
      }
      else //close
      {
          this.state.hjPusher.closeFaceu();
          this.state.bFaceu = false;
          this.switchIcon(index);
      }
  }

  switchIcon(index : number)
  {
      this.state.pusherSettings[index].value = !this.state.pusherSettings[index].value
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
    } else if (action instanceof SelectorAction) {
      this.state.previewAndPusherSelector.currentIndex = action.payload
      if (action.payload == 1) {
        // 推礼物
        this.state.doubleScreenChange()
        this.state.hjPusher.setGiftPusher(true)
      } else {
        // 不推礼物
        this.state.doubleScreenChange()
        this.state.hjPusher.setGiftPusher(false)
      }
    } else if (action instanceof DoubleScreenAction) {
      this.state.doubleScreen = !this.state.doubleScreen

      // //fixme lfs only test
      // this.state.bFaceProtected = !this.state.bFaceProtected;
      // if (this.state.bFaceProtected)
      // {
      //   this.state.hjPusher.setFaceProtected(true);
      // }
      // else
      // {
      //   this.state.hjPusher.setFaceProtected(false);
      // }

    }
    else if (action instanceof FaceuAction) {
        this.faceutCtrl(action.payload, action.url);
        this.state.doubleScreenChange()      
    }
    else if (action instanceof FaceDetectAction)
    {
        console.log("FaceDetectAction enter");
        this.faceDetectCtrl(action.payload);
    }
    else if (action instanceof SpeechRecognizerAction)
    {
        this.state.bSpeechRecognizer = !this.state.bSpeechRecognizer;
        if (this.state.bSpeechRecognizer)
        {
          this.state.hjPusher.openSpeechRecognizer((buffer: ArrayBuffer) =>
          {
            console.log(`Speech recognizer buffer received, size: ${buffer.byteLength}`);

          });
          ToastUtils.showToast("打开语音获取")
        }
        else
        {
          this.state.hjPusher.closeSpeechRecognizer();
          ToastUtils.showToast("关闭语音获取")
        }
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
    this.state.hjPusher.openPusher(this.state.config, {uid: 2342, device: "Harmony", sn: "HJPusher"}, (str: string) => {
      console.log("wkshhh", str)
    })
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
