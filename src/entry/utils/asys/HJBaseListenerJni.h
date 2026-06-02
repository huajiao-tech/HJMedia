#pragma once

#include "HJPrerequisites.h"
#include <jni.h>

NS_HJ_BEGIN

class HJBaseListenerJni
{
public:
    HJ_DEFINE_CREATE(HJBaseListenerJni);
    HJBaseListenerJni();
    virtual ~HJBaseListenerJni();

    int init(jobject jListener);
    int notify(int id, int64_t ret, const std::string& info);

private:
    jobject mListener = nullptr;
    jmethodID mObjectNotifyId = nullptr;
};

NS_HJ_END


