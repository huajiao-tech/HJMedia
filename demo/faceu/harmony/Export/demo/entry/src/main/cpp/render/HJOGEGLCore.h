#pragma once


#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <memory>
#include <string>

class HJOGEGLCore
{
public:
    HJOGEGLCore();
    virtual ~HJOGEGLCore();
    
    int init();
    int makeCurrent(EGLSurface i_surface);
    int swap(EGLSurface i_surface);

    int makeCurrent();
    int swap();

    int EGLSurfaceRelease(EGLSurface i_surface);
    EGLSurface EGLSurfaceCreate(void *window);

    int EGLOffScreenSurfaceCreate(int width, int height);
    EGLSurface EGLGetOffScreenSurface() const;

    void done();
    void makeNoCurrent() const;
    EGLContext getEglContext() const;
private:
    void priDone();

    std::string m_insName = "";

    EGLNativeWindowType m_eglWindow;

    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLConfig  m_eglConfig   = EGL_NO_CONFIG_KHR;

    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLSurface m_eglOffScreenSurface = EGL_NO_SURFACE;

    EGLContext m_eglContext = EGL_NO_CONTEXT;
};

