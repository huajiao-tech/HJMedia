#pragma once

#include "HJPrerequisites.h"
#include "HJTransferInfo.h"
#include "HJOGEGLSurface.h"
#include "HJComUtils.h"

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

class HJOGEGLCore;
class HJOGRenderWindowBridge;
using HJOGRenderWindowBridgeQueue = std::deque<std::shared_ptr<HJOGRenderWindowBridge>>;
using HJOGRenderWindowBridgeQueueIt = HJOGRenderWindowBridgeQueue::iterator;
class HJOGEGLSurface;

typedef enum HJOGRenderEnvCbMsg{
    OGRenderEnvMsg_NeedSurface = 0,
    OGRenderEnvMsg_Error = 1,
} EGLSurfaceFlag;

class HJOGRenderEnv
{
public:
    using HJOGRenderEnvCb = std::function<void(int i_msgType, int i_ret, const std::string& i_msg)>;
    using HJOGEGLSurfaceCb = std::function<int(const std::shared_ptr<HJOGEGLSurface>& i_eglSurface, int i_targetStyle)>; 
    HJ_DEFINE_CREATE(HJOGRenderEnv);
    HJOGRenderEnv();
    virtual ~HJOGRenderEnv();

    void setInsName(const std::string& i_insName);
    void setOGRenderEnvCb(HJOGRenderEnvCb i_cb);
    
    int init();
    void done();
    int foreachRender(int i_graphFps, HJRenderEnvUpdate i_update, HJRenderEnvDraw i_draw);
    int procEglSurface(const std::string &i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface>& o_eglSurface);
    bool isNeedEglSurface();
    
    //std::shared_ptr<HJOGRenderWindowBridge> acquireRenderWindowBridge();
    //int releaseRenderWindowBridge(std::shared_ptr<HJOGRenderWindowBridge> i_bridge);
    //const std::shared_ptr<HJOGEGLCore>& getEglCore();
    //HJOGEGLSurfaceQueue& getEglSurfaceQueue();
    //HJOGRenderWindowBridgeQueue& getRenderWindowBridgeQueue();
    const HJOGEGLSurfaceQueue& getEglSurfaceQueue()
    {
        return m_eglSurfaceQueue;
    }
    int testMakeOffCurrent();
    int testMakeCurrent(EGLSurface i_surface);
    int testSwap(EGLSurface i_surface);
    
    
//    int assignMakeCurrent(void *i_window);
//    int assignSwap(void *i_window);
    
private:
    int priDrawEveryTarget(const HJOGEGLSurface::Ptr& i_surface, HJRenderEnvDraw i_draw);
    int priUpdateEglSurface(const std::string& i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface>& o_eglSurface);
    bool priIsEglSurfaceHaveWindow(void *i_window);
//    int priRenderWindowBridgeRelease(std::shared_ptr<HJOGRenderWindowBridge> i_bridge);
//    int priCreateBridge(std::shared_ptr<HJOGRenderWindowBridge> &bridge);
    std::string priGetStateInfo(int i_state);
    int priCoreInit();    
    void priCoreDone();
    
    float m_matrix[16];
    std::string m_insName = "";
    std::shared_ptr<HJOGEGLCore> m_eglCore = nullptr;
    HJOGRenderEnvCb m_cb = nullptr;
    
        
    //HJOGRenderWindowBridgeQueue m_renderWindowBridgeQueue;
    HJOGEGLSurfaceQueue m_eglSurfaceQueue;
    
    int64_t m_renderIdx = 0;
};



NS_HJ_END