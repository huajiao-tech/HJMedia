#include "HJLooperThread.h"
#include "HJFLog.h"

NS_HJ_BEGIN

static thread_local int gThreadId{ -1 };
static std::atomic<int> gThreadCount{ 0 };

bool HJLooperThread::Handler::async(HJRunnable task, int id, uint64_t delayMillis)
{
	if (task) {
		return postDelayed(task, id, delayMillis);
	}
	return false;
}

bool HJLooperThread::Handler::asyncAndClear(HJRunnable task, int id, uint64_t delayMillis)
{
	removeMessages(id);
	if (task) {
		return postDelayed(task, id, delayMillis);
	}
	return true;
}

int HJLooperThread::Handler::sync(Run task)
{
	return runWithScissors(task);
}

bool HJLooperThread::Handler::runOrAsync(HJRunnable task, int id)
{
	if (task) {
		if (mLooper->isCurrentThread()) {
			task();
			return true;
		}
		else {
			return async(task, id);
		}
	}
	return false;
}

int HJLooperThread::currentThread()
{
	return gThreadId;
}

HJLooperThread::Ptr HJLooperThread::quickStart(const std::string& name, size_t identify)
{
	auto thread = std::make_shared<HJLooperThread>(name, identify);
	int ret = thread->init(nullptr);
	if (ret != HJ_OK) {
		return nullptr;
	}

	return thread;
}

int HJLooperThread::internalInit(HJKeyStorage::Ptr i_param)
{
	int ret = HJSyncObject::internalInit(i_param);
	if (ret < 0) {
		return ret;
	}

	do {
		try {
			m_thread = std::thread([](HJLooperThread* thread) {
				gThreadId = gThreadCount.fetch_add(1);
				thread->run();
			}, this);
		}
		catch (.../*const std::exception&*/) {
			ret = HJErrExcep;
			break;
		}

		return HJ_OK;
	} while (false);

	HJLooperThread::internalRelease();
	return ret;
}

void HJLooperThread::internalRelease()
{
	auto looper = getLooper();
	if (looper != nullptr) {
		looper->quit();
	}

	if (m_thread.joinable()) {
		try {
			m_thread.join();
		}
		catch (...) {
		}
	}

	HJSyncObject::internalRelease();
}

void HJLooperThread::run()
{
	try {
        HJFLogi("{}, start.", getName());
		HJLooper::prepare();

		int ret = SYNC_PROD_LOCK([=] {
			CHECK_DONE_STATUS(HJErrAlreadyDone);
			m_looper = HJLooper::myLooper();
			return HJ_OK;
		});
		if (ret < 0) {
			return;
		}

		HJLooper::loop();
        HJFLogi("{}, stop.", getName());
	}
	catch (...) {
		SYNC_PROD_LOCK([=] {
			CHECK_DONE_STATUS();
			m_status = HJSTATUS_Exception;
		});
	}
}

HJLooper::Ptr HJLooperThread::getLooper() {

	{
		SYNCHRONIZED_SYNC(lock);
		if (m_status == HJSTATUS_NONE) {
			return nullptr;
		}

		// If the thread has been started, wait until the looper has been created.
		while (m_status < HJSTATUS_Exception && m_looper == nullptr) {
			try {
				SYNC_WAIT(lock);
			}
			catch (.../*InterruptedException e*/) {
			}
		}
	}

	return m_looper;
}

HJLooperThread::Handler::Ptr HJLooperThread::createHandler()
{
	auto looper = getLooper();
	if (looper == nullptr) {
		return nullptr;
	}

	return std::make_shared<Handler>(looper);
}

NS_HJ_END
