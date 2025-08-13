import { PusherOption } from "../model/SettingsModel";

@ComponentV2
export struct SettingRowComponent {
  @Param title: string = ""
  @Param options: PusherOption[] = [];
  @Param @Require selectedValue: number
  @Event $selectedValue: (value: number) => void = (value: number) => {};

  build() {
    Column() {
      Row() {
        Text(this.title)
          .fontSize(16)
          .fontColor(Color.Black)
        Row() {
          ForEach(this.options, (item: PusherOption) => {
            Row({ space: 3 }) {
              Radio({ value: item.option, group: this.title })
                .checked(this.selectedValue === item.value)
                .height(15)
                .aspectRatio(1)
                .onChange((isChecked) => {
                  if (isChecked) {
                    this.$selectedValue(item.value);
                  }
                })
              Text(item.option).fontSize(10)
            }
          }, (item: PusherOption) => item.option)
        }
      }
      .width('100%')
      .height(56)
      .justifyContent(FlexAlign.SpaceBetween)

      Divider().color('#f0f0f0').strokeWidth(1)
    }
  }
}