#include "HJOGEGLCore.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#if defined(HarmonyOS)  
#define CASE_EGL_STR(value) \
    case value:             \
        return #value
const char *GetEglErrorString()
{
    EGLint error = eglGetError();
    switch (error) {
        CASE_EGL_STR(EGL_SUCCESS);
        CASE_EGL_STR(EGL_NOT_INITIALIZED);
        CASE_EGL_STR(EGL_BAD_ACCESS);
        CASE_EGL_STR(EGL_BAD_ALLOC);
        CASE_EGL_STR(EGL_BAD_ATTRIBUTE);
        CASE_EGL_STR(EGL_BAD_CONTEXT);
        CASE_EGL_STR(EGL_BAD_CONFIG);
        CASE_EGL_STR(EGL_BAD_CURRENT_SURFACE);
        CASE_EGL_STR(EGL_BAD_DISPLAY);
        CASE_EGL_STR(EGL_BAD_SURFACE);
        CASE_EGL_STR(EGL_BAD_MATCH);
        CASE_EGL_STR(EGL_BAD_PARAMETER);
        CASE_EGL_STR(EGL_BAD_NATIVE_PIXMAP);
        CASE_EGL_STR(EGL_BAD_NATIVE_WINDOW);
        CASE_EGL_STR(EGL_CONTEXT_LOST);
        default:
            return "Unknow Error";
    }
}
#undef CASE_EGL_STR
#endif

HJOGEGLCore::HJOGEGLCore()
{

}
HJOGEGLCore::~HJOGEGLCore()
{

}

int HJOGEGLCore::init()
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)        
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_eglDisplay == EGL_NO_DISPLAY)
        {
            HJFLoge("{} eglGetDisplay: unable to get EGL display:{}", m_insName, GetEglErrorString());
            i_err = -1;
            break;
        }

        EGLint majorVersion;
        EGLint minorVersion;
        if (!eglInitialize(m_eglDisplay, &majorVersion, &minorVersion)) 
        {
            HJFLoge("{} eglInitialize: unable to initialize EGL display:{}", m_insName, GetEglErrorString());
            i_err = -1;
            break;
        }

        const EGLint ATTRIB_LIST[] = {
            // Key,value.
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, 
            EGL_RED_SIZE, 8, 
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8, 
            EGL_ALPHA_SIZE, 8, 
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            // End.
            EGL_NONE};

        // Select configuration.
        const EGLint maxConfigSize = 1;
        EGLint numConfigs;
        if (!eglChooseConfig(m_eglDisplay, ATTRIB_LIST, &m_eglConfig, maxConfigSize, &numConfigs)) 
        {
            HJFLoge("{} eglChooseConfig: unable to choose configs:{}", m_insName, GetEglErrorString());
            i_err = -1;
            break;
        }
        const EGLint CONTEXT_ATTRIBS[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, CONTEXT_ATTRIBS);
        if (m_eglContext == EGL_NO_CONTEXT)
        {
            HJFLoge("{} eglCreateContext error:{}", m_insName, GetEglErrorString());
            i_err = -1;
            break;
        }
#endif
    } while (false);
    return i_err;
}
//EGLSurface HJOGEGLCore::getEglSurface(bool i_bOffScreen)
//{
//    return i_bOffScreen ? m_eglOffScreenSurface : m_eglSurface;
//}
int HJOGEGLCore::makeCurrent(EGLSurface i_surface)
{
    int i_err = 0;
    do
    {
 #if defined(HarmonyOS)            
        if (m_eglDisplay == EGL_NO_DISPLAY)
        {
            i_err = -1;
            HJFLoge("{} makeCurrent: no display", m_insName);
            break;
        }
        if (m_eglContext == EGL_NO_CONTEXT)
        {   
            i_err = -1;
            HJFLoge("{} makeCurrent: no context", m_insName);
            break;
        }
        if (!eglMakeCurrent(m_eglDisplay, i_surface, i_surface, m_eglContext))
        {
            i_err = -1;
            HJFLoge("{} makeCurrent: error {}", m_insName, GetEglErrorString());
            break;
        }
#endif     
    } while (false);
    return i_err;
}
int HJOGEGLCore::swap(EGLSurface i_surface)
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)            
        if (m_eglDisplay == EGL_NO_DISPLAY)
        {
            i_err = -1;
            HJFLoge("{} eglSwapBuffers: no display", m_insName);
            break;
        }
        if (!eglSwapBuffers(m_eglDisplay, i_surface))
        {
            i_err = -1;
            HJFLoge("eglSwapBuffers: error {}", GetEglErrorString());
            break;
        }
#endif
    } while (false);
    return i_err;
}


int HJOGEGLCore::EGLSurfaceRelease(EGLSurface i_surface)
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)   
        if (m_eglDisplay == EGL_NO_DISPLAY)
        {
            i_err = -1;
            HJFLoge("{} eglSwapBuffers: no display", m_insName);
            break;
        }       
        if (i_surface != EGL_NO_SURFACE)
        {          
            if (!eglDestroySurface(m_eglDisplay, i_surface))
            {
                HJFLoge("{} eglDestroySurface: unable to destroy surface {}", m_insName, GetEglErrorString());
                break;
            }        
            //lfs must call this; else next create eglCreateWindowSurface return 0 error; if make current, must make noting current; 
            eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                 
            i_surface = EGL_NO_SURFACE;
        }
#endif
    } while (false);
    HJFLogi("EGLSurfaceRelease:{}", i_err);
    return i_err;
}
EGLSurface HJOGEGLCore::EGLSurfaceCreate(void *window)
{
    EGLSurface eglSurface = nullptr;
    do
    {
#if defined(HarmonyOS)
        m_eglWindow = reinterpret_cast<EGLNativeWindowType>(window);
        if (!m_eglWindow) 
        {
            HJFLoge("{} eglWindow_ is null", m_insName);
            break;
        }
        if (m_eglDisplay == EGL_NO_DISPLAY)
        {
            HJFLoge("{} eglSwapBuffers: no display", m_insName);
            break;
        }  
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_eglWindow, NULL);
        if (eglSurface == EGL_NO_SURFACE)
        {
            HJFLoge("{} eglCreateWindowSurface: error {}", m_insName, GetEglErrorString());
            eglSurface = nullptr;
            break;
        }
#endif
    } while (false);
    return eglSurface;
}

EGLSurface HJOGEGLCore::EGLGetOffScreenSurface() const
{
    return m_eglOffScreenSurface;
}
//EGLSurface HJOGEGLCore::EGLGetSurface() const
//{
//    return m_eglSurface;
//}
int HJOGEGLCore::EGLOffScreenSurfaceCreate(int width, int height)
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)        
         if (m_eglDisplay != EGL_NO_DISPLAY)
         {
            const EGLint surfaceAttribs[] = {
                EGL_WIDTH, width,
                EGL_HEIGHT, height,
                EGL_NONE};   
            
            m_eglOffScreenSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, surfaceAttribs);
            if (m_eglOffScreenSurface == EGL_NO_SURFACE)
            {
                i_err = -1;
                HJFLoge("{} eglCreatePbufferSurface: error {}", m_insName, GetEglErrorString());
                break;
            }
        }
#endif
    } while (false);
    return i_err;
}

void HJOGEGLCore::done()
{
    priDone();
}

void HJOGEGLCore::makeNoCurrent() const {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

EGLContext HJOGEGLCore::getEglContext() const {
    return m_eglContext;
}

void HJOGEGLCore::priDone()
{
#if defined(HarmonyOS)     
    if (m_eglDisplay != EGL_NO_DISPLAY)
    {
        if (m_eglOffScreenSurface != EGL_NO_SURFACE)
        {
            eglDestroySurface(m_eglDisplay, m_eglOffScreenSurface);
            m_eglOffScreenSurface = EGL_NO_SURFACE;
            HJFLogi("{} eglDestroySurface m_eglOffScreenSurface", m_insName);
        }
        if (m_eglContext != EGL_NO_CONTEXT)
        {
            eglDestroyContext(m_eglDisplay, m_eglContext);
            m_eglContext = EGL_NO_CONTEXT;
            HJFLogi("{} eglDestroyContext m_eglContext", m_insName);
        }
        HJFLogi("eglMakeCurrent no surface and no context");
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(m_eglDisplay);
        HJFLogi("{} eglTerminate", m_insName);
        m_eglDisplay = EGL_NO_DISPLAY; 
    }
#endif
}

NS_HJ_END