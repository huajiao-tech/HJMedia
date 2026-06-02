# iOS Player Audio Session Lifecycle Design

**日期**: 2026-03-23

**目标**: 为 `HJMusicPlayer` 增加前后台、音频打断、耳机/蓝牙路由变更处理，并引入可复用的 `HJPlayerAudioSessionController`，统一管理播放器侧的 `AudioSession` 协同行为。

## 1. 背景与范围

当前 iOS 播放器相关文件已经具备：

- `src/entry/player/isys/HJMusicPlayer.h/.mm`
- `src/entry/player/isys/HJLifeCycle.h/.mm`
- `src/entry/player/isys/HJAudioSession.h/.mm`

当前缺口：

- `HJMusicPlayer` 虽然声明实现了 `HJLifeCycleDelegate`，但没有实际处理前后台切换
- 没有处理中断开始/结束后的暂停与恢复
- 没有处理耳机拔出、蓝牙设备断开等 route change
- 没有提供“是否由 `HJMusicPlayer` 内部管理 `AudioSession`”的开关

本次范围：

- 在 `HJMusicPlayer` 初始化 options 中新增 `HJMusicPlayerOptionManageAudioSession`
- 默认值设为 `NO`
- 新增 `HJPlayerAudioSessionController`，供 `HJMusicPlayer` 和后续 iOS 播放器对象复用
- `HJLifeCycle` 补 `AVAudioSessionRouteChangeNotification` 转发
- 为 `HJMusicPlayer` 增加前后台、打断、耳机插拔的行为处理

非本次范围：

- 不改底层 C++ 播放核心
- 不新增复杂的 session 池或多播放器仲裁器
- 不实现“耳机重新插入自动恢复”
- 不实现“回到前台自动恢复”
- 不把业务特定策略塞入 `HJLifeCycle`

## 2. 总体方案

采用“事件转发层 + 可复用 session/controller 层 + 播放器接入层”的方案。

分层如下：

1. `HJLifeCycle`
   - 只负责接收 iOS 系统通知并转发
   - 不做播放器策略判断

2. `HJPlayerAudioSessionController`
   - 负责 `AudioSession` 配置、激活/停用、打断恢复决策、路由变更处理
   - 每个 `HJMusicPlayer` 实例持有一个 controller
   - controller 内部统一使用 `HJAudioSession.sharedInstance`

3. `HJMusicPlayer`
   - 负责真正的播放、暂停、恢复
   - 通过 controller 协议接收“应暂停/应恢复”的动作
   - 不直接解析系统通知 userInfo

设计原则：

- 事件分发与业务策略解耦
- `HJMusicPlayer` 是否内部接管 `AudioSession` 由 option 明确控制
- 即使不接管 `AudioSession` 配置，也要保护播放状态
- 中断与路由变更恢复必须基于明确状态，不做“盲恢复”

## 3. Option 设计

新增：

- `HJMusicPlayerOptionManageAudioSession`

类型：

- `NSNumber (BOOL)`

默认值：

- `NO`

语义：

- `YES`
  - `HJMusicPlayer` 内部配置并激活 `HJAudioSession`
  - 默认配置为 `Playback + Default`
  - `prepare/openURL/resume` 前由 controller 调用 `HJAudioSession`
  - `pause/stop/destroy` 时由 controller 停用 session

- `NO`
  - `HJMusicPlayer` 不调用 `HJAudioSession` 设置 category / active
  - 但仍监听并处理打断、前后台、耳机拔出，负责播放器状态保护
  - 也就是说，session 配置权在外部，播放器状态协同仍由内部承担

## 4. 生命周期与事件行为

### 4.1 前后台

- `didEnterBackground`
  - 不主动暂停
  - 不停用 session

- `willEnterForeground` / `didBecomeActive`
  - 不自动恢复
  - 只刷新 controller 内部 app state

### 4.2 音频打断

监听：

- `AVAudioSessionInterruptionNotification`

策略：

- `Interruption Began`
  - 如果播放器当前正在播，记录：
    - `m_was_playing_before_interruption = YES`
    - `m_paused_by_interruption = YES`
  - 然后触发播放器暂停

- `Interruption Ended`
  - 只有同时满足以下条件才自动恢复：
    - `m_was_playing_before_interruption == YES`
    - `m_paused_by_interruption == YES`
    - `AVAudioSessionInterruptionOptionShouldResume` 存在
  - 自动恢复后清理 interruption 相关状态
  - 如果不满足条件，只清状态，不恢复

说明：

- 电话、其他应用抢占、Siri 等都统一走这一套 interruption 逻辑

### 4.3 耳机/蓝牙路由变更

监听：

- `AVAudioSessionRouteChangeNotification`

策略：

- 当 reason 为 `AVAudioSessionRouteChangeReasonOldDeviceUnavailable`
  - 如果播放器当前正在播，则自动暂停
  - 记录 `m_paused_by_route_change = YES`

- 当 reason 为新设备接入或其他非移除场景
  - 不自动恢复

说明：

- 首版只做“旧设备移除 -> 暂停”
- 不做“重新插入 -> 自动恢复”

## 5. HJPlayerAudioSessionController 设计

### 5.1 实例粒度

- 每个 `HJMusicPlayer` 持有一个独立的 `HJPlayerAudioSessionController`
- controller 内部统一访问 `HJAudioSession.sharedInstance`

这样可以让每个 player 独立维护：

- 是否接管 `AudioSession`
- interruption 前是否正在播放
- 是否因为打断暂停
- 是否因为 route change 暂停

### 5.2 owner 协议

controller 不直接依赖 `HJMusicPlayer`，而通过一个小协议访问 owner：

- 查询当前播放器状态
- 请求播放器执行 pause/resume

建议协议语义：

- `hj_audioSessionControllerIsPlaying`
- `hj_audioSessionControllerIsPaused`
- `hj_audioSessionControllerPauseForReason:`
- `hj_audioSessionControllerResumeForReason:`

### 5.3 controller 对外职责

- 启动/停止监听 `HJLifeCycle`
- 在需要时配置/激活/停用 `HJAudioSession`
- 处理 interruption userInfo
- 处理 route change userInfo
- 维护自动恢复相关状态位

## 6. HJLifeCycle 改造

协议新增：

- `- (void)audioSessionRouteChange:(NSNotification *)notification;`

注册/注销新增：

- `AVAudioSessionRouteChangeNotification`

`HJLifeCycle` 保持轻量，只负责通知分发，不解析 reason，不做 pause/resume 决策。

## 7. HJMusicPlayer 改造

`HJMusicPlayer` 增加：

- 新 option 常量 `HJMusicPlayerOptionManageAudioSession`
- 一个 `HJPlayerAudioSessionController *`
- 在 `init` 时根据 option 初始化 controller
- 在 `prepare/openURL/resume/pause/stop/destroy` 调 controller 对应入口
- 实现 controller owner 协议

调用协同：

- `prepare/openURL/resume`
  - 先让 controller 做“播放前准备”
  - 如果配置/激活失败，返回错误并通知 delegate

- `pause`
  - 执行 player pause 后，通知 controller 更新内部状态

- `stop/destroy`
  - 先 reset player，再通知 controller 做清理和必要的 session 停用

## 8. 测试策略

为了控制改动范围，仍采用独立 ObjC++ 测试文件，不先接入 CMake 测试目标。

测试分两层：

1. `HJPlayerAudioSessionController` 测试
   - `manageAudioSession = NO` 时不设置/激活 session
   - interruption begin 会 pause
   - interruption end 只有满足 `shouldResume` 且中断前在播才 resume
   - `OldDeviceUnavailable` 会 pause

2. `HJMusicPlayer` 集成测试
   - option 默认值是 `NO`
   - `destroy` 后注销监听
   - controller 可驱动 player pause/resume

## 9. 后续扩展点

后续如果需要，可在下一轮继续增加：

- `AVAudioSessionMediaServicesWereLost/Reset` 处理
- 耳机重新插入自动恢复策略
- 多播放器 session 仲裁
- route change 细粒度策略，如蓝牙优先、speaker fallback
