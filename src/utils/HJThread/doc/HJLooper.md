# HJLooper

## Purpose

This document explains `HJLooper`, the thread-local message loop at the core of the `HJThread` subsystem.

It serves three audiences:

1. LLMs / coding agents
Purpose: understand how queued work is actually executed and what assumptions `HJHandler` and `HJLooperThread` rely on.

2. Maintainers
Purpose: recall `prepare` / `loop` / `quit` semantics, thread-local restrictions, and weak-target dispatch behavior.

3. Other readers
Purpose: understand what a looper is in this repository and how it differs from just “a thread”.

## What It Is

`HJLooper` is the message loop bound to one `HJLooperThread`.

It owns:

- one `HJMessageQueue`
- one logical looper-thread identity

It provides:

- `prepare()` to create the thread-local looper
- `loop()` to run the dispatch loop
- `myLooper()` to fetch the current thread-local looper
- `quit()` to terminate the loop through the queue

It is not:

- a thread object
- a handler
- a general-purpose thread-local utility for arbitrary threads

## Code Location

- [HJLooper.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.h)
- [HJLooper.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.cpp)

Primary related classes:

- [HJLooperThread.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooperThread.h)
- [HJHandler.h](/f:/Source/hjmedia/src/utils/HJThread/HJHandler.h)
- [HJMessageQueue.h](/f:/Source/hjmedia/src/utils/HJThread/HJMessageQueue.h)

## Main API

| API | Purpose | Return | Notes |
| --- | --- | --- | --- |
| `prepare()` | Create the current thread's looper | `void` | Throws if thread is not an `HJLooperThread` or looper already exists |
| `loop()` | Run the dispatch loop | `void` | Blocks until queue returns `nullptr` |
| `myLooper()` | Get current thread-local looper | `Ptr` | Returns `nullptr` if not prepared |
| `isCurrentThread()` | Check whether caller is the owner thread | `bool` | Compares looper thread ID |
| `quit()` | Request looper termination | `void` | Calls `mQueue->quit(false)` |

## Thread-Local Model

`HJLooper` uses:

```cpp
static thread_local HJLooper::Ptr sThreadLocal{};
```

That means:

- each thread can have its own thread-local looper pointer
- but this implementation intentionally only allows looper creation on `HJLooperThread`

So while the storage is thread-local, the allowed creation context is tightly controlled.

## `prepare()` Semantics

`prepare()` enforces two restrictions:

1. current thread must be an `HJLooperThread`
2. current thread must not already have a looper

If either restriction is violated, it throws an exception.

Concretely:

- if `HJLooperThread::currentThread() < 0`, creation is rejected
- if `sThreadLocal != nullptr`, creation is rejected

This is important because the repository uses `HJLooper` as a managed subsystem, not as an open-ended threading primitive.

## `loop()` Semantics

`loop()`:

1. fetches `myLooper()`
2. throws if current thread has not called `prepare()`
3. repeatedly calls `loopOnce(me)`
4. exits when `loopOnce(...)` returns `false`

This means `loop()` is:

- blocking
- single-thread owned
- entirely driven by `HJMessageQueue::next()`

## `loopOnce(...)` Behavior

`loopOnce(...)` is the real execution step.

Behavior:

1. call `mQueue->next()`
2. if queue returns `nullptr`, stop looping
3. lock `msg->target`
4. if target is gone, skip dispatch and continue
5. otherwise call `target->dispatchMessage(msg)`
6. recycle the message

Important consequence:

- queued work can disappear silently if the target handler has already expired
- this is by design, because handlers are weak targets in this subsystem

That point matters a lot when reviewing teardown correctness.

## Queue Interaction

`HJLooper` itself is intentionally thin:

- scheduling policy lives in `HJMessageQueue`
- dispatch target semantics live in `HJHandler`
- looper just drives “next message -> dispatch -> recycle”

So when debugging odd looper behavior, you usually need to inspect:

- queue timing and quit behavior in `HJMessageQueue`
- message/handler dispatch behavior in `HJHandler`

## Thread Ownership

`HJLooper` stores:

- `mThread = HJLooperThread::currentThread()`

and later uses it in:

- `isCurrentThread()`

This is how callers such as `HJHandler::runWithScissors(...)` and `HJLooperThread::Handler::runOrAsync(...)` determine whether they are already on the target looper thread.

## `quit()` Semantics

`quit()` directly calls:

```cpp
mQueue->quit(false);
```

So current behavior is an “unsafe” quit:

- all queued messages are cleared
- queue is awakened
- `loop()` exits once `next()` returns `nullptr`

This is worth remembering because `HJMessageQueue` also supports `quit(true)` semantics internally, but `HJLooper` does not expose that option.

## Error / Exception Behavior

Important exception cases:

- `prepare()` throws if called on non-`HJLooperThread`
- `prepare()` throws if called twice on the same thread
- `loop()` throws if called before `prepare()`
- `dispatchMessage(...)` exceptions are rethrown out of `loopOnce(...)`

Repository implication:

- `HJLooperThread::run()` is the layer that catches exceptions around `HJLooper::loop()`
- raw looper usage should assume exceptions can escape dispatch

## Typical Usage

Direct usage is usually hidden behind `HJLooperThread`, but conceptually it looks like:

```cpp
HJLooper::prepare();
auto looper = HJLooper::myLooper();
HJLooper::loop();
```

More commonly, callers interact with it indirectly:

```cpp
auto thread = HJLooperThread::quickStart("worker");
auto handler = thread->createHandler();
```

## Known Caveats

Important caveats to remember:

1. `HJLooper` is not a general-purpose construct for arbitrary threads; creation is restricted to `HJLooperThread`.

2. Only one looper may exist per thread.

3. Expired handler targets are silently skipped during dispatch.

4. `quit()` always uses `quit(false)` and therefore clears all pending messages.

5. If you only read `HJLooper`, you will miss the real scheduling details in `HJMessageQueue`.

## What LLMs Should Read Next

If the task touches `HJLooper`, read in this order:

1. [HJLooper.h](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.h)
2. [HJLooper.cpp](/f:/Source/hjmedia/src/utils/HJThread/HJLooper.cpp)
3. [HJMessageQueue.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJMessageQueue.md)
4. [HJHandler.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJHandler.md)
5. [HJLooperThread.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooperThread.md)

## Recommended Review Focus

When reviewing `HJLooper` usage or modifications, prioritize:

1. whether looper creation still happens only on `HJLooperThread`
2. whether quit semantics match caller expectations
3. whether handler-target expiry is acceptable for the caller
4. exception propagation out of dispatch
5. whether code is incorrectly treating looper as a thread owner rather than a message loop
