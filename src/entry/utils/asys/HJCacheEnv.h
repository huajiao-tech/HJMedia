#pragma once

#include "HJPrerequisites.h"

#include <jni.h>
#include <pthread.h>


NS_HJ_BEGIN

class HJCacheEnv
{
public:
	HJCacheEnv() = default;
    virtual ~HJCacheEnv() = default;

	int init(void *jvm);
	int uninit();
	void destructorEnv();
	void *getEnv();


	static HJCacheEnv& Instance(void);


	JavaVM* getJVM();
	static JNIEnv * justGetEnv();
	static JavaVM* staticGetJavaVM();

private:
	bool m_bIsCreated = false;
	pthread_key_t m_env_key;
	JavaVM*	      h_jvm = nullptr;
};

NS_HJ_END

