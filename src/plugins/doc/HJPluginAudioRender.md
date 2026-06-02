# HJPluginAudioRender

## Purpose

This document serves three audiences:

1. LLMs / coding agents
Purpose: quickly understand the actual audio render contract and avoid misreading it as a thin device wrapper.

2. Maintainers
Purpose: recall lifecycle, threading, preroll/buffering behavior, timeline synchronization, and PCM callback buffering.

3. Other readers
Purpose: understand what this plugin is responsible for in the player graph and what is left to platform subclasses.

## What It Is

`HJPluginAudioRender` is the base class for platform audio output plugins.

It is responsible for:

- consuming decoded/resampled audio frames from upstream
- managing stream start/stop around pause / eof / input updates
- handling preroll and buffering transitions
- synchronizing playback head into `HJTimeline`
- applying mute / volume to output PCM
- exporting rendered PCM snapshots through graph events

It is not:

- the platform device implementation itself
- the decoder or resampler
- the full player control layer

Platform subclasses implement the actual device interaction through:

- `initRender(...)`
- `releaseRender()`
- `setStreamRunning(...)`
- `getRenderTimestamp()`

## Code Location

- [HJPluginAudioRender.h](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.h)
- [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)

Primary related components:

- [HJTimeline.h](/f:/Source/hjmedia/src/plugins/HJTimeline.h)
- [HJTimeline.cpp](/f:/Source/hjmedia/src/plugins/HJTimeline.cpp)
- [HJPluginAudioResampler.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioResampler.cpp)
- [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)

## Responsibilities

`HJPluginAudioRender` combines several responsibilities that are easy to miss if only reading the class name:

1. Device lifecycle wrapper
- asynchronously initialize and release the platform audio renderer

2. Stream running-state coordinator
- convert input updates, pause changes, and EOF completion into `Start` / `StopPause` / `StopEof` run operations

3. Playback clock source
- when a full audio frame is consumed, update `HJTimeline`

4. Buffering / preroll controller
- wait until enough queued audio exists before starting steady playback
- emit buffering start/stop notifications

5. Rendered PCM exporter
- keep a ring buffer of submitted PCM
- periodically drain already-rendered PCM based on render timestamp
- report `EVENT_GRAPH_RENDERED_PCM_ID`

## Public / Important API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `setMute(bool)` | Toggle mute state | `HJ_OK` / error | Thread-safe |
| `isMuted()` | Query mute state | `bool` | Thread-safe |
| `setVolume(float)` | Set output volume | `HJ_OK` / error | Clamped to `0.0 ~ 1.0` |
| `getVolume()` | Query output volume | `float` | Thread-safe |
| `internalInit(param)` | Base initialization entry | `HJ_OK` / error | Expects `audioInfo` and `timeline` |
| `runFlush()` | Flush timeline state | `HJ_OK` / error | Called on clear-frame path |
| `fillAudioBuffer(...)` | Pull upstream audio and fill device buffer | `(ret, validSize, kernalFrame)` | Core render-side data path |
| `onInputUpdated()` | React to new queued input | `void` | May post `RunOp::Start` |
| `onPauseStateChanged(bool)` | React to pause/resume | `void` | May post start/stop ops |
| `runPCMCallback(...)` | Periodic rendered-PCM export | `void` | Driven by timer when enabled |

Subclass responsibilities:

| API | Meaning |
| --- | --- |
| `initRender(audioInfo)` | Create/init platform audio device |
| `releaseRender()` | Release platform audio device |
| `setStreamRunning(running, eofStop)` | Start or stop platform stream |
| `getRenderTimestamp()` | Return platform render head timestamp in milliseconds |

## Initialization

`internalInit(...)` requires:

- `audioInfo`
- `timeline`

Optional inputs:

- `thread`
- `prerollDurationMs`
- `pcmCallbackIntervalMs`
- `audioDeviceName` on Windows

Initialization flow:

1. Duplicate params and forward to `HJPlugin::internalInit(...)`.
2. Schedule `runInit(audioInfo)` asynchronously on the plugin handler thread.
3. Cache `audioInfo` and `timeline`.
4. Compute PCM callback ring-buffer capacity from:
   - sample rate
   - bytes per sample
   - channels
   - callback interval
   - `samplesPerFrame`
5. Reset preroll and PCM callback state.
6. Start timer for `runPCMCallback(...)` if PCM callback export is enabled.

Important detail:

- actual device init is asynchronous
- `internalInit(...)` returning `HJ_OK` does not mean the platform renderer is already fully ready

That readiness transition happens later in `runInit(...)`.

## Lifecycle

### `runInit(...)`

`runInit(...)` runs on the handler thread and calls subclass `initRender(...)`.

On success:

- `m_audioInfo->m_blockAlign` is updated from the actual renderer block alignment
- plugin status becomes `Ready`
- `HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING` is reported
- `m_inited = true`

On failure:

- plugin status becomes `Exception`
- `HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT` is reported

### `internalRelease()`

Release flow:

1. Synchronize onto handler thread and call `releaseRender()`
2. call `HJPlugin::internalRelease()`
3. if renderer had been initialized, report `HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING`
4. clear timeline and audio info
5. reset preroll / PCM callback state

Important note from code:

- release ordering is intentionally designed to keep start/stop notifications ordered relative to the render thread teardown

## Thread Model

There are several thread contexts here:

1. Caller thread
- may call init / mute / volume / pause-related operations indirectly

2. Plugin handler thread
- runs `runInit(...)`
- runs `runRunOp(...)`
- may own timers

3. Platform render thread / callback thread
- subclass-specific
- typically where actual audio device consumption happens

4. PCM callback timer path
- periodically polls `getRenderTimestamp()` and drains already-rendered PCM from the local ring buffer

Synchronization patterns:

- `SYNC_PROD_LOCK` protects mutable state
- `SYNC_CONS_LOCK` protects reads
- `m_pcmCallbackMutex` protects the PCM ring buffer
- some release logic explicitly synchronizes on `m_handler`

## Stream Running State

One of the most important current design points is that audio stream start/stop is not directly tied to only one API.

Instead, the base class uses `RunOp`:

- `Start`
- `StopPause`
- `StopEof`

Triggers:

- `onInputUpdated()`:
  - if not paused and not already running, set `m_running = true` and post `Start`

- `onPauseStateChanged(true)`:
  - if running, set `m_running = false` and post `StopPause`

- `onPauseStateChanged(false)`:
  - if queued input exists and stream is not running, set `m_running = true` and post `Start`

- `onPlaybackCompleted()`:
  - set `m_running = false`
  - discard pending PCM callback data
  - post `StopEof`

`runRunOp(...)` then calls subclass `setStreamRunning(...)`.

This split is important because:

- state decisions are made in the base class
- actual device control remains platform-specific

## Preroll and Buffering

`fillAudioBuffer(...)` contains the main preroll/buffering behavior.

Core logic:

- before steady playback, `m_prerolling` is true
- if queued audio duration is below `m_prerollDurationMs` and there is no control frame waiting:
  - emit `HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING` once
  - fill requested output with silence
  - do not start consuming real frames yet

When enough audio is queued:

- `m_prerolling = false`
- real frame consumption begins

If later no frame is available:

- it re-enters buffering mode
- emits start-buffering if needed
- fills silence

When valid audio frame consumption resumes:

- buffering flag is cleared
- `HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING` is reported

This means buffering is not only “device empty”; it is explicitly tied to queue depth and control-frame awareness.

## Main Data Path

The core output path is `fillAudioBuffer(...)`.

High-level behavior:

1. If preroll is insufficient, output silence.
2. Pull frame from upstream input queue.
3. Handle control frames:
   - clear frame -> `runFlush()`
   - EOF frame -> query `QUERY_CAN_PLUGIN_EOF_ID`, maybe `onPlaybackCompleted()`
4. Copy PCM from current frame into output buffer.
5. When a full kernel frame is consumed:
   - update `HJTimeline` with:
     - `streamIndex`
     - `PTS`
     - `speed`
   - release current frame and continue

This is the key semantic point:

- timeline is updated when the renderer has fully consumed the frame payload
- not when decoder/resampler merely produced it

## Timeline Integration

`HJPluginAudioRender` is a primary playback clock source in player flows.

When a kernel frame is fully consumed inside `fillAudioBuffer(...)`, it calls:

```cpp
timeline->setTimestamp(kernalFrame->m_streamIndex,
                       kernalFrame->getPTS(),
                       kernalFrame->getSpeed());
```

That means:

- graph-level playback position is driven by rendered audio progress
- player `getCurrentTimestamp()` ultimately depends on this path

If this behavior changes, `HJGraphMusicPlayer` and `HJGraphVodPlayer` progress semantics will change too.

## PCM Callback Export

This is one of the most important features missing from the old doc.

The base class keeps a ring buffer of submitted PCM:

- `appendPCMCallbackData(...)` writes PCM into the buffer
- `runPCMCallback(...)` uses `getRenderTimestamp()` to estimate how many frames have actually been rendered
- it drains only already-rendered PCM
- then reports `EVENT_GRAPH_RENDERED_PCM_ID`

Why this exists:

- to export PCM close to real playback progress
- to avoid simply forwarding queued PCM that has not actually been heard yet

Important caveats:

- this is approximate and depends on subclass `getRenderTimestamp()`
- render timestamp regression triggers ring-buffer reset logic
- the callback buffer intentionally drops old PCM if capacity is exceeded

## Mute and Volume

Mute and volume are handled in the base class:

- `setMute()` stores mute flag
- `setVolume()` clamps and stores volume
- `applyOutputVolume(...)` scales PCM samples in-place

Supported scaling paths:

- `float`
- `int32`
- `int16` and default fallback

This means subclasses do not need to implement basic software mute/volume unless they want device-side handling.

## Flush / EOF Semantics

### Flush

`runFlush()` currently:

- flushes the shared timeline

Notably, the old code comment shows PCM callback discard was considered but is currently disabled there.

### EOF

When an EOF frame is received:

1. query `QUERY_CAN_PLUGIN_EOF_ID`
2. if graph allows EOF:
   - call `onPlaybackCompleted()`
   - stop running
   - discard pending PCM callback data
   - post `StopEof`
   - output silence for the current request

This shows that final EOF behavior is coordinated with graph-level policy rather than decided by render alone.

## Known Caveats

Key caveats to remember:

1. `internalInit()` is asynchronous in effect; ready state is established later in `runInit()`.

2. This class is much more than a thin render wrapper; it contains buffering, stream state, timeline, and PCM export logic.

3. Correctness depends on subclass `getRenderTimestamp()` and `setStreamRunning(...)`.

4. `fillAudioBuffer(...)` is a hot path; changes here affect latency, buffering, EOF behavior, timeline updates, and PCM callback semantics together.

5. Timeline flush is currently lightweight; if you change flush semantics, check seek/reset/player progress behavior.

## Typical Usage

### Base initialization

```cpp
auto render = HJCreates<MyAudioRender>();
auto param = HJKeyStorage::create();
(*param)["audioInfo"] = audioInfo;
(*param)["timeline"] = timeline;
(*param)["thread"] = sharedThread; // optional
render->init(param);
```

### Upstream linkage

```cpp
render->addInputPlugin(resampler, HJMEDIA_TYPE_AUDIO, 0);
```

### Teardown

```cpp
render->done();
```

## What LLMs Should Read Next

If the task touches audio render behavior, read in this order:

1. [HJPluginAudioRender.h](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.h)
2. [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)
3. [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)
4. platform subclass implementation
5. graph using the renderer, such as [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)

## Recommended Review Focus

When reviewing `HJPluginAudioRender` changes, prioritize:

1. render-thread / handler-thread correctness
2. preroll and buffering regressions
3. EOF and flush behavior
4. timeline update correctness
5. PCM callback ring-buffer correctness
6. mute / volume hot-path cost
