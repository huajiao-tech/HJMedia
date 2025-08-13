@Builder
export function PageBuilder() {
  PlayerPage()
}

@ComponentV2
export struct PlayerPage {
  @Local featureList: Array<string> = ["直播播放器", "点播播放器", "多流播放器"]

  build() {
    NavDestination() {
      Column() {
        Repeat(this.featureList)
          .each((feature) => {
            Button(feature.item, { stateEffect: true, type: ButtonType.Capsule })
              .width('80%')
              .height(40)
              .margin(20)
              .onClick(() => {

              })
          })
      }
    }.title("PlayerPage")
  }
}