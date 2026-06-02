# HJPluginSpeedControl

## 用途
用于音频播放过程中的速度控制与自适应缓冲调节。该插件根据缓存观测数据与配置参数，动态调整音频处理器速度，输出带 speed 信息的音频帧，供渲染侧更新时间轴并实现平滑播放。

## 主要接口

| 方法 | 参数 | 返回 | 线程限制/说明 |
| --- | --- | --- | --- |
| `init(HJKeyStorage::Ptr param)` | `audioInfo`(必填), `thread`(可选共享线程), `pluginListener`(可选回调) | `HJ_OK`/错误码 | 线程安全；由 `HJPlugin` 管理；可能创建内部线程并在 `afterInit` 触发调度 |
| `deliver(size_t srcKeyHash, HJMediaFrame::Ptr& frame)` | 上游投递帧 | `HJ_OK`/错误码 | 线程安全；会触发调度并进入速度控制处理 |
| `flush(size_t srcKeyHash)` | 清空输入队列并通知下游 | `HJ_OK`/错误码 | 线程安全；用于切流/清理缓存 |
| `done()` | - | `HJ_OK`/错误码 | 线程安全；释放线程与内部资源 |

> `runTask` 为内部调度入口，不建议直接调用。

## 生命周期与线程安全说明
- 继承自 `HJPlugin`，建议通过 `Create` 创建并调用 `init` 完成初始化；`done` 负责释放线程与资源。
- `runTask` 在插件专属线程（或共享线程的 handler）中串行执行，内部通过锁保护状态与音频处理器的访问。
- 上下游数据流经 `deliver/receive/deliverToOutputs`，输入/输出队列由 `HJPlugin` 的同步机制保护。
- 通过 `pluginListener` 可接收自动调节参数通知（`HJ_PLUGIN_NOTIFY_AUTODELAY_PARAMS`），回调需避免阻塞与重入。

## 常见用法 / 清理示例

```cpp
auto speedControl = HJPluginSpeedControl::Create<HJPluginSpeedControl>("speedControl", graphInfo);
auto param = HJKeyStorage::create();
(*param)["audioInfo"] = audioInfo;
(*param)["thread"] = sharedThread;          // 可选：共享线程
(*param)["pluginListener"] = pluginListener;

speedControl->init(param);

// 连接上下游
graph->connectPlugins(audioResampler, speedControl, HJMEDIA_TYPE_AUDIO);
graph->connectPlugins(speedControl, audioRender, HJMEDIA_TYPE_AUDIO);

// 结束
speedControl->done();
speedControl.reset();
```
