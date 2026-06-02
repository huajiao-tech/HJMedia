# HJPluginDemuxer

## Purpose

This document serves three audiences:

1. LLMs / coding agents
Purpose: quickly understand demuxer lifecycle, stale-init protection, seek/reset behavior, and downstream delivery semantics.

2. Maintainers
Purpose: recall how demuxer instance switching, cached-frame reuse, stream-open reporting, and EOF coordination work.

3. Other readers
Purpose: understand the role of the demuxer plugin inside the graph pipeline and what guarantees it actually provides.

## What It Is

`HJPluginDemuxer` is the graph-facing demuxer plugin base class.

It wraps a concrete `HJBaseDemuxer` implementation and is responsible for:

- owning and switching the active demuxer instance
- opening / resetting / seeking asynchronously through the plugin handler thread
- pulling compressed media frames from the demuxer
- caching one frame when downstream is back-pressured
- forwarding frames to downstream plugins by media type
- reporting media type, stream-open, seek-succeeded, and demuxer error events

It is not:

- the actual file/network demux implementation
- the decoder
- the final owner of EOF policy

Concrete subclasses provide the actual demuxer by implementing:

- `createDemuxer()`

## Code Location

- [HJPluginDemuxer.h](/f:/Source/hjmedia/src/plugins/HJPluginDemuxer.h)
- [HJPluginDemuxer.cpp](/f:/Source/hjmedia/src/plugins/HJPluginDemuxer.cpp)

Typical users:

- [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)
- [HJGraphVodPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphVodPlayer.cpp)

## Responsibilities

`HJPluginDemuxer` has several distinct responsibilities:

1. Demuxer instance lifecycle
- create, initialize, quit, release, and replace the current `HJBaseDemuxer`

2. Async control plane
- serialize `openURL`, `reset`, and `seek` through the handler thread

3. Media metadata state
- maintain current `mediaType`
- maintain current `duration`
- preserve/restore selected audio track

4. Frame production
- pull frames from `HJBaseDemuxer`
- assign stream index offset across demuxer recreation
- cache one undeliverable frame for retry

5. Graph event integration
- report:
  - `EVENT_MEDIA_TYPE_ID`
  - `EVENT_STREAM_OPENED_ID`
  - `EVENT_SEEK_SUCCEEDED_ID`
  - demuxer error notifies

## Important API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `openURL(url)` | Switch to a new media URL | `HJ_OK` / error | Quits current demuxer first, then posts async init |
| `reset(delay)` | Reinitialize current media source | `HJ_OK` / error | Uses current `m_mediaUrl`; may delay init |
| `seek(timestamp)` | Seek active demuxer | `HJ_OK` / error | Removes pending runTask, posts async seek |
| `switchAudioTrack(trackId)` | Switch selected audio track | `HJ_OK` / error | Current build uses direct call path |
| `getMediaInfo()` | Return media metadata | `HJMediaInfo::Ptr` | Returns current demuxer media info if available |
| `getDuration()` | Return current duration | `int64_t` | Cached atomic |
| `getMediaType()` | Return current media type bitmask | `uint32_t` | Cached in plugin state |

Core internal flow APIs:

- `postInit(...)`
- `runInit(...)`
- `runSeek(...)`
- `runTask(...)`
- `runEof(...)`
- `deliverToOutputs(...)`
- `quitDemuxer()`
- `releaseDemuxer()`

## Initialization and Reinitialization

### `internalInit(...)`

`internalInit(...)`:

1. extracts optional `mediaUrl` and optional shared `thread`
2. forwards to `HJPlugin::internalInit(...)`
3. generates message IDs for:
   - init
   - seek
   - switch-audio-track
4. if `mediaUrl` exists, immediately posts async init with `InitReason::InternalInit`

Important point:

- a demuxer instance is not synchronously initialized inside `internalInit(...)`
- initialization is queued onto the handler thread through `postInit(...)`

### `postInit(...)`

`postInit(...)` schedules `runInit(...)` through `asyncAndClear(...)`.

That means:

- repeated init/reset/open requests collapse to the latest one of the same message ID
- stale initialization work is intentionally dropped

### `runInit(...)`

`runInit(...)`:

1. clears `m_hasReportedStreamOpened` for initial open cases
2. calls `initDemuxer(...)` under producer lock
3. on success:
   - reports `EVENT_MEDIA_TYPE_ID`
   - conditionally reports `EVENT_STREAM_OPENED_ID`
   - sets plugin status to `Ready`
4. on failure:
   - transitions to `Exception` or `Stoped`
   - reports demuxer-init error notify when appropriate

The stream-open reporting policy is subtle:

- `InternalInit` and `OpenURL` always reset the stream-open reporting state
- `Reset` reports stream-open only once unless it had not been reported yet

That avoids duplicate stream-open notifications in common reset flows.

## Demuxer Versioning and Stale-Init Protection

This is one of the most important design points.

Relevant fields:

- `m_quittingDemuxer`
- `m_quitIndex`
- `m_initIndex`
- `m_demuxerSync`

`quitDemuxer()`:

- marks `m_quittingDemuxer = true`
- snapshots current demuxer
- sets `m_quitIndex = m_initIndex`
- increments `m_initIndex`
- calls `setQuit(true)` on the old demuxer if it exists

`initDemuxer(...)` then creates a new demuxer only when:

- `m_quitIndex < i_initIndex`

Meaning:

- older init tasks can run late
- but only the latest generation is allowed to create the next demuxer instance

This is how the plugin avoids stale open/reset work reviving an old source after a newer request has already replaced it.

## Metadata State

After a successful `m_demuxer->init(i_url)`, `initDemuxer(...)` extracts metadata and updates:

- `m_mediaType`
- `m_duration`
- `m_audioTrackId`

Behavior details:

- `m_mediaType` is reconstructed from current media info
- `m_duration` is stored atomically
- if `m_audioTrackId < 0`, it is initialized from the demuxer's selected track
- otherwise, the plugin attempts to re-apply the previously chosen audio track on the new demuxer instance

This means audio track choice is intended to survive demuxer recreation.

## Seek Semantics

### Public `seek(...)`

Public `seek(...)` does not synchronously seek the demuxer.

Instead it:

1. reads handler and message IDs
2. removes pending runTask messages
3. posts async `runSeek(i_timestamp)` through `asyncAndClear(...)`

This makes seek:

- serialized on the handler thread
- coalesced when repeated rapidly

### `runSeek(...)`

`runSeek(...)`:

1. verifies status is seekable
2. calls `m_demuxer->seek(i_timestamp)`
3. if old status was EOF, moves status back to `Ready`
4. clears cached current frame
5. calls `runFlush()`
6. reports `EVENT_SEEK_SUCCEEDED_ID`

Error handling:

- demuxer quitting during seek -> `Stoped`
- fatal seek error -> `Exception` plus `HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_SEEK`

Important consequence:

- successful seek is not only a demuxer operation
- it also resets plugin-side cached frame and flushes downstream state

## Main Data Path

The heart of the demuxer plugin is `runTask(...)`.

High-level flow:

1. Try to reuse cached frame from `m_currentFrame`.
2. If none is cached, call `getOutputFrame(...)`.
3. If frame is EOF:
   - call `runEof(streamIndex)`
4. Otherwise:
   - ask graph `QUERY_CAN_DELIVER_TO_OUTPUTS_ID`
   - if downstream cannot accept it, cache the frame and return `HJ_WOULD_BLOCK`
5. Deliver frame to downstream outputs

This means the demuxer has one-frame backpressure memory.

That cached-frame behavior is important because downstream queue pressure does not force immediate frame loss.

## `getOutputFrame(...)`

`getOutputFrame(...)` does the actual `m_demuxer->getFrame(...)` call.

Behavior details:

- requires plugin status to be `Ready`
- returns `HJ_WOULD_BLOCK` if no frame is currently available
- converts demuxer quit into `Stoped`
- converts demuxer fatal failure into `Exception` and reports `HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME`
- increments returned frame's `m_streamIndex` by `m_streamIndexOffset`
- stores resulting stream index into plugin state

The stream-index offset matters because demuxer recreation should not make downstream consumers observe the new instance as an older stream generation.

## EOF Semantics

When `runTask(...)` sees an EOF frame:

1. it calls `runEof(i_streamIndex)`
2. `runEof(...)` first moves plugin status to `EOF`
3. then it asks graph `QUERY_CAN_PLUGIN_EOF_ID`
4. if graph says EOF is not yet allowed, it returns `HJ_WOULD_BLOCK`
5. otherwise it returns `HJ_OK`

Important point:

- demuxer plugin does not decide final EOF by itself
- graph policy remains the authority on whether EOF can propagate

This is critical for repeat/restart behavior in player graphs.

## Delivery Semantics

`deliverToOutputs(...)` forwards frames by media type:

- video outputs receive video frames and, when appropriate, EOF for video
- audio outputs receive audio frames and, when appropriate, EOF for audio

Additional behavior:

- for video frames, demux time metadata is attached:
  - `passThroughDemuxSystemTime`
  - `passThroughIsKey`

That metadata is later useful for playback-delay statistics downstream.

## Audio Track Switching

Current active code path is the direct one:

- call `m_demuxer->switchAudioTrack(i_trackId)` synchronously under producer lock
- if successful, update `m_audioTrackId`

There is also an alternative async handler-based implementation in the file, but it is currently disabled with `#if 0`.

So the current behavior is simpler than the presence of `m_runSwitchAudioTrackId` might suggest.

## Release Semantics

### `beforeDone()`

`beforeDone()` calls `quitDemuxer()` first.

That matters because active demuxer operations may be blocked in I/O and need `setQuit(true)` to unwind.

### `releaseDemuxer()`

`releaseDemuxer()`:

- resets `m_duration`
- calls `m_demuxer->done()`
- clears `m_demuxer`
- updates `m_streamIndexOffset = m_streamIndex + 1`
- clears cached current frame

This is a key semantic point:

- next demuxer generation starts with a newer stream-index base

## Known Caveats

Important caveats to remember:

1. Public `openURL`, `reset`, and `seek` are async in effect; they schedule work rather than fully completing it inline.

2. The plugin intentionally allows only one cached undeliverable frame; it is not an arbitrary queue.

3. `EVENT_STREAM_OPENED_ID` has dedup logic tied to init reason.

4. `switchAudioTrack()` currently uses the direct path, not the async path shown later in the file.

5. Stale-init protection is generation-based and depends on `quitDemuxer()` being called before new init scheduling.

## Typical Usage

```cpp
auto demuxer = HJCreates<HJPluginFFDemuxer>();
auto param = HJKeyStorage::create();
(*param)["mediaUrl"] = mediaUrl;
demuxer->init(param);

demuxer->addOutputPlugin(audioDecoder, HJMEDIA_TYPE_AUDIO);
demuxer->addOutputPlugin(videoDecoder, HJMEDIA_TYPE_VIDEO);

demuxer->openURL(otherUrl);
demuxer->seek(30'000);
demuxer->reset(0);
demuxer->done();
```

## What LLMs Should Read Next

If the task touches demuxer behavior, read in this order:

1. [HJPluginDemuxer.h](/f:/Source/hjmedia/src/plugins/HJPluginDemuxer.h)
2. [HJPluginDemuxer.cpp](/f:/Source/hjmedia/src/plugins/HJPluginDemuxer.cpp)
3. concrete demuxer subclass / `HJBaseDemuxer`
4. graph using it, such as [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)
5. downstream decoder docs

## Recommended Review Focus

When reviewing `HJPluginDemuxer` changes, prioritize:

1. stale-init and quit-generation correctness
2. seek/reset/open reentrancy and ordering
3. stream-open notification correctness
4. frame caching / backpressure behavior
5. EOF coordination with graph policy
6. stream-index continuity across demuxer recreation
