#pragma once

#include "HJObject.h"
#include "HJPrerequisites.h"
#include "HJSync.h"
#include "HJAny.h"

NS_HJ_BEGIN

// 用内部std::shared_mutex声明一个作用域锁lock
#define SYNCHRONIZED_SYNC_LOCK		SYNCHRONIZED_LOCK(m_sync)

#define SYNCHRONIZED_SYNC(lock)		SYNCHRONIZED(m_sync, lock)
#define SYNC_WAIT					m_sync.wait
#define SYNC_CONS_LOCK				m_sync.consLock
#define SYNC_PROD_LOCK				m_sync.prodLock

// 检查是否处于Done状态
#define CHECK_DONE_STATUS(...)		if (m_status == HJSTATUS_Done) \
										return __VA_ARGS__

#define SHARED_FROM_THIS			std::dynamic_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this())

#define MUST_HAVE_PARAMETERS			if (!i_param) return HJErrInvalidParams
#define GET_PARAMETER(TYPE, NANE)		auto NANE = i_param->getValue<TYPE>(#NANE)
#define GET_PARAMETER2(TYPE, NANE, DEFAULT)		TYPE NANE = (DEFAULT); \
												if (i_param->haveValue(#NANE)) { \
													NANE = i_param->getValue<TYPE>(#NANE); \
												}
#define MUST_GET_PARAMETER(TYPE, NANE)	GET_PARAMETER(TYPE, NANE); \
										if (!NANE) return HJErrInvalidParams
//#define GET_STRING(NANE)				const std::string& NANE = i_param->getString(#NANE)
//#define MUST_GET_STRING(NANE)			GET_STRING(NANE); \
//										if (NANE.empty()) return HJErrInvalidParams


// HJSyncObject 同步基类，提供 init/done 生命周期控制，详见 doc/HJSyncObject.md
class HJSyncObject : public HJObject
{
public:
	HJ_DEFINE_CREATE(HJSyncObject);

	HJSyncObject(const std::string& i_name = "HJSyncObject", size_t i_identify = 0)
		: HJObject(i_name, i_identify) {}
	virtual ~HJSyncObject() { done(); }

	virtual int init(HJKeyStorage::Ptr i_param = nullptr) {
		return SYNC_PROD_LOCK([this, i_param] {
			int ret = internalInit(i_param);
			if (ret == HJErrAlreadyDone) {
				return HJErrAlreadyDone;
			}
			else if (ret < 0) {
				internalRelease();
				m_status = HJSTATUS_NONE;
			}
			else if (ret == HJ_OK) {
				m_status = HJSTATUS_Inited;
				afterInit();
			}

			return ret;
		});
	}

	virtual int done() {
		auto ret = beforeDone();
		if (ret != HJ_OK) {
			return ret;
		}

		ret = SYNC_PROD_LOCK([this] {
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

	virtual HJStatus getStatus() {
		return SYNC_CONS_LOCK([this] {
			return m_status;
		});
	}

protected:
	virtual int internalInit(HJKeyStorage::Ptr i_param) {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (m_status > HJSTATUS_NONE) {
			return HJ_INITED;
		}

		return HJ_OK;
	}

	virtual void internalRelease() {}

	virtual int beforeDone() { return HJ_OK; }	// 注：不在锁内，线程不安全，可能被重复调用

	virtual void afterInit() {}	// 注：已在m_sync锁内，线程安全

	HJSync m_sync;
	HJStatus m_status{ HJSTATUS_NONE };
};

NS_HJ_END
