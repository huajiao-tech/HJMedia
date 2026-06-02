#pragma once

#include "HJPrerequisites.h"
#include <jni.h>

NS_HJ_BEGIN

typedef enum HJAudioListenerFlag
{
    HJAudioListenerFlag_PCM       = 0x01,
    HJAudioListenerFlag_AACHEADER = 0x02,
    HJAudioListenerFlag_AACDat    = 0x04,
} HJAudioListenerFlag;

class HJAudioListenerJni
{
public:
    HJ_DEFINE_CREATE(HJAudioListenerJni)

    HJAudioListenerJni();
    virtual ~HJAudioListenerJni();


    int init(jobject i_javaObj);
    int notify(int i_sampleRate, int i_channels, int i_sampleFmt, int i_bytesPerSample, int64_t i_data, int i_nSize, int i_nFlag);

private:
    jobject mListener = nullptr;
    jmethodID mObjectNotifyId = nullptr;
};

NS_HJ_END



