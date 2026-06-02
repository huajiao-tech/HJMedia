#include "HJLooperThread.h"
#include "HJTime.h"
#include "HJFLog.h"

NS_HJ_BEGIN

static thread_local int gThreadId{ -1 };
static std::atomic<int> gThreadCount{ 0 };

HJLooperThread::Handler::~Handler()
{
	SYNCHRONIZED_LOCK(m_timerSync);

	for (auto it = m_timerMap.begin(); it != m_timerMap.end();) {
		auto timer = it->second;
		if (timer) {
			SYNCHRONIZED(timer->sync, timerLock);
			timer->quitting = true;
			timer->run = nullptr;
		}
		it = m_timerMap.erase(it);
	}
}

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

int  HJLooperThread::Handler::runOrSync(Run task)
{
	if (task == nullptr) {
		return HJErrInvalidParams;
	}

	if (mLooper->isCurrentThread()) {
		return task();
	}
	else {
		return sync(task);
	}
}

int HJLooperThread::Handler::openTimer(Timer::Run run, int den, int num, int id)
{
	if (!run || den <= 0 || num <= 0) {
		HJFLoge("openTimer error! (!run || den({}) <= 0 || num({}) <= 0)", den, num);
		return -1;
	}

	if (id < 0) {
		id = genMsgId();
	}

	auto timer = std::make_shared<Timer>();
	std::weak_ptr<Timer> wTimer = timer;
	Wtr wHandler = SHARED_FROM_THIS;
	timer->run = [wTimer, wHandler, num, den, id, run] {
		auto timer = wTimer.lock();
		if (!timer) {
			return;
		}
		SYNCHRONIZED_LOCK(timer->sync);
		if (timer->quitting) {
			return;
		}
		auto handler = wHandler.lock();
		if (!handler) {
			return;
		}

		uint64_t schedule = timer->start + timer->numIndex * num * 1000 / den;
		timer->numIndex++;
		uint64_t next = timer->start + timer->numIndex * num * 1000 / den;

		run(schedule, next);

		uint64_t now = HJCurrentSteadyMS();
		if (now < next) {
			handler->postDelayed(timer->run, id, next - now);
		}
		else {
			handler->postDelayed(timer->run, id, 0);
		}
	};
	timer->quitting = false;
	timer->start = HJCurrentSteadyMS();
	timer->numIndex = 0;
	{
		SYNCHRONIZED_LOCK(m_timerSync);
		if (m_timerMap.find(id) != m_timerMap.end()) {
			HJFLoge("openTimer error! id({}) already exists.", id);
			return -1;
		}
		m_timerMap[id] = timer;
	}

	postDelayed(timer->run, id, 0);

	return id;
}

void HJLooperThread::Handler::closeTimer(int id)
{
	SYNCHRONIZED_LOCK(m_timerSync);

	auto it = m_timerMap.find(id);
	if (it != m_timerMap.end()) {
		removeMessages(it->first);
		auto timer = it->second;
		if (timer) {
			SYNCHRONIZED(timer->sync, timerLock);
			timer->quitting = true;
			timer->run = nullptr;
		}
		m_timerMap.erase(it);
	}
}

int HJLooperThread::currentThread()
{
	return gThreadId;
}

HJLooperThread::Ptr HJLooperThread::quickStart(const std::string& name, size_t identify)
{
	auto thread = std::make_shared<HJLooperThread>(name, identify);
	int ret = thread->init();
	if (ret != HJ_OK) {
		return nullptr;
	}

	return thread;
}

HJLooperThread::~HJLooperThread()
{
	done();
}

int HJLooperThread::internalInit(HJKeyStorage::Ptr i_param)
{
	int ret = HJSyncObject::internalInit(i_param);
	if (ret != HJ_OK) {
		return ret;
	}
	if (m_quitting.load()) {
		return HJErrAlreadyDone;
	}

	try {
		m_thread = std::thread([](HJLooperThread* thread) {
			gThreadId = gThreadCount.fetch_add(1);
			thread->run();
		}, this);
	}
	catch (.../*const std::exception&*/) {
		return HJErrExcep;
	}

	return HJ_OK;
}

int HJLooperThread::beforeDone()
{
	m_quitting.store(true);

	auto looper = getLooper();
	if (looper != nullptr) {
		if (looper->isCurrentThread()) {
			return HJErrNotSupport;
		}
		looper->quit();
	}

	return HJSyncObject::beforeDone();
}

void HJLooperThread::internalRelease()
{
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

		if (m_quitting.load()) {
			return;
		}

		HJLooper::prepare();

		int ret = SYNC_PROD_LOCK([this] {
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
		SYNC_PROD_LOCK([this] {
			CHECK_DONE_STATUS();
			m_status = HJSTATUS_Exception;
		});
	}
}

HJLooper::Ptr HJLooperThread::getLooper()
{
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
