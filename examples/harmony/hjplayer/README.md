![](https://img.shields.io/badge/license-LGPL2.1-blue)

# HJPlayer

HJPlayer属于HJMedia媒体开源的一部分， HJMedia SourceCode: [huajiao-tech/HJMedia · GitHub](https://github.com/huajiao-tech/HJMedia)

## 简介

HJPlayer是一款专注于直播场景的高性能播放器。它基于高度模块化、可扩展的插件化架构构建，支持多实例运行与灵活的视频后处理，并对首屏速度和播放延迟等核心指标进行了系统性优化，旨在为复杂的直播应用提供稳定、流畅的观看体验。

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

保证setWindow()调用时机问题：保证setWindow()openPlayer()之后调用，注意setWindow()中state参数有三种状态，详见下文SetWindowState介绍，确保正确传递（可参考demo中使用方式）

## API介绍

### HJPlayer

#### `contextInit(valid: boolean, logDir: string, logLevel: number, logMode: number, maxSize: number, maxFiles: number)`

初始化播放器上下文。

-   `valid`: `boolean` - 是否开启日志。
-   `logDir`: `string` - 日志目录。
-   `logLevel`: `number` - 日志级别。
-   `logMode`: `number` - 日志模式 (见 `LogMode`)。
-   `maxSize`: `number` - 单个日志文件最大大小。
-   `maxFiles`: `number` - 最多日志文件数量。

#### `preloadUrl(url: string)`

预加载URL。

-   `url`: `string` - 要预加载的URL，秒开场景下可使用此方法。

#### `createPlayer()`

创建一个播放器实例。

#### `destroyPlayer()`

销毁一个播放器实例。

#### `setWindow(setWindowInfo: SetWindowInfo)`

设置渲染窗口。

-   `setWindowInfo`: `SetWindowInfo` - 窗口信息 (见 `SetWindowInfo`)。

#### `openPlayer(openPlayerInfo: OpenPlayerInfo, stateCall: (str: string) => void, stateInfo: MediaStateInfo, statCall: (str: string) => void)`

打开播放器。

-   `openPlayerInfo`: `OpenPlayerInfo`   - 播放器信息 (见 `OpenPlayerInfo`)。
-   `stateCall`: `(str: string) => void` - 状态回调。
-   `stateInfo`: `MediaStateInfo`        - 设置为默认值即可。
-   `statCall`: `(str: string) => void`  - 统计回调。

#### `closePlayer()`

关闭播放器。

#### `exitPlayer()`

退出播放器。

#### `setMute(mute: boolean)`

设置是否静音。

-   `mute`: `boolean` - `true`为静音, `false`为取消静音。

#### `setFaceInfo(width: number, height: number, faceinfo: string)`

设置人脸信息。

-   `width`: `number` - 图像宽度。
-   `height`: `number` - 图像高度。
-   `faceinfo`: `string` - 人脸信息 (JSON 字符串)，含有人脸窗口、角度、置信度、关键点等信息，若未检测到人脸则为空字符串。

#### `nativeSourceOpen(isPBO: boolean): number`

打开原生数据源，此函数调用后可以使显存数据到达内存，以便后续进行人脸检测。

-   `isPBO`: `boolean` - 是否使用PBO模式，此处设置为true。
-   **Returns**: `number` - `0` for success, otherwise failure.

#### `nativeSourceClose()`

关闭原生数据源。

#### `nativeSourceAcquire(): HJNativeSourceData | null`

从原生数据源获取数据。

-   **Returns**: `HJNativeSourceData | null` - 数据或 `null`。

#### `openFaceu(url: string): number`

打开萌颜，萌颜效果必须在人脸检测开启后才能生效。

-   `url`: `string` - 萌颜资源URL。
-   **Returns**: `number` - `0` for success, otherwise failure.

#### `closeFaceu()`

关闭FaceU。

### HJFaceDetectMgr

#### `openFaceDetect(): Promise<number>`

开启人脸检测模块。

-   **Returns**: `Promise<number>` - A promise that resolves with `0` on success.

#### `closeFaceDetect(): Promise<number>`

关闭人脸检测。

-   **Returns**: `Promise<number>` - A promise that resolves with `0` on success.

#### `registerFaceInfoCb(cb: (width: number, height: number, faceInfo: string) => void)`

注册人脸信息回调。

-   `cb`: `(width: number, height: number, faceInfo: string) => void` - 回调函数。

#### `registerNativeSourceOpenCb(cb: () => number)`

注册原生数据源打开回调。

-   `cb`: `() => number` - 回调函数。

#### `registerNativeSourceAcquireCb(cb: () => HJNativeSourceData | null)`

注册原生数据源获取回调。

-   `cb`: `() => HJNativeSourceData | null` - 回调函数。

#### `registerNativeSourceCloseCb(cb: () => void)`

注册原生数据源关闭回调。

-   `cb`: `() => void` - 回调函数。

### Enums and Interfaces

#### `OpenPlayerInfo`

-   `url`: `string` - 播放URL。
-   `fps`: `number` - 渲染帧率。
-   `videoCodecType`: `HJPlayerVideoCodecType` - 视频编解码器类型，可使用软解码和鸿蒙硬加速解码。
-   `sourceType`: `HJPlayerSourceType` - 播放源类型，如果使用左右分屏透明礼物使用HJPlayerSourceType_SPLITSCREEN，否则一律使用HJPlayerSourceType_SERIES
-   `playerType`: `HJPlayerType` - 播放类型，HJPlayerType_LIVESTREAM为直播，HJPlayerType_VOD为点播。
-   `bSplitScreenMirror`: `boolean` -如果使用左右分屏透明礼物播放，是否启动水平轴镜像。
-   `disableMFlag`: `number` - 详见HJPlayerMediaType具体枚举值，默认为HJPLAYER_MEDIA_TYPE_NONE

HJPlayerMediaType
{
    HJPLAYER_MEDIA_TYPE_NONE       = 0x0000,
    HJPLAYER_MEDIA_TYPE_VIDEO      = 0x0001,
    HJPLAYER_MEDIA_TYPE_AUDIO      = 0x0002,
    HJPLAYER_MEDIA_TYPE_DATA       = 0x0004,
    HJPLAYER_MEDIA_TYPE_SUBTITLE   = 0x0008,
}

#### `SetWindowInfo`    - 上层窗口设置

-   `surfaceId`: `string`       - 上层窗口ID。
-   `width`: `number`           - 上层窗口宽度。
-   `height`: `number`          - 上层窗口高度。 
-   `state`: `SetWindowState`   -上层窗口状态。

#### `LogMode`

-   `CONSOLE = 1 << 1`    - 打印日志到控制台
-   `FILE = 1 << 2`       - 打印日志到文件

#### `SetWindowState`

-   `TARGET_CREATE = 0`  - 上层窗口创建时调用
-   `TARGET_CHANGE = 1`  - 上层窗口分辨率变化时调用
-   `TARGET_DESTROY = 2` - 上层窗口销毁时调用

#### `HJPlayerVideoCodecType`

-   `HJPlayerVideoCodecType_SoftDefault = 0`  - 软解码
-   `HJPlayerVideoCodecType_OHCODEC = 1`      - 鸿蒙硬加速解码
-   `HJPlayerVideoCodecType_VIDEOTOOLBOX = 2`
-   `HJPlayerVideoCodecType_MEDIACODEC = 3`

#### `HJPlayerSourceType`

-   `HJPlayerSourceType_UNKNOWN = 0`
-   `HJPlayerSourceType_Bridge = 1`      - 暂时不使用
-   `HJPlayerSourceType_SPLITSCREEN = 2` - 左右分屏透明礼物播放
-   `HJPlayerSourceType_SERIES = 3`      - 其他方式播放

#### `HJPlayerNotifyType`

(Includes various notification types for player state)

#### `HJPlayerType`

-   `HJPlayerType_UNKNOWN = 0`
-   `HJPlayerType_LIVESTREAM = 1` - 直播流播放
-   `HJPlayerType_VOD = 2`        - 点播流播放