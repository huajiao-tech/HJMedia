#if defined(HJ_LOG_TAG)
#undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJInferenceContextJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJJNIField.h"
#include "HJInferenceContextExport.h"
#include "HJFLog.h"

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_INFERENCE_DECL(HJInferenceContextJni, sig)

extern "C"
{
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(inferenceContextInit)(JNIEnv* env, jclass thiz, jstring logDir, jint logLevel, jint logMode, jint max_size, jint max_files);
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(inferenceContextUnInit)(JNIEnv* env, jclass thiz);
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(inferenceContextInit)(JNIEnv* env, jclass thiz, jstring logDir, jint logLevel, jint logMode, jint max_size, jint max_files)
{
    (void)thiz;
    int i_err = HJ_OK;
    const char* dir = HJJNIField::JStringToString(env, logDir);
    do
    {
        HJEntryContextInfo info;
        if (dir)
        {
            info.logDir = dir;
        }
        info.logLevel = logLevel;
        info.logMode = logMode;
        info.logMaxFileSize = max_size;
        info.logMaxFileNum = max_files;

        i_err = inferenceContextInit(info);
        if (i_err < 0)
        {
            HJFLoge("inferenceContextInit failed i_err:{}", i_err);
            break;
        }
    } while (false);
    HJJNIField::JStringDestroy(env, dir, logDir);
    return i_err;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(inferenceContextUnInit)(JNIEnv* env, jclass thiz)
{
    (void)env;
    (void)thiz;
    inferenceContextUnInit();
    return HJ_OK;
}

