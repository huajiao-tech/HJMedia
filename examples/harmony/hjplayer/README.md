![](https://img.shields.io/badge/license-LGPL2.1-blue)

# hjplayer

hjplayer属于HJMedia媒体开源的一部分， HJMedia SourceCode: [huajiao-tech/HJMedia · GitHub](https://github.com/huajiao-tech/HJMedia)

## 简介

hjplayer是一款轻量级、高性能的鸿蒙平台媒体播放工具，支持直播流和带透明通道的视频礼物点播播放，适用于多种媒体播放场景。

## 下载安装

`ohpm install @hj-live/hjplayer`

## 主要功能

* 多媒体格式支持：
    - RTMP直播流播放
    - MP4动画礼物播放
    - 左右分屏带透明通道的视频礼物播放
* 音频控制：
    - 静音功能
* 播放控制：
    - 实时播放状态监控
    - 自定义播放地址配置

## 使用说明

1. 申请权限
    - ohos.permission.INTERNET

上述权限已经在当前har包module.json中配置完成。

2. 初始化推流器日志输出

```typescript
let logLevel = 2;
let logMode = LogMode.CONSOLE | LogMode.FILE;
let logPath = '/data/storage/el2/base/haps/entry/files/playerLog/';
let bLog = true;
let logSize = 1024 * 1024 * 5;
let logCount = 5;
HJPlayer.contextInit(bLog, logPath, logLevel, logMode, logSize, logCount);
```

3. 创建播放器

```typescript
// 创建一个HJPlayer实例
hjPlayer: HJPlayer = new HJPlayer()

// 创建一个播放器实例
this.state.hjPlayer.createPlayer()
// 打开播放器
this.state.hjPlayer.openPlayer(this.state.openPlayerInfo, (str: string) => {
  const strJson: ESObject = JSON.parse(str)
  console.info("uio123", strJson.type, strJson.msgInfo);
  if (strJson.type == HJPlayerNotifyType.HJ_PLAYER_NOTIFY_CLOSEDONE) { // 可以释放播放器资源
    if (this.state.openPlayerInfo.playerType == HJPlayerType.HJPlayerType_LIVESTREAM) {
      this.state.hjPlayer.exitPlayer()
      this.state.hjPlayer.destroyPlayer()
    }
  }
}, { uid: 2342, device: "Harmony", sn: "HJPlayer" }, (str: string) => {
  console.info("uio123", str)
})
```

4. 显示播放画面
```typescript
// 给底层渲染线程增加一个输出目的，此处增加一个XComponent用于预览, 并继承XComponentController管理XComponent生命周期
XComponent({
  type: XComponentType.SURFACE,
  controller: this.playerXComponentController
})

onSurfaceCreated(surfaceId: string): void {
  this.playerXComponentStore.dispatch(new WindowAction({
    surfaceId: surfaceId,
    width: 1,
    height: 1,
    state: SetWindowState.TARGET_CREATE
  }))
}

onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
  this.playerXComponentStore.dispatch(new WindowAction({
    surfaceId: surfaceId,
    width: rect.surfaceWidth,
    height: rect.surfaceHeight,
    state: SetWindowState.TARGET_CHANGE
  }))
}

onSurfaceDestroyed(surfaceId: string) {
  this.playerXComponentStore.dispatch(new WindowAction({
    surfaceId: surfaceId,
    width: 0,
    height: 0,
    state: SetWindowState.TARGET_DESTROY
  }))
}
```

具体播放器使用流程可详见Demo代码  [Demo源码](https://github.com/huajiao-tech/HJMedia)

## 注意事项

首先需要打开播放器，然后可以调用setWindow(setWindowInfo: SetWindowInfo)增加输出目的，如XComponent

保证setWindow()调用时机问题：保证setWindow()openPlayer()之后调用，注意setWindow()中state参数有三种状态，确保正确传递（可参考demo中使用方式）

```typescript
export enum SetWindowState {
  TARGET_CREATE = 0, // 创建，增加surface时使用
  TARGET_CHANGE = 1, // 修改，surface宽高变化时使用
  TARGET_DESTROY = 2 //  销毁，surface销毁时使用 
}
```