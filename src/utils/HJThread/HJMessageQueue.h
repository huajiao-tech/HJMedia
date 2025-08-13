#pragma once

#include "HJMessage.h"

NS_HJ_BEGIN

class HJMessageQueue final
{
public:
	using Ptr = std::shared_ptr<HJMessageQueue>;

	HJMessage::Ptr mMessages{};

	HJMessageQueue();
	~HJMessageQueue();
	HJMessage::Ptr next();
	void quit(bool safe);
	bool enqueueMessage(HJMessage::Ptr msg, uint64_t when);
	void removeMessages(HJHandlerPtr h, int what, HJSyncObject::Ptr object);

private:
	void dispose();
	void removeAllMessagesLocked();
	void removeAllFutureMessagesLocked();

	uint64_t mPtr{};
	bool mQuitting{};
	bool mBlocked{};

	HJSync m_sync;

private:
	struct MessageQueue_ravenwood {
		HJSync mPoller;
		volatile bool mPendingWake{};
	};

	static uint64_t nativeInit();
	static void nativeDestroy(uint64_t ptr);
	static void nativePollOnce(uint64_t ptr, int timeoutMillis);
	static void nativeWake(uint64_t ptr);
};

NS_HJ_END
