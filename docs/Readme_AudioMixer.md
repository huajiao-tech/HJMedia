# AudioMixer

## 概述

`HJAudioMixerJni` 是 Android 侧音频混音 JNI 封装，位于：

`examples/android/HJMedia/hjmediasdk/src/main/java/com/HJMediasdk/entry/player/HJAudioMixerJni.java`

它负责：

- 初始化混音器
- 接收多路 PCM 输入
- 控制每路输入音量和静音
- 控制总静音
- 刷新或移除输入
- 把混音后的 PCM 通过 raw 回调或 UI 回调返回给 Java

相关配置类：

- `HJAudioMixerConfig`
- `HJAudioMixerInputDesc`

## 全局 Context 初始化 播放器和混音器共用一个上下文/一个动态库，故APP启动时初始化一次即可

`HJPlayerContextJni` 是播放器/混音模块共用的静态全局上下文初始化入口，位于：

`examples/android/HJMedia/hjmediasdk/src/main/java/com/HJMediasdk/entry/player/HJPlayerContextJni.java`

关键点：

- `contextInit(...)` 是静态全局初始化，只会真正执行一次
- 内部通过 `s_bContextInit` 和同步锁保证多线程安全
- `contextUnInit()` 对应全局释放
- 混音器和播放器都依赖这一层 context，建议在使用前统一初始化

推荐调用方式：

```java
String logDir = getFilesDir().getAbsolutePath() + File.separator + "player";
HJPlayerContextJni.contextInit(
        logDir,
        HJCommonInterface.HJLOGLevel_INFO,
        HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
        5 * 1024 * 1024,
        5);
```

## 常量

`HJAudioMixerJni` 当前暴露了两个常量：

```java
public static final int AV_SAMPLE_FMT_S16 = 1;
public static final int HJAudioListenerFlag_PCM = 0x01;
```

说明：

- `AV_SAMPLE_FMT_S16` 表示 16-bit PCM
- `HJAudioListenerFlag_PCM` 表示回调里是 PCM 数据

## HJAudioMixerConfig

`HJAudioMixerConfig` 位于：

`examples/android/HJMedia/hjmediasdk/src/main/java/com/HJMediasdk/entry/player/HJAudioMixerConfig.java`

字段：

```java
public int outputSampleRate = 48000;
public int outputChannels = 2;
public int outputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
public int maxInputs = 8;
public int frameSamples = 1024;
public int syncWindowMs = 42;
public int lateThresholdMs = 150;
public boolean enableCompand = true;
public boolean enableLimiter = true;
public List<HJAudioMixerInputDesc> inputs = new ArrayList<>();
```

说明：

- `outputSampleRate/outputChannels/outputSampleFmt`: 混音输出格式
- `maxInputs`: 最多输入路数
- `frameSamples`: 单帧采样数
- `syncWindowMs`: 多路同步窗口
- `lateThresholdMs`: 输入晚到阈值
- `enableCompand`: 是否启用压扩
- `enableLimiter`: 是否启用 limiter
- `inputs`: 预注册输入描述列表

## HJAudioMixerInputDesc

`HJAudioMixerInputDesc` 位于：

`examples/android/HJMedia/hjmediasdk/src/main/java/com/HJMediasdk/entry/player/HJAudioMixerInputDesc.java`

字段：

```java
public String inputId = "";
public int sampleRate = 48000;
public int channels = 2;
public int sampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
public float volume = 1.0f;
public boolean muted = false;
```

`inputId` 是每一路输入的唯一标识，后续 `pushPcm`、`setInputVolume`、`setInputMute`、`flushInput`、`removeInput` 都依赖它。

## 初始化与释放

接口：

```java
public int init(HJAudioMixerConfig config,
                HJBaseListener listener,
                HJAudioRawPcmListener rawListener,
                HJAudioListener.UI audioListener)
public void done()
public void release()
```

说明：

- `listener`: 通用事件回调
- `rawListener`: 直接拿 native PCM 指针的回调
- `audioListener`: JNI 内部复制到 `ByteBuffer` 后的 UI 回调
- `release()` 内部直接调用 `done()`

## 输入控制接口

```java
public int setInputVolume(String inputId, float volume)
public int setInputMute(String inputId, boolean muted)
public int flushInput(String inputId)
public int removeInput(String inputId, boolean drain)
```

说明：

- `setInputVolume(...)`: 调整单路输入音量
- `setInputMute(...)`: 控制单路输入静音
- `flushInput(...)`: 清空该输入当前缓存
- `removeInput(inputId, drain)`: 删除输入
  - `drain=true`: 尽量排空残留数据再移除
  - `drain=false`: 直接移除

## 总控接口

```java
public int setMasterMute(boolean muted)
public boolean isMasterMuted()
```

用于控制混音器总输出静音。

## PCM 输入接口

### 1. 直接推 native 指针

```java
public int pushNativePcm(String inputId,
                         long dataPtr,
                         int size,
                         int samples,
                         int channels,
                         int sampleRate,
                         int sampleFmt,
                         long ptsMs)
```

适合上游已经在 native 层持有 PCM 数据的场景。

### 2. 推 Java ByteBuffer

```java
public int pushPcm(String inputId,
                   ByteBuffer buffer,
                   int size,
                   int samples,
                   int channels,
                   int sampleRate,
                   int sampleFmt,
                   long ptsMs)
```

适合 Java 侧自己生成或处理 PCM 后送入混音器。

参数说明：

- `inputId`: 对应的输入路 ID
- `size`: 当前 PCM 字节数
- `samples`: 当前 PCM 采样数
- `channels`: 声道数
- `sampleRate`: 采样率
- `sampleFmt`: 采样格式
- `ptsMs`: 时间戳，单位毫秒

## 回调

### HJBaseListener

接口：

```java
int onNotify(int id, long value, String desc);
```

用途：

- 接收 mixer 事件
- `desc` 可能包含输入状态或混音统计信息

### HJAudioRawPcmListener

接口：

```java
int notify(int sampleRate,
           int channels,
           int sampleFmt,
           int bytesPerSample,
           long data,
           int size,
           int flag,
           long ptsMs);
```

用途：

- 直接拿到 native PCM 指针
- 适合继续往 native 链路转发，避免多一次 Java 拷贝

### HJAudioListener.UI

接口：

```java
int notify(int sampleRate,
           int channels,
           int sampleFmt,
           int bytesPerSample,
           ByteBuffer buffer,
           int flag);
```

用途：

- 适合 Java/UI 层使用
- `HJAudioMixerJni` 内部会通过 `nativeAcquirePCM(...)` 把 native PCM 拷贝到复用的 `DirectByteBuffer`

## 推荐调用顺序

1. `HJPlayerContextJni.contextInit(...)`
2. 构造 `HJAudioMixerConfig`
3. `new HJAudioMixerJni()`
4. `init(config, listener, rawListener, audioListener)`
5. 通过 `pushNativePcm(...)` 或 `pushPcm(...)` 持续送入各路 PCM
6. 运行期间按需调用 `setInputVolume`、`setInputMute`、`setMasterMute`
7. 结束时调用 `done()`
8. 不再使用播放器/混音器时调用 `HJPlayerContextJni.contextUnInit()`

## 最小示例

```java
HJPlayerContextJni.contextInit(
        logDir,
        HJCommonInterface.HJLOGLevel_INFO,
        HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
        5 * 1024 * 1024,
        5);

HJAudioMixerConfig config = new HJAudioMixerConfig();
config.outputSampleRate = 48000;
config.outputChannels = 2;
config.outputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
config.maxInputs = 2;
config.inputs.add(new HJAudioMixerInputDesc("player0", 48000, 2, HJAudioMixerJni.AV_SAMPLE_FMT_S16));
config.inputs.add(new HJAudioMixerInputDesc("player1", 48000, 2, HJAudioMixerJni.AV_SAMPLE_FMT_S16));

HJAudioMixerJni mixer = new HJAudioMixerJni();
int ret = mixer.init(
        config,
        (id, value, desc) -> 0,
        (sampleRate, channels, sampleFmt, bytesPerSample, data, size, flag, ptsMs) -> 0,
        (sampleRate, channels, sampleFmt, bytesPerSample, buffer, flag) -> 0);

if (ret >= 0) {
    mixer.setInputVolume("player0", 1.0f);
    mixer.setInputMute("player1", false);
}

// 销毁
mixer.done();
HJPlayerContextJni.contextUnInit();
```

## 参考示例

完整示例可参考：

`examples/android/HJMedia/app/src/main/java/com/example/ui/AudioMixerActivity.java`
