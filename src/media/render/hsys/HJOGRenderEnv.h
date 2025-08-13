#pragma once

#include "HJPrerequisites.h"
#include "HJTransferInfo.h"
#include "HJOGRenderWindowBridge.h"
#include <deque>
#include "HJOGEGLSurface.h"
#if defined(HarmonyOS)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <native_window/graphic_error_code.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <native_buffer/native_buffer.h>
#endif

NS_HJ_BEGIN
 
class HJThreadPool;
class HJThreadTimer;
class HJOGEGLCore;

typedef enum HJOGRenderEnvCbMsg{
    OGRenderEnvMsg_NeedSurface = 0,
    OGRenderEnvMsg_Error = 1,
} EGLSurfaceFlag;

class HJOGRenderEnv
{
public:
    using HJOGRenderEnvCb = std::function<void(int i_msgType, int i_ret, const std::string& i_msg)>;

    HJ_DEFINE_CREATE(HJOGRenderEnv);
    HJOGRenderEnv();
    virtual ~HJOGRenderEnv();

    void setInsName(const std::string& i_insName);
    void setOGRenderEnvCb(HJOGRenderEnvCb i_cb);
    
    int activeInit();
    void activeDone();
    HJOGRenderWindowBridge::Ptr activeRenderWindowBridgeAcquire(const std::string &i_renderMode);
    int activeRenderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge);
    int activeEglSurfaceProc(const std::string &i_renderTargetInfo);
//    int activeEglSurfaceAdd(const std::string &i_renderTargetInfo);
//    int activeEglSurfaceChanged(const std::string& i_renderTargetInfo);
//    int activeEglSurfaceRemove(const std::string& i_renderTargetInfo);
    std::shared_ptr<HJOGEGLCore> activeGetEglCore();
    OGRenderWindowBridgeQueue& activeGetRenderWindowBridgeQueue();
    OGEGLSurfaceQueue& activeGetEglSurfaceQueue();
    
    int init(int i_renderFps = 30);
    void done();
    HJOGRenderWindowBridge::Ptr renderWindowBridgeAcquire(const std::string &i_renderMode);
    int renderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge);
    int eglSurfaceAdd(const std::string &i_renderTargetInfo);
    int eglSurfaceChanged(const std::string& i_renderTargetInfo);
    int eglSurfaceRemove(const std::string& i_renderTargetInfo);
    
    
    
private:
    bool priIsEglSurfaceHaveWindow(void *i_window);
    int priRenderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge);
    int priCreateBridge(const std::string &i_renderMode, HJOGRenderWindowBridge::Ptr &bridge);
    std::string priGetStateInfo(int i_state);
    int priCoreInit();    
    void priCoreDone();
    void priRender();
    int priDraw();
    int priDrawEveryTarget(EGLSurface i_eglSurface, int i_targetWidth, int i_targetHeight);
    float m_matrix[16];
    std::string m_insName = "";
    std::shared_ptr<HJThreadPool> m_threadPool = nullptr;
    std::shared_ptr<HJThreadTimer> m_timer = nullptr;
    int m_renderFps = 30;
    std::shared_ptr<HJOGEGLCore> m_eglCore = nullptr;
    HJOGRenderEnvCb m_cb = nullptr;
    int priUpdateEglSurface(const std::string& i_renderTargetInfo);

//#if defined(HarmonyOS)
//    
//    std::shared_ptr<SLGLSimpleDraw> m_draw = nullptr;
//	GLuint m_texture = 0;
//    OH_NativeImage* m_nativeImage = nullptr;
//    OHNativeWindow *m_nativeWindow = nullptr;
//#endif
    static int s_asyncTastkClearId;

    int m_srcWidth = 720;
    int m_srcHeight = 1280;
    
    int m_width = 0;
    int m_height = 0;

    OGRenderWindowBridgeQueue m_renderWindowBridgeQueue;
    OGEGLSurfaceQueue m_eglSurfaceQueue;
};



NS_HJ_END