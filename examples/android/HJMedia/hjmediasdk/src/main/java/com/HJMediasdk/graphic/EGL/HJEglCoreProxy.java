package com.HJMediasdk.graphic.EGL;

import android.annotation.SuppressLint;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.os.Build;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;

public final class HJEglCoreProxy implements HJIEGLProxy
{
    private static final String TAG = "EglCoreProc";  
    public static final int FLAG_RECORDABLE = 0x01;
	public static final int FLAG_TRY_GLES3 = 0x02;
	public static final int FLAG_3D = 0x04;

    HJEgl10Core mEgl10Core = null;
    HJEglCore mEglCore   = null;
	private Object mOffscreenEglSurface = null;
    
    private int m_sdk_int = Build.VERSION.SDK_INT;
    
    public HJEglCoreProxy(Object sharedContext, int i_nFlag)
    {
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
			mEgl10Core = new HJEgl10Core((javax.microedition.khronos.egl.EGLContext)sharedContext, i_nFlag);
		}
		else
		{
			mEglCore = new HJEglCore((android.opengl.EGLContext)sharedContext, i_nFlag);
		}

		try {
			mOffscreenEglSurface = createOffscreenSurface(1, 1);
		} catch (RuntimeException e) {
			if (mEgl10Core != null) {
				mEgl10Core.release();
			}
			if (mEglCore != null) {
				mEglCore.release();
			}
			throw e;
		}
    }

    @Override
    public Object getEGLContext()
	{
		Object obj = null;
		if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
			obj = (Object)mEgl10Core.getEGLContext();
		}
		else
		{
			obj = (Object)mEglCore.getEGLContext();
		}
		return obj;
	}

	@Override
    public int makeNothingCurrent() 
    {
    	int i_err = 0;
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
    		mEgl10Core.makeNothingCurrent();
		}
		else
		{
			mEglCore.makeNothingCurrent();
		} 	
    	return i_err;
    }

	@Override
    public void releaseSurface(Object eglSurface) 
    {    	
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
    		mEgl10Core.releaseSurface((javax.microedition.khronos.egl.EGLSurface)eglSurface);
		}
		else
		{
			mEglCore.releaseSurface((android.opengl.EGLSurface)eglSurface);
		} 	
    }

	@Override
	@SuppressLint("NewApi")
	public Object createWindowSurface(Object surface) 
    {
    	Object eglSurface = null;
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
    		eglSurface = mEgl10Core.createWindowSurface(surface);
    		if (eglSurface == EGL10.EGL_NO_SURFACE) 
    		{
    			eglSurface = null;
    		}
		}
		else
		{
			eglSurface = mEglCore.createWindowSurface(surface);
			if (eglSurface == EGL14.EGL_NO_SURFACE)
			{
				eglSurface = null;
			}
		} 
    	return eglSurface;
    }

    private Object createOffscreenSurface(int width, int height)
    {
        Object eglSurface = null;
        if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
        {
            eglSurface = mEgl10Core.createOffscreenSurface(width, height);
            if (eglSurface == EGL10.EGL_NO_SURFACE)
            {
                eglSurface = null;
            }
        }
        else
        {
            eglSurface = mEglCore.createOffscreenSurface(width, height);
            if (eglSurface == EGL14.EGL_NO_SURFACE)
            {
                eglSurface = null;
            }
        }
        return eglSurface;
    }

	@Override
    public int makeCurrent(Object eglSurface)
    {
        if (eglSurface == null) {
            eglSurface = mOffscreenEglSurface;
        }

    	int i_err = 0;
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
    		i_err = mEgl10Core.makeCurrent((javax.microedition.khronos.egl.EGLSurface)eglSurface);
		}
		else
		{
			i_err = mEglCore.makeCurrent((android.opengl.EGLSurface)eglSurface);
		} 
    	return i_err;
    }

	public int swap()
	{
		int i_err = 0;
		int ret = 0;
		if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
			ret = mEgl10Core.swap((javax.microedition.khronos.egl.EGLSurface)mOffscreenEglSurface);

			if (ret == EGL10.EGL_SUCCESS)
			{
				i_err = 0;
			}
			else if (ret == EGL11.EGL_CONTEXT_LOST)
			{
				i_err = -1;
			}
			else
			{
				i_err = ret;
			}
		}
		else
		{
			if (mEglCore.swapBuffers((android.opengl.EGLSurface)mOffscreenEglSurface))
			{
				i_err = 0;
			}
			else
			{
				i_err = -1;
			}
		}
		return i_err;
	}

	@Override
    public int swap(Object eglSurface) 
    {
    	if (eglSurface == null)
		{
			return swap();
		}
    	int i_err = 0;
    	int ret = 0;
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
    		ret = mEgl10Core.swap((javax.microedition.khronos.egl.EGLSurface)eglSurface);
    		
    		if (ret == EGL10.EGL_SUCCESS)
    		{
    			i_err = 0;
    		}
    		else if (ret == EGL11.EGL_CONTEXT_LOST)
    		{
    			i_err = -1;
    		}
    		else
    		{
    			i_err = ret;
    		}
		}
		else
		{
			if (mEglCore.swapBuffers((android.opengl.EGLSurface)eglSurface))
			{
				i_err = 0;
			}
			else
			{
				i_err = -1;
			}
		} 
    	return i_err;
    }

	@Override
    public void release() 
    {
        if (mOffscreenEglSurface != null) {
			makeNothingCurrent();
            releaseSurface(mOffscreenEglSurface);
            mOffscreenEglSurface = null;
        }

        if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2) {
            mEgl10Core.release();
            mEgl10Core = null;
        } else {
            mEglCore.release();
            mEglCore = null;
        }
    }

	@Override
    public void setPresentationTime(Object eglSurface, long nsecs) 
    {
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
    		mEgl10Core.setPresentationTime((javax.microedition.khronos.egl.EGLSurface)eglSurface, nsecs);
		}
    	else
    	{
    		mEglCore.setPresentationTime((android.opengl.EGLSurface)eglSurface, nsecs);
		}
    }

	@Override
    @SuppressLint("NewApi")
	public boolean isValid(Object surface)
    {
    	boolean isValid = true;
    	if (m_sdk_int < Build.VERSION_CODES.JELLY_BEAN_MR2)
    	{
    		isValid = ((javax.microedition.khronos.egl.EGLSurface)surface != EGL10.EGL_NO_SURFACE);
    	}
    	else
    	{
    		isValid = ((android.opengl.EGLSurface)surface != EGL14.EGL_NO_SURFACE);   	    
    	}
    	return isValid;
    }

	@Override
	public long getNativeContext() {
    	return 0;
	}
/*
    public static boolean haveEGLContext() {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2) {
			return Egl10Core.haveEGLContext();
		}
		else {
			return EglCore.haveEGLContext();
		}
	}
*/
	public static boolean haveEGLContext() {
		EGLContext eglContext = null;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
			eglContext = EGL14.eglGetCurrentContext();
		}
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
			if (eglContext != null && !eglContext.equals(EGL14.EGL_NO_CONTEXT)) {
				return true;
			}
		}

		EGL10 egl = (EGL10) javax.microedition.khronos.egl.EGLContext.getEGL();
		javax.microedition.khronos.egl.EGLContext egl10Context = egl.eglGetCurrentContext();
		if (egl10Context != null && !egl10Context.equals(EGL10.EGL_NO_CONTEXT)) {
			return true;
		}

		return false;
	}
}