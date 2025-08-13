#include "HJMessageQueue.h"
#include "HJTime.h"

NS_HJ_BEGIN

HJMessageQueue::HJMessageQueue()
{
	mPtr = nativeInit();
}

HJMessageQueue::~HJMessageQueue()
{
	dispose();
}

void HJMessageQueue::dispose()
{
	if (mPtr != 0) {
		nativeDestroy(mPtr);
		mPtr = 0;
	}
}

HJMessage::Ptr HJMessageQueue::next()
{
	// Return here if the message loop has already quit and been disposed.
	// This can happen if the application tries to restart a looper after quit
	// which is not supported.
	const auto ptr = mPtr;
	if (ptr == 0) {
		return nullptr;
	}

	int nextPollTimeoutMillis = 0;
	for (;;) {
		nativePollOnce(ptr, nextPollTimeoutMillis);

		{
			SYNCHRONIZED_SYNC_LOCK;
			// Try to retrieve the next message.  Return if found.
			const uint64_t now =  HJCurrentSteadyMS();
			HJMessage::Ptr msg = mMessages;
			if (msg != nullptr) {
				if (now < msg->when) {
					// Next message is not ready.  Set a timeout to wake up when it is ready.
//					nextPollTimeoutMillis = (int) Math.min(msg.when - now, Integer.MAX_VALUE);
					nextPollTimeoutMillis = static_cast<int>(std::min<uint64_t>(msg->when - now, HJ_INT_MAX));
				}
				else {
					// Got a message.
					mBlocked = false;
					mMessages = msg->next;
					msg->next = nullptr;
					return msg;
				}
			}
			else {
				// No more messages.
				nextPollTimeoutMillis = -1;
			}

			// Process the quit message now that all pending messages have been handled.
			if (mQuitting) {
				dispose();
				return nullptr;
			}

			mBlocked = true;
		}
	}
}

void HJMessageQueue::quit(bool safe)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (mQuitting) {
		return;
	}
	mQuitting = true;

	if (safe) {
		removeAllFutureMessagesLocked();
	}
	else {
		removeAllMessagesLocked();
	}

	// We can assume mPtr != 0 because mQuitting was previously false.
	nativeWake(mPtr);
}

bool HJMessageQueue::enqueueMessage(HJMessage::Ptr msg, uint64_t when)
{
	if (msg->target == nullptr)	{
		throw std::invalid_argument("Message must have a target.");
	}

	{
		SYNCHRONIZED_SYNC_LOCK;
		if (mQuitting) {
			msg->recycleUnchecked();
			return false;
		}

		msg->when = when;
		auto p = mMessages;
		bool needWake;
		if (p == nullptr || when == 0 || when < p->when) {
			// New head, wake up the event queue if blocked.
			msg->next = p;
			mMessages = msg;
			needWake = mBlocked;
		}
		else {
			// Inserted within the middle of the queue.  Usually we don't have to wake
			// up the event queue unless there is a barrier at the head of the queue
			// and the message is the earliest asynchronous message in the queue.
			needWake = mBlocked && p->target == nullptr;

			HJMessage::Ptr prev;
			for (;;) {
				prev = p;
				p = p->next;
				if (p == nullptr || when < p->when) {
					break;
				}
				if (needWake) {
					needWake = false;
				}
			}
			msg->next = p; // invariant: p == prev.next
			prev->next = msg;
		}

		// We can assume mPtr != 0 because mQuitting is false.
		if (needWake) {
			nativeWake(mPtr);
		}
	}
	return true;
}

void HJMessageQueue::removeMessages(HJHandlerPtr h, int what, HJSyncObject::Ptr object)
{
	if (h == nullptr) {
		return;
	}

	{
		SYNCHRONIZED_SYNC_LOCK;
		auto p = mMessages;

		// Remove all messages at front.
		while (p != nullptr && p->target == h && p->what == what
			&& (object == nullptr || p->obj == object)) {
			auto n = p->next;
			mMessages = n;
			p->recycleUnchecked();
			p = n;
		}

		// Remove all messages after front.
		while (p != nullptr) {
			auto n = p->next;
			if (n != nullptr) {
				if (n->target == h && n->what == what
					&& (object == nullptr || n->obj == object)) {
					auto nn = n->next;
					n->recycleUnchecked();
					p->next = nn;
					continue;
				}
			}
			p = n;
		}
	}
}

void HJMessageQueue::removeAllMessagesLocked()
{
	auto p = mMessages;
	while (p != nullptr) {
		auto n = p->next;
		p->recycleUnchecked();
		p = n;
	}
	mMessages = nullptr;
}

void HJMessageQueue::removeAllFutureMessagesLocked()
{
	const uint64_t now = HJCurrentSteadyMS();
	auto p = mMessages;
	if (p != nullptr) {
		if (p->when > now) {
			removeAllMessagesLocked();
		}
		else {
			HJMessage::Ptr n;
			for (;;) {
				n = p->next;
				if (n == nullptr) {
					return;
				}
				if (n->when > now) {
					break;
				}
				p = n;
			}
			p->next = nullptr;

			do {
				p = n;
				n = p->next;
				p->recycleUnchecked();
			} while (n != nullptr);
		}
	}
}

uint64_t HJMessageQueue::nativeInit()
{
	const auto q = new MessageQueue_ravenwood();
	const auto id = reinterpret_cast<uint64_t>(q);
	return id;
}

void HJMessageQueue::nativeDestroy(uint64_t ptr)
{
	delete reinterpret_cast<MessageQueue_ravenwood*>(ptr);
}

void HJMessageQueue::nativePollOnce(uint64_t ptr, int timeoutMillis)
{
	auto q = reinterpret_cast<MessageQueue_ravenwood*>(ptr);
	{
		SYNCHRONIZED(q->mPoller, lock);
		try {
			if (q->mPendingWake) {
				// Calling with pending wake returns immediately
			}
			else if (timeoutMillis == 0) {
				// Calling epoll_wait() with 0 returns immediately
			}
			else if (timeoutMillis == -1) {
				q->mPoller.wait(lock);
			}
			else {
				q->mPoller.wait(lock, timeoutMillis);
			}
		}
		catch (.../*InterruptedException e*/) {
//			Thread.currentThread().interrupt();
		}
		// Any reason for returning counts as a "wake", so clear pending
		q->mPendingWake = false;
	}
}

void HJMessageQueue::nativeWake(uint64_t ptr)
{
	auto q = reinterpret_cast<MessageQueue_ravenwood*>(ptr);
	{
		SYNCHRONIZED_LOCK(q->mPoller);
		q->mPendingWake = true;
		q->mPoller.notifyAll();
	}
}

NS_HJ_END
