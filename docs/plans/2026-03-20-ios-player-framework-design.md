# iOS Player Framework Design

**日期**: 2026-03-20

**目标**: 将 `src/entry/player` 统一产出为 iOS 可用的 `HJPlayer.framework`，对外导出纯 Objective-C 接口 `HJMusicPlayer` 与 `HJPlayerContext`，并新增 `examples/ios/HJPlayerDemo` 演示音乐播放器常用能力。

## 1. 背景与范围

当前仓库已经具备：

- `src/entry/player/asys/HJMusicPlayerJni.cpp`：Android 侧音乐播放器导出层
- `src/entry/player/asys/HJPlayerContextJni.cpp`：Android 侧上下文导出层
- `src/graphs/HJGraphMusicPlayer.*`：跨平台音乐播放核心
- `src/entry/HJEntryContext.*`：播放器环境上下文初始化能力

本次需求范围：

- 在 `src/entry/player/isys` 下构建 iOS 导出层
- 对外只暴露纯 Objective-C 头文件，不暴露 C++ 类型
- 统一通过现有 CMake iOS 工程产出 `HJPlayer.framework`
- 在 `examples/ios/HJPlayerDemo` 下构建 demo app
- demo 默认播放资源为 `examples/ios/HJPlayerDemo/res/c58733ac51124fe38cdc6540a7b8fa46.mkv`

非本次范围：

- 不新增独立 `.xcodeproj/.xcworkspace`
- 不设计视频渲染播放器 UI
- 不扩展完整媒体信息导出，仅暴露 demo 所需字段

## 2. 总体方案

采用“Objective-C 包装层 + 复用现有 C++ 播放核心”的方案。

分层如下：

1. `HJPlayer.framework` 公共接口层
   - `HJMusicPlayer.h`
   - `HJPlayerContext.h`

2. `HJPlayer.framework` iOS 实现层
   - `HJMusicPlayer.mm`
   - `HJPlayerContext.mm`

3. 现有 C++ 核心层
   - `HJGraphMusicPlayer`
   - `HJEntryContext`
   - `HJGraph` / `HJEventBus`
   - iOS 音频渲染插件 `HJPluginAudioIOSRender`

设计原则：

- 对外 API 纯 Objective-C
- Objective-C 头文件禁止暴露 `std::string`、`shared_ptr`、C++ enum/class`
- iOS 包装层只做参数转换、状态维护、事件转发
- 播放、暂停、恢复、seek、音量、静音、音轨切换全部复用现有 C++ 播放核心

## 3. 目录与目标调整

### 3.1 新增目录

新增：

- `src/entry/player/isys/`
- `docs/plans/`

### 3.2 新增文件

新增 iOS 导出层文件：

- `src/entry/player/isys/HJMusicPlayer.h`
- `src/entry/player/isys/HJMusicPlayer.mm`
- `src/entry/player/isys/HJPlayerContext.h`
- `src/entry/player/isys/HJPlayerContext.mm`

新增 demo 文件：

- `examples/ios/HJPlayerDemo/CMakeLists.txt`
- `examples/ios/HJPlayerDemo/AppDelegate.h`
- `examples/ios/HJPlayerDemo/AppDelegate.mm`
- `examples/ios/HJPlayerDemo/ViewController.h`
- `examples/ios/HJPlayerDemo/ViewController.mm`
- `examples/ios/HJPlayerDemo/SceneDelegate.h`
- `examples/ios/HJPlayerDemo/SceneDelegate.mm`
- `examples/ios/HJPlayerDemo/main.m`
- `examples/ios/HJPlayerDemo/Info.plist.in`

### 3.3 需要修改的构建文件

- `src/entry/player/CMakeLists.txt`
- `CMakeLists.txt`

调整目标：

- `src/entry/player/CMakeLists.txt`
  - 在 iOS 下纳入 `isys/*.mm`
  - 设置 `HJMusicPlayer.h`、`HJPlayerContext.h` 为 public headers
- 根 `CMakeLists.txt`
  - 在 `IOS` 分支新增 `add_subdirectory(examples/ios/HJPlayerDemo)`
- `examples/ios/HJPlayerDemo/CMakeLists.txt`
  - 构建 `HJPlayerDemo.app`
  - 链接 `HJPlayer.framework`
  - 将默认 `mkv` 资源打包到 app bundle
  - 将 `HJPlayer.framework` 拷贝到 app 的 `Frameworks` 目录

## 4. HJPlayerContext 设计

`HJPlayerContext` 作为播放器环境上下文，单例模式，App 启动时初始化一次。

### 4.1 对外接口

建议接口：

- `+ (instancetype)sharedInstance`
- `- (NSInteger)startWithConfig:(HJPlayerContextConfig *)config`
- `- (void)stop`
- `@property (nonatomic, assign, readonly, getter=isStarted) BOOL started`

### 4.2 配置模型

`HJPlayerContextConfig` 采用 Objective-C 对象封装，字段包括：

- `logEnabled`
- `logDirectory`
- `logLevel`
- `logMode`
- `logMaxFileSize`
- `logMaxFileCount`
- `mediasDirectory`
- `mediasCacheMax`
- `downloadRetryMax`

### 4.3 内部实现映射

`HJPlayerContext.mm` 内部负责：

- 将 Objective-C 配置转为 `HJEntryContextPlayerInfo`
- 调用 `HJEntryContext::init(HJEntryType_Player, info)`
- 记录初始化状态和最近一次返回值

约束：

- 重复初始化不重复触发底层初始化
- `AppDelegate` 在 `didFinishLaunchingWithOptions` 中调用一次
- 如果未显式配置日志目录，默认使用 `NSTemporaryDirectory()`

## 5. HJMusicPlayer 设计

`HJMusicPlayer` 是面向 iOS 业务层的音乐播放器对象，对外只暴露纯 Objective-C API，内部持有 C++ 播放句柄。

### 5.1 对外接口

播放器主要接口：

- `- (instancetype)initWithDelegate:(id<HJMusicPlayerDelegate>)delegate`
- `- (NSInteger)prepare`
- `- (NSInteger)openURL:(NSString *)url`
- `- (NSInteger)pause`
- `- (NSInteger)resume`
- `- (NSInteger)stop`
- `- (NSInteger)seekToMilliseconds:(int64_t)positionMs`
- `- (NSInteger)setMute:(BOOL)mute`
- `- (NSInteger)setVolume:(float)volume`
- `- (int64_t)getCurrentTimestamp`
- `- (NSInteger)selectAudioTrack:(NSInteger)trackId`
- `- (void)destroy`

只读属性建议：

- `prepared`
- `paused`
- `muted`
- `duration`
- `volume`
- `audioTracks`
- `mediaInfo`

### 5.2 openURL 语义

`openURL:` 语义定义如下：

- 打开成功后直接进入播放态
- 不要求额外先调 `play`
- 如果 `url` 为空，则优先回退到默认 bundle 资源
- 如果当前实例已有资源在播，先 stop/reset，再打开新资源

### 5.3 pause / resume / stop 语义

- `pause`：暂停当前播放
- `resume`：从暂停态恢复
- `stop`：停止并关闭当前资源，清空播放状态，但对象实例仍可再次 `openURL:`

由于当前 `HJGraphMusicPlayer` 的 `close()` 语义较弱，首版 `stop` 设计为销毁并重建内部 player handle，不依赖底层 `close()`

### 5.4 音量与进度

- `setVolume:` 输入范围固定为 `0.0 ~ 1.0`
- 进入底层前统一 clamp
- 播放进度不通过 delegate 自动推送
- 外部通过 `getCurrentTimestamp` 主动查询当前时间戳

## 6. Delegate 与数据模型

### 6.1 Delegate 设计

采用 `delegate` 为主，不使用 block 作为主回调方式。

建议事件：

- `musicPlayerDidPrepare:`
- `musicPlayerDidPause:`
- `musicPlayerDidResume:`
- `musicPlayerDidStop:`
- `musicPlayerDidReachEOF:`
- `musicPlayer:didFailWithErrorCode:message:`
- `musicPlayer:didUpdateDuration:`
- `musicPlayer:didUpdateAudioTracks:`
- `musicPlayer:didUpdateMediaInfo:`

约束：

- 全部 delegate 回调切回主线程
- delegate 使用 `weak`

### 6.2 音轨信息模型

`HJAudioTrackInfo` 建议字段：

- `trackId`
- `displayName`
- `title`
- `language`
- `codecName`
- `channels`
- `sampleRate`
- `defaultTrack`
- `selected`

数据来源：

- `HJGraphMusicPlayer::getAudioTrackDisplayInfos()`

### 6.3 媒体信息模型

`HJPlayerMediaInfo` 首版仅暴露 demo 所需字段：

- `url`
- `duration`
- `audioCodec`
- `sampleRate`
- `channels`
- `trackCount`

如果底层无法稳定直接取完整字段，允许首版按“已知信息 + 音轨汇总”构造。

## 7. Objective-C 与 C++ 的内部映射

### 7.1 内部句柄

`HJMusicPlayer.mm` 内部维护私有 handle，建议至少包含：

- 互斥锁
- `HJGraphPlayer::Ptr`
- delegate 弱引用桥接
- 当前 `duration`
- 当前 `audioTracks`
- 当前 `mediaInfo`
- 当前状态位

### 7.2 prepare 初始化逻辑

`prepare` 内部逻辑：

1. 检查 `HJPlayerContext` 是否已初始化
2. 创建 `HJGraphType_MUSIC`
3. 注册 `HJEventBus` 事件
4. 构建默认 `HJAudioInfo`
   - sample rate: `48000`
   - channels: `2`
   - sample fmt: `1`
   - bytes per sample: `2`
5. 调用底层 `player->init(param)`

### 7.3 事件总线映射

事件映射：

- `EVENT_GRAPH_DURATION_ID`
  - 更新 `duration`
  - 回调 `didUpdateDuration`
- `EVENT_GRAPH_EOF_ID`
  - 回调 `didReachEOF`

首版不额外转发 `EVENT_GRAPH_AUDIO_FRAME_ID`

### 7.4 底层方法映射

Objective-C 到 C++ 的方法对应关系：

- `openURL:` -> `player->openURL(HJMediaUrl::Ptr)`
- `pause` -> `player->pause()`
- `resume` -> `player->resume()`
- `seekToMilliseconds:` -> `player->seek(positionMs)`
- `setMute:` -> `player->setMute(mute)`
- `setVolume:` -> `player->setVolume(volume)`
- `getCurrentTimestamp` -> `player->getCurrentTimestamp()`
- `selectAudioTrack:` -> `player->switchAudioTrack(trackId)`
- `audioTracks` -> `player->getAudioTrackDisplayInfos()`

## 8. HJPlayerDemo 设计

### 8.1 目标

新增 `HJPlayerDemo`，用于验证：

- framework 导出是否正确
- App 启动时上下文初始化是否正常
- 音乐播放器基本控制是否可用
- 音频信息展示是否正确

### 8.2 默认资源

默认资源固定为：

- `examples/ios/HJPlayerDemo/res/c58733ac51124fe38cdc6540a7b8fa46.mkv`

运行时通过 `NSBundle` 定位，不使用源码绝对路径。

### 8.3 页面结构

建议使用单页面 `UIViewController`，不引入 storyboard 复杂结构。

页面包含：

- 文件名与播放状态标签
- 音频信息区域
  - duration
  - codec
  - sample rate
  - channels
  - 当前音轨
- 进度区域
  - `UISlider`
  - 当前时间 / 总时长标签
- 控制按钮
  - `Open Default`
  - `Pause`
  - `Resume`
  - `Stop`
- 音量控制
  - `UISlider`
  - 静音开关
- 音轨切换区域
  - 若存在多音轨，使用 `UISegmentedControl` 或列表

### 8.4 进度刷新策略

由于进度改为外部主动获取，demo 使用 `NSTimer` 每 `500ms` 调用一次：

- `getCurrentTimestamp`

UI 更新逻辑：

- `duration` 由 delegate `didUpdateDuration` 更新
- 当前进度由定时器更新

### 8.5 App 生命周期接入

`AppDelegate` 负责：

- 在 `didFinishLaunchingWithOptions` 中调用 `HJPlayerContext`
- 构造默认上下文配置

默认日志配置建议：

- `logEnabled = YES`
- `logDirectory = NSTemporaryDirectory()`
- `logMode = HJLogLMode_CONSOLE`
- `logMaxFileSize = 5 * 1024 * 1024`
- `logMaxFileCount = 2`

## 9. 错误处理与边界策略

### 9.1 错误返回

Objective-C 层统一返回 `NSInteger`，底层返回码原样透出。

### 9.2 失败场景处理

- `HJPlayerContext` 未初始化就调用播放器接口
  - 返回 `HJErrNotInited`
- `openURL:` 传空且默认资源不存在
  - 返回 `HJErrInvalidParams`
- `selectAudioTrack:` 传不存在的 `trackId`
  - 直接返回底层错误码

### 9.3 参数收敛

- `setVolume(NaN)` -> `1.0`
- `setVolume(<0)` -> `0.0`
- `setVolume(>1)` -> `1.0`
- `seekToMilliseconds(<0)` -> `0`

### 9.4 幂等性

重复调用：

- `pause`
- `resume`
- `stop`

应尽量保持幂等，避免异常状态扩散。

## 10. 风险与注意事项

主要风险：

1. 当前 `HJGraphMusicPlayer` 对 stop 后复用实例的语义不完整
   - 规避方式：stop 后重建内部 player handle

2. `HJMediaInfo` 对外字段过多且结构复杂
   - 规避方式：首版只导出 demo 必需字段

3. iOS framework public header 暴露范围过大
   - 规避方式：public headers 仅保留 `HJMusicPlayer.h`、`HJPlayerContext.h`

4. eventBus 回调线程非主线程
   - 规避方式：统一切回主线程再触发 delegate

## 11. 测试与验收

### 11.1 Framework 验收

- `HJPlayerContext` 重复初始化仅生效一次
- `HJMusicPlayer prepare -> openURL -> pause -> resume -> stop` 返回正常
- `openURL:nil` 能回退默认资源
- 播放过程中 `getCurrentTimestamp` 能递增
- 暂停后 `getCurrentTimestamp` 基本停止增长
- `setVolume(-1 / 0 / 0.5 / 2)` 行为符合 clamp 预期
- `seekToMilliseconds(-1000)` 不崩溃并回到起始附近
- 多音轨文件时可正确导出并切换音轨

### 11.2 Demo 验收

- App 启动时 `HJPlayerContext` 初始化成功
- 点击 `Open Default` 后能播放默认 `mkv`
- `Pause / Resume / Stop` 正常工作
- 进度条拖动后能 seek
- 音量滑条和静音按钮生效
- 音频信息区显示正确

### 11.3 构建验收

- `build_ios.sh` 生成统一 Xcode 工程
- 工程内包含：
  - `HJPlayer.framework`
  - `HJPlayerDemo`
- `HJPlayerDemo.app` 内嵌：
  - `HJPlayer.framework`
  - 默认 `mkv` 资源

## 12. 实施拆分建议

为避免一次修改超过 3 个以上文件导致任务失控，建议实现阶段拆成以下小任务：

1. 只完成 `HJPlayer.framework` iOS 导出层
2. 只完成 `HJPlayerContext` 单例和初始化接入
3. 只完成 `HJPlayerDemo` app 与资源打包
4. 最后做统一构建与联调验证

以上拆分符合当前仓库约束，也便于逐步验证 framework、context、demo 三个层次。
