# HJPluginVideoRender

## 用途
视频渲染插件：消费视频帧并输出到平台渲染桥或共享内存，同时与 `HJTimeline` 同步，向上层回调渲染事件与统计信息。

## 主要接口
| 方法 | 参数 | 返回 | 线程限制/说明 |
| ---- | ---- | ---- | -------- |
| `init(param)` | `param` 需包含 `timeline`；可选 `thread`/`createThread`/`pluginListener`/`onlyFirstFrame`/`deviceType`；HarmonyOS 下可选 `bridge`；Windows 下可选 `sharedMemoryProducer`；可选 `HJStatContext`（用于统计上报） | `HJ_OK`/错误码 | 在 `m_sync` 写锁内完成初始化；成功后可能创建或复用线程与 handler |
| `addInputPlugin(plugin, type, trackId)` | 上游插件、媒体类型、轨道号 | `HJ_OK`/错误码 | 持有 `m_inputSync` 写锁；建议仅在图构建期调用 |
| `deliver(srcKeyHash, frame)` | 源 key、视频帧 | `HJ_OK`/错误码 | 线程安全；触发任务调度 |
| `flush(srcKeyHash)` | 源 key | `HJ_OK`/错误码 | 线程安全；清空输入队列并触发调度 |
| `setPause(pause)` | `pause` 布尔值 | void | 线程安全；影响时间线推进与渲染节奏 |
| `done()` | - | `HJ_OK`/错误码 | 线程安全；进入 `Done` 状态并释放资源 |
| `pluginListener` 回调 | `HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME`/`HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME`/`HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF` | `HJ_OK` | 回调在渲染线程/handler 线程触发，避免阻塞 |

## 生命周期与线程安全
- 继承 `HJPlugin`：`init()` 成功后进入 `Inited` 状态，并由 handler 线程串行执行 `runTask()`。
- `m_timeline`、`m_bridge`、`m_sharedMemoryProducer` 的访问应在 `m_sync` 锁内且状态非 `Done` 时进行，避免与 `internalRelease()` 竞争。
- 渲染输出调用（如 `produceFromPixel`/`write2`）应在锁外执行，防止回调或阻塞造成锁重入/死锁。
- `done()` 会设置 `HJSTATUS_Done` 并在锁外执行 `internalRelease()`，此后不应再访问渲染资源。

## 常见用法
```cpp
auto timeline = HJTimeline::Create();
auto render = HJCreates<HJPluginVideoRender>();
auto param = HJKeyStorage::create();
(*param)["timeline"] = timeline;
(*param)["createThread"] = true;
(*param)["onlyFirstFrame"] = false;
(*param)["deviceType"] = HJDEVICE_TYPE_NONE;
(*param)["pluginListener"] = [](const HJNotification::Ptr ntf) {
    switch (ntf->getID()) {
    case HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME:
        // handle frame rendered
        break;
    case HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME:
        // handle first frame
        break;
    case HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF:
        // handle EOF
        break;
    }
    return HJ_OK;
};
render->init(param);

// connect upstream
render->addInputPlugin(videoDecoder, HJMEDIA_TYPE_VIDEO, 0);
```

## 清理示例
```cpp
render->done();
```
