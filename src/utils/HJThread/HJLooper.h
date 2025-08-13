#pragma once

#include "HJMessageQueue.h"

NS_HJ_BEGIN

class HJLooper final
{
public:
	using Ptr = std::shared_ptr<HJLooper>;

	HJMessageQueue::Ptr mQueue;

	static void prepare();
	static void loop();
	static Ptr myLooper();

	HJLooper();
	bool isCurrentThread();
	void quit();

private:
	static bool loopOnce(const Ptr me);

	int mThread;
};

NS_HJ_END
