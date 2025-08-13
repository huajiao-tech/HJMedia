#include "HJOGEGLSurface.h"

NS_HJ_BEGIN

HJOGEGLSurface::HJOGEGLSurface()
{

}
HJOGEGLSurface::~HJOGEGLSurface()
{

}

void HJOGEGLSurface::setTargetWidth(int i_width)
{
    m_width = i_width;
}
void HJOGEGLSurface::setTargetHeight(int i_height)
{
    m_height = i_height;
}
int HJOGEGLSurface::getTargetWidth() const
{
    return m_width;
}
int HJOGEGLSurface::getTargetHeight() const
{
    return m_height;
}

void HJOGEGLSurface::setEGLSurface(EGLSurface i_eglSurface)
{
    m_eglSurface = i_eglSurface;
}
EGLSurface HJOGEGLSurface::getEGLSurface() const
{
    return m_eglSurface;
}

void HJOGEGLSurface::setWindow(void* i_window)
{
    m_window = i_window;
}
void* HJOGEGLSurface::getWindow() const
{
    return m_window;
}
int HJOGEGLSurface::getSurfaceType() const
{
    return m_surfaceType;
}
void HJOGEGLSurface::setSurfaceType(int i_type)
{
    m_surfaceType = i_type;
}
void HJOGEGLSurface::addRenderIdx()
{
    m_renderIdx++;
}
int64_t HJOGEGLSurface::getRenderIdx() const
{
    return m_renderIdx;
}
void HJOGEGLSurface::setFps(int i_fps)
{
    m_fps = i_fps;
}
int HJOGEGLSurface::getFps() const
{
    return m_fps;
}

NS_HJ_END