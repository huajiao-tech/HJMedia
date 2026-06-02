# HJPluginAudioResampler

## Purpose

This document serves three audiences:

1. LLMs / coding agents
Purpose: quickly understand the real responsibilities of the audio resampler and avoid reducing it to only “sample-rate conversion”.

2. Maintainers
Purpose: recall converter/FIFO behavior, control-frame handling, error paths, and output semantics.

3. Other readers
Purpose: understand where audio format normalization happens in the graph and why FIFO packing may be enabled.

## What It Is

`HJPluginAudioResampler` is the audio format normalization stage between decoded PCM and audio render.

It is responsible for:

- converting input audio frames to target `HJAudioInfo`
- optionally repacking converted PCM through `HJAudioFifo`
- forwarding audio frames downstream in render-friendly frame sizes
- clearing converter/FIFO state on flush
- reporting converter and FIFO failures separately

It is not:

- the decoder
- the final audio renderer
- the playback clock source

## Code Location

- [HJPluginAudioResampler.h](/f:/Source/hjmedia/src/plugins/HJPluginAudioResampler.h)
- [HJPluginAudioResampler.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioResampler.cpp)

Main upstream/downstream neighbors:

- upstream [HJPluginAudioFFDecoder.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioFFDecoder.cpp)
- downstream [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)

## Responsibilities

`HJPluginAudioResampler` combines two related but distinct jobs:

1. Audio conversion
- via `HJAudioConverter`
- normalize sample rate / sample format / channel layout into target `HJAudioInfo`

2. Optional frame repacking
- via `HJAudioFifo`
- regroup converted PCM into fixed-size output frames

This distinction matters because:

- without FIFO, converted output may be emitted directly
- with FIFO, one input frame may produce zero, one, or multiple output frames

## Important API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `internalInit(param)` | Initialize converter and optional FIFO | `HJ_OK` / error | Requires `audioInfo`; optional `fifo`, `thread` |
| `internalRelease()` | Release converter/FIFO state | `void` | Re-entrant teardown |
| `onInputAdded(srcKeyHash, type)` | Register upstream input hash | `void` | Stores the active input route |
| `runFlush()` | Reset resampler state and forward flush | `HJ_OK` / error | Resets FIFO and timing trace state |
| `runTask(delay)` | Main resample/repack loop | `HJ_OK` / error | Pulls one input, processes, may drain FIFO |
| `processMediaFrame(route, inFrame)` | Convert one input frame | `(ret, outFrame, fifo)` | Returns direct output or FIFO mode indicator |
| `getAndDeliverOutputFrame(route)` | Pull one output frame from FIFO and deliver | `(ret, outFrame)` | Used only when FIFO mode is active |

## Initialization

`internalInit(...)` requires:

- `audioInfo`

Optional inputs:

- `thread`
- `fifo`

Initialization flow:

1. forward thread/createThread setup to `HJPlugin::internalInit(...)`
2. create `HJAudioConverter(audioInfo)`
3. if FIFO mode is enabled:
   - create `HJAudioFifo`
   - initialize it with target sample count
4. cache `m_audioInfo`

Important point:

- the target audio format is defined entirely by `audioInfo`
- converter and optional FIFO are both configured around that same target format

## Converter vs FIFO

This class is easiest to understand if you separate these two layers:

### Converter

`HJAudioConverter`:

- consumes one decoded audio frame
- outputs one converted audio frame in target format

### FIFO

`HJAudioFifo`:

- consumes converted output frames
- buffers samples internally
- emits fixed-size output frames when enough data has accumulated

So the pipeline is:

`decoded input -> converter -> optional FIFO -> downstream render`

## Main Run Loop

`runTask(...)` does the following:

1. pull one input frame from the active input route
2. if no frame is available, return `HJ_WOULD_BLOCK`
3. if clear frame:
   - call `runFlush()`
   - stop this iteration
4. if EOF frame:
   - forward EOF directly downstream
5. otherwise:
   - call `processMediaFrame(...)`
6. if `processMediaFrame(...)` returns a direct output frame:
   - deliver it downstream immediately
7. if FIFO mode is active and direct output is null:
   - repeatedly call `getAndDeliverOutputFrame(...)`
   - deliver all currently available packed output frames

This means one `runTask(...)` iteration may:

- emit nothing
- emit one direct converted frame
- emit multiple FIFO-packed frames

## Direct Output Mode

When FIFO is not enabled:

- `processMediaFrame(...)` converts one input frame
- returns `outFrame != nullptr`
- `runTask(...)` delivers it directly

This is the simpler mode and keeps latency lower when repacking is unnecessary.

## FIFO Output Mode

When FIFO is enabled:

- converted frame is pushed into `m_fifo`
- `processMediaFrame(...)` returns `outFrame = nullptr` and `fifo = true`
- `runTask(...)` drains ready frames from FIFO through `getAndDeliverOutputFrame(...)`

Important semantic detail:

- FIFO output frames are stamped with current `m_streamIndex`

That means:

- packed output frames conceptually belong to the current input stream generation
- stream generation is preserved even though one input may lead to multiple repacked outputs

## Flush Semantics

`runFlush()`:

- resets `m_last_time`
- resets FIFO if it exists
- then forwards generic flush behavior through `HJPlugin::runFlush()`

This is important because resampler flush is not only a queue flush:

- it also clears repacking state
- it drops any partial FIFO accumulation

That behavior is required for seek/reset correctness.

## EOF Semantics

EOF handling here is intentionally simple:

- input EOF frame is forwarded directly downstream
- no additional repacking/coordination logic is done inside the resampler

This is different from decoder or renderer logic:

- resampler is not the component that decides final playback completion
- it simply preserves EOF as a control signal in the audio chain

## Error Semantics

`processMediaFrame(...)` distinguishes two fatal failure classes:

1. converter failure
- `m_converter->convert(...) == nullptr`
- reports `HJ_PLUGIN_NOTIFY_ERROR_AUDIOCONVERTER_CONVERT`

2. FIFO add failure
- `m_fifo->addFrame(...) < 0`
- reports `HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME`

On fatal error:

- plugin status becomes `Exception`

This separation is important because:

- converter failure means format conversion itself failed
- FIFO failure means repacking/storage failed after conversion

Those are different debugging paths.

## Timing / Output Trace Detail

`getAndDeliverOutputFrame(...)` keeps a lightweight timing trace:

- stores last delivered PTS in `m_last_time`
- logs when PTS delta becomes larger than 25 ms

This is not core business logic, but it is useful when inspecting irregular output cadence from FIFO-packed frames.

## Thread Model

`HJPluginAudioResampler` uses standard plugin handler-thread execution:

- `runTask(...)` is serialized through plugin scheduling
- converter/FIFO state is guarded by `SYNC_PROD_LOCK`
- downstream delivery happens after protected processing

That means:

- conversion and FIFO mutation should be treated as handler-thread-only logic
- delivery may fan out to downstream plugins after internal state has been updated

## Known Caveats

Important caveats to remember:

1. This class does more than sample-rate conversion; FIFO packing changes output cadence and frame granularity.

2. In FIFO mode, one input frame can yield multiple downstream output frames.

3. Flush resets FIFO accumulation, so seek/reset may discard partial packed audio.

4. EOF is forwarded, not finalized here.

5. Output frame `streamIndex` in FIFO mode is inherited from the current input generation, not recalculated per packed chunk.

## Typical Usage

```cpp
auto resampler = HJCreates<HJPluginAudioResampler>();
auto param = HJKeyStorage::create();
(*param)["audioInfo"] = audioInfo;   // target format
(*param)["fifo"] = true;             // optional
// (*param)["thread"] = sharedThread; // optional
resampler->init(param);

resampler->addInputPlugin(audioDecoder, HJMEDIA_TYPE_AUDIO, 0);
resampler->addOutputPlugin(audioRender, HJMEDIA_TYPE_AUDIO, 0);
```

## What LLMs Should Read Next

If the task touches `HJPluginAudioResampler`, read in this order:

1. [HJPluginAudioResampler.h](/f:/Source/hjmedia/src/plugins/HJPluginAudioResampler.h)
2. [HJPluginAudioResampler.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioResampler.cpp)
3. [HJPluginAudioFFDecoder.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioFFDecoder.cpp)
4. [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)
5. `HJAudioConverter` / `HJAudioFifo` implementation if format/packing behavior is under investigation

## Recommended Review Focus

When reviewing `HJPluginAudioResampler` changes, prioritize:

1. converter target-format correctness
2. FIFO enable/disable behavior
3. flush/reset correctness
4. EOF forwarding behavior
5. exception path correctness
6. output cadence and packed-frame semantics
