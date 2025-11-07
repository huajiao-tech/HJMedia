import { NavStackUtil } from '../../util/NavStackUitl';
import { PreviewAction, PusherState, PusherStore } from '../store/PusherStore';

@Builder
export function PageBuilder() {
  PusherPage()
}

@Preview
@ComponentV2
export struct PusherPage {
  pusherStore: PusherStore = new PusherStore()
  state: PusherState = this.pusherStore.getState()

  build() {
    NavDestination() {
      Column() {
        this.configList()
        Row() {
          Image($r("app.media.preview_icon"))
            .width(80)
            .aspectRatio(1)
            .onClick(() => {
              this.pusherStore.dispatch(new PreviewAction())
            })
        }
        .padding({ left: 30, right: 30 })
        .width('100%')
        .margin({ top: 100 })
        .justifyContent(FlexAlign.Center)
      }
      .padding({ left: 20, right: 20 })
    }
    .title('推流')
    .menus([{ value: "", icon: $r("app.media.settings_icon"), action: () => {
      NavStackUtil.jumpPush({ name: "SettingsPage" })
    }}])
  }

  @Builder
  configList() {
    Repeat(this.state.pusherConfigArray)
      .template("string", (item) => {
        Column() {
          Row() {
            Text(item.item.title)
              .fontSize(16)

            TextInput({ text: item.item.value as string})
              .fontSize(16)
              .width(180)
              .onChange((value) => {
                item.item.value = value
              })
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)

          if (item.index === this.state.pusherConfigArray.length - 1) {
            Blank()
              .color('#f5f5f5')
              .height(5)
              .margin({ top: 10, bottom: 10 })
          } else {
            Divider()
              .color('#f5f5f5')
              .strokeWidth(1)
              .margin({ top: 10, bottom: 10 })
          }
        }
      })
      .template("boolean", (item) => {
        Column() {
          Row() {
            Text(item.item.title)
              .fontSize(16)

            Toggle({ type: ToggleType.Switch, isOn: item.item.value as boolean })
              .width(40)
              .height(25)
              .onChange((value) => {
                item.item.value = value
              })
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)

          Blank()
            .color('#f5f5f5')
            .height(5)
            .margin({ top: 10, bottom: 10 })
        }
      })
      .template("slider", (item) => {
        Column() {
          Row() {
            Text(item.item.title)
              .fontSize(16)
            Text(`${item.item.value}`)
              .fontSize(16)
              .fontColor('#007AFF')
          }
          .width('100%')
          .justifyContent(FlexAlign.SpaceBetween)
          .padding({ top: 16 })

          Slider({
            value: item.item.value as number,
            min: item.item.min,
            max: item.item.max,
            step: 1
          })
            .blockColor('#007AFF')
            .trackColor('#f0f0f0')
            .selectedColor('#007AFF')
            .onChange((value) => {
              item.item.value = Math.round(value)
            })
            .padding({ bottom: 8 })
        }
      })
      .templateId((item, index) => {
        return item.type
      })
      .each(item => {})
  }
}