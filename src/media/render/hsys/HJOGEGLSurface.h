#pragma once

#include "HJPrerequisites.h"
#include "HJOGEGLCore.h"
#include "HJTransferInfo.h"
#include <functional>

NS_HJ_BEGIN

class HJOGEGLSurface
{
public:
    using HJOGEGLSurfaceFunc = std::function<int()>;
    
    HJ_DEFINE_CREATE(HJOGEGLSurface);
    HJOGEGLSurface();
    virtual ~HJOGEGLSurface();

    void setInsName(const std::string& i_insName)
    { 
        m_insName = i_insName;
    }
    const std::string &getInsName() const 
    {
        return m_insName;
    }

    void setTargetWidth(int i_width);
    void setTargetHeight(int i_height);
    int getTargetWidth() const;
    int getTargetHeight() const;

    void setEGLSurface(EGLSurface i_eglSurface);
    EGLSurface getEGLSurface() const;
    
    int getSurfaceType() const;
    void setSurfaceType(int i_type);

    void setWindow(void* i_window);
    void* getWindow() const;
    
    void addRenderIdx();
    int64_t getRenderIdx() const;
    
    void setFps(int i_fps);
    int getFps() const;
    
    void setMakeCurrentCb(HJOGEGLSurfaceFunc i_funcMakecurrent)
    {
        m_funcMakecurrent = i_funcMakecurrent;
    }
    HJOGEGLSurfaceFunc procMakeCurrentCb()
    {
        return m_funcMakecurrent;
    }
    void setSwapCb(HJOGEGLSurfaceFunc i_funcSwap)
    {
        m_funcSwap = i_funcSwap;
    }
    HJOGEGLSurfaceFunc procSwapCb()
    {
        return m_funcSwap;
    }
    
private:
    HJOGEGLSurfaceFunc m_funcMakecurrent = nullptr;
    HJOGEGLSurfaceFunc m_funcSwap = nullptr;
    
    int m_width = 0;
    int m_height = 0;
    std::string m_insName = "HJOGEGLSurface";
    int m_surfaceType = HJOGEGLSurfaceType_Default;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    void *m_window = nullptr;
    int64_t m_renderIdx = 0;
    int m_fps = 30;
};

using HJOGEGLSurfaceQueue = std::deque<HJOGEGLSurface::Ptr>;
using HJOGEGLSurfaceQueueIt = HJOGEGLSurfaceQueue::iterator;

NS_HJ_END