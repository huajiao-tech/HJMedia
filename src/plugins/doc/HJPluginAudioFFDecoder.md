# HJPluginAudioFFDecoder

## Purpose

This document serves three audiences:

1. LLMs / coding agents
Purpose: understand what this class adds on top of `HJPluginCodec` and avoid re-deriving the audio decoder path from generic codec code every time.

2. Maintainers
Purpose: recall the audio decoder's initialization contract, runTask behavior, flush/reinit semantics, and output event characteristics.

3. Other readers
Purpose: understand where audio decoding lives in the graph and how it differs from the generic codec base.

## What It Is

`HJPluginAudioFFDecoder` is the FFmpeg-based audio decoder plugin used in player graphs.

It is a thin but important specialization of `HJPluginCodec`:

- media type is fixed to audio
- codec factory is fixed to audio decoder creation
- optional `audioInfo` is mapped into `streamInfo` during init
- input route is marked to emit audio-duration events

It is not:

- a standalone codec framework
- the resampler
- the audio renderer

## Code Location

- [HJPluginAudioFFDecoder.h](/f:/Source/hjmedia/src/plugins/HJPluginAudioFFDecoder.h)
- [HJPluginAudioFFDecoder.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioFFDecoder.cpp)

Shared base logic lives in:

- [HJPluginCodec.cpp](/f:/Source/hjmedia/src/plugins/HJPluginCodec.cpp)
- [HJPluginCodec.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginCodec.md)

Typical graph users:

- [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)
- [HJGraphVodPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphVodPlayer.cpp)
- [HJGraphLivePlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphLivePlayer.cpp)

## Responsibilities

`HJPluginAudioFFDecoder` is responsible for:

- receiving compressed audio frames from upstream demuxer
- decoding them through the codec base path
- forwarding decoded PCM frames downstream
- rebuilding codec on flush frames when stream info changes or seek/reset requires it
- propagating audio-duration statistics through input event flags

Most heavy lifting is inherited from `HJPluginCodec`, but this subclass still defines the audio-specific contract.

## Important API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `internalInit(param)` | Prepare audio decoder init params | `HJ_OK` / error | Maps `audioInfo` to `streamInfo` before base init |
| `onInputAdded(srcKeyHash, type)` | Register upstream input | `void` | Enables `EVENT_FLAG_AUDIO_DURATION` on input queue |
| `runTask(delay)` | Main decode loop | `HJ_OK` / error | Pulls input, handles control frames, decodes, drains outputs |
| `getType()` | Returns fixed media type | `HJMEDIA_TYPE_AUDIO` | Audio-only specialization |
| `createCodec()` | Creates underlying decoder | `HJBaseCodec::Ptr` | Uses `HJBaseCodec::createADecoder()` |

Important inherited helpers used by `runTask(...)`:

- `receiveInputFrame(...)`
- `processFlushFrame(...)`
- `processEofFrame(...)`
- `processMediaFrame(...)`
- `getOutputFrame(...)`
- `deliverToOutputs(...)`

## Initialization

### `internalInit(...)`

`internalInit(...)` performs one audio-specific step before delegating to the codec base:

- if `audioInfo` is provided, it is stored into `streamInfo`

Specifically:

```cpp
if (audioInfo) {
    (*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(audioInfo);
}
```

Then it forwards:

- `createThread = (thread == nullptr)`

into `HJPluginCodec::internalInit(...)`.

What that means in practice:

- if caller already knows target audio stream info, codec can be initialized immediately
- otherwise codec may later be rebuilt from flush-frame metadata

This is a key difference between "known stream from graph setup" and "late stream info from pipeline control frame".

## Input Registration

`onInputAdded(...)` calls the codec base implementation, then marks the input route with:

- `EVENT_FLAG_AUDIO_DURATION`

This is important because downstream graph logic often uses accumulated audio duration as a flow-control and playback-state signal.

In other words:

- the audio decoder is not just producing PCM
- it also participates in the audio-duration event pipeline

## Main Decode Path

`runTask(...)` is the main handler-thread decode loop.

High-level flow:

1. Pull one input frame using `receiveInputFrame(...)`.
2. If no frame is available, return `HJ_WOULD_BLOCK`.
3. If clear frame:
   - call `runFlush()`
   - stop this iteration
4. If flush frame:
   - call `processFlushFrame(...)`
   - rebuild codec if stream info is valid
5. If EOF frame:
   - call `processEofFrame(...)`
6. Call `processMediaFrame(...)` to feed compressed audio into codec.
7. Drain decoder outputs in a loop with `getOutputFrame(...)`.
8. Deliver every decoded output frame downstream.
9. If output EOF frame appears, set plugin status to `EOF`.

This means one `runTask(...)` iteration can:

- consume one compressed frame
- produce zero, one, or multiple decoded frames

That is normal decoder behavior and should be preserved when reviewing this code.

## Flush / Reinit Semantics

Flush handling is inherited from `HJPluginCodec::processFlushFrame(...)`.

Key behavior:

- tries to extract `HJStreamInfo` from input frame first
- falls back to `mediaInfo` if needed
- rebuilds codec with the discovered stream info
- transitions status back to `Ready` on success
- reports codec-init or invalid-data errors on failure

For `HJPluginAudioFFDecoder`, this matters in audio scenarios such as:

- seek/reset where downstream expects clean decode restart
- stream-change cases where audio format metadata arrives through flush control frame

The old short description "flush rebuilds codec" is true but incomplete; the real dependency is valid stream info extraction.

## EOF Semantics

EOF handling is shared through `HJPluginCodec::processEofFrame(...)` and output-drain logic.

Behavior details:

- if plugin status is still only `Inited`, incoming EOF may be forwarded and result in `HJ_WOULD_BLOCK`
- after normal decode path, if decoder outputs an EOF frame, `runTask(...)` marks this plugin as `EOF`

Important point:

- this class can receive EOF as input
- but final EOF state is established when output drain reaches EOF frame

That distinction matters when analyzing downstream completion timing.

## Error Semantics

Inherited error paths from `HJPluginCodec` are the real operational contract here:

- invalid stream data -> `HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA`
- codec init failure -> `HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT`
- codec run failure -> `HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN`
- codec get-frame failure -> `HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME`

So when debugging audio decode problems, you should not stop at `HJPluginAudioFFDecoder.cpp`; the real error state machine lives in `HJPluginCodec.cpp`.

## Audio-Specific Differences From `HJPluginCodec`

This class is intentionally thin, but the thinness itself is the point.

What it adds beyond the base:

1. Audio stream bootstrap
- `audioInfo -> streamInfo`

2. Audio duration eventing
- input route sets `EVENT_FLAG_AUDIO_DURATION`

3. Fixed codec type
- `getType() == HJMEDIA_TYPE_AUDIO`
- `createCodec() == createADecoder()`

Everything else should be understood as inherited codec behavior, not audio-decoder-local invention.

## Known Caveats

Important caveats to remember:

1. This class looks small, but most of its real behavior lives in `HJPluginCodec`.

2. If you only read this file and not `HJPluginCodec.cpp`, you will misunderstand flush/error/EOF behavior.

3. Immediate codec initialization depends on whether `audioInfo` is already available during `init(...)`.

4. One input frame can generate multiple output frames; decode loops must not assume one-in/one-out.

5. Audio duration eventing depends on `onInputAdded(...)`, so changing input registration semantics can break graph-level queue logic.

## Typical Usage

```cpp
auto decoder = HJCreates<HJPluginAudioFFDecoder>();
auto param = HJKeyStorage::create();
(*param)["audioInfo"] = audioInfo;   // optional but useful when known
// (*param)["thread"] = sharedThread; // optional
decoder->init(param);

decoder->addInputPlugin(demuxer, HJMEDIA_TYPE_AUDIO, 0);
decoder->addOutputPlugin(resampler, HJMEDIA_TYPE_AUDIO, 0);
```

## What LLMs Should Read Next

If the task touches `HJPluginAudioFFDecoder`, read in this order:

1. [HJPluginAudioFFDecoder.h](/f:/Source/hjmedia/src/plugins/HJPluginAudioFFDecoder.h)
2. [HJPluginAudioFFDecoder.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioFFDecoder.cpp)
3. [HJPluginCodec.cpp](/f:/Source/hjmedia/src/plugins/HJPluginCodec.cpp)
4. [HJPluginCodec.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginCodec.md)
5. downstream [HJPluginAudioResampler.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioResampler.cpp)

## Recommended Review Focus

When reviewing `HJPluginAudioFFDecoder` changes, prioritize:

1. whether audio init params are still mapped correctly into codec stream info
2. flush/reinit correctness
3. EOF output timing
4. handler-thread-only codec access
5. audio-duration event propagation
