import { SettingRowComponent } from '../component/SettingRowComponent';
import {
  SettingsState,
  SettingsStore,
  VideoBitrateAction,
  AdaptiveBitrateAction,
  ResetSettingsAction,
  SaveSettingsAction
} from '../store/SettingsStore';

@Builder
export function PageBuilder() {
  SettingsPage()
}

@ComponentV2
export struct SettingsPage {
  settingsStore: SettingsStore = new SettingsStore()
  state: SettingsState = this.settingsStore.getState()

  aboutToAppear(): void {
    this.settingsStore.aboutToAppear()
  }

  build() {
    NavDestination() {
      Column() {
        Column({ space: 8 }) {
          Repeat(this.state.pusherVideoSettings)
            .each(item => {
              SettingRowComponent({
                title: item.item.title,
                options: item.item.options,
                selectedValue: item.item.value!!,
              })
            })
          Column() {
            Row() {
              Text('视频码率')
                .fontSize(16)
              Text(`${this.state.videoBitrate}kbps`)
                .fontSize(16)
                .fontColor('#007AFF')
            }
            .width('100%')
            .justifyContent(FlexAlign.SpaceBetween)
            .padding({ top: 16 })

            Slider({
              value: this.state.videoBitrate,
              min: 400,
              max: 2200,
              step: 10
            })
              .blockColor('#007AFF')
              .trackColor('#f0f0f0')
              .selectedColor('#007AFF')
              .onChange((value) => {
                this.settingsStore.dispatch(new VideoBitrateAction(Math.round(value)));
              })
              .padding({ bottom: 8 })

            Row() {
              Checkbox()
                .select(this.state.isAdaptiveBitrateEnabled)
                .selectedColor('#007AFF')
                .onChange((value) => {
                  this.settingsStore.dispatch(new AdaptiveBitrateAction(value));
                })
              Text('启动码率自适应')
                .fontSize(15)
                .fontColor(Color.Gray)
                .onClick(() => this.settingsStore.dispatch(new AdaptiveBitrateAction(!this.state.isAdaptiveBitrateEnabled)))
            }
            .justifyContent(FlexAlign.Start)
            .width('100%')
          }
          .padding({ bottom: 16 })

          Divider().color('#f0f0f0').strokeWidth(8)

          Repeat(this.state.pusherAudioSettings)
            .each(item => {
              SettingRowComponent({
                title: item.item.title,
                options: item.item.options,
                selectedValue: item.item.value!!,
              })
            })
        }
        .width('100%')
        .layoutWeight(1)

        Row({ space: 16 }) {
          Button('重置', { type: ButtonType.Normal })
            .layoutWeight(1)
            .height(44)
            .backgroundColor(Color.White)
            .fontColor(Color.Black)
            .onClick(() => {
              this.settingsStore.dispatch(new ResetSettingsAction());
            })

          Button('保存', { type: ButtonType.Normal })
            .layoutWeight(1)
            .height(44)
            .backgroundColor('#007AFF')
            .onClick(() => {
              this.settingsStore.dispatch(new SaveSettingsAction());
            })
        }
        .width('100%')
        .padding(16)
        .border({ width: { top: 1 }, color: '#f0f0f0' })
      }
      .padding({ left: 16, right: 16})
      .width('100%')
      .height('100%')
      .backgroundColor('#f7f7f7')
    }
    .title('设置')
  }
}