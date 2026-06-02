# HJAudioCompositeManager

## 概述

`HJAudioCompositeManager` 当前版本是一个“麦克风采集 + 多 player 输入 + AAC 混音输出”的 Android 音频合成管理器。

核心职责：

- 使用 `HJAudioCaptureCtrl` 采集麦克风 PCM
- 使用多个 `HJMusicPlayerJni` 打开不同音频源
- 通过 `HJAudioMixerJni` 把 `mic + N 个 player` 做混音
- 通过 `HJAudioListener` 向上层回调 AAC header 和 AAC 音频帧

当前实现对应源码：

- `examples/android/HJMedia/hjmediasdk/src/main/java/com/HJMediasdk/audio/HJAudioCompositeManager.java`
- `examples/android/HJMedia/app/src/main/java/com/example/ui/AudioCompositeActivity.java`
- `examples/android/HJMedia/app/src/main/java/com/example/audio/AudioEffectSimulate.java`

当前版本的关键点：

- player 由外部传入 `uniquePlayerName` 管理
- manager 内部使用 `Map<String, PlayerSession>` 保存多实例 player 状态
- 每个 player 都有独立 listener、独立 mixer input、独立 player 音量、独立 mix 音量
- mixer 仍然是全局单例
- `setEnableMixing(boolean)` 仍然是全局开关
- mixer 输出固定为 AAC，可选 ADTS 或 RAW

## 内部结构

### PlayerSession

每个 player 在 manager 内部对应一个 `PlayerSession`，主要包含：

- `uniquePlayerName`：外部传入的 player 唯一 key
- `mixerInputId`：送入 mixer 的 input id，当前格式为 `player:<uniquePlayerName>`
- `HJMusicPlayerJni musicPlayer`
- `PlayerListener listener`
- `float playerVolume`
- `float mixVolume`
- `boolean mute`
- `long durationMs`
- `int[] trackIds`
- `boolean mixerInputAdded`

### 全局资源

manager 自身只维护一套全局资源：

- `HJAudioCaptureCtrl mAudioCaptureCtrl`
- `HJAudioMixerJni mAudioMixer`
- `HJAudioListener mAudioListener`
- `CapturePcmListener mCapturePcmListener`

这意味着：

- 麦克风采集是全局的
- AAC 输出是全局的
- master mute 是全局的
- mic 音量是全局的
- enable mixing 是全局的

## 公开回调

### CapturePcmListener

接口：

```java
public interface CapturePcmListener
{
    int onCapturePcm(ByteBuffer buffer, int samples, int sampleBytes, int channels);
}
```

说明：

- 回调的是麦克风采集到的原始 PCM
- `buffer` 为当前一帧 PCM
- `samples` 为单声道采样数
- `sampleBytes` 为单采样字节数
- `channels` 为声道数
- 返回值 `< 0` 时，本次采集帧不会继续送入 mixer

典型用途：

- 实时变声、混响、EQ、增益
- 录制前 PCM 预处理
- 采集侧波形分析或音量统计

### PlayerListener

接口：

```java
public interface PlayerListener
{
    void onTracksReady(String uniquePlayerName, int[] tracks);
    void onDurationReady(String uniquePlayerName, long durationMs);
    void onEof(String uniquePlayerName);
}
```

说明：

- `uniquePlayerName` 是当前触发回调的 player 主键
- `onTracksReady(...)`：player 打开后返回可切换音轨 id 列表
- `onDurationReady(...)`：player 打开后返回总时长，单位毫秒
- `onEof(...)`：对应 player 播放到结尾时回调

注意：

- listener 是 per-player 绑定的，不是 manager 级共享 listener
- close/reopen 后旧实例回调会通过 session 校验被丢弃，避免串到新实例

### HJAudioListener

通过 `setListener(HJAudioListener listener)` 设置。

接口：

```java
public interface HJAudioListener
{
    int onExtraReady(ByteBuffer extra, int size);
    int onCapAAC(ByteBuffer outputFrame, int size, long sysTimeMs, int flag);
    int onErr(int extra);
}
```

说明：

- `onExtraReady(...)`
  - mixer 输出 AAC header / codec config 时回调
  - RAW AAC 模式下通常用于配置解码器
  - ADTS 模式下通常可以忽略
- `onCapAAC(...)`
  - 回调 AAC 音频数据
  - 当前内部统一用 `HJAudioListener.AAC_MDAT` 作为 `flag`
  - `setIsADTS(true)` 时输出 ADTS AAC
  - `setIsADTS(false)` 时输出 RAW AAC
- `onErr(...)`
  - 当前 manager 没有主动派发业务错误到这里

## 生命周期

### `HJAudioCompositeManager()`

仅创建 Java 层对象，不会初始化 capture、mixer 或 player。

### `int init()`

初始化内部模块：

- 创建并初始化 `HJAudioMixerJni`
- 创建并初始化 `HJAudioCaptureCtrl`
- 注册 capture 回调和 mixer 回调

当前实现细节：

- 方法上带 `@SuppressLint("MissingPermission")`
- 调用前仍要求已经授予 `RECORD_AUDIO`
- 重复调用会先释放旧的 capture、mixer 和全部 player session，再重新初始化

返回值：

- `0` 表示成功
- `< 0` 表示失败

### `void release()`

释放全部内部资源：

- 释放麦克风采集
- 关闭所有 player
- 释放 mixer

说明：

- 会遍历 `mPlayerSessionMap`，释放所有 `HJMusicPlayerJni`
- 调用后如果要继续使用，需要重新 `init()`

### `void captureClose()`

仅关闭麦克风采集，不释放 mixer 和 player。

适用场景：

- 改由业务侧自行调用 `pushCustomPCM(...)` 给 `mic` 输入路送数据
- 某些连麦场景需要停止系统采集，但保留混音器和 player

## 音频格式配置

### `void setCaptureAudioConfig(int sampleRate, int channels)`

设置麦克风采集格式。

- `sampleRate <= 0` 时回退默认 `44100`
- `channels` 会归一化到 `1` 或 `2`

### `void setPlayerAudioConfig(int sampleRate, int channels)`

设置 player 回调给 mixer 的目标格式。

- `sampleRate <= 0` 时回退默认 `48000`
- `channels` 会归一化到 `1` 或 `2`

### `void setMixAudioConfig(int sampleRate, int channels)`

设置 mixer 输出格式。

- `sampleRate <= 0` 时回退默认 `48000`
- `channels` 会归一化到 `1` 或 `2`

### `void setBitrate(int bitrate)`

保留接口。

说明：

- 历史上用于 AAC 编码码率设置
- 当前实现中该值不会实际下发到底层 mixer 编码器

### `void setIsADTS(boolean isADTS)`

设置 AAC 输出格式：

- `true`：输出 ADTS AAC
- `false`：输出 RAW AAC

## 混音控制

### `void setEnableMixing(boolean enableMixing)`

控制所有 player 是否参与 mixer。

说明：

- `true`：全部已打开 player 参与混音
- `false`：全部 player 从 mixer 中移除，只保留 `mic`
- 这是全局开关，不是 per-player 开关
- 关闭时不会关闭 player，仅停止 player PCM 送入 mixer

### `int mixerSetPlayerVolume(String uniquePlayerName, float volume)`

设置指定 player 在 mixer 里的输入音量。

说明：

- 影响的是 player 送入 mixer 的比例
- 不影响 player 自身本地播放音量
- 如果该 player 当前尚未加入 mixer，则只缓存音量，待加入时生效

### `int mixerSetMicVolume(float volume)`

设置 `mic` 输入路在 mixer 中的音量。

### `void mixerSetMute(boolean mute)`

设置 mixer 总静音。

说明：

- `true`：整体输出静音
- `false`：恢复输出

## 外部回调设置

### `void setListener(HJAudioListener listener)`

设置混音 AAC 输出监听器。

典型用途：

- 录制
- 推流
- AAC 数据缓存
- 解码回放

### `void setCapturePcmListener(CapturePcmListener listener)`

设置麦克风 PCM 监听器。

典型用途：

- 在采集 PCM 送入 mixer 前做音效处理
- 和 `AudioEffectSimulate#doEffect(...)` 配合

## 自定义 PCM 输入

### `int pushCustomPCM(int channels, int sampleRate, ByteBuffer buffer, long timeMs)`

手动向内部 `mic` 输入路推送一帧 PCM。

说明：

- input id 固定为 `mic`
- `sampleFmt` 固定为 `HJAudioMixerJni.AV_SAMPLE_FMT_S16`
- `bytesPerSample` 固定为 `2`
- `timeMs` 作为该帧送入 mixer 的时间戳

适用场景：

- `captureClose()` 后由业务侧自定义 mic PCM
- 接入外部语音或预处理后的 PCM 数据

## 多 Player 控制接口

### `int playerOpen(String uniquePlayerName, String url, PlayerListener listener)`

打开一个指定 key 的 player。

规则：

- `uniquePlayerName` 必须由外部传入，且不能为空
- `url` 不能为空
- 调用前必须先 `init()`
- 同 key 重复 open 时，旧 session 会先被关闭，再创建新实例

当前实现细节：

- 每个 player 内部对应一个独立 `HJMusicPlayerJni`
- `HJMusicPlayerJni.init(...)` 当前固定用 `repeats = 1`
- mixer input id 当前格式是 `player:<uniquePlayerName>`

### `void playerClose(String uniquePlayerName)`

关闭指定 key 的 player，并从 mixer 中移除对应输入。

### `int playerPause(String uniquePlayerName)`

暂停指定 player。

### `int playerResume(String uniquePlayerName)`

恢复指定 player。

### `int playerSeek(String uniquePlayerName, long timestamp)`

指定 player 跳转，单位毫秒。

### `int playerSetMute(String uniquePlayerName, boolean mute)`

设置指定 player 自身静音。

注意：

- 这是 player 自身静音，不是 mixer 总静音

### `boolean playerIsMuted(String uniquePlayerName)`

获取指定 player 当前静音状态。

### `int playerSetVolume(String uniquePlayerName, float volume)`

设置指定 player 自身音量。

注意：

- 这是本地播放音量
- 不等同于 mixer 输入音量

### `float playerGetVolume(String uniquePlayerName)`

获取指定 player 自身音量。

### `long playerGetCurrentTimestamp(String uniquePlayerName)`

获取指定 player 当前播放时间戳，单位毫秒。

### `long playerGetDuration(String uniquePlayerName)`

获取指定 player 当前缓存的总时长，单位毫秒。

说明：

- 值来自 `EVENT_GRAPH_STREAM_OPENED_ID` 事件后缓存的 duration

### `int playerSwitchAudioTrack(String uniquePlayerName, int trackId)`

切换指定 player 音轨。

### `int playeSwitchAudioTrack(String uniquePlayerName, int trackId)`

兼容保留方法，内部直接转发到 `playerSwitchAudioTrack(...)`。

## mixer 输入拓扑

当前 manager 初始化时，mixer 拓扑是：

- 固定输入：`mic`
- 动态输入：`player:<uniquePlayerName>`

当前 `maxInputs` 固定为 `3`，按当前 demo 足够覆盖：

- `mic`
- `player0`
- `player1`

如果后续 Activity 要扩展到 3 个以上 player，需要同步调整 `DEFAULT_MAX_INPUTS`。

## 与 AudioCompositeActivity 的关系

当前示例页：

- `examples/android/HJMedia/app/src/main/java/com/example/ui/AudioCompositeActivity.java`

当前 demo 实现方式：

- Activity 固定创建两个 panel：`player0` 和 `player1`
- 每个 panel 都有自己的 URL、open/close、pause/resume、seek、player volume、mix volume、tracks、time、status
- 每个 panel 调用 manager 时都传入自己的 `uniquePlayerName`
- manager 回调 `onTracksReady/onDurationReady/onEof` 时，Activity 按 `uniquePlayerName` 分发到对应 panel

当前默认资源路径：

- `player0` 默认文件：`c58733ac51124fe38cdc6540a7b8fa46.mkv`
- `player1` 默认文件：`momoda.mp3`

## 典型调用顺序

```java
HJAudioCompositeManager manager = new HJAudioCompositeManager();
manager.setCaptureAudioConfig(44100, 1);
manager.setPlayerAudioConfig(48000, 2);
manager.setMixAudioConfig(48000, 2);
manager.setIsADTS(true);
manager.setEnableMixing(true);
manager.setCapturePcmListener((buffer, samples, sampleBytes, channels) -> 0);
manager.setListener(new HJAudioListener() {
    @Override
    public int onExtraReady(ByteBuffer extra, int size) { return 0; }

    @Override
    public int onCapAAC(ByteBuffer outputFrame, int size, long sysTimeMs, int flag) { return 0; }

    @Override
    public int onErr(int extra) { return 0; }
});

int initRet = manager.init();
if (initRet >= 0) {
    manager.playerOpen("player0", path0, new HJAudioCompositeManager.PlayerListener() {
        @Override
        public void onTracksReady(String uniquePlayerName, int[] tracks) {}

        @Override
        public void onDurationReady(String uniquePlayerName, long durationMs) {}

        @Override
        public void onEof(String uniquePlayerName) {}
    });

    manager.playerOpen("player1", path1, new HJAudioCompositeManager.PlayerListener() {
        @Override
        public void onTracksReady(String uniquePlayerName, int[] tracks) {}

        @Override
        public void onDurationReady(String uniquePlayerName, long durationMs) {}

        @Override
        public void onEof(String uniquePlayerName) {}
    });
}
```

## AudioEffectSimulate

`AudioEffectSimulate` 是一个采集侧 PCM 音效处理辅助类，常见用法是在 `HJAudioCompositeManager#setCapturePcmListener(...)` 中调用。

源码：

- `examples/android/HJMedia/app/src/main/java/com/example/audio/AudioEffectSimulate.java`

常见配合方式：

- 在 `CapturePcmListener` 中调用 `doEffect(...)`
- 对采集侧 PCM 原地做处理后，再由 manager 送入 mixer
