#include "HJCacheEnv.h"
#include "HJFLog.h"
#include "HJError.h"

NS_HJ_BEGIN

void HJCacheEnv::destructorEnv()
{
	if (h_jvm)
	{
		h_jvm->DetachCurrentThread();
	}
}

void *HJCacheEnv::getEnv()
{
	void* env = nullptr;
	do 
	{
		if (!h_jvm)
		{
			HJFLoge("jvm is NULL");
			break;
		}

		JNIEnv *p_env = nullptr;
		if ((JNI_OK == h_jvm->GetEnv((void **)&p_env, JNI_VERSION_1_4)) && p_env)
		{
			env = (void *)p_env;
			break;
		}

		if (h_jvm->AttachCurrentThread(&p_env, nullptr) < 0)
		{
			break;
		}
		env = (void*)p_env;

		int ret = pthread_setspecific(m_env_key, this);
		if (ret != 0)
		{
			env = nullptr;
            HJFLoge("setspecific error");
			break;
		}
	} while (false);
	return env;
}

static void pri_destructor(void *arg)
{
	HJCacheEnv *the = (HJCacheEnv *)arg;
	if (the)
	{
        HJFLogi("HJCacheEnv destructor thread enter");
		the->destructorEnv();
        HJFLogi("HJCacheEnv destructor thread end");
	}
}
HJCacheEnv& HJCacheEnv::Instance(void)
{
    static HJCacheEnv instance;
    return instance;
}
int HJCacheEnv::init(void *jvm)
{
	int i_err = HJ_OK;
	do 
	{
		int ret = pthread_key_create(&m_env_key, pri_destructor);
		if (ret != 0)
		{
			i_err = HJErrInvalidParams;
			break;
		}
		h_jvm = (JavaVM*)jvm;
		m_bIsCreated = true;
	} while (false);
	return i_err;
}
int HJCacheEnv::uninit()
{
	int i_err = HJ_OK;
	do
	{
		if (m_bIsCreated)
		{
			pthread_key_delete(m_env_key);
			m_bIsCreated = false;
		}
	} while (false);
	return i_err;
}

JNIEnv * HJCacheEnv::justGetEnv()
{
	return (JNIEnv*)Instance().getEnv();
}
JavaVM* HJCacheEnv::staticGetJavaVM()
{
	return Instance().getJVM();
}
JavaVM* HJCacheEnv::getJVM()
{
	return h_jvm;
}

NS_HJ_END
