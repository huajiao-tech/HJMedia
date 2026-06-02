package com.HJMediasdk.graphic.EGL;

import android.annotation.SuppressLint;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLExt;
import android.opengl.EGLSurface;
import android.os.Build;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.HJMediasdk.utils.HJLog;

/**
 * Core EGL state (display, context, config).
 * <p>
 * The EGLContext must only be attached to one thread at a time.  This class is not thread-safe.
 */
@SuppressLint("NewApi")
public final class HJEglCore {
    private static final String TAG = "EglCore";

    /**
     * Constructor flag: surface must be recordable.  This discourages EGL from using a
     * pixel format that cannot be converted efficiently to something usable by the video
     * encoder.
     */
    public static final int FLAG_RECORDABLE = 0x01;

    /**
     * Constructor flag: ask for GLES3, fall back to GLES2 if not available.  Without this
     * flag, GLES2 is used.
     */
    public static final int FLAG_TRY_GLES3 = 0x02;

    public static final int FLAG_3D = 0x04;

    // Android-specific extension.
    private static final int EGL_RECORDABLE_ANDROID = 0x3142;

    private EGLDisplay mEGLDisplay = EGL14.EGL_NO_DISPLAY;
    private EGLContext mEGLContext = EGL14.EGL_NO_CONTEXT;
    private EGLConfig mEGLConfig = null;
    private EGLConfig mPBufferEGLConfig = null;
    private int mGlVersion = -1;
    private int mFlags;

    public EGLContext getEGLContext() {
        return mEGLContext;
    }

    /**
     * Prepares EGL display and context.
     * <p>
     * Equivalent to EglCore(null, 0).
     */
    public HJEglCore() {
        this(null, 0);
    }

    /**
     * Prepares EGL display and context.
     * <p>
     * @param sharedContext The context to share, or null if sharing is not desired.
     * @param flags Configuration bit flags, e.g. FLAG_RECORDABLE.
     */
    public HJEglCore(EGLContext sharedContext, int flags) {
        EGL14.eglGetError();
        if (!mEGLDisplay.equals(EGL14.EGL_NO_DISPLAY)) {
            throw new RuntimeException("EGL already set up!");
        }

        if (sharedContext == null) {
            sharedContext = EGL14.EGL_NO_CONTEXT;
        }

        mFlags = flags;

        mEGLDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        EGL14.eglGetError();    // todo: it's no sence.
        if (mEGLDisplay.equals(EGL14.EGL_NO_DISPLAY)) {
            throw new RuntimeException("Unable to get EGL14 display!");
        }

        int[] version = new int[2];
        boolean ret = EGL14.eglInitialize(mEGLDisplay, version, 0, version, 1);
        int error = EGL14.eglGetError();
        if (!ret || error != EGL14.EGL_SUCCESS) {
            release();
            throw new RuntimeException("Unable to initialize EGL14! EGL error: 0x" + Integer.toHexString(error));
        }

        // Try to get a GLES3 context, if requested.
        if ((flags & FLAG_TRY_GLES3) != 0) {
            HJLog.i(TAG, "Trying GLES 3");
            EGLConfig config = getConfig(flags, 3);
            error = EGL14.eglGetError();
            if (config == null || error != EGL14.EGL_SUCCESS) {
                release();
                throw new RuntimeException("Unable to find a suitable EGLConfig for version 3! EGL error: 0x" + Integer.toHexString(error));
            }

            int[] attrib3_list = {
                    EGL14.EGL_CONTEXT_CLIENT_VERSION, 3,
                    EGL14.EGL_NONE
            };
            EGLContext context = EGL14.eglCreateContext(mEGLDisplay, config, sharedContext,
                    attrib3_list, 0);
            error = EGL14.eglGetError();
            if (context.equals(EGL14.EGL_NO_CONTEXT) || error != EGL14.EGL_SUCCESS) {
                release();
                throw new RuntimeException("Unable to create a EGLContext! EGL error: 0x" + Integer.toHexString(error));
            }
            HJLog.i(TAG, "Got GLES 3 config");
            mEGLConfig = config;
            mEGLContext = context;
            mGlVersion = 3;
        }

        if (mEGLContext.equals(EGL14.EGL_NO_CONTEXT)) {  // GLES 2 only, or GLES 3 attempt failed
        	HJLog.i(TAG, "Trying GLES 2");
            EGLConfig config = getConfig(flags, 2);
            error = EGL14.eglGetError();
            if (config == null || error != EGL14.EGL_SUCCESS) {
                release();
                throw new RuntimeException("Unable to find a suitable EGLConfig! EGL error: 0x" + Integer.toHexString(error));
            }

            int[] attrib2_list = {
                    EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
                    EGL14.EGL_NONE
            };
            EGLContext context = EGL14.eglCreateContext(mEGLDisplay, config, sharedContext,
                    attrib2_list, 0);
            error = EGL14.eglGetError();
            if (context.equals(EGL14.EGL_NO_CONTEXT) || error != EGL14.EGL_SUCCESS) {
                release();
                throw new RuntimeException("Unable to create a EGLContext! EGL error: 0x" + Integer.toHexString(error));
            }
            mEGLConfig = config;
            mEGLContext = context;
            mGlVersion = 2;
        }

        // Confirm with query.
        int[] values = new int[1];
        ret = EGL14.eglQueryContext(mEGLDisplay, mEGLContext, EGL14.EGL_CONTEXT_CLIENT_VERSION,
                values, 0);
        error = EGL14.eglGetError();
//        if (!ret || error != EGL14.EGL_SUCCESS) {
//            release();
//            throw new RuntimeException("eglQueryContext: EGL error: 0x" + Integer.toHexString(error));
//        }
        HJLog.i(TAG, "EGLContext created, client version " + values[0]);
    }

    /**
     * Finds a suitable EGLConfig.
     *
     * @param flags Bit flags from constructor.
     * @param version Must be 2 or 3.
     */
    private EGLConfig getConfig(int flags, int version) {
        int renderableType = EGL14.EGL_OPENGL_ES2_BIT;
        if (version >= 3) {
            renderableType |= EGLExt.EGL_OPENGL_ES3_BIT_KHR;
        }

        // The actual surface is generally RGBA or RGBX, so situationally omitting alpha
        // doesn't really help.  It can also lead to a huge performance hit on glReadPixels()
        // when reading into a GL_RGBA buffer.
        int[] attribList2d = {
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                //EGL14.EGL_DEPTH_SIZE, 16,
                //EGL14.EGL_STENCIL_SIZE, 8,
                EGL14.EGL_RENDERABLE_TYPE, renderableType,
                EGL14.EGL_NONE, 0,      // placeholder for recordable [@-3]
                EGL14.EGL_NONE
        };

        int[] attribList3d = {
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                EGL14.EGL_DEPTH_SIZE, 16,
//                EGL14.EGL_STENCIL_SIZE, 8,
                EGL14.EGL_RENDERABLE_TYPE, renderableType,
                EGL14.EGL_NONE, 0,      // placeholder for recordable [@-3]
                EGL14.EGL_NONE
        };

        int[] attribList = attribList2d;
        if ((flags & FLAG_3D) != 0)
        {
            attribList = attribList3d;
        }

        if ((flags & FLAG_RECORDABLE) != 0) {
            attribList[attribList.length - 3] = EGL_RECORDABLE_ANDROID;
            attribList[attribList.length - 2] = 1;
        }
        EGLConfig[] configs = new EGLConfig[1];
        int[] numConfigs = new int[1];
        if (!EGL14.eglChooseConfig(mEGLDisplay, attribList, 0, configs, 0, configs.length,
                numConfigs, 0)) {
            HJLog.w(TAG, "unable to find RGB8888 / " + version + " EGLConfig");
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
        if (!mEGLDisplay.equals(EGL14.EGL_NO_DISPLAY)) {
            // Android is unusual in that it uses a reference-counted EGLDisplay.  So for
            // every eglInitialize() we need an eglTerminate().
            EGL14.eglMakeCurrent(mEGLDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE,
                    EGL14.EGL_NO_CONTEXT);
            EGL14.eglDestroyContext(mEGLDisplay, mEGLContext);
            EGL14.eglReleaseThread();
            EGL14.eglTerminate(mEGLDisplay);
            EGL14.eglGetError();
        }

        mEGLDisplay = EGL14.EGL_NO_DISPLAY;
        mEGLContext = EGL14.EGL_NO_CONTEXT;
        mEGLConfig = null;
        mPBufferEGLConfig = null;
        mGlVersion = -1;
        mFlags = 0;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (!mEGLDisplay.equals(EGL14.EGL_NO_DISPLAY)) {
                // We're limited here -- finalizers don't run on the thread that holds
                // the EGL state, so if a surface or context is still current on another
                // thread we can't fully release it here.  Exceptions thrown from here
                // are quietly discarded.  Complain in the log file.
            	HJLog.w(TAG, "WARNING: EglCore was not explicitly released -- state may be leaked");
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
        EGL14.eglDestroySurface(mEGLDisplay, eglSurface);
        EGL14.eglGetError();
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
                EGL14.EGL_NONE
        };
        EGLSurface eglSurface = EGL14.eglCreateWindowSurface(mEGLDisplay, mEGLConfig, surface,
                surfaceAttribs, 0);
        checkEglError("eglCreateWindowSurface");
        if (eglSurface.equals(EGL14.EGL_NO_SURFACE)) {
            throw new RuntimeException("surface was null");
        }
        return eglSurface;
    }

    private EGLConfig getPBufferConfig() {
        int renderableType = EGL14.EGL_OPENGL_ES2_BIT;
        if (mGlVersion >= 3) {
            renderableType |= EGLExt.EGL_OPENGL_ES3_BIT_KHR;
        }

        // The actual surface is generally RGBA or RGBX, so situationally omitting alpha
        // doesn't really help.  It can also lead to a huge performance hit on glReadPixels()
        // when reading into a GL_RGBA buffer.
        int[] attribList2d = {
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                EGL14.EGL_SURFACE_TYPE, EGL14.EGL_PBUFFER_BIT,
                EGL14.EGL_NONE
        };
        int[] attribList3d = {
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                EGL14.EGL_DEPTH_SIZE, 16,
//                EGL14.EGL_STENCIL_SIZE, 8,
                EGL14.EGL_SURFACE_TYPE, EGL14.EGL_PBUFFER_BIT,
                EGL14.EGL_NONE
        };

        int[] attribList = attribList2d;
        if ((mFlags & FLAG_3D) != 0) {
            attribList = attribList3d;
        }

        EGLConfig[] configs = new EGLConfig[1];
        int[] numConfigs = new int[1];
        if (!EGL14.eglChooseConfig(mEGLDisplay, attribList, 0, configs, 0, configs.length,
                numConfigs, 0)) {
            HJLog.w(TAG, "unable to find RGB8888");
            return null;
        }
        return configs[0];
    }

    /**
     * Creates an EGL surface associated with an offscreen buffer.
     */
    public EGLSurface createOffscreenSurface(int width, int height) {
        if (mPBufferEGLConfig == null) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
                mPBufferEGLConfig = getPBufferConfig();
                checkEglError("eglChooseConfig");
            } else {
                mPBufferEGLConfig = mEGLConfig;
            }
            if (mPBufferEGLConfig == null) {
                throw new RuntimeException("EGLConfig was null");
            }
        }

        int[] surfaceAttribs = {
                EGL14.EGL_WIDTH, width,
                EGL14.EGL_HEIGHT, height,
                EGL14.EGL_NONE
        };
        EGLSurface eglSurface = EGL14.eglCreatePbufferSurface(mEGLDisplay, mPBufferEGLConfig,
                surfaceAttribs, 0);
        checkEglError("eglCreatePbufferSurface");
        if (eglSurface.equals(EGL14.EGL_NO_SURFACE)) {
            throw new RuntimeException("PbufferSurface was null");
        }
        return eglSurface;
    }

    /**
     * Makes our EGL context current, using the supplied surface for both "draw" and "read".
     */
    public int makeCurrent(EGLSurface eglSurface) {
        if (mEGLDisplay.equals(EGL14.EGL_NO_DISPLAY)) {
            // called makeCurrent() before create?
        	HJLog.i(TAG, "NOTE: makeCurrent w/o display");
            return -1;
        }
        boolean ret = EGL14.eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext);
        EGL14.eglGetError();
        if (!ret) {
        	HJLog.e(TAG, "eglMakeCurrent failed");
        	return -1;
        }
        return 0;
    }

    /**
     * Makes our EGL context current, using the supplied "draw" and "read" surfaces.
     */
    public int makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) {
        if (mEGLDisplay.equals(EGL14.EGL_NO_DISPLAY)) {
            // called makeCurrent() before create?
        	HJLog.i(TAG, "NOTE: makeCurrent w/o display");
        }
        boolean ret = EGL14.eglMakeCurrent(mEGLDisplay, drawSurface, readSurface, mEGLContext);
        EGL14.eglGetError();
        if (!ret) {
            HJLog.e(TAG, "eglMakeCurrent(draw,read) failed");
            return -1;
        }
        return 0;
    }

    /**
     * Makes no context current.
     */
    public int makeNothingCurrent() {
        boolean ret = EGL14.eglMakeCurrent(mEGLDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_CONTEXT);
        EGL14.eglGetError();
        if (!ret) {
        	HJLog.e(TAG, "makeNothingCurrent::eglMakeCurrent failed");
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
        boolean ret = EGL14.eglSwapBuffers(mEGLDisplay, eglSurface);
        EGL14.eglGetError();
        return ret;
    }

    /**
     * Sends the presentation time stamp to EGL.  Time is expressed in nanoseconds.
     */
    public void setPresentationTime(EGLSurface eglSurface, long nsecs) {
        EGLExt.eglPresentationTimeANDROID(mEGLDisplay, eglSurface, nsecs);
        EGL14.eglGetError();
    }

    /**
     * Returns true if our context and the specified surface are current.
     */
    public boolean isCurrent(EGLSurface eglSurface) {
        EGLSurface surface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW);
        EGLContext context = EGL14.eglGetCurrentContext();
        EGL14.eglGetError();
        return mEGLContext.equals(context) && eglSurface.equals(surface);
    }

    /**
     * Performs a simple surface query.
     */
    public int querySurface(EGLSurface eglSurface, int what) {
        int[] value = new int[1];
        EGL14.eglQuerySurface(mEGLDisplay, eglSurface, what, value, 0);
        EGL14.eglGetError();
        return value[0];
    }

    /**
     * Queries a string value.
     */
    public String queryString(int what) {
        String str = EGL14.eglQueryString(mEGLDisplay, what);
        EGL14.eglGetError();
        return str;
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
    public static void logCurrent(String msg) {
        EGLDisplay display = EGL14.eglGetCurrentDisplay();
        EGLContext context = EGL14.eglGetCurrentContext();
        EGLSurface surface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW);
        EGL14.eglGetError();
        HJLog.i(TAG, "Current EGL (" + msg + "): display=" + display + ", context=" + context +
                ", surface=" + surface);
    }

    public static boolean haveEGLContext() {
        EGLContext context = EGL14.eglGetCurrentContext();
        EGL14.eglGetError();
        if (context == null || context.equals(EGL14.EGL_NO_CONTEXT)) {
            return false;
        }
        return true;
    }

    /**
     * Checks for EGL errors.  Throws an exception if an error has been raised.
     */
    private void checkEglError(String msg) {
        int error = EGL14.eglGetError();
        if (error != EGL14.EGL_SUCCESS) {
            throw new RuntimeException(msg + ": EGL error: 0x" + Integer.toHexString(error));
        }
    }
}
