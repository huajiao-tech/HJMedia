import { HJPlayer, LogMode, HJPlayerContextInitConfig, HJOptionDir, HJPlayerDownloadInfo } from "hjplayer"
import { NavStackUtil } from "../../util/NavStackUitl"
import { HJDownloadNotify } from '../../common/VideoConstant';

@Builder
export function PageBuilder() {
  PlayerPage()
}

@ComponentV2
export struct PlayerPage {
  featureList: Array<string> = ["播放器", "下载", "取消"]
  workDir = '/data/storage/el2/base/haps/entry/files/'
  res_url = 'https://live-pull-3.huajiao.com/main/HJ_0_tc_3_main__h265_272144016_1770275649395_8293_O.flv?txSecret=e11ae520ff8be6d0c50c3767168a96eb&txTime=698594E7'
  res_rid = ''
  @Local downloadProgress: number = 0

  aboutToAppear(): void {
    let logLevel = 2;
    let logMode = LogMode.CONSOLE | LogMode.FILE;
    let logPath = this.workDir;
    let bLog = true;
    let logSize = 1024 * 1024 * 5;
    let logCount = 5;
    let config: HJPlayerContextInitConfig = {
      valid: bLog,
      logDir: logPath,
      logLevel: logLevel,
      logMode: logMode,
      maxSize: logSize,
      maxFiles: logCount,
      medias_dir: this.workDir +  'hjmedias',
      medias_cache_max: 200,
      other_dirs_options: [
        {
          name: this.workDir + "gift",
          max: 100
        } as HJOptionDir,
        {
          name: this.workDir + "shorts",
          max: 200
        } as HJOptionDir
      ],
      download_retry_max: 10
    };
    HJPlayer.contextInit(config);
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
                if (feature.item === "播放器") {
                  NavStackUtil.jumpPush({ name: "PlayerSettingPage" })
                } else if (feature.item === "下载") {
                  let info: HJPlayerDownloadInfo = {
                    url: this.res_url,
                    dir: this.workDir +  'hjmedias',
                    rid: "",
                    preCacheSize: -1,
                    priority: 1
                  };
                  this.res_rid = HJPlayer.download(info, (str: string) => {
                    const notify = JSON.parse(str) as HJDownloadNotify;
                    this.downloadProgress = notify.validSize * 100 / notify.totalSize;
                    console.info("PlayerPage notify type:" + notify.type + ", rid: " + notify.rid + ", totalSize:" + notify.totalSize + ", validSize:" + notify.validSize + ", status:" + notify.status + ", progress:" + this.downloadProgress);
                  });
                  console.info("PlayerPage res rid: " + this.res_rid);
                } else if (feature.item === "取消") {
                  HJPlayer.cancelDownload(this.res_rid);
                }
              })
          })
      }

      Progress({ value: this.downloadProgress, total: 100, type: ProgressType.Linear })
        .width('80%')
        .margin({ top: 20 })
      Text(`${this.downloadProgress.toFixed(1)}%`)
        .fontSize(14)
        .margin({ top: 5 })
    }.title("PlayerPage")
  }
}