# iOS Audio Session Design

**日期**: 2026-03-23

**目标**: 在 `src/entry/player/isys` 下新增 `HJAudioSession` Objective-C 封装，统一管理 `AVAudioSession` 的设置、查询与少量显式控制能力，供 iOS 播放器与后续采集场景复用。

## 1. 背景与范围

当前 `src/entry/player/isys` 已有：

- `HJMusicPlayer`：iOS 音乐播放器包装层
- `HJPlayerContext`：播放器上下文初始化
- `HJLifeCycle`：应用生命周期与音频中断通知注册

当前缺口：

- 仓库内还没有统一的 `AVAudioSession` 包装层
- 业务侧如果直接操作 `AVAudioSession`，会把系统字符串、路由对象和配置顺序散落到上层代码
- `category/mode/options`、采样率、IO buffer、输入输出通道数、扬声器切换等常用能力缺少统一入口

本次范围：

- 提供 `HJAudioSession` 单例封装
- 提供配置模型 `HJAudioSessionConfiguration`
- 提供查询模型 `HJAudioSessionPortInfo`、`HJAudioSessionRouteInfo`
- 提供设置与查询 API
- 提供最小错误包装与参数校验

非本次范围：

- 不处理 interruption / route change 事件分发
- 不绑定 `HJLifeCycle`
- 不实现打断恢复、耳机拔出自动暂停等业务策略
- 不新增 CMake 测试目标

## 2. 总体方案

采用“Objective-C 纯接口 + Objective-C++ 内部映射 `AVAudioSession`”方案。

分层如下：

1. 公共接口层
   - `HJAudioSession.h`
   - 对外暴露自定义枚举、配置对象、查询对象和操作方法

2. iOS 实现层
   - `HJAudioSession.mm`
   - 负责 `HJ*` 枚举与 `AVAudioSession` 系统类型的双向映射
   - 负责配置应用顺序、参数校验、错误返回和查询转换

设计原则：

- 对外 API 保持 Objective-C 可直接使用
- 不把 `AVAudioSessionPortDescription` 等系统对象直接暴露给业务层
- 对系统调用失败返回 `NSError`
- 对配置顺序敏感的操作统一放到 `applyConfiguration:error:` 中执行
- 写操作保证串行，避免多线程下 session 状态交错

## 3. 对外类型设计

### 3.1 枚举与位掩码

新增以下类型：

- `HJAudioSessionCategory`
- `HJAudioSessionMode`
- `HJAudioSessionCategoryOptions`
- `HJAudioSessionRouteSharingPolicy`
- `HJAudioSessionSetActiveOptions`
- `HJAudioSessionPortOverride`
- `HJAudioSessionRecordPermission`

这些类型负责屏蔽 `AVAudioSessionCategoryPlayback`、`AVAudioSessionModeMoviePlayback` 等系统常量，避免业务代码依赖 Apple 字符串常量。

### 3.2 配置模型

`HJAudioSessionConfiguration` 字段：

- `category`
- `mode`
- `categoryOptions`
- `routeSharingPolicy`
- `preferredSampleRate`
- `preferredIOBufferDuration`
- `preferredInputNumberOfChannels`
- `preferredOutputNumberOfChannels`
- `preferSpeaker`
- `allowHapticsAndSystemSoundsDuringRecording`

约束：

- 数值型 preferred 参数小于等于 0 表示“不设置”
- `preferSpeaker` 仅允许在 `playAndRecord` 类别下使用
- `allowHapticsAndSystemSoundsDuringRecording` 仅在系统支持时调用

### 3.3 查询模型

`HJAudioSessionPortInfo` 字段：

- `uid`
- `portName`
- `portType`
- `channels`

`HJAudioSessionRouteInfo` 字段：

- `inputs`
- `outputs`

目的：

- 对上层暴露稳定、可序列化、可打印的只读数据模型
- 避免直接依赖系统路由对象生命周期

## 4. 接口设计

`HJAudioSession` 采用单例：

- `+ (instancetype)sharedInstance`

设置方法：

- `- (BOOL)applyConfiguration:(HJAudioSessionConfiguration *)configuration error:(NSError * _Nullable *)error`
- `- (BOOL)setActive:(BOOL)active error:(NSError * _Nullable *)error`
- `- (BOOL)setActive:(BOOL)active options:(HJAudioSessionSetActiveOptions)options error:(NSError * _Nullable *)error`
- `- (BOOL)setPreferredInputByUID:(NSString *)uid error:(NSError * _Nullable *)error`
- `- (BOOL)overrideOutputAudioPort:(HJAudioSessionPortOverride)portOverride error:(NSError * _Nullable *)error`
- `- (void)requestRecordPermission:(void (^)(BOOL granted))completionHandler`

查询属性：

- `category`
- `mode`
- `categoryOptions`
- `sampleRate`
- `preferredSampleRate`
- `ioBufferDuration`
- `preferredIOBufferDuration`
- `inputLatency`
- `outputLatency`
- `inputNumberOfChannels`
- `outputNumberOfChannels`
- `inputAvailable`
- `otherAudioPlaying`
- `secondaryAudioShouldBeSilencedHint`
- `recordPermission`
- `availableInputs`
- `currentRoute`

## 5. 核心实现细节

### 5.1 配置应用顺序

`applyConfiguration:error:` 固定顺序：

1. 校验参数组合
2. 设置 `category + mode + routeSharingPolicy + categoryOptions`
3. 设置 `preferredSampleRate`
4. 设置 `preferredIOBufferDuration`
5. 设置 preferred 输入输出通道数
6. 设置 `allowHapticsAndSystemSoundsDuringRecording`
7. 根据 `preferSpeaker` 执行 `overrideOutputAudioPort`

这样可以减少 session 在中间态下被系统拒绝的概率。

### 5.2 错误处理

新增 `HJAudioSessionErrorDomain`，包装两类错误：

- 本地校验错误
- 系统 API 返回的 `NSError`

本地校验至少包括：

- `configuration == nil`
- `preferSpeaker` 与 `category` 不匹配
- 指定输入 `uid` 在 `availableInputs` 中不存在
- 未知枚举值无法映射到系统类型

### 5.3 线程模型

- 内部维护一个串行 `dispatch_queue_t`
- 所有设置型 API 在该队列上同步执行
- 查询型属性直接读取 `AVAudioSession.sharedInstance`

这能避免上层并发调用 `setCategory:`、`setActive:`、`overrideOutputAudioPort:` 时的顺序竞争。

## 6. 测试策略

为了控制改动范围，本次不接入正式 CMake 测试目标，采用单独的 ObjC++ 测试文件通过 `xcrun clang++` 执行。

首批测试覆盖：

1. `applyConfiguration:` 可成功应用基础 playback 配置
2. `setActive:YES/NO` 可正常返回
3. `availableInputs`、`currentRoute` 查询对象结构有效
4. 非 `playAndRecord` 场景下开启 `preferSpeaker` 会失败

## 7. 后续扩展点

后续如果播放器或采集侧需要，可以在下一轮单独扩展：

- interruption / route change 监听转发
- 与 `HJLifeCycle` 的集成
- 话筒权限状态监听
- 电话/闹钟/蓝牙切换后的恢复策略
