# HJAudioCompositeManagerChange

## 概述

本文档用于说明 `HJAudioCompositeManager` 从旧版单 player 设计迁移到当前多 player 设计后的主要变化点。

本次变化不只是接口改名，而是内部状态管理、回调模型、Activity UI 结构和 mixer 输入拓扑都发生了调整。

## 1. 整体模型变化

### 旧版

- manager 内部只有一个 `HJMusicPlayerJni`
- player mixer input 固定写死为 `"player"`
- Activity 只有一套 player 控件
- listener 只有一套，默认绑定当前单 player

### 新版

- manager 内部改成 `Map<String, PlayerSession>`
- 外部必须传入 `uniquePlayerName`
- 每个 player 拥有独立 `HJMusicPlayerJni`
- 每个 player 拥有独立 `listener`
- 每个 player 拥有独立 `mixerInputId`
- Activity 固定接入 `player0` 和 `player1` 两套 panel

## 2. player 主键变化

### 旧版

- 没有外部 player key
- manager 内部默认只有一个 player

### 新版

- 外部调用方必须传入 `uniquePlayerName`
- 这个 key 直接作为 manager 内部 player 主键
- 同一个 key 重复 open 时，旧实例会先关闭，再创建新实例

影响：

- 调用方必须明确知道自己在操作哪个 player
- 回调方也能明确知道事件来源于哪个 player

## 3. 回调接口变化

### 旧版

旧接口：

```java
public interface PlayerOpenListener
{
    void onTracksReady(int[] tracks);
    void onDurationReady(long durationMs);
    void onEof();
}
```

问题：

- 回调里没有 player 身份信息
- 一旦扩展到多实例，回调无法判断属于哪个 player

### 新版

新接口：

```java
public interface PlayerListener
{
    void onTracksReady(String uniquePlayerName, int[] tracks);
    void onDurationReady(String uniquePlayerName, long durationMs);
    void onEof(String uniquePlayerName);
}
```

收益：

- 每次回调都明确携带 `uniquePlayerName`
- Activity 可以按 key 精确分发到对应 panel
- 避免多实例状态串线

## 4. manager 内部结构变化

### 旧版

旧版主要字段：

- `HJMusicPlayerJni mMusicPlayer`
- `PlayerOpenListener mPlayerOpenListener`
- 固定 mixer input：`"player"`

### 新版

新增 `PlayerSession` 概念：

- `String uniquePlayerName`
- `String mixerInputId`
- `HJMusicPlayerJni musicPlayer`
- `PlayerListener listener`
- `float playerVolume`
- `float mixVolume`
- `boolean mute`
- `long durationMs`
- `int[] trackIds`
- `boolean mixerInputAdded`

并引入：

- `Map<String, PlayerSession> mPlayerSessionMap`

结果：

- player 生命周期从“单对象字段”变成“按 key 管理的 session”
- player 状态缓存不再互相覆盖

## 5. mixer 输入模型变化

### 旧版

- 固定输入：
  - `mic`
  - `player`
- `maxInputs = 2`

### 新版

- 固定输入：
  - `mic`
- 动态输入：
  - `player:<uniquePlayerName>`
- 当前 `maxInputs = 3`

原因：

- 当前 demo 固定有 `player0` 和 `player1`
- 需要覆盖 `mic + player0 + player1`

注意：

- 如果后续要增加第三个 player，需要继续调大 `maxInputs`

## 6. 混音开关语义变化

### 旧版

- `setEnableMixing(boolean)` 只控制唯一那个 player 是否参与混音

### 新版

- `setEnableMixing(boolean)` 仍然是全局开关
- 但它会统一作用到所有已打开的 player
- 打开时统一 add input
- 关闭时统一 remove input

保留不变：

- mic 输入始终是全局的
- mic 音量仍然是全局的
- master mute 仍然是全局的

## 7. 音量控制变化

### 旧版

- `playerSetVolume(float volume)`：控制唯一 player 的本地音量
- `mixerSetPlayerVolume(float volume)`：控制唯一 player 的 mixer 输入音量

### 新版

- `playerSetVolume(String uniquePlayerName, float volume)`：按 key 控制本地音量
- `mixerSetPlayerVolume(String uniquePlayerName, float volume)`：按 key 控制 mixer 输入音量

新的行为边界：

- player volume：影响 player 自身播放
- mix volume：影响 player 送入 mixer 的比例
- 两者按 player 独立保存，不互相覆盖

## 8. 生命周期与竞态处理变化

### 旧版

- 关闭 player 时直接释放单个 `mMusicPlayer`
- 回调默认认为只会来自当前唯一实例

风险：

- 如果未来扩成多实例，旧回调容易串到新状态

### 新版

- `playerClose(uniquePlayerName)` 只关闭对应 key 的 session
- 回调时会校验：
  - session 是否仍存在
  - 当前 `HJMusicPlayerJni` 是否仍是该 session 的活跃实例

收益：

- close/reopen 后旧实例事件不会污染新实例状态
- 双 player 并发时互不干扰

## 9. Activity 结构变化

### 旧版

- `AudioCompositeActivity` 只有一套 UI：
  - 一个 URL
  - 一组 open/close/pause/resume
  - 一组 seek
  - 一组 player volume
  - 一组 mix volume
  - 一组 tracks
  - 一个状态区

### 新版

- Activity 内引入 `PlayerPanel`
- 固定创建两个 panel：
  - `player0`
  - `player1`
- 每个 panel 独立拥有：
  - URL
  - open/close/pause/resume
  - progress/time
  - player volume
  - mix volume
  - tracks
  - status

回调分发方式：

- manager 回调带回 `uniquePlayerName`
- Activity 根据 key 找到对应 `PlayerPanel`
- 只更新自己的 panel

## 10. 默认演示资源变化

### 当前 demo

- `player0` 默认文件：`c58733ac51124fe38cdc6540a7b8fa46.mkv`
- `player1` 默认文件：`momoda.mp3`

这意味着当前示例已经不再是“一个默认地址”，而是“两路默认地址”。

## 11. 典型接口迁移对照

### 打开 player

旧版：

```java
manager.playerOpen(path, listener);
```

新版：

```java
manager.playerOpen("player0", path0, listener0);
manager.playerOpen("player1", path1, listener1);
```

### 暂停 player

旧版：

```java
manager.playerPause();
```

新版：

```java
manager.playerPause("player0");
manager.playerPause("player1");
```

### 设置 mixer 中的 player 音量

旧版：

```java
manager.mixerSetPlayerVolume(0.8f);
```

新版：

```java
manager.mixerSetPlayerVolume("player0", 0.8f);
manager.mixerSetPlayerVolume("player1", 0.5f);
```

### 切换音轨

旧版：

```java
manager.playeSwitchAudioTrack(trackId);
```

新版：

```java
manager.playerSwitchAudioTrack("player0", trackId);
```

兼容说明：

- 当前源码里仍保留了 `playeSwitchAudioTrack(String uniquePlayerName, int trackId)` 作为转发方法
- 但主接口已经变成 `playerSwitchAudioTrack(...)`

## 12. 迁移后的收益

本次改造带来的直接收益：

- 支持多个 player 同时播放并参与混音
- player 状态、音量、seek、track 切换互不影响
- 回调具备来源标识，UI 可以精确更新
- manager 结构从单实例字段提升为可扩展的 session 管理模型

当前边界：

- 当前 demo UI 固定只做了两个 player：`player0`、`player1`
- 当前 `DEFAULT_MAX_INPUTS = 3`，只覆盖 `mic + 2 player`
- 如果后续继续扩展 player 数量，需要同步扩大 mixer 输入上限和 UI 结构
