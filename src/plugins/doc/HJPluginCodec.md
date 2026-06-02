# HJPluginCodec

## Purpose
- Base class for codec plugins, encapsulating codec creation, initialization, run, and output retrieval with unified error handling and status transitions.
- Provides common logic for flush-based codec reinit and passthrough metadata propagation for audio/video decoders and encoders.

## Main APIs

| Method | Parameters | Returns | Threading Notes |
| --- | --- | --- | --- |
| `internalInit(param)` | `param`: optional config (`streamInfo`, `thread`, `createThread`, `pluginListener`, etc.) | `HJ_OK`/error | Called under `m_sync` producer lock; derived classes should call base first |
| `internalRelease()` | - | `void` | May be called outside lock; must be re-entrant and thread-safe |
| `afterInit()` | - | `void` | Called under `m_sync` producer lock; updates status based on `m_codec` |
| `onInputAdded(srcKeyHash, type)` | upstream hash and media type | `void` | Triggered from `addInputPlugin` write-lock path; stores input hash |
| `runFlush()` | - | `int` | Calls `codec->flush()` under producer lock; clears passthrough map; then notifies downstream flush |
| `initCodec(streamInfo)` | stream info | `HJ_OK`/error | Called under producer lock; creates and initializes codec |
| `releaseCodec()` | - | `void` | Re-entrant resource release |
| `processFlushFrame(route, inFrame)` | route trace and input frame | `HJ_OK`/error | Rebuilds codec under producer lock; may send error notification |
| `processMediaFrame(route, inFrame)` | route trace and input frame | `HJ_OK`/error | Calls `codec->run()` under producer lock; updates `m_streamIndex` |
| `getOutputFrame(route)` | route trace | `(err, frame)` | Calls `codec->getFrame()` under producer lock and handles passthrough |
| `passThroughSetInput(inFrame)` | input frame | `void` | Writes passthrough entry while processing input |
| `passThroughSetOutput(outFrame)` | output frame | `void` | Applies passthrough metadata on output |

> Note: This class is designed for single-input usage; multi-input safety must be guaranteed by the caller.

## Lifecycle and Thread Safety
- Inherits `HJSyncObject`; `init/done` are protected by `SYNC_PROD_LOCK`, and `m_status` is updated under lock.
- `codec->init/run/getFrame/flush` are executed on the producer-lock path to prevent concurrent access.
- `internalRelease` may run outside the lock; it must be re-entrant and thread-safe.
- Error notifications are sent via `pluginListener`, typically outside locks.

## Common Usage
### Derived class example
```cpp
class MyDecoder : public HJPluginCodec {
public:
    MyDecoder() : HJPluginCodec("MyDecoder") {}

protected:
    HJMediaType getType() override { return HJMEDIA_TYPE_VIDEO; }
    HJBaseCodec::Ptr createCodec() override { return HJBaseCodec::createVDecoder(); }
};

auto plugin = HJCreates<MyDecoder>();
auto param = HJKeyStorage::create();
(*param)["streamInfo"] = streamInfo;
plugin->init(param);
```

### Cleanup example
```cpp
plugin->done(); // Triggers internalRelease and releases codec/thread resources
```
