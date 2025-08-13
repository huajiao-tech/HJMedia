#pragma once

#include "HJLooper.h"

NS_HJ_BEGIN

class HJHandler : public std::enable_shared_from_this<HJHandler>
{
public:
	using Ptr = std::shared_ptr<HJHandler>;
	using Run = std::function<int(void)>;

	virtual void handleMessage(HJMessage::Ptr msg) {};
	virtual void dispatchMessage(HJMessage::Ptr msg);

	HJHandler(HJLooper::Ptr looper);

	// Handle Runnable
	virtual bool post(HJRunnable r) final;
	virtual bool postDelayed(HJRunnable r, int what, uint64_t delayMillis) final;
	virtual int runWithScissors(Run r, uint64_t timeout = 0) final;

	// Handle Message
	virtual bool sendMessageDelayed(HJMessage::Ptr msg, uint64_t delayMillis) final;
	virtual bool sendMessageAtTime(HJMessage::Ptr msg, uint64_t uptimeMillis);
	virtual void removeMessages(int what) final;

protected:
	bool enqueueMessage(HJMessageQueue::Ptr queue, HJMessage::Ptr msg, uint64_t uptimeMillis);
	static HJMessage::Ptr getPostMessage(HJRunnable r);

	HJLooper::Ptr mLooper{};
	HJMessageQueue::Ptr mQueue{};

private:
	class BlockingRunnable final : public HJSyncObject
	{
	public:
		using Ptr = std::shared_ptr<BlockingRunnable>;

		BlockingRunnable(Run task) : mTask(task) {};
		int postAndWait(HJHandler::Ptr handler, uint64_t timeout);

	private:
		Run mTask;
		int mRet{ HJErrAlreadyDone };
	};
};

NS_HJ_END
