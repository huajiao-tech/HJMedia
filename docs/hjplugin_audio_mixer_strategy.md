# HJPluginAudioMixer 混音策略总结

本文档基于当前代码实现整理，重点描述 `HJPluginAudioMixer` 的调度策略，而不是设计设想。核心代码位于：

- `src/plugins/HJPluginAudioMixer.cpp`
- `src/media/HJAudioMixer.cc`
- `src/graphs/HJGraphAudioMixer.cpp`
- `examples/windows/XMediaTest/XMediaTest.cc`

其中：

- `HJPluginAudioMixer` 负责时序、等待、首包对齐、补静音、丢旧帧
- `HJAudioMixer` 负责把本轮准备好的多路音频帧送入 FFmpeg `amix`
- `HJGraphAudioMixer` 负责 graph 层输入管理和对外接口

## 1. 整体原则

`HJPluginAudioMixer` 的核心目标不是“严格等所有输入完全对齐后再输出”，而是：

1. 用 mixer 自己的输出时间轴推进混音
2. 启动时适度等一等首包
3. 稳态时给慢路一个有限等待窗口
4. 超过窗口后不阻塞整体，改为补静音或丢旧帧

因此它的行为更接近“低延迟、容错推进”的实时混音器，而不是离线精确对齐器。

## 2. 输出时间轴

`HJPluginAudioMixer` 内部维护 `m_mix_samples` 和 `m_output_frames`。

- `m_mix_samples` 表示当前混音点在输出采样轴上的位置
- `mix_pts` 由 `m_mix_samples` 按输出采样率换算成毫秒
- 每成功输出一帧后，`m_mix_samples += frame_samples`

这意味着：

- 一旦开始输出，后续时间轴按 `frame_samples` 固定步长推进
- 单路输入变慢不会把整体时钟拖住
- 输出 PTS 依赖 sample timeline，而不是每轮简单累加毫秒，能避免累计舍入漂移

## 3. 首包对齐

### 3.1 起混前提

`runTask()` 在真正混音前会先做两层门控：

1. 必须至少有一个 `attached` 输入
2. 当前至少有一路输入队列里存在 pending frame

否则直接返回 `HJ_WOULD_BLOCK`，本轮不输出。

### 3.2 起混条件

启动阶段通过 `m_output_frames == 0` 判定。

启动时只统计：

- 已 attach
- 当前未 EOF
- 未 mute

的输入作为待等输入。

关键计数：

- `startup_wait_inputs`：启动阶段需要考虑的输入数
- `startup_ready_inputs`：这些输入里当前队头已经是普通音频帧的数量
- `queued_duration_max`：所有输入队列里最大的音频缓存时长

启动判定规则如下：

1. 如果只有 1 路待等输入，则该路有首帧就可以起混
2. 如果有多路待等输入，则需要满足：
   - 至少有 1 路 ready
   - 并且“所有待等路都 ready”或“达到等待截止线”

等待截止线由 `wait_deadline_reached` 决定：

- 当 `late_threshold_ms <= 0` 时，视为立即到达截止线
- 否则当 `queued_duration_max >= late_threshold_ms` 时到达截止线

这说明当前实现的启动策略是：

- 优先等多路首包
- 但不会无限等
- 一旦快路已经积压到阈值，就允许先开混

### 3.3 起混锚点

启动阶段真正采用的起混锚点是：

- 所有 startup-ready 路中最早的 `first_pts`

然后把它换算成 sample timeline，写入 `m_mix_samples`。

所以首个 `mix_pts` 不是固定从 0 开始，也不是“谁先触发 runTask 就锚谁”，而是“当前 ready 首帧里最早的那个 PTS”。

### 3.4 首轮是否参与混音

即使已经满足起混条件，也不代表所有路的首帧都会进首轮混音。

当前首轮消费条件统一是：

`preview_pts <= mix_pts + sync_window_ms`

也就是说：

- 队头首帧在当前 `mix_pts` 的未来容忍窗口内，可以直接消费
- 队头首帧比当前轮次晚太多，则本轮不会提前消费
- 这种情况下该路本轮会补静音，首帧继续留在队列中等待后续轮次

当前实现已经不再对“未见过首帧”的路做无条件消费。

## 4. 冷启动阶段

冷启动阶段的关键特点是“先定锚，再逐路决定本轮给真帧还是静音”。

### 4.1 冷启动不做 late drop

启动阶段不会先清旧帧，代码上只有非 startup 分支才允许 `dropLatePreviewFrames()`。

因此启动阶段队头判断只看当前 preview frame：

- 如果是普通帧，参与 startup-ready 判断
- 如果是 `ClearFrame`，直接触发全局 flush/reset
- 如果是 `EOF`，该路视作不再参与启动等待

这意味着冷启动更偏向“直接按当前首包开局”，而不是“先清 backlog 再对齐”。

### 4.2 冷启动逐路决策

定好 `mix_pts` 以后，每一路只会落到下面几类结果之一：

1. 队头是 `EOF`
   - 消费 EOF
   - 该路置 `m_eof = true`
   - 当前轮不取真实音频
2. 队头是普通帧，且满足 `preview_pts <= mix_pts + sync_window_ms`
   - 消费真实帧
   - 该路置 `m_seen_first_frame = true`
3. 队头不存在，或存在但不在消费窗口内
   - 本轮补静音
   - `silence_fill_frames` 递增

如果某路本轮没有拿到真实帧，会被标记为 `starved`；之后重新拿到真实帧时，会切回 `recovered`。

## 5. 稳态阶段

当 `m_output_frames > 0` 后进入稳态。

稳态和冷启动最大的差异有两点：

1. 允许 late drop
2. 存在短暂 steady wait 机制

### 5.1 稳态先做 late drop

稳态下每一路在真正决定本轮是否取帧前，会先执行 `dropLatePreviewFrames()`。

当前 late drop 规则是循环执行的：

`preview_pts + late_drop_window_ms < mix_pts`

只要队头仍然明显落后于当前混音点，就持续丢弃，直到：

- 队头追到窗口内
- 队列为空
- 遇到 `ClearFrame`
- 遇到 `EOF`

这里的 `late_drop_window_ms` 取值为：

- `late_threshold_ms > 0` 时，用 `late_threshold_ms`
- 否则退化为 `sync_window_ms`

这表示当前代码里：

- `late_threshold_ms` 主要决定“过去方向”的丢帧容忍度
- `sync_window_ms` 主要决定“未来方向”的消费窗口

### 5.2 稳态等待慢路

稳态等待只对下面这类输入生效：

- 已 attach
- 未 mute
- 已经见过首个真实帧，即 `m_seen_first_frame == true`
- 当前未 EOF

这些路构成 `steady_wait_inputs`。

如果满足以下条件，当前轮会返回 `HJ_WOULD_BLOCK`，选择再等一轮：

1. `late_threshold_ms > 0`
2. `steady_wait_inputs > 1`
3. `steady_ready_inputs < steady_wait_inputs`
4. 还没有达到等待截止线

这说明稳态等待有几个明显特征：

- 只在多路稳态输入之间生效
- 单路时不会 steady wait
- 新接入但还没真正出过声的路，不会拖住稳态等待
- 到达截止线后就不再继续等，直接推进

### 5.3 稳态逐路决策

late drop 之后，每路本轮仍按统一规则处理：

1. `EOF`
   - 消费 EOF
   - 标记该路结束
2. 在消费窗口内
   - 取真实帧
3. 无帧或未来帧太晚
   - 补静音

所以稳态的核心原则是：

- 过去太久的帧丢掉
- 当前可用的帧消费
- 未来太晚的帧留待后续轮次
- 当前轮不能等太久就用静音补齐

## 6. 各种边界行为

### 6.1 热插入新路

新路 attach 以后，在稳态中：

- 该路本身允许先做 late drop
- 但在 `m_seen_first_frame` 之前，不会进入 steady wait 集合

结果是：

- 新路如果 backlog 很旧，会先被追帧
- 新路如果首帧太未来，本轮先静音，不会强行提前并入
- 新路不会把已经稳定运行的混音整体卡住

这是一种偏低延迟的热插入策略。

### 6.2 mute

被 mute 的路：

- 仍然 attach 在 mixer 中
- 但不参与 startup/steady 等待计数
- 本轮即使拿到真实帧，最终也会改用静音帧送入 `HJAudioMixer`

所以 mute 的语义更接近“保留输入位，但混音结果按静音处理”。

### 6.3 EOF

某一路遇到 `EOF` 时：

- 当轮消费 EOF 控制帧
- 该路置 `m_eof = true`
- 后续不再参与真实取帧

只要还有其他未 EOF 的 attach 路，混音仍会继续。

### 6.4 all_eof

当前实现里 `all_eof` 的判定时机在本轮逐路消费之前。

因此存在一个边界：

- 如果“最后一路”的 EOF 恰好是在本轮才被消费到
- 那么本轮开始时 `all_eof` 仍然会被判断为 `false`
- 这一轮仍可能继续走一次 `mixFrames()`

所以最后一路 EOF 的那一拍，理论上可能多产出一帧纯静音输出；下一轮才真正进入 `all_eof` 并停止继续输出。

### 6.5 ClearFrame

`ClearFrame` 不是单路局部 flush，而是全局 reset 信号。

任一路队头遇到 `ClearFrame` 时，行为都是：

1. 消费该控制帧
2. `m_mix_samples` 清零
3. `m_output_frames` 清零
4. 清空各路运行态，包括 `seen_first_frame / starved / eof`
5. 执行 `runFlush()`

之后 mixer 会重新进入一次新的冷启动流程。

### 6.6 无输入或无 pending

如果没有 attach 输入，或者所有输入队列都没有 pending frame：

- 当前轮直接 `HJ_WOULD_BLOCK`
- 不会凭空生成静音混音帧

这说明当前实现不会在“完全没数据”的情况下主动维持空跑时钟。

## 7. 参数语义

### 7.1 `frame_samples`

定义每轮目标输出帧的采样点数，直接决定：

- 单帧输出时长
- `mix_pts` 推进步长
- 静音补帧大小

### 7.2 `sync_window_ms`

控制“未来方向”的消费窗口。

当队头普通帧满足：

`preview_pts <= mix_pts + sync_window_ms`

时，本轮允许直接消费。

因此它主要影响：

- 首包对齐时晚到首帧能否跟上当前轮
- 稳态时未来帧能否提前被接纳

### 7.3 `late_threshold_ms`

当前代码里它承担两个角色：

1. startup/steady wait 的截止线
2. 稳态 late drop 的过去方向容忍窗口

具体表现为：

- `queued_duration_max >= late_threshold_ms` 时，不再继续等慢路
- `preview_pts + late_threshold_ms < mix_pts` 时，旧帧会被丢弃

如果它小于等于 0，则：

- wait_deadline 视为立即到达
- late drop 退化为使用 `sync_window_ms`

### 7.4 `enable_compand` 与 `enable_limiter`

这两个参数不改变 `HJPluginAudioMixer` 的调度逻辑，只影响 `HJAudioMixer` 底层 filter graph：

- `enable_compand`：在 `amix` 后串 `compand`
- `enable_limiter`：在末尾串 `alimiter`

## 8. 与 HJAudioMixer 的分工

`HJPluginAudioMixer` 并不直接做 sample 级累加。

它做的是：

1. 为每个 slot 选出本轮输入帧，或者构造静音帧
2. 形成 `mix_frames`
3. 把这一轮的帧集合交给 `HJAudioMixer::mixFrames()`

`HJAudioMixer` 再把这些输入帧送入 FFmpeg `AVFilterGraph`，由 `amix` 完成真正混音。

所以 plugin 层关注的是“这轮每路给什么”，而 media 层关注的是“拿到这些帧后如何合成一帧输出”。

## 9. 当前实现一句话总结

当前 `HJPluginAudioMixer` 的实际策略可以概括成：

“启动时等到最小可开混条件，用最早 ready 首帧做锚点；稳态时先丢明显过旧的帧，再有限等待慢路，超时后补静音推进；整体混音时间轴始终由 mixer 自己的 sample timeline 驱动，不被单路拖死。”

## 10. 推荐关注的回归场景

建议持续保留下面这些测试场景：

1. 多路首帧存在明显偏移，验证起混锚点和首轮补静音行为
2. 单路 backlog 堆积多帧旧包，验证 late drop 会循环追平
3. 热插入一路，验证不会把未来首帧强行提前并入
4. 稳态缺帧，验证 `starved` 和 `recovered` 状态切换
5. 最后一路 EOF，验证停止输出时机
6. `ClearFrame` 触发全局 reset 后重新冷启动

