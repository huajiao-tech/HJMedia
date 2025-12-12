![](https://img.shields.io/badge/license-LGPL2.1-blue)

# hjpusher

hjpusher属于HJMedia媒体开源的一部分， HJMedia SourceCode: [huajiao-tech/HJMedia · GitHub](https://github.com/huajiao-tech/HJMedia)

## 简介

hjpusher是一款轻量级、高性能的鸿蒙平台直播推流工具，支持从摄像头和麦克风采集音视频数据并进行实时推流。

## 下载安装

`ohpm install @hj-live/hjpusher`

## 主要功能

* 多输入源支持：
  - 摄像头视频采集
  - 麦克风音频采集
  - PNG序列帧播放推流
* 编码格式支持：
  - 视频：H.264、H.265/HEVC
  - 音频：AAC等主流格式
* 推流功能：
  - 自定义推流地址配置
  - 推流状态实时监控
* 录制功能：
  - 推流成功后自动/手动录制
  - 本地保存直播内容
    
## 使用说明
1. 申请权限
   - ohos.permission.CAMERA
   - ohos.permission.MICROPHONE
   - ohos.permission.INTERNET

上述权限已经在当前har包module.json中配置完成，只需要在使用相机和麦克风之前需要申请一下权限

```typescript
try {
  let atManager = abilityAccessCtrl.createAtManager();
  atManager.requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA', 'ohos.permission.MICROPHONE'])
    .then((data) => {
      // Logger.info('Entry', 'requestPre() data: ' + JSON.stringify(data));
    }).catch((err: BusinessError) => {
    // Logger.info('Entry', 'requestPre() data: ' + JSON.stringify(err));
  })
} catch (err) {
  // Logger.error('Entry', 'requestPre() data: ' + JSON.stringify(err));
}
```

2. 初始化推流器日志输出

```typescript
let logLevel = 2;
let logMode = LogMode.CONSOLE | LogMode.FILE;
let logPath = '/data/storage/el2/base/haps/entry/files/';
let bLog = true;
let logSize = 1024 * 1024 * 5;
let logCount = 5;
Pusher.contextInit(bLog, logPath, logLevel, logMode, logSize, logCount);
```

3. 创建推流器，并绑定相机预览输出流

```typescript
// 创建一个HJPusher实例
hjPusher: HJPusher = new HJPusher()

// 创建一个推流器实例并打开, 保存底层的nativeImage的surfaceId用于后续绑定到相机预览输出流接收采集的视频数据
this.state.hjPusher.createPusher();
const previewInfo = new PreviewInfo();
previewInfo.realWidth = this.state.config.videoConfig.width
previewInfo.realHeight = this.state.config.videoConfig.height
let previewSurfaceId: bigint = this.state.hjPusher.openPreview(previewInfo, (str: string) => {});

// 绑定surfaceId，打开相机
cameraService.initCamera(getContext() as Context);
cameraService.bindSurfaceId(previewSurfaceId.toString());
cameraService.startPreview(0)
```

4.预览

```typescript
// 给底层渲染线程增加一个输出目的，此处增加一个XComponent用于预览
XComponent({
  id: 'PusherXComponent',
  type: XComponentType.SURFACE,
  controller: this.pusherXComponentController
})

// 重写XComponentController生命周期方法
class PusherXComponentController extends XComponentController {
  private hjPusher: HJPusher

  constructor(hjPusher: HJPusher) {
    super();
    this.hjPusher = hjPusher;
  }

  onSurfaceCreated(surfaceId: string): void {
    this.hjPusher.setWindow({
      surfaceId: surfaceId,
      width: 1,
      height: 1,
      state: SetWindowState.TARGET_CREATE
    })
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
    this.hjPusher.setWindow({
      surfaceId: surfaceId,
      width: rect.surfaceWidth,
      height: rect.surfaceHeight,
      state: SetWindowState.TARGET_CHANGE
    })
  }

  async onSurfaceDestroyed(surfaceId: string) {

  }
}
```

5. 推流

```typescript
// 参数配置
export const pusherPreset = new PusherConfigPreset(
{
codecID: VideoCodecType.HJVCodecH265,
width: 720,
height: 1280,
bitrate: 2 * 1024 * 1024,
frameRate: 30,
gopSize: 60
videoIsROIEnc: true, // 是否开启ROI编码,如果启动ROI需要人脸检测模块同步打开
},
{
codecID: AudioCodecType.HJCodecAAC,
bitrate: 164 * 1024,
sampleFmt: 1,
samplesRate: 48000,
channels: 2
},
""
)

// 推流  
this.state.hjPusher.openPusher(this.state.config)
```

具体推流流程可详见Demo代码  [Demo源码](https://github.com/huajiao-tech/HJMedia)

## 效果演示

![预览](preview.png)

## 注意事项

当前推流器底层是使用nativeImage接收相机采集的数据，然后可以调用setWindow(setWindowInfo: SetWindowInfo)增加输出目的，如XComponent，
当前推流器接收相机采集的视频数据，通过接收相机的预览输出流（此处不能使用相机的视频输出流，实测发现视频输出流会存在卡顿和延迟）

保证setWindow()调用时机问题：保证setWindow()在createPusher()之后调用，注意setWindow()中state参数有三种状态，确保正确传递（可参考demo中使用方式）

```typescript
export enum SetWindowState {
  TARGET_CREATE = 0, // 创建，增加surface时使用
  TARGET_CHANGE = 1, // 修改，surface宽高变化时使用
  TARGET_DESTROY = 2 //  销毁，surface销毁时使用 
}
```

相机开启和释放：鸿蒙中的aboutToDisappear()生命周期并不是组件在屏幕上不可见立即调用的，当快速进出预览页面时，会导致预览页第一次的aboutToDisappear()在预览页第二次aboutToAppear()调用后
再调用导致错误释放相机资源会出现预览黑屏，可参考demo中cameraService的处理方式，或者使用onPageHide()可以保证页面在屏幕上不可见时立马调用（仍需注意生命周期时序）

tips: 如需保持应用在后台持续运行，请参考官网[长时任务](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/continuous-task)

## API

### HJPusher

#### `contextInit(valid: boolean, logDir: string, logLevel: number, logMode: number, maxSize: number, maxFiles: number)`

初始化推流器上下文。

-   `valid`: `boolean` - 是否开启日志。
-   `logDir`: `string` - 日志目录。
-   `logLevel`: `number` - 日志级别。
-   `logMode`: `number` - 日志模式 (见 `LogMode`)。
-   `maxSize`: `number` - 单个日志文件最大大小。
-   `maxFiles`: `number` - 最多日志文件数量。

#### `createPusher()`

创建一个推流器实例。

#### `destroyPusher()`

销毁一个推流器实例。

#### `openPreview(previewInfo: PreviewInfo, pusherStateCall: (str: string) => void): bigint`

打开预览。

-   `previewInfo`: `PreviewInfo` - 预览信息 (见 `PreviewInfo`)。
-   `pusherStateCall`: `(str: string) => void` - 状态回调。
-   **Returns**: `bigint` - The surface ID for the preview.

#### `closePreview()`

关闭预览。

#### `setWindow(setWindowInfo: SetWindowInfo)`

设置渲染窗口。

-   `setWindowInfo`: `SetWindowInfo` - 窗口信息 (见 `SetWindowInfo`)。

#### `openPusher(pusherConfig: PusherConfig, stateInfo: MediaStateInfo, stateCall: (str: string) => void)`

打开推流器。

-   `pusherConfig`: `PusherConfig` - 推流器配置 (见 `PusherConfig`)。
-   `stateInfo`: `MediaStateInfo`  - 取默认值即可。
-   `stateCall`: `(str: string) => void` - 状态回调。

#### `closePusher()`

关闭推流器。

#### `setMute(mute: boolean)`

设置是否静音。

-   `mute`: `boolean` - `true`为静音, `false`为取消静音。

#### `openRecorder(recorderInfo: RecorderInfo): number`

打开录制器。

-   `recorderInfo`: `RecorderInfo` - 录制器信息 (见 `RecorderInfo`)。
-   **Returns**: `number` - `0` for success, otherwise failure.

#### `closeRecorder()`

关闭录制器。

#### `openPngSeq(url: string)`

打开PNG序列。

-   `url`: `string` - PNG序列资源URL。

#### `setFaceInfo(width: number, height: number, faceinfo: string)`

设置人脸信息。

-   `width`: `number`    - 图像宽度。
-   `height`: `number`   - 图像高度。
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

#### `setFaceProtected(i_bFaceProtected: boolean)`

设置人脸保护模式，启用后，若检测到直播间无人出现则对画面进行模糊处理。

-   `i_bFaceProtected`: `boolean` - `true` to enable face protection.

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

#### `setVideoEncQuantOffset(i_quantoffset: number)`

设置ROI视频编码画质，其中参数-51 - 51 默认为-3


### Enums and Interfaces

#### `PusherConfig`

-   `videoConfig`: `VideoConfig`  - 视频信息
-   `audioConfig`: `AudioConfig`  - 音频信息
-   `url`: `string`               - 推流地址

#### `VideoConfig`

-   `codecID`: `VideoCodecType`
-   `width`: `number`
-   `height`: `number`
-   `bitrate`: `number`
-   `frameRate`: `number`
-   `gopSize`: `number`
-   `videoIsROIEnc`: `boolean`    

#### `AudioConfig`

-   `codecID`: `AudioCodecType`
-   `bitrate`: `number`
-   `sampleFmt`: `number`
-   `samplesRate`: `number`
-   `channels`: `number`

#### `PreviewInfo`

-   `realWidth`: `number`   - 设置底层中间处理画面的宽
-   `realHeight`: `number`  - 设置底层中间处理画面的高
-   `previewFps`: `number`  - 设置底层渲染帧率

#### `RecorderInfo`

-   `recordUrl`: `string`

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

#### `VideoCodecType`

-   `HJCodecUnknown = -1`
-   `HJCodecH264 = 27`
-   `HJVCodecH265 = 173`

#### `AudioCodecType`

-   `HJCodecUnknown = -1`
-   `HJCodecAAC = 86018`

#### `PusherStateType`

(Includes various notification types for pusher state)