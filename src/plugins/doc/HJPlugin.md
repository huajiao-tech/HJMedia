# HJPlugin

Base plugin abstraction that manages input/output links, task dispatch (optional thread), and status sync. Derived classes mainly implement `runTask`.

## Key APIs

| Method | Description | Parameters | Returns | Thread Notes |
| ---- | ---- | ---- | ---- | -------- |
| `addInputPlugin(Ptr plugin, HJMediaType type, int trackId)` | Register upstream and create input queue | upstream plugin, media type, track id | `HJ_OK`/error | Holds `m_inputSync` write lock |
| `addOutputPlugin(Ptr plugin, HJMediaType type, int trackId)` | Register downstream output | downstream plugin, media type, track id | `HJ_OK`/error | Holds `m_outputSync` write lock |
| `deliver(size_t srcKeyHash, HJMediaFrame::Ptr& frame)` | Push a frame into input queue and trigger dispatch | source hash, frame | `HJ_OK`/error | Thread-safe |
| `flush(size_t srcKeyHash)` | Clear input queue and notify downstream flush | source hash | `HJ_OK`/error | Thread-safe; includes async handler work |
| `receive(size_t srcKeyHash, int64_t* size)` | Pop one frame from input queue | source hash, optional remaining size out | frame or `nullptr` | Thread-safe |
| `preview(size_t srcKeyHash)` | Peek the head frame without popping | source hash | frame or `nullptr` | Thread-safe |
| `runTask(int64_t* delay)=0` | Dispatch task implemented by derived class | outputs next delay | `HJ_OK`/error | Runs on handler thread (if present) |
| `deliverToOutputs(HJMediaFrame::Ptr& frame)` | Forward frame to all outputs | frame | void | Typically called inside `runTask` |
| `setPause(bool pause)` | Toggle pause and call `onPauseStateChanged` | pause | void | Thread-safe |

> Note: `Input::eventFlags` controls what statistics `reportFrameDequeInfo` reports during `deliver/flush/receive` (audio duration, video frames, key frames, etc.). The queue-size flag is currently not emitted.

## Lifecycle and Thread Safety

- `init(param)`: called under `m_sync` lock. On success, status becomes `Inited` and `afterInit` runs. Common params: `thread` (shared `HJLooperThread`), `createThread` (auto-create if no shared thread), `pluginListener` (event callback).
- Dispatch: if `m_handler` exists, `postTask` schedules `runTask`, and re-schedules based on returned `delay`, ensuring serial execution on the handler thread.
- Locks: input/output maps are guarded by `m_inputSync`/`m_outputSync`; handler pointer by `m_handlerSync`. Derived classes must guard their own resources.
- Shutdown: `done()` sets `HJSTATUS_Done` and calls `internalRelease` to release handler/thread; base destructor calls `done()`. The destructor also disconnects inputs/outputs and releases the weak graph reference.

## Example Usage

```cpp
auto plugin = HJCreates<MyPlugin>(); // MyPlugin derives HJPlugin
auto param = HJKeyStorage::create();
(*param)["createThread"] = true;
(*param)["pluginListener"] = myListener;
plugin->init(param);

// Link upstream/downstream
plugin->addInputPlugin(demuxer, HJMEDIA_TYPE_AUDIO, 0);
plugin->addOutputPlugin(audioRender, HJMEDIA_TYPE_AUDIO, 0);

// Control
plugin->setPause(true);
plugin->setPause(false);

plugin->done();
```
