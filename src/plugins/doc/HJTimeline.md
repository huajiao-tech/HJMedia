# HJTimeline

## Purpose

This document serves three audiences:

1. LLMs / coding agents
Purpose: establish stable context quickly so they do not repeatedly re-read the implementation or guess the time model.

2. Maintainers
Purpose: recall the timeline's state, synchronization semantics, listener behavior, and integration points.

3. Other readers
Purpose: understand what `HJTimeline` is responsible for and how it is used in player/render pipelines.

## What It Is

`HJTimeline` is a lightweight playback timeline controller used as a shared synchronization reference inside graph/plugin pipelines.

Its job is to:

- store the latest accepted playback anchor
- extrapolate current playback timestamp from steady clock and playback speed
- support pause / play transitions
- allow renderers or other consumers to observe timeline updates

It is not:

- a scheduler by itself
- a queue
- a complete media clock arbitration framework

## Code Location

- [HJTimeline.h](/f:/Source/hjmedia/src/plugins/HJTimeline.h)
- [HJTimeline.cpp](/f:/Source/hjmedia/src/plugins/HJTimeline.cpp)

Primary users in the current codebase:

- [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)
- [HJPluginVideoRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginVideoRender.cpp)
- [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)
- [HJGraphVodPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphVodPlayer.cpp)

## Responsibilities

`HJTimeline` maintains these core pieces of state:

- `m_streamIndex`: current accepted stream sequence index
- `m_timestamp`: anchor media timestamp in milliseconds
- `m_sysTimestamp`: steady-clock time when the anchor was last refreshed
- `m_speed`: playback speed used for extrapolation
- `m_paused`: whether timeline progression is frozen

From those values, `getTimestamp(...)` returns a derived "current playback position".

## Public API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `addListener(name, listener)` | Register a named listener | `void` | Replaces listener with the same name |
| `removeListener(name)` | Remove a named listener | `void` | Safe to call repeatedly |
| `setTimestamp(streamIndex, timestamp, speed)` | Update the playback anchor | `bool` | Rejects invalid input and older stream indices |
| `getTimestamp(out streamIndex, out timestamp, out speed)` | Read the current extrapolated timeline | `bool` | Returns `false` when timeline has no valid anchor |
| `flush()` | Reset timeline state | `void` | Clears stream validity and paused state |
| `play()` | Resume timeline progression | `void` | Only emits play notify on paused -> playing transition |
| `pause()` | Freeze timeline progression | `void` | Commits elapsed time into `m_timestamp` before pausing |

Timeline notifications:

- `HJ_TIMELINE_NOTIFY_UPDATED`
- `HJ_TIMELINE_NOTIFY_PLAY`
- `HJ_TIMELINE_NOTIFY_PAUSE`

## Real Semantics

### `setTimestamp(...)`

`setTimestamp` succeeds only when:

- `streamIndex >= 0`
- `speed > 0`
- object is not already `Done`
- incoming `streamIndex` is not older than the stored `m_streamIndex`

If accepted:

- `m_streamIndex` is updated
- `m_timestamp` becomes the new anchor timestamp
- `m_speed` is updated
- `m_sysTimestamp` is refreshed when timeline is not paused
- `HJ_TIMELINE_NOTIFY_UPDATED` is emitted outside the lock

Important detail:

- the implementation allows equal `streamIndex`
- the implementation rejects only strictly older `streamIndex`

That matters during reset / seek / decoder recreation flows where stream sequence is used as a coarse monotonic guard.

### `getTimestamp(...)`

If timeline has no valid anchor (`m_streamIndex < 0`), it returns `false`.

Otherwise:

- if paused, it returns the stored `m_timestamp`
- if playing, it returns:

```cpp
current = m_timestamp + (now - m_sysTimestamp) * m_speed
```

This means the returned timestamp is an extrapolated playback position, not necessarily the last exact renderer callback timestamp.

### `pause()`

If timeline is currently playing:

- it computes elapsed steady-clock time since `m_sysTimestamp`
- folds that elapsed time into `m_timestamp`
- marks `m_paused = true`
- emits `HJ_TIMELINE_NOTIFY_PAUSE`

This is important because after pause, future `getTimestamp(...)` calls should stay stable.

### `play()`

If timeline is currently paused:

- it refreshes `m_sysTimestamp` to the current steady clock
- marks `m_paused = false`
- emits `HJ_TIMELINE_NOTIFY_PLAY`

It does not change `m_timestamp`; it resumes extrapolation from the frozen anchor.

### `flush()`

`flush()` currently resets:

- `m_streamIndex = -1`
- `m_paused = false`

It does not explicitly reset every field.

Interpretation:

- timeline validity is primarily defined by `m_streamIndex`
- after flush, consumers should treat the timeline as invalid until a new `setTimestamp(...)` arrives

## Thread Model

`HJTimeline` inherits from `HJSyncObject` and uses:

- `SYNC_PROD_LOCK` for state mutation
- `SYNC_CONS_LOCK` for reads and listener snapshotting

Thread-safety characteristics:

- writes are serialized
- reads are protected
- listeners are copied under lock and invoked outside the lock

That last point is intentional: timeline callbacks must not run while timeline's internal lock is held, otherwise plugin/render callback chains would be much easier to deadlock.

## Lifecycle

Even though `HJTimeline` is lightweight, in this repository it is typically still used with normal `HJSyncObject` lifecycle:

- create
- `init(...)`
- share across graph/plugins
- `done()` during teardown

This is important because actual graph code does call `init(nullptr)` on the timeline, for example in:

- [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)
- [HJGraphVodPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphVodPlayer.cpp)

So the old statement "no extra init needed" is not a good repository-level description anymore.

On release, `internalRelease()`:

- clears listeners
- resets `m_streamIndex`
- clears paused state

and then delegates to `HJSyncObject::internalRelease()`.

## How It Is Used

### Audio-driven timeline

In audio playback flows, `HJPluginAudioRender` updates the timeline when audio output actually advances.

See:

- [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)

This is the most important usage pattern in music playback:

- audio render acts as the playback clock source
- graph queries timeline for current playback position

### Video render as listener / consumer

`HJPluginVideoRender` registers a listener on timeline and wakes itself on:

- `HJ_TIMELINE_NOTIFY_PLAY`
- `HJ_TIMELINE_NOTIFY_UPDATED`

See:

- [HJPluginVideoRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginVideoRender.cpp)

This means timeline is both:

- a clock source
- a lightweight event trigger for render scheduling

## Design Notes

### Why use `streamIndex`

`streamIndex` is used as a monotonic guard to prevent timeline rollback across stream generations.

This helps when:

- seek/reset creates a new stream sequence
- old frames arrive late
- stale consumers should not move the clock backward

The guard is coarse but effective for this plugin architecture.

### Why use steady clock

Extrapolation uses `HJCurrentSteadyMS()`, not wall-clock time.

That is the correct choice for playback progression because steady clock is monotonic and not affected by wall-clock jumps.

### Why notify outside the lock

Timeline listeners often post tasks into render/plugin code paths.

If callbacks were executed under the timeline lock, lock inversion and re-entrancy problems would be much more likely.

## Known Caveats

These are worth remembering when modifying code around `HJTimeline`:

1. `flush()` resets validity mainly through `m_streamIndex`, not through a full field wipe.

2. `getTimestamp(...)` returns an extrapolated value, so callers must not assume it came from the latest exact renderer callback.

3. Equal `streamIndex` updates are allowed, so "same generation, newer anchor" is a valid pattern.

4. Timeline itself does not define who the master clock is; repository usage decides that. In `HJGraphMusicPlayer`, audio render is the practical clock source.

5. Listener callbacks can trigger more work immediately, so timeline notifications are lightweight but not free.

## Typical Usage

### Renderer updates timeline

```cpp
timeline->setTimestamp(frame->m_streamIndex, frame->getPTS(), frame->getSpeed());
```

### Consumer reads current playback position

```cpp
int64_t streamIndex{};
int64_t timestamp{};
double speed{};
if (timeline->getTimestamp(streamIndex, timestamp, speed)) {
    // use timestamp as current playback head
}
```

### Video render listens for timeline changes

```cpp
timeline->addListener("VideoRender", [](const HJNotification::Ptr ntf) {
    switch (ntf->getID()) {
    case HJ_TIMELINE_NOTIFY_UPDATED:
    case HJ_TIMELINE_NOTIFY_PLAY:
        // wake render loop
        break;
    case HJ_TIMELINE_NOTIFY_PAUSE:
        // pause-specific handling
        break;
    }
    return HJ_OK;
});
```

### Teardown

```cpp
timeline->removeListener(name);
timeline->done();
```

## What LLMs Should Read Next

If the task touches timeline behavior, read in this order:

1. [HJTimeline.h](/f:/Source/hjmedia/src/plugins/HJTimeline.h)
2. [HJTimeline.cpp](/f:/Source/hjmedia/src/plugins/HJTimeline.cpp)
3. [HJPluginAudioRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginAudioRender.cpp)
4. [HJPluginVideoRender.cpp](/f:/Source/hjmedia/src/plugins/HJPluginVideoRender.cpp)
5. the graph using it, such as [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)

## Recommended Review Focus

When reviewing changes related to `HJTimeline`, prioritize:

1. pause/play correctness
2. extrapolated timestamp correctness
3. stream generation rollback issues
4. listener re-entrancy / deadlock risk
5. seek/reset/flush interactions
