#if defined(HJ_LOG_TAG)
#	undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJPlayerContextJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJFLog.h"
#include "HJCacheEnv.h"
#include "HJEntryContext.h"

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_PLAYER_DECL(HJPlayerContextJni, sig)

extern "C"
{
	JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(playerContextInit)(JNIEnv *env, jclass thiz, jstring logDir, jint logLevel, jint logMode, jint max_size, jint max_files);
	JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(playerContextUnInit)(JNIEnv *env, jclass thiz);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(playerContextInit)(JNIEnv *env, jclass thiz, jstring logDir, jint logLevel, jint logMode, jint max_size, jint max_files)
{
	int i_err = HJ_OK;
	LOG_I("native context Init entry");
	const char* dir = HJJNIField::JStringToString(env, logDir);
    do {
        HJEntryContextPlayerInfo info;
        info.logDir = dir;
        info.logLevel = logLevel;
        info.logMode = logMode;
        info.logMaxFileSize = max_size;
        info.logMaxFileNum = max_files;


        i_err = HJEntryContext::init(HJEntryType_Player, info);
        if (i_err < 0)
        {
            HJFLoge("renderContextInit, error");
            break;
        }

        JavaVM*	h_jvm;
        env->GetJavaVM(&h_jvm);
        i_err = HJCacheEnv::Instance().init(h_jvm);
        if (i_err < 0)
        {
            LOG_E("HJCacheEnv init, error");
            break;
        }
        HJFLogi("native context Init end======:{}", i_err);
    } while (false);
    HJJNIField::JStringDestroy(env, dir, logDir);
	return i_err;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(playerContextUnInit)(JNIEnv *env, jclass thiz) {
    int i_err = HJ_OK;
    LOG_I("native context Uninit entry");
    do {
        HJFLogi("cache env uninit enter");
        HJCacheEnv::Instance().uninit();
        HJFLogi("cache env uninit end");
        HJEntryContext::unInit();

    } while (false);
    LOG_I("native context Uninit end======");
    return i_err;
}

