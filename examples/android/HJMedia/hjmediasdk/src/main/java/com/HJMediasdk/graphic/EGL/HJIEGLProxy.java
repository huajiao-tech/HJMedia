package com.HJMediasdk.graphic.EGL;

public interface HJIEGLProxy {
    Object getEGLContext();
    int makeNothingCurrent();
    void releaseSurface(Object eglSurface);
    Object createWindowSurface(Object surface);
    int makeCurrent(Object eglSurface);
    int swap(Object eglSurface);
    void release();
    void setPresentationTime(Object eglSurface, long nsecs);
    boolean isValid(Object surface);
    long getNativeContext();
}
