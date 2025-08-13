import { VideoConstant } from '../../common/VideoConstant';
import { PusherXComponent } from '../component/PusherXComponent';
import { PusherConfigPreset, pusherPreset, VideoStateData } from '../model/PusherPreviewModel';
import {
  CameraAction,
  GiftAction,
  MicrophoneAction,
  PusherAction,
  PusherPreviewState,
  PusherPreviewStore,
  RecorderAction,
  RecorderActionPayLoad,
  TimerAction
} from '../store/PusherPreviewStore';
import { WindowUtils } from '../../util/WindowUtils';
import { AppStorageV2, window } from '@kit.ArkUI';

@Builder
export function PageBuilder() {
  PusherPreviewPage()
}

const TAG = 'PusherPreviewPage'

@ComponentV2
struct PusherPreviewPage {
  pusherStore: PusherPreviewStore = new PusherPreviewStore()
  state: PusherPreviewState = this.pusherStore.getState()
  config = AppStorageV2.connect(PusherConfigPreset, () => pusherPreset)!

  aboutToAppear(): void {
    // VideoConstant.VIDEO_STATE_LIST.forEach((name) => {
    //   this.state.videoStateData.push(new VideoStateData(name, '1.0.0', ''))
    // })
    this.pusherStore.aboutToAppear()
  }

  aboutToDisappear(): void {
    this.pusherStore.aboutToDisappear()
  }

  onPageShow(): void {
    WindowUtils.setWindowLayoutFullScreen(true)
    if (this.config.isLandScape) {
      WindowUtils.setOrientation(window.Orientation.LANDSCAPE)
    }
  }

  onPageHide(): void {
    WindowUtils.setWindowLayoutFullScreen(false)
    if (this.config.isLandScape) {
      WindowUtils.setOrientation(window.Orientation.UNSPECIFIED)
    }
  }

  build() {
    NavDestination() {
      Column() {
        Stack({ alignContent: Alignment.Bottom }) {
          RelativeContainer() {
            PusherXComponent({hjPusher: this.state.hjPusher})
          }
          .size({ width: "100%", height: "100%" })

          Column() {
            this.PusherTimer()
            Column() {
              // this.PusherDetail()
              this.BottomSetting()
            }
          }
          .justifyContent(FlexAlign.SpaceBetween)
          .size({ width: "100%", height: "100%" })
        }
        .size({ width: "100%", height: "100%" })
      }
      .size({ width: "100%", height: "100%" })
    }
    .onShown(() => {
      this.onPageShow();
    })
    .onHidden(() => {
      this.onPageHide();
    })
    .hideTitleBar(true)
  }

  @Builder
  PusherTimer() {
    Row() {
      Row()
        .width(8)
        .aspectRatio(1)
        .borderRadius("50%")
        .backgroundColor("#ffff0000")
        .margin({ right: 3 })
      Text(this.state.hour == 0 ? "" : "" + this.state.hour + ':')
        .fontColor(Color.White)
        .fontSize(16)
      TextTimer({ controller: this.state.textTimerController })
        .format('mm:ss')
        .fontColor(Color.White)
        .fontSize(16)
        .onTimer((utc: number, elapsedTime: number) => {
          this.pusherStore.dispatch(new TimerAction(elapsedTime))
        })
    }
    .margin({ top: 40, right: 20 })
    .width("100%")
    .justifyContent(FlexAlign.End)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  PusherDetail() {
    Column() {
      ForEach(this.state.videoStateData, (item: VideoStateData) => {
        if (item) {
          Text(`${item.name}：${item.value} ${item.unit}`)
            .margin({ bottom: 10 })
        }
      })
    }
    .width("100%")
    .alignItems(HorizontalAlign.Start)
  }

  @Builder
  BottomSetting() {
    Row() {
      Row() {
        this.PusherSettingList("left")
      }
      .layoutWeight(1)
      .justifyContent(FlexAlign.SpaceEvenly)

      Image(this.state.playState ? $r("app.media.play_1_icon") : $r("app.media.play_0_icon"))
        .width(50).aspectRatio(1).objectFit(ImageFit.Fill)
        .onClick(() => {
          this.pusherStore.dispatch(new PusherAction())
        })

      Row() {
        this.PusherSettingList("right")
      }
      .layoutWeight(1)
      .justifyContent(FlexAlign.SpaceEvenly)
    }.width('100%')
    .margin({ bottom: 20 })
    .justifyContent(FlexAlign.SpaceEvenly)
  }

  @Builder
  PusherSettingList(layout: string) {
    Repeat(this.state.pusherSettings)
      .template(layout, item => {
        Image(item.item.value ? item.item.primaryIcon : item.item.secondaryIcon)
          .width(30).height(30).objectFit(ImageFit.Fill)
          .onClick(() => {
            if (item.item.type == "microphone") {
              this.pusherStore.dispatch(new MicrophoneAction(item.index))
            } else if (item.item.type == "recorder") {
              this.pusherStore.dispatch(new RecorderAction(new RecorderActionPayLoad(item.index, "url")))
            } else if (item.item.type == "camera") {
              this.pusherStore.dispatch(new CameraAction(item.index))
            } else if (item.item.type == "gift") {
              this.pusherStore.dispatch(new GiftAction(this.getUIContext().getHostContext()?.resourceDir + "/ShuangDanCaiShen"))
            }
          })
      })
      .templateId((item, index) => {
        return item.layout
      })
      .each(item => {})
  }
}