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