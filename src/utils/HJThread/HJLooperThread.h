#pragma once

#include "HJHandler.h"

NS_HJ_BEGIN

class HJLooperThread : virtual public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJLooperThread>;

	class Handler final : public HJHandler
	{
	public:
		using Ptr = std::shared_ptr<Handler>;

		Handler(HJLooper::Ptr looper)
			: HJHandler(looper) {}

		bool async(HJRunnable task, int id = 0, uint64_t delayMillis = 0);
		bool asyncAndClear(HJRunnable task, int id = 1, uint64_t delayMillis = 0);
		int sync(Run task);
		bool runOrAsync(HJRunnable task, int id = 0);
		int genMsgId() {
			return m_lastMsgId.fetch_add(1);
		}

	private:
		std::atomic<int> m_lastMsgId{ 3 };
	};

	static int currentThread();
	static Ptr quickStart(const std::string& name, size_t identify = -1);

	HJLooperThread(const std::string& name = "HJLooperThread", size_t identify = -1)
		: HJSyncObject(name, identify) {}

	virtual Handler::Ptr createHandler();

protected:
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void run();
	virtual HJLooper::Ptr getLooper();

private:
	std::thread m_thread;
	HJLooper::Ptr m_looper{};
};

NS_HJ_END
