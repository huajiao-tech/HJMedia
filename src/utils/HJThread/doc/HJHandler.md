# HJHandler

## Purpose

This document explains `HJHandler`, the object used to post callbacks and messages onto an `HJLooper`.

It serves three audiences:

1. LLMs / coding agents
Purpose: understand how async, delayed, and synchronous cross-thread execution are actually expressed in this repository.

2. Maintainers
Purpose: recall dispatch rules, weak-target semantics, timeout behavior, and the internal `BlockingRunnable` mechanism.

3. Other readers
Purpose: understand how work is sent to a looper thread and what guarantees `HJHandler` does and does not provide.

## What It Is

`HJHandler` is a looper-bound dispatcher.

It provides two main styles of work submission:

- callback-style work via `post(...)` / `postDelayed(...)`
- explicit message-style work via `sendMessageDelayed(...)`

It also provides:

- synchronous cross-thread execution via `runWithScissors(...)`
- message cancellation via `removeMessages(...)`

It is not:

- the message queue itself
- the owner of the worker thread
- a global scheduler

## Code Location

- [HJHandler.h](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.h)
- [HJHandler.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.cpp)

Key related classes:

- [HJLooper.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.h)
- [HJMessageQueue.h](/f:/Source/hjmedia/src/utils/HJThread/HJMessageQueue.h)
- [HJMessage.h](/f:/Source/hjmedia/src/utils/HJThread/HJMessage.h)

## Main API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `post(r)` | Post callback immediately | `bool` | Convenience wrapper |
| `postDelayed(r, what, delayMillis)` | Post callback with delay and optional `what` | `bool` | Callback is wrapped into a message |
| `sendMessageDelayed(msg, delayMillis)` | Send explicit message after delay | `bool` | Delay is based on steady clock |
| `sendMessageAtTime(msg, uptimeMillis)` | Send explicit message at absolute steady time | `bool` | Returns false if queue unavailable |
| `removeMessages(what, obj)` | Remove queued messages matching `what` and optional `obj` | `void` | Only affects this handler's messages |
| `runWithScissors(run, timeout)` | Execute task on target looper and wait | `int` | May run inline if already on target looper |
| `dispatchMessage(msg)` | Dispatch callback or `handleMessage` | `void` | Callback wins over virtual message handling |
| `handleMessage(msg)` | Override point for message-based handlers | `void` | Default no-op |

## Construction

`HJHandler` must be constructed with a non-null `HJLooper`.

Constructor behavior:

- stores `mLooper`
- stores `mQueue = looper->mQueue`

If looper is null, constructor throws.

This matters because `HJHandler` is not meant to exist in an “unbound” state.

## Dispatch Semantics

`dispatchMessage(...)` is simple but important:

1. if `msg->callback != nullptr`, run the callback
2. otherwise call `handleMessage(msg)`

This means:

- posted runnable-style work does not go through `handleMessage(...)`
- `handleMessage(...)` is only used when caller sends an explicit message without callback

That ordering is a core semantic rule and should not be changed casually.

## Callback Posting

### `post(...)`

`post(r)` is equivalent to:

- wrap runnable into a message
- send it with zero delay

### `postDelayed(...)`

`postDelayed(r, what, delayMillis)`:

- wraps runnable into a message
- sets `what`
- schedules it after `delayMillis`

Important note:

- the `what` field is often used later for cancellation via `removeMessages(...)`
- in this repository, helper layers such as `HJLooperThread::Handler::asyncAndClear(...)` rely heavily on this pattern

## Message Sending

### `sendMessageDelayed(...)`

This converts relative delay into an absolute steady-clock target time:

```cpp
sendMessageAtTime(msg, HJCurrentSteadyMS() + delayMillis)
```

Negative delays are clamped to zero.

### `sendMessageAtTime(...)`

This calls `enqueueMessage(...)` on the underlying queue.

If queue is unavailable, it returns false.

## Enqueue Semantics

`enqueueMessage(...)` does one very important thing before passing message to the queue:

- sets `msg->target = SHARED_FROM_THIS`

That means:

- queued messages reference the handler weakly
- if handler is destroyed before dispatch, message can later be dropped by the looper/queue

This weak-target design is fundamental to teardown behavior in the repository.

## Cancellation Semantics

`removeMessages(what, obj)` removes queued messages for:

- this handler
- matching `what`
- matching optional `obj`

This is narrower than “clear all pending work”.

Important implication:

- if two different logical operations share the same `what`, they may cancel each other
- if `obj` is not supplied, all matching `what` messages on this handler are candidates

## `runWithScissors(...)`

This is the synchronous cross-thread execution API.

Behavior:

1. validate input runnable and timeout
2. if caller is already on target looper:
   - run inline immediately
3. otherwise:
   - create `BlockingRunnable`
   - post it to the handler
   - wait for completion or timeout

This means `runWithScissors(...)` is:

- fast-path inline on same thread
- blocking on cross-thread path

## `BlockingRunnable` Semantics

`BlockingRunnable` is the internal helper used by `runWithScissors(...)`.

Important details:

- it inherits `HJSyncObject`
- it stores the callable and result code
- posted callback runs `mTask()`, catches exceptions, stores result, then calls `done()`
- posted message stores:
  - callback
  - `msg->obj = SHARED_FROM_THIS`

That `obj` field matters because timeout path removes the queued message using:

- `what`
- `obj`

So `BlockingRunnable` is not just “wait on condition”; it is also the identity used for best-effort cancellation on timeout.

## Timeout Semantics

This is one of the most important behavioral caveats.

If timeout expires in `postAndWait(...)`:

- handler removes the queued message by `what` and `obj`
- returns `HJErrTimeOut`

But this does not strictly guarantee that the task never executed.

Why:

- the looper may already have dequeued the message
- the callback may already be running
- cancellation only removes still-queued work

So the correct mental model is:

- timeout means “caller stopped waiting”
- not “target task definitely did not run”

This is exactly the kind of detail that should be surfaced to both humans and LLMs before they use sync calls casually.

## Error / Exception Behavior

Validation errors:

- null runnable -> exception
- negative timeout -> exception

Execution errors:

- if task throws inside `BlockingRunnable`, result becomes `HJErrExcep`
- if message cannot be enqueued, `postAndWait(...)` returns `HJErrFatal`

## Typical Usage

### Post callback

```cpp
handler->post([] {
    doWork();
});
```

### Post delayed callback with cancel key

```cpp
handler->postDelayed([] {
    refresh();
}, refreshMsgId, 50);
```

### Cancel pending latest-only work

```cpp
handler->removeMessages(refreshMsgId);
```

### Run synchronously on target looper

```cpp
int ret = handler->runWithScissors([&] {
    return doWorkAndReturnStatus();
}, 1000);
```

## Known Caveats

Important caveats to remember:

1. `dispatchMessage(...)` prioritizes callback over `handleMessage(...)`.

2. Handler target is weak; queued messages may be dropped if handler is gone.

3. `runWithScissors(...)` timeout does not strictly mean the task did not execute.

4. `removeMessages(...)` is keyed by `what` and optional `obj`, not by function identity.

5. `HJHandler` does not own the looper thread; it assumes looper lifetime is managed elsewhere.

## What LLMs Should Read Next

If the task touches `HJHandler`, read in this order:

1. [HJHandler.h](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.h)
2. [HJHandler.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.cpp)
3. [HJLooper.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooper.md)
4. [HJMessageQueue.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJMessageQueue.md)
5. [HJLooperThread.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooperThread.md)

## Recommended Review Focus

When reviewing `HJHandler` usage or modifications, prioritize:

1. whether sync execution can deadlock
2. whether timeout semantics are acceptable for the caller
3. whether message IDs are used safely for cancellation
4. whether handler lifetime is long enough for queued work
5. whether caller really wants callback-style or message-style dispatch
