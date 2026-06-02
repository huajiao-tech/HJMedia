#include "HJBaseListenerJni.h"
#include "HJError.h"
#include "HJCacheEnv.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJBaseListenerJni::HJBaseListenerJni()
{

}
HJBaseListenerJni::~HJBaseListenerJni()
{
    if (mListener)
    {
        HJFLogi("HJBaseListenerJni destructor");
        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (env)
        {
            HJFLogi("HJBaseListenerJni DeleteGlobalRef");
            env->DeleteGlobalRef(mListener);
        }
        mListener = nullptr;
    }
    mObjectNotifyId = nullptr;
    HJFLogi("~HJBaseListenerJni end");
}
int HJBaseListenerJni::init(jobject jListener)
{
    int i_err = HJ_OK;
    do
    {
        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (!env) {
            i_err = HJErrInvalidParams;
            break;
        }

        mListener = env->NewGlobalRef(jListener);
        if (!mListener) {
            i_err = HJErrInvalidParams;
            break;
        }
        jclass cls = env->GetObjectClass(jListener);
        if (!cls) {
            i_err = HJErrInvalidParams;
            break;
        }
        mObjectNotifyId = env->GetMethodID(cls, "onNotify", "(IJLjava/lang/String;)I");
        env->DeleteLocalRef(cls);
        if (!mObjectNotifyId) {
            i_err = HJErrInvalidParams;
            break;
        }
    } while (false);
    return i_err;
}

int HJBaseListenerJni::notify(int id, int64_t ret, const std::string& info)
{
    int i_err = HJ_OK;
    do {
        if (!mListener) {
            i_err = HJErrInvalidParams;
            break;
        }

        JNIEnv* env = HJCacheEnv::justGetEnv();
        if (!env) {
            i_err = HJErrInvalidParams;
            break;
        }

        jstring jInfo;
        if (info.empty())
        {
            jInfo = env->NewStringUTF("");
        } else
        {
            jInfo = env->NewStringUTF(info.c_str());
        }
        i_err = env->CallIntMethod(mListener, mObjectNotifyId, id, ret, jInfo);
        env->DeleteLocalRef(jInfo);
    } while (false);
    return i_err;

}

NS_HJ_END

