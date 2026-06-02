# HJMessageQueue

## Purpose

This document explains `HJMessageQueue`, the ordered message queue that backs `HJLooper`.

It serves three audiences:

1. LLMs / coding agents
Purpose: understand scheduling, blocking, wakeup, quit behavior, and expired-target cleanup before reasoning about looper/handler behavior.

2. Maintainers
Purpose: recall queue ordering, `next()` semantics, safe vs unsafe quit, and removal/cleanup rules.

3. Other readers
Purpose: understand how delayed tasks are stored and how the message loop blocks until work is ready.

## What It Is

`HJMessageQueue` is the time-ordered message store used by `HJLooper`.

It is responsible for:

- storing pending `HJMessage` nodes in `when` order
- blocking until the next message is due or until explicitly woken
- returning the next dispatchable message
- clearing messages during quit
- removing messages by handler / `what` / `obj`
- dropping expired weak-target messages

It is not:

- the dispatcher
- the thread owner
- the callback execution engine

## Code Location

- [HJMessageQueue.h](/f:/Source/hjmedia/src/utils/HJThread/HJMessageQueue.h)
- [HJMessageQueue.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJMessageQueue.cpp)

Related classes:

- [HJLooper.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.h)
- [HJHandler.h](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.h)
- [HJMessage.h](/f:/Source/hjmedia/src/utils/HJThread/HJMessage.h)

## Main API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `next()` | Block until next dispatchable message is ready | `HJMessage::Ptr` / `nullptr` | Returns `nullptr` when queue is quitting/disposed |
| `enqueueMessage(msg, when)` | Insert message in time order | `bool` | Returns false when queue is already quitting |
| `quit(safe)` | Mark queue quitting and clear messages | `void` | `safe=true` keeps due-now/past messages; `false` clears all |
| `removeMessages(handler, what, obj)` | Remove matching queued messages | `void` | Also cleans expired-target messages encountered during scan |

## Internal Model

Core state:

- `mMessages`: singly linked list of pending messages sorted by `when`
- `mQuitting`: queue shutdown flag
- `mBlocked`: whether `next()` is currently blocked waiting
- `mPtr`: native-style poller handle

Synchronization:

- `m_sync` protects queue state and linked-list mutation

Wake/poll simulation:

- `nativePollOnce(...)`
- `nativeWake(...)`

These are implemented with an internal condition-variable style helper (`MessageQueue_ravenwood`), not a real OS epoll queue.

## Message Ordering

Messages are kept ordered by `when`.

Insertion behavior in `enqueueMessage(...)`:

- if queue is empty, message becomes head
- if new `when` is earlier than current head, message becomes new head
- otherwise it is inserted into the sorted linked list

So the queue is:

- ordered by due time
- not FIFO across different `when`

## `next()` Semantics

`next()` is the most important method.

High-level behavior:

1. if queue has already been disposed (`mPtr == 0`), return `nullptr`
2. poll/wait using `nativePollOnce(...)`
3. under lock:
   - inspect queue head
   - if head exists but is not due yet:
     - compute timeout until due time
   - if head is due:
     - detach it from queue
     - if target is still alive, return it
     - if target expired, recycle and continue loop
   - if queue empty:
     - wait indefinitely
4. if `mQuitting` is set:
   - dispose queue
   - return `nullptr`

This means `next()` is:

- blocking
- due-time aware
- resilient to expired targets
- the point where quitting becomes final

## Expired Target Cleanup

One subtle but important behavior:

- even if a message still exists in the linked list, it may no longer be dispatchable because `msg->target` is a weak reference

`next()` handles this by:

- popping the message
- checking `msg->target.lock()`
- recycling it and continuing if target is gone

`removeMessages(...)` also removes expired-target messages while scanning.

So expired-target cleanup happens in more than one place, which is by design.

## Blocking and Wakeup

`next()` computes `nextPollTimeoutMillis` as:

- `0` for immediate poll
- positive timeout until next due message
- `-1` when queue is empty and should wait indefinitely

Wake behavior:

- when earlier work is inserted at the head and queue is blocked, `nativeWake(...)` is called
- quit also calls `nativeWake(...)`

This allows a looper waiting for a later message (or no message) to wake up when queue conditions change.

## `enqueueMessage(...)` Semantics

`enqueueMessage(...)` requires:

- `msg->target` must already exist

If target is expired before enqueue, it throws.

If queue is already quitting:

- message is recycled
- function returns false

Wake policy:

- inserting a new head may wake the blocked queue
- inserting in the middle usually does not wake

The implementation also contains logic around expired head targets when deciding wake necessity.

## Quit Semantics

`quit(bool safe)` marks the queue as quitting and clears messages.

Two modes:

### `quit(false)`

- remove all queued messages immediately

### `quit(true)`

- keep messages whose `when <= now`
- remove only future messages

In this repository, `HJLooper::quit()` currently calls:

- `mQueue->quit(false)`

So the common looper shutdown path is the unsafe/full-clear variant.

This is a very important practical detail for reviewing teardown behavior.

## Removal Semantics

`removeMessages(handler, what, obj)` removes:

- messages owned by the given handler
- with matching `what`
- and matching `obj` if `obj` is provided

It scans:

- queue head repeatedly
- then the remaining linked list

While scanning, it also removes messages whose target handler has already expired.

This means removal is:

- targeted
- opportunistic cleanup for dead targets

## Dispose Semantics

`dispose()` destroys the internal poller handle (`mPtr`) and sets it to zero.

Once disposed:

- `next()` returns `nullptr`
- queue restart is not supported

This matches the comment in the code: restarting a looper after quit is not supported.

## Native Poller Emulation

Despite the `native*` naming, the current implementation is an in-process synchronization shim:

- `nativeInit()` allocates a `MessageQueue_ravenwood`
- `nativePollOnce()` waits on condition variable semantics
- `nativeWake()` sets `mPendingWake` and notifies

This naming mimics Android-style layering, but behavior is local C++ synchronization rather than a platform event fd/epoll abstraction.

## Error / Exception Behavior

Key failure/edge behaviors:

- enqueue without valid target -> exception
- enqueue while quitting -> recycle and return false
- `next()` after dispose -> `nullptr`
- repeated `quit()` is ignored after first call

## Typical Usage

Direct use is usually hidden behind `HJHandler` and `HJLooper`, but conceptually:

```cpp
queue->enqueueMessage(msg, when);
auto next = queue->next();
```

More commonly:

```cpp
handler->postDelayed(task, msgId, 50);
handler->removeMessages(msgId);
looper->quit();
```

## Known Caveats

Important caveats to remember:

1. `next()` is where quit becomes terminal and where disposed queue returns `nullptr`.

2. Expired-target messages may be dropped in both `next()` and `removeMessages(...)`.

3. `quit(false)` clears all pending messages, not just future ones.

4. `mPtr` disposal means queue restart is not supported.

5. Wake logic depends on whether the queue is blocked and where the new message lands in time order.

## What LLMs Should Read Next

If the task touches `HJMessageQueue`, read in this order:

1. [HJMessageQueue.h](/f:/Source/hjmedia/src/utils/HJThread/HJMessageQueue.h)
2. [HJMessageQueue.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJMessageQueue.cpp)
3. [HJLooper.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooper.md)
4. [HJHandler.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJHandler.md)
5. [HJMessage.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJMessage.md)

## Recommended Review Focus

When reviewing `HJMessageQueue` usage or modifications, prioritize:

1. time ordering correctness
2. quit semantics expectations
3. expired-target cleanup behavior
4. wakeup correctness when inserting earlier work
5. whether caller assumes queue can restart after quit
