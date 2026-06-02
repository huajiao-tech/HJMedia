# HJPluginAudioOHRender

## 用途
OH 平台的音频渲染实现，基于 OH 音频回调拉取数据并输出。通过 `fillAudioBuffer` 从输入队列获取音频帧并写入系统缓冲。

## 主要接口

| 接口 | 参数 | 返回 | 线程限制/说明 |
| --- | --- | --- | --- |
| `flush(size_t srcKeyHash)` | `srcKeyHash`：输入队列键 | `HJ_OK`/错误码 | 清空输入队列。 |
| `onDataCallback(void* audioData, int32_t audioDataSize)` | `audioData`：输出缓冲区，`audioDataSize`：字节数 | `OH_AudioData_Callback_Result` | 系统音频回调线程；必须保持回调轻量。 |
| `initRender(const HJAudioInfo::Ptr& info)` | `info` | `HJ_OK`/错误码 | 创建并启动 OH 渲染器。 |
| `releaseRender()` | - | `void` | 停止并释放 OH 渲染器与 builder。 |

## 生命周期与线程安全
- `initRender` 创建 `OH_AudioStreamBuilder` 并生成渲染器，随后启动渲染。
- `onDataCallback` 在系统音频回调线程触发，内部调用 `fillAudioBuffer` 获取数据；回调必须轻量，避免阻塞。
- `releaseRender` 负责停止、flush 并释放渲染器资源。

## 常见用法/清理示例
```cpp
auto render = HJCreates<HJPluginAudioOHRender>();
auto param = HJKeyStorage::create();
(*param)["audioInfo"] = audioInfo;
(*param)["timeline"] = timeline;
(*param)["pluginListener"] = listener;
render->init(param);

// 停止与清理
render->done();
```
