# HJMessage

## Purpose
`HJMessage` is the unit passed through the `HJThread` message loop. It carries scheduling metadata, dispatch ownership, optional payload, and the callback or `what` code that a `HJHandler` will consume.

This document is meant to help three readers:
- LLM agents quickly understand what a message really contains and what gets reset or destroyed during recycling.
- Maintainers recall object-pool behavior, `obj` cleanup semantics, and the relationship between `HJMessage`, `HJHandler`, and `HJMessageQueue`.
- Other readers see how a message moves through the Looper/Handler system without rereading all implementation files.

## What It Is
`HJMessage` is a small heap object with two roles:
- A queue node used by `HJMessageQueue` to build a time-ordered linked list of pending work.
- A pooled reusable message object to reduce allocation churn for frequent post/send operations.

Code location:
- `src/utils/HJThread/HJMessage.h`
- `src/utils/HJThread/HJMessage.cpp`

## Field Semantics
- `what`
  - Integer message code typically interpreted by `HJHandler::handleMessage(...)`.
- `arg1`, `arg2`
  - Small inline integer payload fields.
- `obj`
  - Optional strong reference payload of type `HJSyncObject::Ptr`.
  - Recycle will call `obj->done()` before clearing it.
- `when`
  - Target dispatch time used by `HJMessageQueue` ordering.
- `target`
  - Weak reference to the destination `HJHandler`.
  - It is assigned when the message is enqueued, not when it is created.
- `callback`
  - Optional runnable used by `HJHandler::dispatchMessage(...)`.
  - When present, it takes precedence over `handleMessage(...)`.
- `next`
  - Intrusive linked-list pointer.
  - It is used both by the pending queue and by the static object pool.

## Object Pool Model
`HJMessage` uses a process-wide static pool guarded by `sPoolSync`.

Internal state:
- `sPool`
  - Head of the free-list.
- `sPoolSize`
  - Current pool size.
- `MAX_POOL_SIZE`
  - Hard limit of `50`.

The pool exists to reuse message objects after dispatch/removal instead of allocating a fresh object for every post/send path.

## `obtain()` Semantics
`obtain()` first tries to pop one message from the static pool.

Behavior:
- If the pool is non-empty, it removes the head node, clears its `next`, decreases `sPoolSize`, and returns that instance.
- If the pool is empty, it allocates a new `HJMessage`.

Important implication:
- `obtain()` does not initialize semantic fields for a new use beyond whatever `recycleUnchecked()` already cleared earlier.
- Callers and higher layers are responsible for filling `what`, `obj`, `callback`, `when`, and other fields before enqueue.

## `recycleUnchecked()` Semantics
`recycleUnchecked()` resets the message and attempts to return it to the static pool.

What it clears:
- `what = 0`
- `arg1 = 0`
- `arg2 = 0`
- `when = 0`
- `target.reset()`
- `callback = nullptr`

What it additionally does:
- If `obj != nullptr`, it calls `obj->done()` and then clears `obj`.

Pool return behavior:
- If `sPoolSize < 50`, the message is pushed back to the pool via `next`.
- If the pool is already full, the message is simply left out of the pool and normal reference counting decides its lifetime.

## `obj` Ownership And Cleanup
The most important non-obvious rule in this class is:
- Recycling a message is also a cleanup point for `obj`.

This matters because `obj` is not just a passive payload pointer. It may represent a synchronization helper or cancellation-aware object whose `done()` method has side effects.

Practical consequence:
- Do not treat `obj` as data that survives recycle unchanged.
- If external code still depends on the same `HJSyncObject`, it must hold its own reference and understand that message recycle may already have called `done()`.

## Queue And Dispatch Relationship
`HJMessage` itself does not know how to dispatch. It only carries the information required by other classes:
- `HJHandler` sets `target` and decides whether the message is callback-driven or `what`-driven.
- `HJMessageQueue` sorts by `when`, links messages via `next`, and may discard messages whose weak `target` has already expired.
- `HJLooper` fetches the next due message from the queue and executes dispatch on the owning thread.

## Typical Usage
Common flow:
1. A caller obtains a message, directly or indirectly through `HJHandler`.
2. The caller fills fields such as `what`, `obj`, or `callback`.
3. `HJHandler` assigns `target` and enqueues the message with a `when`.
4. `HJMessageQueue` stores it in due-time order.
5. `HJLooper` pulls the message when due.
6. After dispatch or removal, the message can be recycled and possibly returned to the pool.

## Known Caveats
- `target` is a weak reference, so a queued message may become undeliverable before dispatch if the handler is already gone.
- `next` has dual meaning: pending-queue linkage and pool free-list linkage. Reviews should treat it as internal infrastructure, not business payload.
- `recycleUnchecked()` is intentionally aggressive and should only run when higher layers know the message is no longer in active use.
- `recycleMessagePool()` clears the pool head but does not walk every pooled object to run extra cleanup logic.

## What LLMs Should Read Next
To understand how `HJMessage` is produced, queued, and consumed, read in this order:
1. `src/utils/HJThread/doc/HJHandler.md`
2. `src/utils/HJThread/doc/HJMessageQueue.md`
3. `src/utils/HJThread/doc/HJLooper.md`
4. `src/utils/HJThread/doc/HJLooperThread.md`

## Recommended Review Focus
When reviewing code that touches `HJMessage`, focus on:
- Whether `obj` cleanup via `done()` is still safe.
- Whether a new usage accidentally relies on strong `target` ownership.
- Whether any new field would also need reset behavior during recycle.
- Whether message reuse could observe stale state because a caller forgot to overwrite a field before enqueue.
