import { PusherXComponent } from '../component/PusherXComponent';
import { VideoStateData } from '../model/PusherPreviewModel';
import {
  PusherPreviewState, PusherPreviewStore,
} from '../store/PusherPreviewStore';
import {
  CameraAction,
  DoubleScreenAction,
  GiftAction,
  MicrophoneAction,
  PusherAction,
  RecorderAction,
  RecorderActionPayLoad,
  SelectorAction,
  TimerAction
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
              this.pusherStore.dispatch(new SelectorAction(1))
            } else if (item.item.type == "gift_self") {
              this.pusherStore.dispatch(new GiftAction(this.getUIContext().getHostContext()?.resourceDir + "/ShuangDanCaiShen"))
              this.pusherStore.dispatch(new SelectorAction(0))
            } else if (item.item.type == "doubleScreen") {
              this.pusherStore.dispatch(new DoubleScreenAction())
            }
          })
      })
      .templateId((item, index) => {
        return item.layout
      })
      .each(item => {})
  }
}