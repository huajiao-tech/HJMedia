import { HJPlayer, LogMode } from "hjplayer"
import { NavStackUtil } from "../../util/NavStackUitl"

@Builder
export function PageBuilder() {
  PlayerPage()
}

@ComponentV2
export struct PlayerPage {
  featureList: Array<string> = ["直播播放器"]

  aboutToAppear(): void {
    let logLevel = 2;
    let logMode = LogMode.CONSOLE | LogMode.FILE;
    let logPath = '/data/storage/el2/base/haps/entry/files/';
    let bLog = true;
    let logSize = 1024 * 1024 * 5;
    let logCount = 5;
    HJPlayer.contextInit(bLog, logPath, logLevel, logMode, logSize, logCount);
  }

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
                NavStackUtil.jumpPush({ name: "PlayerSettingPage" })
              })
          })
      }
    }.title("PlayerPage")
  }
}