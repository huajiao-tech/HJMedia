# HJPluginAudioWASRender

## 用途
Windows 平台的音频渲染实现，基于 WASAPI 事件回调驱动。负责初始化音频设备、获取渲染缓冲并写入音频数据，同时在系统默认设备变化时可重置设备。

## 主要接口

| 接口 | 参数 | 返回 | 线程限制/说明 |
| --- | --- | --- | --- |
| `resetDevice(const std::string& deviceName)` | `deviceName`：设备名，空表示默认设备 | `HJ_OK`/错误码 | 线程安全；设置事件并异步触发 `runReset()`。 |
| `onDefaultDeviceChanged()` | - | `void` | 由系统线程调用；内部转发到 `resetDevice()`。 |
| `runTask(int64_t* delay)` | `delay`：下一次调度延迟 | `HJ_OK`/错误码 | 运行在 handler 线程；等待事件并写入音频数据。 |
| `initRender(const HJAudioInfo::Ptr& info)` | `info` | `HJ_OK`/错误码 | 初始化 COM、设备枚举器、WASAPI 资源。 |
| `releaseRender()` | - | `void` | 释放设备、取消注册回调并 `CoUninitialize()`。 |
| `initDevice(const HJAudioInfo::Ptr& info)` | `info` | `HJ_OK`/错误码 | 初始化音频设备与渲染客户端。 |

## 生命周期与线程安全
- `initRender` 内部完成 COM 初始化、设备枚举器创建，并注册 `IMMNotificationClient`。
- `runTask` 在 handler 线程中等待音频事件，调用 `fillAudioBuffer` 填充缓冲区，并释放 WASAPI 缓冲。
- `resetDevice` 会设置重置事件并通过 handler 线程触发 `runReset()`，避免在系统回调线程中做重操作。
- `onDefaultDeviceChanged` 由系统回调线程触发，只做轻量逻辑与异步转发。

## 常见用法/清理示例
```cpp
auto render = HJCreates<HJPluginAudioWASRender>();
auto param = HJKeyStorage::create();
(*param)["audioInfo"] = audioInfo;
(*param)["timeline"] = timeline;
(*param)["pluginListener"] = listener;
(*param)["audioDeviceName"] = std::string("My Device"); // 可选
render->init(param);

// 设备切换
render->resetDevice("");

// 停止与清理
render->done();
```
