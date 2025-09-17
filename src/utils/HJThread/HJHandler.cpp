#include "HJHandler.h"
#include "HJError.h"
#include "HJTime.h"

NS_HJ_BEGIN

void HJHandler::dispatchMessage(HJMessage::Ptr msg)
{
	if (msg->callback != nullptr) {
		msg->callback();
	}
	else {
		handleMessage(msg);
	}
}

HJHandler::HJHandler(HJLooper::Ptr looper)
{
	if (!looper) {
		throw std::invalid_argument("looper mast not be null");
	}
	mLooper = looper;
	mQueue = looper->mQueue;
}

bool HJHandler::post(HJRunnable r)
{
	return sendMessageDelayed(getPostMessage(r), 0);
}

bool HJHandler::postDelayed(HJRunnable r, int what, uint64_t delayMillis)
{
	return sendMessageDelayed(getPostMessage(r)->setWhat(what), delayMillis);
}

int HJHandler::runWithScissors(Run r, uint64_t timeout)
{
	if (r == nullptr)
	{
		throw std::invalid_argument("runnable must not be null");
	}
	if (timeout < 0)
	{
		throw std::invalid_argument("timeout must be non-negative");
	}

	if (HJLooper::myLooper() == mLooper) {
		return r();
	}

	BlockingRunnable::Ptr br = std::make_shared<BlockingRunnable>(r);
	return br->postAndWait(SHARED_FROM_THIS, timeout);
}

bool HJHandler::sendMessageDelayed(HJMessage::Ptr msg, uint64_t delayMillis)
{
	if (delayMillis < 0) {
		delayMillis = 0;
	}
	return sendMessageAtTime(msg, HJCurrentSteadyMS() + delayMillis);
}

bool HJHandler::sendMessageAtTime(HJMessage::Ptr msg, uint64_t uptimeMillis)
{
	auto queue = mQueue;
	if (queue == nullptr) {
		return false;
	}
	return enqueueMessage(queue, msg, uptimeMillis);
}

bool HJHandler::enqueueMessage(HJMessageQueue::Ptr queue, HJMessage::Ptr msg, uint64_t uptimeMillis)
{
	msg->target = SHARED_FROM_THIS;

	return queue->enqueueMessage(msg, uptimeMillis);
}

void HJHandler::removeMessages(int what)
{
	mQueue->removeMessages(SHARED_FROM_THIS, what, nullptr);
}

HJMessage::Ptr HJHandler::getPostMessage(HJRunnable r)
{
	auto m = HJMessage::obtain();
	m->callback = r;
	return m;
}

int HJHandler::BlockingRunnable::postAndWait(HJHandler::Ptr handler, uint64_t timeout)
{
	auto msg = getPostMessage([=] {
		try {
			mRet = mTask();
		}
		catch (...) {
			mRet = HJErrExcep;
		}

		done();
	});
	msg->obj = SHARED_FROM_THIS;
	if (!handler->sendMessageDelayed(msg, 0)) {
		return HJErrFatal;
	}

	{
		SYNCHRONIZED_SYNC(lock);
		if (timeout > 0) {
			const int64_t expirationTime = HJCurrentSteadyMS() + timeout;
			while (m_status != HJSTATUS_Done) {
				int64_t delay = expirationTime - HJCurrentSteadyMS();
				if (delay <= 0) {
					return HJErrTimeOut; // timeout
				}
				try {
					SYNC_WAIT(lock, delay);
				}
				catch (...) {
				}
			}
		}
		else {
			while (m_status != HJSTATUS_Done) {
				try {
					SYNC_WAIT(lock);
				}
				catch (...) {
				}
			}
		}
	}
	return mRet;
}

NS_HJ_END
