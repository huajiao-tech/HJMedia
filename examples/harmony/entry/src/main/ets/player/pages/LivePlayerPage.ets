import { HJPlayerSourceType, HJPlayerType } from "hjplayer"
import { PlayerSwiper } from "../component/PlayerSwiper"
import { PlayerXComponent } from "../component/PlayerXComponent"
import { GiftAction, LivePlayerState, LivePlayerStore, SoundAction } from "../store/LivePlayerStore"

@Builder
export function PageBuilder() {
  LivePlayerPage()
}

@ComponentV2
export struct LivePlayerPage {
  @Provider("LivePlayerStore") store: LivePlayerStore = new LivePlayerStore()
  state: LivePlayerState = this.store.getState()

  aboutToAppear(): void {
    this.store.aboutToAppear()
  }

  aboutToDisappear(): void {
    this.store.aboutToDisappear()
  }

  build() {
    NavDestination() {
      RelativeContainer() {
        PlayerSwiper()
        if (this.state.isGiftShow) {
          PlayerXComponent({
            url: this.getUIContext().getHostContext()?.resourceDir + "/ShuangDanCaiShen.mp4",
            sourceType: HJPlayerSourceType.HJPlayerSourceType_SPLITSCREEN,
            playerType: HJPlayerType.HJPlayerType_VOD,
          })
            .hitTestBehavior(HitTestMode.Transparent)
        }
        this.BottomSetting()
      }
    }
    .hideTitleBar(true)
    .onShown(() => {
      this.store.onPageShow()
    })
    .onHidden(() => {
      this.store.onPageHide()
    })
  }

  @Builder
  BottomSetting() {
    Row() {
      Row() {
        this.PusherSettingList("left")
      }
      .layoutWeight(1)
      .justifyContent(FlexAlign.SpaceEvenly)

      Row() {
        this.PusherSettingList("right")
      }
      .layoutWeight(1)
      .justifyContent(FlexAlign.SpaceEvenly)
    }.width('100%')
    .margin({ bottom: 20 })
    .justifyContent(FlexAlign.SpaceEvenly)
    .alignRules({
      bottom: { anchor: "__container__", align: VerticalAlign.Bottom },
    })
  }

  @Builder
  PusherSettingList(layout: string) {
    Repeat(this.state.pusherSettings)
      .template(layout, item => {
        Image(item.item.value ? item.item.primaryIcon : item.item.secondaryIcon)
          .width(30).height(30).objectFit(ImageFit.Fill)
          .onClick(() => {
            if (item.item.type == "sound") {
              this.store.dispatch(new SoundAction(0))
            } else if (item.item.type == "gift") {
              this.store.dispatch(new GiftAction(this.getUIContext().getHostContext()?.resourceDir + "/ShuangDanCaiShen.mp4"))
            }
          })
      })
      .templateId((item, index) => {
        return item.layout
      })
      .each(item => {})
  }
}