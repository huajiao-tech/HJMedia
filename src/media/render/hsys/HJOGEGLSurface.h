#pragma once

#include "HJPrerequisites.h"
#include "HJOGEGLCore.h"

NS_HJ_BEGIN

typedef enum HJOGEGLSurfaceType
{
    HJOGEGLSurfaceType_Default   = 0,
    HJOGEGLSurfaceType_Encoder   = 1,
} HJOGEGLSurfaceType;

class HJOGEGLSurface
{
public:
    HJ_DEFINE_CREATE(HJOGEGLSurface);
    HJOGEGLSurface();
    virtual ~HJOGEGLSurface();

    void setInsName(const std::string& i_insName)
    { 
        m_insName = i_insName;
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
    
private:
    int m_width = 0;
    int m_height = 0;
    std::string m_insName = "HJOGEGLSurface";
    int m_surfaceType = HJOGEGLSurfaceType_Default;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    void *m_window = nullptr;
    int64_t m_renderIdx = 0;
    int m_fps = 30;
};

using OGEGLSurfaceQueue = std::deque<HJOGEGLSurface::Ptr>;
using OGEGLSurfaceQueueIt = OGEGLSurfaceQueue::iterator;

NS_HJ_END