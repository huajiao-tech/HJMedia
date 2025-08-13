#pragma once

#include "HJSyncObject.h"

NS_HJ_BEGIN

class HJHandler;
using HJHandlerPtr = std::shared_ptr<HJHandler>;

class HJMessage final : public std::enable_shared_from_this<HJMessage>
{
public:
	using Ptr = std::shared_ptr<HJMessage>;

	int what{};
	int arg1{};
	int arg2{};
	HJSyncObject::Ptr obj{};
	uint64_t when{};
	HJHandlerPtr target{};
	HJRunnable callback{};
	Ptr next{};

	static Ptr obtain();
	static Ptr obtain(HJHandlerPtr h);
	void recycleUnchecked();
	Ptr setWhat(int what);
	HJMessage() = default;

public:
	static void recycleMessagePool();

private:
	static HJSync sPoolSync;
	static Ptr sPool;
	static int sPoolSize;
};

NS_HJ_END
