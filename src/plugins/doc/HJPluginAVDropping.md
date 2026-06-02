# HJPluginAVDropping

## 用途
用于 A/V 缓冲控制与丢帧策略：当输入队列音频累计时长超过阈值时，按音频时长+关键帧约束丢弃前序帧，优先保持关键视频帧与控制帧，避免下游堆积。

## 主要接口

| 方法 | 参数 | 返回 | 线程限制/说明 |
| --- | --- | --- | --- |
| `init(HJKeyStorage::Ptr param)` | `thread`(可选共享线程)、`pluginListener`(可选) | `HJ_OK`/错误码 | 由 `HJPlugin` 提供；内部可能创建线程并在 `afterInit` 触发调度 |
| `setDropPerameter(int64_t maxAudioMs, int64_t minAudioMs)` | 最大/最小音频时长阈值，单位 ms | `HJ_OK`/错误码 | 线程安全；使用输入锁写入配置 |
| `deliver(size_t srcKeyHash, HJMediaFrame::Ptr& frame)` | 上游投递帧 | `HJ_OK`/错误码 | 线程安全；会更新统计并触发调度 |
| `flush(size_t srcKeyHash)` | 清空输入队列并通知下游 | `HJ_OK`/错误码 | 线程安全；会触发 clear frame 机制 |
| `done()` | - | `HJ_OK`/错误码 | 线程安全；释放线程与资源 |

> `runTask` 为内部调度入口，不建议直接调用。

## 生命周期与线程安全
- 继承自 `HJPlugin`，建议通过 `Create` 创建并调用 `init` 完成初始化；`done` 负责释放线程与队列。
- `runTask` 在插件专属线程（或共享线程的 handler）中串行执行，内部调用 `receive/tryDropFrames/deliverToOutputs`。
- 输入/输出队列由 `HJPlugin` 的 `m_inputSync/m_outputSync` 保护；丢帧阈值在 `setDropPerameter` 中写入，读写通过同一锁同步。
- `deliverToOutputs` 会在锁外向下游插件回调，避免锁内重入。

## 常见用法 / 清理示例

```cpp
auto dropping = HJPluginAVDropping::Create<HJPluginAVDropping>("dropping", graphInfo);
auto param = HJKeyStorage::create();
(*param)["thread"] = sharedThread;          // 可选：共享线程
(*param)["pluginListener"] = pluginListener;
dropping->init(param);

// 配置丢帧阈值（ms）
dropping->setDropPerameter(10000, 5000);

// 连接上下游后开始工作
graph->connectPlugins(demuxer, dropping, HJMEDIA_TYPE_AV);

// 结束
dropping->done();
dropping.reset();
```
