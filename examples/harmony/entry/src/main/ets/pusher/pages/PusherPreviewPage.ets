import { PusherXComponent } from '../component/PusherXComponent';
import { VideoStateData } from '../model/PusherPreviewModel';
import {
  PusherPreviewState, PusherPreviewStore,
} from '../store/PusherPreviewStore';
import {
  CameraAction,
  DoubleScreenAction,
  GiftAction,
  FaceuAction,
  FaceDetectAction,
  MicrophoneAction,
  PusherAction,
  RecorderAction,
  RecorderActionPayLoad,
  SelectorAction,
  TimerAction,
  SpeechRecognizerAction
} from '../store/action/PusherPreviewAction';

@Builder
export function PageBuilder() {
  PusherPreviewPage()
}

const TAG = 'PusherPreviewPage'

@ComponentV2
struct PusherPreviewPage {
  pusherStore: PusherPreviewStore = new PusherPreviewStore()
  state: PusherPreviewState = this.pusherStore.getState()

  aboutToAppear(): void {
    this.pusherStore.aboutToAppear()
  }

  aboutToDisappear(): void {
    this.pusherStore.aboutToDisappear()
  }

  onPageShow(): void {
    this.pusherStore.onPageShow()
  }

  onPageHide(): void {
    this.pusherStore.onPageHide()
  }

  build() {
    NavDestination() {
      RelativeContainer() {
        PusherXComponent({ hjPusher: this.state.hjPusher })
        Repeat([0, 1])
          .each(item => {
            Column()
              .width(this.state.config.isLandScape ? "50%" : "100%")
              .height(this.state.config.isLandScape ? "100%" : "50%")
              .position({ x: item.index == 0 ? 0 : this.state.selectorX, y: item.index == 0 ? 0 : this.state.selectorY })
              .border({
                color: "#7CFC00",
                width: item.index === this.state.previewAndPusherSelector.currentIndex ? 4 : 0
              })
              .onClick(() => {
                this.pusherStore.dispatch(new SelectorAction(item.index))
              })
              .visibility(this.state.doubleScreen ? Visibility.Visible : Visibility.None)
          })

        Column() {
          this.PusherTimer()
          Column() {
            this.PusherDetail()
            this.BottomSetting()
          }
        }
        .justifyContent(FlexAlign.SpaceBetween)
        .size({ width: "100%", height: "100%" })
        .hitTestBehavior(HitTestMode.None)
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
      ForEach(this.state.pusherLiveInfo, (item: VideoStateData) => {
        if (item) {
          Text(`${item.name}：${item.value == 0 ? "--" : item.value} ${item.unit}`)
            .fontSize(10)
            .fontColor(Color.White)
            .margin({ bottom: 10 })
        }
      })
    }
    .width("100%")
    .padding({ left: 20 })
    .alignItems(HorizontalAlign.Start)
  }

  @Builder
  BottomSetting() {
    Scroll() {
      Row() {
        this.PusherSettingList("left")

        Image(this.state.playState ? $r("app.media.play_1_icon") : $r("app.media.play_0_icon"))
          .width(50).aspectRatio(1).objectFit(ImageFit.Fill)
          .margin({ left: 10, right: 10 }) // Add margin to the play button for spacing
          .onClick(() => {
            this.pusherStore.dispatch(new PusherAction())
          })

        this.PusherSettingList("right")
      }
      .padding({ left: 10, right: 10 }) // Add padding to the Row for start/end spacing
      .alignItems(VerticalAlign.Center)
    }
    .scrollable(ScrollDirection.Horizontal)
    .scrollBar(BarState.Off)
    .width('100%')
    .margin({ bottom: 20 })
    .align(Alignment.Center)
  }

  @Builder
  PusherSettingList(layout: string) {
    Repeat(this.state.pusherSettings)
      .template(layout, item => {
        Image(item.item.value ? item.item.primaryIcon : item.item.secondaryIcon)
          .width(30).height(30).objectFit(ImageFit.Fill)
          .margin({ left: 8, right: 8 })
          .onClick(() => {
            if (item.item.type == "microphone") {
              this.pusherStore.dispatch(new MicrophoneAction(item.index))
            } else if (item.item.type == "recorder") {
              this.pusherStore.dispatch(new RecorderAction(new RecorderActionPayLoad(item.index, "url")))
            } else if (item.item.type == "camera") {
              this.pusherStore.dispatch(new CameraAction(item.index))
            } else if (item.item.type == "gift") {
              this.pusherStore.dispatch(new GiftAction(this.getUIContext().getHostContext()?.resourceDir + "/ShuangDanCaiShen"))
              this.pusherStore.dispatch(new SelectorAction(1))
            } else if (item.item.type == "gift_self") {
              this.pusherStore.dispatch(new GiftAction(this.getUIContext().getHostContext()?.resourceDir + "/ShuangDanCaiShen"))
              this.pusherStore.dispatch(new SelectorAction(0))
            } else if (item.item.type == "doubleScreen") {
              this.pusherStore.dispatch(new DoubleScreenAction())
            } else if (item.item.type == "faceu"){
              this.pusherStore.dispatch(new FaceuAction(item.index, "/data/storage/el2/base/haps/entry/files/90237_1"));//this.getUIContext().getHostContext()?.resourceDir + "/800624_1_1504602801"))
            } else if (item.item.type == "facedetect"){
              this.pusherStore.dispatch(new FaceDetectAction(item.index))
            } else if (item.item.type == "speechRecognizer"){
              this.pusherStore.dispatch(new SpeechRecognizerAction(item.index))
            }
          })
      })
      .templateId((item, index) => {
        return item.layout
      })
      .each(item => {})
  }
}