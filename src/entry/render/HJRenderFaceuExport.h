#pragma once

#include "HJCommonInterface.h"
#include "HJExport.h"


#if defined(ANDROID_LIB)
    #include "HJRenderFaceuNotify.h"
#endif

typedef void(*HJFaceuCallback)(const char*, int);

typedef struct HJFacePoint {
    float x;
    float y;
} HJFacePoint;

typedef struct HJFaceRect {
    float x;
    float y;
    float w;
    float h;
} HJFaceRect;

typedef struct HJSingleFaceInfo {
    HJFaceRect rect;
    HJFacePoint points[5];
} HJSingleFaceInfo;

typedef struct HJFacePointInfo
{
    HJSingleFaceInfo *faces = NULL;  
    int faceCount = 0;
    int width = 0;
    int height = 0;
} HJFacePointInfo;

typedef enum HJRenderFaceuNotifyType
{
    HJ_FACEU_NOTIFY_UNKNOWN = -1,
    HJ_FACEU_NOTIFY_COMPLETE = 1,
} HJRenderFaceuNotifyType;

#if defined(ANDROID_LIB)
    HJExport void* faceuInitTrans(HJFaceuListener i_listener);
#endif

HJExport void* faceuInit(HJFaceuCallback i_callback, bool i_bUseEnv = false);
HJExport int faceuAdd(void *i_handle, const char *i_uniqueKey, const char *i_faceuUrl, bool i_bDebugPoints);
HJExport int faceuRemove(void* i_handle, const char *i_uniqueKey);
HJExport int faceuRemoveAll(void* i_handle);
//HJExport int faceuRender(void* i_handle, int i_width, int i_height, const HJPoint2D points[9], int i_nPointsCnt, bool i_bContainFace, unsigned char*& o_pRGBA);
HJExport int faceuRender(void* i_handle, HJFacePointInfo* i_faceInfo, unsigned char*& o_pRGBA);
HJExport void faceuDone(void* i_handle);




