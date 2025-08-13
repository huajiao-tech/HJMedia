#pragma once

#include "HJObject.h"
#include "HJPrerequisites.h"
#include "HJSync.h"
#include "HJAny.h"

NS_HJ_BEGIN

// 用内部std::recursive_mutex声明一个作用域锁lock
#define SYNCHRONIZED_SYNC_LOCK		SYNCHRONIZED_LOCK(m_sync)

#define SYNCHRONIZED_SYNC(lock)		SYNCHRONIZED(m_sync, lock)
#define SYNC_WAIT					m_sync.wait
//#define SYNC_NOTIFY_ALL				m_sync.notifyAll()
//#define SYNC_NOTIFY					m_sync.notify()
//#define SYNC_LOCK					m_sync.lock()
//#define SYNC_TRY_LOCK				m_sync.tryLock()
//#define SYNC_UNLOCK					m_sync.unlock()
#define SYNC_CONS_LOCK				m_sync.consLock
#define SYNC_PROD_LOCK				m_sync.prodLock

// 检查是否处于Done状态
#define CHECK_DONE_STATUS(...)		if (m_status == HJSTATUS_Done) \
										return __VA_ARGS__

#define SHARED_FROM_THIS			std::dynamic_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this())

#define GET_PARAMETER(TYPE, NANE)	auto NANE = (i_param != nullptr) ? i_param->getValue<TYPE>(#NANE) : (TYPE)0


class HJSyncObject : public HJObject
{
public:
	HJ_DEFINE_CREATE(HJSyncObject);

	HJSyncObject(const std::string& i_name = "HJSyncObject", size_t i_identify = -1)
		: HJObject(i_name, i_identify) {}
	virtual ~HJSyncObject() {
		HJSyncObject::done();
	}

	virtual int init(HJKeyStorage::Ptr i_param) {
		return SYNC_PROD_LOCK([=] {
			int ret = internalInit(i_param);
			if (ret == HJ_OK) {
				m_status = HJSTATUS_Inited;
				afterInit();
			}

			return ret;
		});
	}

	virtual int done() {
		beforeDone();

		int ret = SYNC_PROD_LOCK([=] {
			CHECK_DONE_STATUS(HJErrAlreadyDone);
			m_status = HJSTATUS_Done;
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			return ret;
		}

		internalRelease();
		return HJ_OK;
	}

protected:
	virtual int internalInit(HJKeyStorage::Ptr i_param) {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (m_status > HJSTATUS_NONE && m_status < HJSTATUS_Exception) {
			return HJErrAlreadyInited;
		}
		if (m_status == HJSTATUS_Exception) {
			internalRelease();
		}

		m_status = HJSTATUS_Initializing;
		return HJ_OK;
	}

	virtual void internalRelease() {
		if (m_status != HJSTATUS_Done) {
			m_status = HJSTATUS_Released;
		}
	}

	virtual void afterInit() { }	// 注：在m_sync锁内
	virtual void beforeDone() { }	// 注：不在m_sync锁内

	HJSync m_sync;
	HJStatus m_status{ HJSTATUS_NONE };
};

NS_HJ_END
