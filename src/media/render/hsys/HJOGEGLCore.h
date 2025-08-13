#pragma once

#include "HJPrerequisites.h"


#if defined(HarmonyOS)
    #include <EGL/egl.h>
    #include <EGL/eglext.h>
    #include <GLES3/gl3.h>
#else
    typedef void *EGLConfig;
    typedef void *EGLSurface;
    typedef void *EGLContext;
    typedef void *EGLDisplay;
    typedef unsigned long  int  EGLNativeWindowType;
    #define EGL_NO_DISPLAY 0
    #define EGL_NO_CONFIG_KHR 0
    #define EGL_NO_SURFACE 0
    #define EGL_NO_CONTEXT 0
    typedef int EGLint;

#endif

NS_HJ_BEGIN

class HJOGEGLCore
{
public:
    HJ_DEFINE_CREATE(HJOGEGLCore);
    HJOGEGLCore();
    virtual ~HJOGEGLCore();

    void setInsName(const std::string& i_insName)
    { 
        m_insName = i_insName;
    }

    int init();
    //EGLSurface getEglSurface(bool i_bOffScreen);
    int makeCurrent(EGLSurface i_surface);
    int swap(EGLSurface i_surface);


    int EGLSurfaceRelease(EGLSurface i_surface);
    EGLSurface EGLSurfaceCreate(void *window);

    int EGLOffScreenSurfaceCreate(int width, int height);
    EGLSurface EGLGetOffScreenSurface() const;
    //EGLSurface EGLGetSurface() const;
    void done();
    void makeNoCurrent() const;
    EGLContext getEglContext() const;
private:
    void priDone();

    std::string m_insName = "";

    EGLNativeWindowType m_eglWindow;

    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLConfig  m_eglConfig   = EGL_NO_CONFIG_KHR;

    //EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLSurface m_eglOffScreenSurface = EGL_NO_SURFACE;

    EGLContext m_eglContext = EGL_NO_CONTEXT;
};



NS_HJ_END