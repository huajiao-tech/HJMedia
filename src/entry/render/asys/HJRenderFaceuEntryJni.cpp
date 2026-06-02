#if defined(HJ_LOG_TAG)
#	undef HJ_LOG_TAG
#endif
#define HJ_LOG_TAG "HJRenderFaceuEntryJni"

#include "HJMediaJni.h"
#include "HJPrerequisites.h"
#include "HJError.h"
#include "HJRenderFaceuExport.h"
#include "HJFLog.h"
#include "HJFacePointsMadeup.h"
#include "HJFacePointMgr.h"
#include "HJComEvent.h"
#include "HJBaseListenerJni.h"
#include <vector>
#include "HJReflCpp.h"
#include "HJReflCppJNIField.h"

REFL_AUTO_SIMPLE(HJFacePoint, x, y)
REFL_AUTO_SIMPLE(HJFaceRect, x, y, w, h)
REFL_AUTO_SIMPLE(HJSingleFaceInfo, rect, points)
REFL_AUTO_SIMPLE(HJFacePointInfo, faceCount, width, height)


typedef struct HJFacePointInfoEx
{
    std::vector<HJSingleFaceInfo> faces;
    int faceCount = 0;
    int width = 0;
    int height = 0;
} HJFacePointInfoEx;
REFL_AUTO_SIMPLE(HJFacePointInfoEx, faces, faceCount, width, height)

NS_HJ_USING;

#undef HJ_JNIFUN_MEDIA_DECL
#define HJ_JNIFUN_MEDIA_DECL(sig) HJ_MEDIA_RENDER_DECL(HJRenderFaceuEntryJni, sig)

extern "C"
{
    JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuInit)(JNIEnv *env, jobject thiz, jobject i_listener);
    JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuAdd)(JNIEnv *env, jobject thiz, jstring uniqueKey,jstring faceuUrl, jboolean i_bDebugPoints);
    JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuRemove)(JNIEnv *env, jobject thiz, jstring uniqueKey);
    JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuRemoveAll)(JNIEnv *env, jobject thiz);
    JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuRender)(JNIEnv *env, jobject thiz, jobject i_facePointInfoObj);
    JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuDone)(JNIEnv *env, jobject thiz);
}
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuInit)(JNIEnv *env, jobject thiz, jobject i_listener)
{
    int i_err = HJ_OK;
    do
    {
        HJBaseListenerJni::Ptr jListener = HJBaseListenerJni::Create();
        i_err = jListener->init(i_listener);
        if (i_err < 0)
        {
            HJFLoge("jListener init failed i_err:{}", i_err);
            break;
        }
        HJFaceuListener listener = [jListener](const std::string& i_uniqueKey, const HJNotification::Ptr ntf)
	    {
            int type = HJ_FACEU_NOTIFY_UNKNOWN;
            std::string msg = ntf->getMsg();
            switch (ntf->getID())
            {
            case HJVIDEORENDERGRAPH_EVENT_FACEU_COMPLETE:
                type = HJ_FACEU_NOTIFY_COMPLETE;
                break;
            }
            HJFLogi("faceuNotify uniqueKey:{} type:{} msg:{}", i_uniqueKey, type, msg);

            return jListener->notify(type, 0, i_uniqueKey);
	    };

        void *handle = faceuInitTrans(listener);
        if (!handle)
        {
            HJFLoge("faceuInit failed i_err:{}", i_err);
            i_err = HJErrFatal;
            break;
        }
        HJJNIField::SetLongField(env, thiz, (int64_t)handle, HJJNIField::m_handleName);
    } while (false);
    return i_err;
}
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuAdd)(JNIEnv *env, jobject thiz, jstring uniqueKey,jstring faceuUrl, jboolean i_bDebugPoints)
{
    int i_err = HJ_OK;
    const char* key = HJJNIField::JStringToString(env, uniqueKey);
    const char* url = HJJNIField::JStringToString(env, faceuUrl);
    do {
        HJFLogi("nativeFaceuAdd key:{} url:{} i_bDebugPoints:{}", key, url, i_bDebugPoints);

        void *handle = (void *)HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName);
        if (!handle)
        {
            HJFLoge("mHandle is null i_err:{}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }

        i_err = faceuAdd(handle, key, url, i_bDebugPoints);
        if (i_err < 0)
        {
            HJFLoge("faceuAdd failed i_err:{}", i_err);
            break;
        }
    } while (false);
    HJJNIField::JStringDestroy(env, url, faceuUrl);
    HJJNIField::JStringDestroy(env, key, uniqueKey);
    return i_err;

}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuRemove)(JNIEnv *env, jobject thiz, jstring uniqueKey)
{
    int i_err = HJ_OK;
    const char* key = HJJNIField::JStringToString(env, uniqueKey);
    do
    {
        HJFLogi("nativeFaceuRemove key:{}", key);
        void *handle = (void *)HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName);
        if (!handle)
        {
            HJFLoge("mHandle is null i_err:{}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }
        i_err = faceuRemove(handle, key);
        if (i_err < 0)
        {
            HJFLoge("faceuRemove failed i_err:{}", i_err);
            break;
        }
    } while (false);
    HJJNIField::JStringDestroy(env, key, uniqueKey);
    return i_err;
}
JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuRemoveAll)(JNIEnv *env, jobject thiz)
{
    int i_err = HJ_OK;
    do {
        HJFLogi("nativeFaceuRemoveAll");
        void *handle = (void *) HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName);
        if (!handle) {
            HJFLoge("mHandle is null i_err:{}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }
        i_err = faceuRemoveAll(handle);
        if (i_err < 0) {
            HJFLoge("faceuRemoveAll failed i_err:{}", i_err);
            break;
        }
    } while (false);
    return i_err;
}
static int priFacePointInfoMap(JNIEnv *env, jobject i_facePointInfoObj, HJFacePointInfo& o_faceInfo, std::vector<HJSingleFaceInfo>& o_faces)
{
    int i_err = HJ_OK;
    if (!i_facePointInfoObj)
    {
        return HJErrInvalidParams;
    }

    jclass clsInfo = env->GetObjectClass(i_facePointInfoObj);
    if (!clsInfo)
    {
        return HJErrInvalidParams;
    }

    jfieldID fidWidth = env->GetFieldID(clsInfo, "width", "I");
    jfieldID fidHeight = env->GetFieldID(clsInfo, "height", "I");
    jfieldID fidFaceCount = env->GetFieldID(clsInfo, "faceCount", "I");
    jfieldID fidFaces = env->GetFieldID(clsInfo, "faces", "Ljava/util/Vector;");
    if (!fidWidth || !fidHeight || !fidFaceCount || !fidFaces)
    {
        env->DeleteLocalRef(clsInfo);
        return HJErrInvalidParams;
    }

    o_faceInfo.width = env->GetIntField(i_facePointInfoObj, fidWidth);
    o_faceInfo.height = env->GetIntField(i_facePointInfoObj, fidHeight);
    int faceCount = env->GetIntField(i_facePointInfoObj, fidFaceCount);

    jobject facesVec = env->GetObjectField(i_facePointInfoObj, fidFaces);
    if (!facesVec)
    {
        env->DeleteLocalRef(clsInfo);
        if (faceCount > 0)
        {
            return HJErrInvalidParams;
        }
        o_faceInfo.faces = nullptr;
        o_faceInfo.faceCount = 0;
        return i_err;
    }

    jclass clsVector = env->GetObjectClass(facesVec);
    jmethodID midSize = env->GetMethodID(clsVector, "size", "()I");
    jmethodID midGet = env->GetMethodID(clsVector, "get", "(I)Ljava/lang/Object;");
    if (!midSize || !midGet)
    {
        env->DeleteLocalRef(clsVector);
        env->DeleteLocalRef(facesVec);
        env->DeleteLocalRef(clsInfo);
        return HJErrInvalidParams;
    }

    jint vecSize = env->CallIntMethod(facesVec, midSize);
    if (faceCount <= 0)
    {
        faceCount = vecSize;
    }
    if (vecSize < faceCount)
    {
        HJFLoge("faces size:{} less than faceCount:{}", vecSize, faceCount);
        faceCount = vecSize;
    }

    if (faceCount <= 0)
    {
        o_faceInfo.faces = nullptr;
        o_faceInfo.faceCount = 0;
        env->DeleteLocalRef(clsVector);
        env->DeleteLocalRef(facesVec);
        env->DeleteLocalRef(clsInfo);
        return i_err;
    }

    o_faces.resize(faceCount);
    for (int i = 0; i < faceCount; i++)
    {
        jobject faceObj = env->CallObjectMethod(facesVec, midGet, i);
        if (!faceObj)
        {
            continue;
        }
        jclass clsFace = env->GetObjectClass(faceObj);
        if (!clsFace)
        {
            env->DeleteLocalRef(faceObj);
            continue;
        }

        jfieldID fidRect = nullptr;
        jfieldID fidPoints = env->GetFieldID(clsFace, "points", "Ljava/util/Vector;");
        if (!fidPoints)
        {
            env->DeleteLocalRef(clsFace);
            env->DeleteLocalRef(faceObj);
            i_err = HJErrInvalidParams;
            break;
        }

        jobject rectObj = nullptr;
        jclass clsClass = env->FindClass("java/lang/Class");
        jclass clsField = env->FindClass("java/lang/reflect/Field");
        if (clsClass && clsField)
        {
            jmethodID midGetField = env->GetMethodID(clsClass, "getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
            jmethodID midFieldGet = env->GetMethodID(clsField, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
            if (midGetField && midFieldGet)
            {
                jstring rectName = env->NewStringUTF("rect");
                jobject fieldObj = env->CallObjectMethod(clsFace, midGetField, rectName);
                env->DeleteLocalRef(rectName);
                if (fieldObj)
                {
                    rectObj = env->CallObjectMethod(fieldObj, midFieldGet, faceObj);
                    env->DeleteLocalRef(fieldObj);
                }
            }
        }
        if (clsClass)
        {
            env->DeleteLocalRef(clsClass);
        }
        if (clsField)
        {
            env->DeleteLocalRef(clsField);
        }

        if (rectObj)
        {
            jclass clsRect = env->GetObjectClass(rectObj);
            if (clsRect)
            {
                jfieldID fidRX = env->GetFieldID(clsRect, "x", "F");
                jfieldID fidRY = env->GetFieldID(clsRect, "y", "F");
                jfieldID fidRW = env->GetFieldID(clsRect, "w", "F");
                jfieldID fidRH = env->GetFieldID(clsRect, "h", "F");
                if (fidRX && fidRY && fidRW && fidRH)
                {
                    o_faces[i].rect.x = env->GetFloatField(rectObj, fidRX);
                    o_faces[i].rect.y = env->GetFloatField(rectObj, fidRY);
                    o_faces[i].rect.w = env->GetFloatField(rectObj, fidRW);
                    o_faces[i].rect.h = env->GetFloatField(rectObj, fidRH);
                }
                else
                {
                    i_err = HJErrInvalidParams;
                }
                env->DeleteLocalRef(clsRect);
            }
            env->DeleteLocalRef(rectObj);
        }
        else
        {
            i_err = HJErrInvalidParams;
        }

        jobject pointsVec = env->GetObjectField(faceObj, fidPoints);
        if (pointsVec)
        {
            jclass clsPointsVec = env->GetObjectClass(pointsVec);
            jmethodID midPointsSize = env->GetMethodID(clsPointsVec, "size", "()I");
            jmethodID midPointsGet = env->GetMethodID(clsPointsVec, "get", "(I)Ljava/lang/Object;");
            if (midPointsSize && midPointsGet)
            {
                jint pointsSize = env->CallIntMethod(pointsVec, midPointsSize);
                if (pointsSize < 5)
                {
                    HJFLoge("points size:{} less than 5", pointsSize);
                    i_err = HJErrInvalidParams;
                }
                else
                {
                    for (int j = 0; j < 5; j++)
                    {
                        jobject ptObj = env->CallObjectMethod(pointsVec, midPointsGet, j);
                        if (!ptObj)
                        {
                            continue;
                        }
                        jclass clsPt = env->GetObjectClass(ptObj);
                        if (clsPt)
                        {
                            jfieldID fidPX = env->GetFieldID(clsPt, "x", "F");
                            jfieldID fidPY = env->GetFieldID(clsPt, "y", "F");
                            if (fidPX && fidPY)
                            {
                                o_faces[i].points[j].x = env->GetFloatField(ptObj, fidPX);
                                o_faces[i].points[j].y = env->GetFloatField(ptObj, fidPY);
                            }
                            else
                            {
                                i_err = HJErrInvalidParams;
                            }
                            env->DeleteLocalRef(clsPt);
                        }
                        env->DeleteLocalRef(ptObj);
                    }
                }
            }
            else
            {
                i_err = HJErrInvalidParams;
            }
            env->DeleteLocalRef(clsPointsVec);
            env->DeleteLocalRef(pointsVec);
        }
        else
        {
            i_err = HJErrInvalidParams;
        }

        env->DeleteLocalRef(clsFace);
        env->DeleteLocalRef(faceObj);

        if (i_err < 0)
        {
            break;
        }
    }

    if (i_err >= 0)
    {
        o_faceInfo.faces = o_faces.data();
        o_faceInfo.faceCount = faceCount;
    }

    env->DeleteLocalRef(clsVector);
    env->DeleteLocalRef(facesVec);
    env->DeleteLocalRef(clsInfo);
    return i_err;
}

JNIEXPORT jint JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuRender)(JNIEnv *env, jobject thiz, jobject i_facePointInfoObj)
{
    int i_err = HJ_OK;
    do
    {
        //HJFLogi("nativeFaceuRender i_width:{} i_height:{}", i_width, i_height);
        void *handle = (void *)HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName);
        if (!handle)
        {
            HJFLoge("mHandle is null i_err:{}", i_err);
            i_err = HJErrInvalidParams;
            break;
        }

        HJFacePointInfoEx faceInfoEx = {};
        i_err = HJReflCppJNIField::deserial(env, i_facePointInfoObj, faceInfoEx);
        if (i_err < 0)
        {
            HJFLoge("deserial i_err:{}", i_err);
            break;
        }
        HJFacePointInfo faceInfo = {};
        faceInfo.width = faceInfoEx.width;
        faceInfo.height = faceInfoEx.height;
        faceInfo.faceCount = faceInfoEx.faceCount;
        if (faceInfo.faceCount > 0)
        {
            faceInfo.faces = faceInfoEx.faces.data();
        }
#if 0
        std::vector<HJSingleFaceInfo> faces;
        i_err = priFacePointInfoMap(env, i_facePointInfoObj, faceInfo, faces);
        if (i_err < 0)
        {
            HJFLoge("priFacePointInfoMap failed i_err:{}", i_err);
            break;
        }
#endif

        unsigned char* pRGBA = nullptr;
        i_err = faceuRender(handle, &faceInfo, pRGBA);
        if (i_err < 0)
        {
            HJFLoge("faceuRender failed i_err:{}", i_err);
            break;
        }
    } while (false);
    return i_err;
}
JNIEXPORT void JNICALL HJ_JNIFUN_MEDIA_DECL(nativeFaceuDone)(JNIEnv *env, jobject thiz)
{
    do
    {
        HJFLogi("nativeFaceuDone");
        void *handle = (void *)HJJNIField::GetLongField(env, thiz, HJJNIField::m_handleName);
        if (!handle)
        {
            HJFLoge("mHandle is null");
            break;
        }
        faceuDone(handle);
    } while (false);
}
