#include "HJAudioListenerJni.h"
#include "HJCacheEnv.h"
#include "HJError.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJAudioListenerJni::HJAudioListenerJni()
{
}

HJAudioListenerJni::~HJAudioListenerJni()
{
    if (mListener)
    {
        HJFLogi("HJAudioListenerJni destructor");
        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (env)
        {
            env->DeleteGlobalRef(mListener);
        }
        mListener = nullptr;
    }
    mObjectNotifyId = nullptr;
}

int HJAudioListenerJni::init(jobject i_javaObj)
{
    int i_err = HJ_OK;
    do
    {
        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (!env || !i_javaObj)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        mListener = env->NewGlobalRef(i_javaObj);
        if (!mListener)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        jclass cls = env->GetObjectClass(i_javaObj);
        if (!cls)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        mObjectNotifyId = env->GetMethodID(cls, "notify", "(IIIIJII)I");
        env->DeleteLocalRef(cls);
        if (!mObjectNotifyId)
        {
            i_err = HJErrInvalidParams;
            break;
        }
    } while (false);

    if (i_err != HJ_OK && mListener)
    {
        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (env)
        {
            env->DeleteGlobalRef(mListener);
        }
        mListener = nullptr;
        mObjectNotifyId = nullptr;
    }

    return i_err;
}

int HJAudioListenerJni::notify(int i_sampleRate, int i_channels, int i_sampleFmt, int i_bytesPerSample,
                               int64_t i_data, int i_nSize, int i_nFlag)
{
    int i_err = HJ_OK;
    do
    {
        if (!mListener || !mObjectNotifyId)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (!env)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        i_err = env->CallIntMethod(mListener, mObjectNotifyId,
                                   static_cast<jint>(i_sampleRate),
                                   static_cast<jint>(i_channels),
                                   static_cast<jint>(i_sampleFmt),
                                   static_cast<jint>(i_bytesPerSample),
                                   static_cast<jlong>(i_data),
                                   static_cast<jint>(i_nSize),
                                   static_cast<jint>(i_nFlag));
    } while (false);

    return i_err;
}

NS_HJ_END
