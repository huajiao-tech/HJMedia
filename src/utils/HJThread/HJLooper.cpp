#include "HJLooperThread.h"

NS_HJ_BEGIN

static thread_local HJLooper::Ptr sThreadLocal{};

void HJLooper::prepare()
{
	if (sThreadLocal != nullptr) {
		throw std::runtime_error("Only one Looper may be created per thread");
	}
	sThreadLocal = std::make_shared<HJLooper>();
}

bool HJLooper::loopOnce(const Ptr me)
{
	auto msg = me->mQueue->next(); // might block
	if (msg == nullptr) {
		// No message indicates that the message queue is quitting.
		return false;
	}

	try {
		msg->target->dispatchMessage(msg);
	}
	catch (.../*Exception exception*/) {
		throw/* exception*/;
	}

	msg->recycleUnchecked();

	return true;
}

void HJLooper::loop()
{
	const auto me = myLooper();
	if (me == nullptr) {
		throw std::runtime_error("No Looper; Looper.prepare() wasn't called on this thread.");
	}

	for (;;) {
		if (!loopOnce(me)) {
			return;
		}
	}
}

HJLooper::Ptr HJLooper::myLooper()
{
	return sThreadLocal;
}

HJLooper::HJLooper()
{
	mQueue = std::make_shared<HJMessageQueue>();
	mThread = HJLooperThread::currentThread();
}

bool HJLooper::isCurrentThread()
{
	return HJLooperThread::currentThread() == mThread;
}

void HJLooper::quit()
{
	mQueue->quit(false);
}

NS_HJ_END
