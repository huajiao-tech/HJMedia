# API v1.0.0

### 1. Introduction

This API documentation includes two parts: the first part is the API interface documentation of the HJPusher SDK, and the second part is the API interface documentation of HJPlayer SDK.

## 2. HJMedia API

### HJPusher API

#### 导入模块

```typescript
import { HJPusher } from '@hj-live/hjpusher'
```

**contextInit**

public static contextInit(valid: boolean, logDir: string, logLevel: number, logMode: number, maxSize: number,
maxFiles: number): void

初始化HJPusherSDK日志系统

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| valid | boolean | 是   | 是否启用日志功能 |
| logDir | string | 是   | 日志文件存储目录路径 |
| logLevel | number | 是   | 日志级别 |
| logMode | number | 是   | 日志输出模式 |
| maxSize | number | 是   | 单个日志文件最大大小(字节) |
| maxFiles | number | 是   | 日志文件最大保留数量 |

**createPusher**

public createPusher(): void

创建推流器实例

**destroyPusher**

public destroyPusher()

销毁推流器实例

**openPreview**

public openPreview(previewInfo: PreviewInfo, pusherStateCall: (str: string) => void): bigint

开启预览

参数：

| 参数名     | 类型      | 必填 | 说明             |
|---------|---------|------|----------------|
| previewInfo | PreviewInfo | 是   | 预览配置信息         |
| pusherStateCall | (str: string) => void) | 是   | 推流器状态回调        |

返回值：

| 类型     | 说明      |
|---------|---------|
| bigint | nativeImage的surfaceId,用于绑定预览输出流 |


**closePreview**

public closePreview(): void

关闭预览

**setWindow**

public setWindow(setWindowInfo: SetWindowInfo): void

设置推流器输出目的

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| setWindowInfo | SetWindowInfo | 是   | 推流器输出目的信息 |

**openPusher**

public openPusher(pusherConfig: PusherConfig): void

启动推流

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| pusherConfig | PusherConfig | 是   | 推流配置信息 |

**closePusher**

public closePusher(): void

关闭推流器

**setMute**

public setMute(mute: boolean): void

设置推流器静音

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| mute | boolean | 是   | 是否静音 |

**openRecorder**

public openRecorder(recorderInfo: RecorderInfo): number

打开录制

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| recorderInfo | RecorderInfo | 是   | 录制配置信息 |

返回值：

| 类型     | 说明       |
|---------|----------|
| number | 录制是否开启成功 |

**closeRecorder**

public closeRecorder(): void

关闭录制

**openPngSeq**

public openPngSeq(url: string): void

播放png序列

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|----|
| url | string | 是   | 路径 |

### HJPlayer API

#### 导入模块

```typescript
import { HJPlayer } from '@hj-live/hjplayer'
```

**contextInit**

public static contextInit(valid: boolean, logDir: string, logLevel: number, logMode: number, maxSize: number,
maxFiles: number): void

初始化HJPusherSDK日志系统

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| valid | boolean | 是   | 是否启用日志功能 |
| logDir | string | 是   | 日志文件存储目录路径 |
| logLevel | number | 是   | 日志级别 |
| logMode | number | 是   | 日志输出模式 |
| maxSize | number | 是   | 单个日志文件最大大小(字节) |
| maxFiles | number | 是   | 日志文件最大保留数量 |

**preloadUrl**

public static preloadUrl(url: string): void

预加载指定URL的媒体资源

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| url | string | 是   | 需要预加载的媒体资源URL |

**createPlayer**

public createPlayer(): void

创建播放器实例

**destroyPlayer**

public destroyPlayer(): void

销毁播放器实例

**setWindow**

public setWindow(setWindowInfo: SetWindowInfo): void

设置播放器输出窗口

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| setWindowInfo | SetWindowInfo | 是   | 播放器输出窗口信息 |

**openPlayer**

public openPlayer(openPlayerInfo: OpenPlayerInfo, stateCall: (str: string) => void, stateInfo: MediaStateInfo, statCall: (str: string) => void): void

启动播放

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| openPlayerInfo | OpenPlayerInfo | 是   | 播放配置信息 |
| stateCall | (str: string) => void | 是   | 播放器状态回调 |
| stateInfo | MediaStateInfo | 是   | 媒体状态信息 |
| statCall | (str: string) => void | 是   | 统计信息回调 |

**closePlayer**

public closePlayer(): void

关闭播放器

**exitPlayer**

public exitPlayer(): void

退出播放器

**setMute**

public setMute(mute: boolean): void

设置播放器静音

参数：

| 参数名     | 类型      | 必填 | 说明 |
|---------|---------|------|-----|
| mute | boolean | 是   | 是否静音 |
