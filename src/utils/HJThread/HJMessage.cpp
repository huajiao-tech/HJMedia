#include "HJMessage.h"

NS_HJ_BEGIN

HJSync HJMessage::sPoolSync;
HJMessage::Ptr HJMessage::sPool{};
int HJMessage::sPoolSize{};

#define MAX_POOL_SIZE 50

HJMessage::Ptr HJMessage::obtain()
{
	{
		SYNCHRONIZED_LOCK(sPoolSync);
		if (sPool != nullptr) {
			auto m = sPool;
			sPool = m->next;
			m->next = nullptr;
			sPoolSize--;
			return m;
		}
	}

	return std::make_shared<HJMessage>();
}

void HJMessage::recycleUnchecked()
{
	what = 0;
	arg1 = 0;
	arg2 = 0;
	if (obj != nullptr) {
		obj->done();
		obj = nullptr;
	}
	when = 0;
	target = nullptr;
	callback = nullptr;

	{
		SYNCHRONIZED_LOCK(sPoolSync);
		if (sPoolSize < MAX_POOL_SIZE) {
			next = sPool;
			sPool = SHARED_FROM_THIS;
			sPoolSize++;
		}
	}
}

HJMessage::Ptr HJMessage::setWhat(int what)
{
	this->what = what;
	return SHARED_FROM_THIS;
}

void HJMessage::recycleMessagePool()
{
	SYNCHRONIZED_LOCK(sPoolSync);
	sPool = nullptr;
}

NS_HJ_END
