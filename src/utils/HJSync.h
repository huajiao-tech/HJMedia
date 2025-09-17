#pragma once

#include <shared_mutex>
#include <condition_variable>
#include "HJCommons.h"
#include "HJMacros.h"

NS_HJ_BEGIN

#define UNIQUE_LOCK					std::unique_lock<std::shared_mutex>
#define SHARED_LOCK					std::shared_lock<std::shared_mutex>

#define SYNCHRONIZED(sync, lock)	UNIQUE_LOCK lock((sync).m)
#define SYNCHRONIZED_LOCK(sync)		SYNCHRONIZED(sync, lock)

struct HJSync
{
	std::shared_mutex m;
	std::condition_variable_any cv;

	using Run = std::function<int(void)>;

	// 禁用拷贝操作
	HJSync(const HJSync&) = delete;
	HJSync& operator=(const HJSync&) = delete;

	// 禁用移动操作
	HJSync(HJSync&&) = delete;
	HJSync& operator=(HJSync&&) = delete;

	// 默认构造函数
	HJSync() = default;

	void wait(UNIQUE_LOCK& lock) {
		cv.wait(lock);
	}

	void wait(UNIQUE_LOCK& lock, int64_t timeout) {
		if (timeout < 0) {
			throw std::invalid_argument("Timeout value cannot be negative");
		}
		else if (timeout == 0) {
			cv.wait(lock);
		}
		else {
			cv.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout));
		}
	}

	void notify() {
		cv.notify_one();
	}

	void notifyAll() {
		cv.notify_all();
	}

	void lock() {
		m.lock();
	}

	bool tryLock() {
		return m.try_lock();
	}

	void unlock() {
		m.unlock();
	}

	// 消费者进锁
	template <typename T>
	auto consLock(T&& run) -> decltype(run()) {
		SHARED_LOCK lock(m);
		cv.wait(lock, [this] { return prodCount == 0; });

		return run();
	}

	// 生产者进锁
	template <typename T>
	auto prodLock(T&& run) -> decltype(run()) {
		++prodCount;
		UNIQUE_LOCK lock(m);

		try {
			if constexpr (std::is_void_v<decltype(run())>) {
				run();
				if (--prodCount == 0) cv.notify_all();
			}
			else {
				auto result = run();
				if (--prodCount == 0) cv.notify_all();
				return result;
			}
		}
		catch (...) {
			if (--prodCount == 0) cv.notify_all();
			throw;
		}
	}

private:
	std::atomic<int> prodCount{};
};

NS_HJ_END
