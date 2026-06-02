package com.HJMediasdk.graphic.EGL;

import android.annotation.SuppressLint;
import android.graphics.SurfaceTexture;
import android.opengl.EGLExt;
import android.view.Surface;
import android.view.SurfaceHolder;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

/**
 * Created by j-gaoshang on 2017/3/29.
 */

@SuppressLint("NewApi")
public final class HJEgl10Core {
    private static final String TAG = "EglCore";

    /**
     * Constructor flag: ask for GLES3, fall back to GLES2 if not available.  Without this
     * flag, GLES2 is used.
     */
    public static final int FLAG_TRY_GLES3 = 0x02;

    public static final int FLAG_3D = 0x04;

    private EGLDisplay mEGLDisplay = EGL10.EGL_NO_DISPLAY;
    private EGLContext mEGLContext = EGL10.EGL_NO_CONTEXT;
    private EGLConfig mEGLConfig = null;
    private EGLConfig mPBufferEGLConfig = null;
    private int mGlVersion = -1;
    private int mFlags;

    private EGL10 mEgl = null;
    private static final int EGL_OPENGL_ES2_BIT = 0x0004;
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

    /**
     * Prepares EGL display and context.
     * <p>
     * Equivalent to EglCore(null, 0).
     */
    public HJEgl10Core() {
        this(null, 0);
    }

    /**
     * Prepares EGL display and context.
     * <p>
     * @param sharedContext The context to share, or null if sharing is not desired.
     * @param flags Configuration bit flags, e.g. FLAG_TRY_GLES3.
     */
    public HJEgl10Core(EGLContext sharedContext, int flags) {
        if (mEGLDisplay != EGL10.EGL_NO_DISPLAY) {
            throw new RuntimeException("EGL already set up");
        }

        if (sharedContext == null) {
            sharedContext = EGL10.EGL_NO_CONTEXT;
        }

        mFlags = flags;

        mEgl = (EGL10)EGLContext.getEGL();

        mEGLDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        if (mEGLDisplay == EGL10.EGL_NO_DISPLAY) {
            throw new RuntimeException("unable to get EGL10 display");
        }
        int[] version = new int[2];
        if (!mEgl.eglInitialize(mEGLDisplay, version)) {
            mEGLDisplay = null;
            throw new RuntimeException("unable to initialize EGL10");
        }

        // Try to get a GLES3 context, if requested.
        if ((flags & FLAG_TRY_GLES3) != 0) {
            //HJLog.i(TAG, "Trying GLES 3");
            EGLConfig config = getConfig(flags, 3);
            if (config != null) {
                int[] attrib3_list = {
                        EGL_CONTEXT_CLIENT_VERSION, 3,
                        EGL10.EGL_NONE
                };
                EGLContext context = mEgl.eglCreateContext(mEGLDisplay, config, sharedContext,
                        attrib3_list);

                if (mEgl.eglGetError() == EGL10.EGL_SUCCESS) {
                    //HJLog.i(TAG, "Got GLES 3 config");
                    mEGLConfig = config;
                    mEGLContext = context;
                    mGlVersion = 3;
                }
            }
        }
        if (mEGLContext == EGL10.EGL_NO_CONTEXT) {  // GLES 2 only, or GLES 3 attempt failed
            //HJLog.i(TAG, "Trying GLES 2");
            EGLConfig config = getConfig(flags, 2);
            if (config == null) {
                throw new RuntimeException("Unable to find a suitable EGLConfig");
            }
            int[] attrib2_list = {
                    EGL_CONTEXT_CLIENT_VERSION, 2,
                    EGL10.EGL_NONE
            };
            EGLContext context = mEgl.eglCreateContext(mEGLDisplay, config, sharedContext,
                    attrib2_list);
            checkEglError("eglCreateContext");
            mEGLConfig = config;
            mEGLContext = context;
            mGlVersion = 2;
        }

        // Confirm with query.
        int[] values = new int[1];
        mEgl.eglQueryContext(mEGLDisplay, mEGLContext, EGL_CONTEXT_CLIENT_VERSION,
                values);
        //HJLog.i(TAG, "EGLContext created, client version " + values[0]);
    }

    /**
     * Finds a suitable EGLConfig.
     *
     * @param flags Bit flags from constructor.
     * @param version Must be 2 or 3.
     */
    private EGLConfig getConfig(int flags, int version) {
        int renderableType = EGL_OPENGL_ES2_BIT;
        if (version >= 3) {
            renderableType |= EGLExt.EGL_OPENGL_ES3_BIT_KHR;
        }

        // The actual surface is generally RGBA or RGBX, so situationally omitting alpha
        // doesn't really help.  It can also lead to a huge performance hit on glReadPixels()
        // when reading into a GL_RGBA buffer.
        int[] attribList2d = {
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8,
//                EGL10.EGL_DEPTH_SIZE, 16,
//                EGL10.EGL_STENCIL_SIZE, 0,
                EGL10.EGL_RENDERABLE_TYPE, renderableType,
                EGL10.EGL_NONE
        };

        int[] attribList3d = {
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8,
                EGL10.EGL_DEPTH_SIZE, 16,
//                EGL10.EGL_STENCIL_SIZE, 8,
                EGL10.EGL_RENDERABLE_TYPE, renderableType,
                EGL10.EGL_NONE
        };

        int[] attribList = attribList2d;
        if ((flags & FLAG_3D) != 0) {
            attribList = attribList3d;
        }

        EGLConfig[] configs = new EGLConfig[1];
        int[] numConfigs = new int[1];
        if (!mEgl.eglChooseConfig(mEGLDisplay, attribList, configs, configs.length,
                numConfigs)) {
            return null;
        }
        return configs[0];
    }

    /**
     * Discards all resources held by this class, notably the EGL context.  This must be
     * called from the thread where the context was created.
     * <p>
     * On completion, no context will be current.
     */
    public void release() {
        if (mEGLDisplay != EGL10.EGL_NO_DISPLAY) {
            // Android is unusual in that it uses a reference-counted EGLDisplay.  So for
            // every eglInitialize() we need an eglTerminate().
            mEgl.eglMakeCurrent(mEGLDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
            mEgl.eglDestroyContext(mEGLDisplay, mEGLContext);
//            mEgl.eglReleaseThread();
            mEgl.eglTerminate(mEGLDisplay);
        }

        mEGLDisplay = EGL10.EGL_NO_DISPLAY;
        mEGLContext = EGL10.EGL_NO_CONTEXT;
        mEGLConfig = null;
        mPBufferEGLConfig = null;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (mEGLDisplay != EGL10.EGL_NO_DISPLAY) {
                // We're limited here -- finalizers don't run on the thread that holds
                // the EGL state, so if a surface or context is still current on another
                // thread we can't fully release it here.  Exceptions thrown from here
                // are quietly discarded.  Complain in the log file.
                //HJLog.w(TAG, "WARNING: EglCore was not explicitly released -- state may be leaked");
                release();
            }
        } finally {
            super.finalize();
        }
    }

    /**
     * Destroys the specified surface.  Note the EGLSurface won't actually be destroyed if it's
     * still current in a context.
     */
    public void releaseSurface(EGLSurface eglSurface) {
        mEgl.eglDestroySurface(mEGLDisplay, eglSurface);
    }

    /**
     * Creates an EGL surface associated with a Surface.
     * <p>
     * If this is destined for MediaCodec, the EGLConfig should have the "recordable" attribute.
     */
    public EGLSurface createWindowSurface(Object surface) {
        if (!(surface instanceof Surface) && !(surface instanceof SurfaceTexture)  && !(surface instanceof SurfaceHolder)) {
            throw new RuntimeException("invalid surface: " + surface);
        }

        // Create a window surface, and attach it to the Surface we received.
        int[] surfaceAttribs = {
                EGL10.EGL_NONE
        };
        EGLSurface eglSurface = mEgl.eglCreateWindowSurface(mEGLDisplay, mEGLConfig, surface,
                surfaceAttribs);
        checkEglError("eglCreateWindowSurface");
        if (eglSurface == null) {
            throw new RuntimeException("surface was null");
        }
        return eglSurface;
    }

    private EGLConfig getPBufferConfig() {
        int renderableType = EGL_OPENGL_ES2_BIT;
        if (mGlVersion >= 3) {
            renderableType |= EGLExt.EGL_OPENGL_ES3_BIT_KHR;
        }

        // The actual surface is generally RGBA or RGBX, so situationally omitting alpha
        // doesn't really help.  It can also lead to a huge performance hit on glReadPixels()
        // when reading into a GL_RGBA buffer.
        int[] attribList2d = {
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8,
                EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT,
                EGL10.EGL_NONE
        };
        int[] attribList3d = {
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8,
                EGL10.EGL_DEPTH_SIZE, 16,
//                EGL14.EGL_STENCIL_SIZE, 8,
                EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT,
                EGL10.EGL_NONE
        };

        int[] attribList = attribList2d;
        if ((mFlags & FLAG_3D) != 0) {
            attribList = attribList3d;
        }

        EGLConfig[] configs = new EGLConfig[1];
        int[] numConfigs = new int[1];
        if (!mEgl.eglChooseConfig(mEGLDisplay, attribList, configs, configs.length,
                numConfigs)) {
            return null;
        }
        return configs[0];
    }
    /**
     * Creates an EGL surface associated with an offscreen buffer.
     */
    public EGLSurface createOffscreenSurface(int width, int height) {
        if (mPBufferEGLConfig == null) {
            mPBufferEGLConfig = getPBufferConfig();
        }

        int[] surfaceAttribs = {
                EGL10.EGL_WIDTH, width,
                EGL10.EGL_HEIGHT, height,
                EGL10.EGL_NONE
        };
        EGLSurface eglSurface = mEgl.eglCreatePbufferSurface(mEGLDisplay, mPBufferEGLConfig,
                surfaceAttribs);
        checkEglError("eglCreatePbufferSurface");
        if (eglSurface == null) {
            throw new RuntimeException("surface was null");
        }
        return eglSurface;
    }

    /**
     * Makes our EGL context current, using the supplied surface for both "draw" and "read".
     */
    public int makeCurrent(EGLSurface eglSurface) {
        if (mEGLDisplay == EGL10.EGL_NO_DISPLAY) {
            // called makeCurrent() before create?
            //HJLog.i(TAG, "NOTE: makeCurrent w/o display");
            return -1;
        }
        if (!mEgl.eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext)) {
            //HJLog.e(TAG, "eglMakeCurrent failed");
            return -1;
        }
        return 0;
    }

    /**
     * Makes our EGL context current, using the supplied "draw" and "read" surfaces.
     */
    public int makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) {
        if (mEGLDisplay == EGL10.EGL_NO_DISPLAY) {
            // called makeCurrent() before create?
            //HJLog.i(TAG, "NOTE: makeCurrent w/o display");
        }
        if (!mEgl.eglMakeCurrent(mEGLDisplay, drawSurface, readSurface, mEGLContext)) {
            //HJLog.e(TAG, "eglMakeCurrent(draw,read) failed");
            return -1;
        }
        return 0;
    }

    /**
     * Makes no context current.
     */
    public int makeNothingCurrent() {
        if (!mEgl.eglMakeCurrent(mEGLDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE,
                EGL10.EGL_NO_CONTEXT)) {
            //HJLog.e(TAG, "eglMakeCurrent failed");
            return -1;
        }
        return 0;
    }

    /**
     * Calls eglSwapBuffers.  Use this to "publish" the current frame.
     *
     * @return false on failure
     */
    public boolean swapBuffers(EGLSurface eglSurface) {
        return mEgl.eglSwapBuffers(mEGLDisplay, eglSurface);
    }

    public int swap(EGLSurface eglSurface) {
        if (!swapBuffers(eglSurface)) {
            return mEgl.eglGetError();
        }
        return EGL10.EGL_SUCCESS;
    }

    /**
     * Sends the presentation time stamp to EGL.  Time is expressed in nanoseconds.
     */
    public void setPresentationTime(EGLSurface eglSurface, long nsecs) {
    	//EGLExt.eglPresentationTimeANDROID(mEGLDisplay, eglSurface, nsecs);
    }

    /**
     * Returns true if our context and the specified surface are current.
     */
    public boolean isCurrent(EGLSurface eglSurface) {
        return mEGLContext.equals(mEgl.eglGetCurrentContext()) &&
                eglSurface.equals(mEgl.eglGetCurrentSurface(EGL10.EGL_DRAW));
    }

    /**
     * Performs a simple surface query.
     */
    public int querySurface(EGLSurface eglSurface, int what) {
        int[] value = new int[1];
        mEgl.eglQuerySurface(mEGLDisplay, eglSurface, what, value);
        return value[0];
    }

    /**
     * Queries a string value.
     */
    public String queryString(int what) {
        return mEgl.eglQueryString(mEGLDisplay, what);
    }

    /**
     * Returns the GLES version this context is configured for (currently 2 or 3).
     */
    public int getGlVersion() {
        return mGlVersion;
    }

    /**
     * Writes the current display, context, and surface to the log.
     */
/*    public static void logCurrent(String msg) {
        EGLDisplay display;
        EGLContext context;
        EGLSurface surface;

        display = EGL14.eglGetCurrentDisplay();
        context = EGL14.eglGetCurrentContext();
        surface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW);
        //HJLog.i(TAG, "Current EGL (" + msg + "): display=" + display + ", context=" + context + ", surface=" + surface);
    }
*/
    public static boolean haveEGLContext() {
        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLContext eglContext = egl.eglGetCurrentContext();
        if (eglContext == null || eglContext.equals(EGL10.EGL_NO_CONTEXT)) {
            return false;
        }
        return true;
    }
    public EGLContext getEGLContext()
    {
        return mEGLContext;
    }
    /**
     * Checks for EGL errors.  Throws an exception if an error has been raised.
     */
    private void checkEglError(String msg) {
        int error;
        if ((error = mEgl.eglGetError()) != EGL10.EGL_SUCCESS) {
            throw new RuntimeException(msg + ": EGL error: 0x" + Integer.toHexString(error));
        }
    }
}
