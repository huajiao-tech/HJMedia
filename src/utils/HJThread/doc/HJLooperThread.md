# HJLooperThread

## Purpose

This document explains `HJLooperThread`, which is the most common entry point into the `HJThread` subsystem.

It serves three audiences:

1. LLMs / coding agents
Purpose: understand thread ownership, looper startup, handler creation, shutdown constraints, and timer behavior.

2. Maintainers
Purpose: recall lifecycle, `done()` restrictions, and the semantics of the nested `Handler` helper.

3. Other readers
Purpose: understand how this repository creates named worker threads that can run queued tasks.

## What It Is

`HJLooperThread` is a `HJSyncObject` that owns:

- one OS thread
- one thread-local `HJLooper` running on that thread
- handler creation for posting work onto that looper

The nested `HJLooperThread::Handler` adds convenience APIs on top of `HJHandler`:

- `async`
- `asyncAndClear`
- `sync`
- `runOrAsync`
- `runOrSync`
- timer helpers

So `HJLooperThread` is not just “a thread wrapper”; it is the lifecycle owner for an entire message-loop execution context.

## Code Location

- [HJLooperThread.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooperThread.h)
- [HJLooperThread.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJLooperThread.cpp)

Direct dependencies:

- [HJLooper.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.h)
- [HJHandler.h](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.h)

## Main API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `quickStart(name, identify)` | Create + init thread in one step | `Ptr` / `nullptr` | Common convenience entry |
| `createHandler()` | Create handler bound to this thread's looper | `Handler::Ptr` / `nullptr` | Waits for looper creation |
| `currentThread()` | Return current `HJLooperThread` ID | `int` | Uses thread-local ID |
| `done()` | Stop looper thread | `int` | Must not be called from the looper thread itself |

Nested handler convenience APIs:

| API | Purpose |
| --- | --- |
| `async(task, id, delay)` | Post delayed or immediate task |
| `asyncAndClear(task, id, delay)` | Remove same-id messages, then post |
| `sync(task)` | Execute synchronously via `runWithScissors` |
| `runOrAsync(task, id)` | Run inline if already on target thread, else async |
| `runOrSync(task)` | Run inline if already on target thread, else sync |
| `genMsgId()` | Generate per-handler message IDs |
| `openTimer(...)` / `closeTimer(...)` | Repeating timer built from re-posted delayed tasks |

## Lifecycle

### Creation

Typical creation path:

1. construct `HJLooperThread`
2. call `init()` or `quickStart(...)`
3. `internalInit(...)` launches a real `std::thread`
4. new thread sets thread-local `gThreadId`
5. thread enters `run()`
6. `run()` calls `HJLooper::prepare()`
7. `run()` stores `m_looper`
8. `run()` calls `HJLooper::loop()`

This means:

- thread start and looper availability are not identical instants
- `createHandler()` may need to wait until `m_looper` exists

### Handler creation

`createHandler()` calls `getLooper()`.

`getLooper()` waits until:

- thread status has advanced
- `m_looper` has been created

So handler creation is safe to call soon after startup, but it is not a trivial getter.

### Shutdown

Shutdown path:

1. `beforeDone()` sets `m_quitting = true`
2. fetch current looper
3. if called from the same looper thread:
   - return `HJErrNotSupport`
4. otherwise:
   - call `looper->quit()`
5. later `internalRelease()` joins the thread

Important consequence:

- actual thread join happens in `internalRelease()`
- looper quit is initiated in `beforeDone()`

## Why `done()` Must Not Run On The Same Looper Thread

This is one of the most important constraints.

If `done()` is called from the same looper thread:

- `beforeDone()` returns `HJErrNotSupport`
- thread teardown does not proceed normally

Why this exists:

- a thread cannot safely join itself
- self-destruction from inside the event loop is error-prone

Repository implication:

- owner objects that live on a looper thread should usually be shut down by another controlling thread

## Thread Identity Model

`HJLooperThread` assigns each started looper thread a numeric ID:

- thread-local `gThreadId`
- global `gThreadCount`

`HJLooper` stores the creating looper-thread ID and later compares it in:

- `isCurrentThread()`

This is how `runOrAsync()` and `runOrSync()` decide whether to execute inline or enqueue.

## Nested `Handler` Semantics

### `async(...)`

Simple wrapper around `postDelayed(...)`.

Use when:

- you always want async execution
- task ordering via queue is desired

### `asyncAndClear(...)`

Removes pending messages with the same ID, then posts the new task.

Use when:

- only the latest request matters
- classic examples are seek, reset, deferred refresh, or restart

This pattern appears frequently in graph/plugin code and is one of the highest-value conventions in the repository.

### `sync(...)`

Runs task synchronously on the target looper using `runWithScissors(...)`.

Use carefully:

- can block caller
- may deadlock if higher-level locks are held

### `runOrAsync(...)` / `runOrSync(...)`

These are convenience “same thread fast path” helpers.

If already on target looper thread:

- run inline

Otherwise:

- enqueue async or sync

These APIs are useful when callers may be on either side of the thread boundary.

## Timer Semantics

The nested `Handler::Timer` is a repeating timer implemented by re-posting delayed tasks.

Important behavior:

- timer computes scheduled time from fixed start plus tick index
- each run receives:
  - `schedule`
  - `next`
- if current time is already past `next`, it reposts immediately

This means timer is:

- drift-aware relative to its start epoch
- not just “sleep fixed delay every time”

`closeTimer(id)`:

- removes queued timer messages
- marks timer as quitting
- clears stored runnable

Handler destructor also cleans all registered timers.

## Error / Exception Behavior

Startup:

- thread creation failure in `internalInit()` returns `HJErrExcep`

Run loop:

- uncaught exceptions inside `run()` move status to `HJSTATUS_Exception`

Shutdown:

- `internalRelease()` joins if the thread is joinable
- exceptions during join are swallowed

## Typical Usage

### Create thread and handler

```cpp
auto thread = HJLooperThread::quickStart("audioThread");
auto handler = thread ? thread->createHandler() : nullptr;
```

### Post latest-only work

```cpp
const int seekMsgId = handler->genMsgId();
handler->asyncAndClear([player] {
    player->seekInternal();
}, seekMsgId);
```

### Run synchronously on looper

```cpp
int ret = handler->runOrSync([&] {
    return doWork();
});
```

### Timer

```cpp
int timerId = handler->openTimer(
    [](uint64_t now, uint64_t next) {
        // periodic callback
    },
    1000, 20);
```

## Known Caveats

Important caveats to remember:

1. `createHandler()` may block waiting for looper creation.

2. `done()` must not be called from the same looper thread.

3. `sync()` / `runOrSync()` are convenient but can participate in deadlocks if callers hold unrelated locks.

4. `asyncAndClear()` only clears messages with the same `what` ID, not arbitrary queued work.

5. Timer callbacks are just queued tasks; they are not a dedicated realtime timer facility.

## What LLMs Should Read Next

If the task touches `HJLooperThread`, read in this order:

1. [HJLooperThread.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooperThread.h)
2. [HJLooperThread.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJLooperThread.cpp)
3. [HJLooper.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooper.md)
4. [HJHandler.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJHandler.md)
5. [HJMessageQueue.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJMessageQueue.md)

## Recommended Review Focus

When reviewing `HJLooperThread` usage or modifications, prioritize:

1. shutdown from the correct thread
2. whether handler creation may race with startup
3. sync-call deadlock potential
4. repeated-task cancellation semantics
5. timer lifetime and cancellation correctness
