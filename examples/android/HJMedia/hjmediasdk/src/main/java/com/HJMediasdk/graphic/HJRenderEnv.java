package com.HJMediasdk.graphic;

import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.os.Build;
import android.os.SystemClock;
import android.view.Surface;
import androidx.annotation.RequiresApi;

import com.HJMediasdk.graphic.EGL.HJEglCoreProxy;
import com.HJMediasdk.utils.HJBaseThread;
import com.HJMediasdk.utils.HJBaseTimer;
import com.HJMediasdk.utils.HJBaseTimerTask;
import com.HJMediasdk.utils.HJLog;


import java.util.ArrayList;
public class HJRenderEnv extends HJBaseThread
{
    private static final String TAG = HJRenderEnv.class.getSimpleName();
    public interface OnRenderFrameStatListener
    {
        void onRenderFrame(long costMs);
    }
    private HJEglCoreProxy mEglCore = null;
    private EGLSurface m_eglSurface = null;
    private boolean m_bUseTimer = true;
    private HJBaseTimer m_gl_timer = null;
    private volatile OnRenderFrameStatListener mOnRenderFrameStatListener = null;
    private final static int MSG_OPEN             = 1;
    private final static int MSG_CLOSE            = 2;
    private final static int MSG_RENDER_FRAME     = 3;
    private final static int FRAME_RATE = 15;
    private final static int MSG_SETSURFACE       = 4;
    public HJRenderEnv()
    {
    }

    public Object getEglCore()
    {
        return mEglCore.getEGLContext();
    }

    private int priInit(boolean i_bUseTimer,
                final int i_dstW, final int i_dstH,
                final Object i_dstObj, int i_fps)
    {
        super.start();

        m_bUseTimer = i_bUseTimer;

        m_outWidth  = i_dstW;
        m_outHeight = i_dstH;

//        if (i_dstObj instanceof SurfaceTexture)
//        {
//            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
//                ((SurfaceTexture)i_dstObj).setDefaultBufferSize(i_dstW,i_dstH);
//            }
//        }

        SyncQueueEvent(MSG_OPEN, new Runnable() {
            @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
            @Override
            public void run()
            {
                mEglCore = new HJEglCoreProxy(null, HJEglCoreProxy.FLAG_TRY_GLES3);
                m_eglSurface = (EGLSurface) mEglCore.createWindowSurface(i_dstObj);
                mEglCore.makeCurrent(m_eglSurface);

                int i_err = 0;
                do {
                    if (m_bUseTimer)
                    {
                        m_gl_timer = new HJBaseTimer();
                        m_gl_timer.schedule(new HJBaseTimerTask()
                        {
                            @Override
                            public void run()
                            {
                                prRenderFrame();
                            }
                        }, 0, 1000 / i_fps);
                    }
                } while (false);
            }
        });
        return 0;
    }

    public int init(boolean i_bUseTimer,
                     final int i_dstW, final int i_dstH,
                     final Object i_dstObj)
    {
        return priInit(i_bUseTimer, i_dstW, i_dstH, i_dstObj, FRAME_RATE);
    }
    public int init(boolean i_bUseTimer,
                    final int i_dstW, final int i_dstH,
                    final Object i_dstObj, int fps)
    {
        return priInit(i_bUseTimer, i_dstW, i_dstH, i_dstObj, fps);
    }
    private long m_statSum = 0;
    private int m_statIdx = 0;

    public void draw()
    {
        priRender();
    }

    public void setOnRenderFrameStatListener(OnRenderFrameStatListener listener)
    {
        mOnRenderFrameStatListener = listener;
    }

    private void prRenderFrame()
    {
        if (m_bUseTimer)
        {
            AsyncQueueClearEvent(MSG_RENDER_FRAME, new Runnable()
            {
                @Override
                public void run() {
                    long t0 = System.currentTimeMillis();
                    priRender();
                    long diff = System.currentTimeMillis() - t0;
                    m_statSum += diff;
                    m_statIdx++;
                    //HJLog.i(TAG, "env render stat average " + (m_statSum / m_statIdx) + " curTime " + diff);
                }
            });
        }
        else
        {
            AsyncQueueEvent(MSG_RENDER_FRAME, new Runnable() {
                @Override
                public void run()
                {
                    priRender();
                }
            });
        }
    }

    private int priRender()
    {
        int i_err = 0;
        long t0 = SystemClock.elapsedRealtime();
        do
        {
            if (m_eglSurface == null)
            {
                priCallbackNoDraw();
                break;
            }
            HJLog.i(TAG, "makeCurrent " + System.currentTimeMillis());
            i_err = mEglCore.makeCurrent(m_eglSurface);
            if (i_err < 0)
            {
                break;
            }
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            GLES20.glViewport(0,0,m_outWidth,m_outHeight);
            HJLog.i(TAG, "2023test glViewport w " + m_outWidth + " h " + m_outHeight);
            priCallbackDraw();

            //fixme
            long time = System.currentTimeMillis() * 1000000;
            mEglCore.setPresentationTime(m_eglSurface, time);
            HJLog.i(TAG, "surfacetexture egl settimer " + time);
            mEglCore.swap(m_eglSurface);

            HJLog.i(TAG, "currentTime " + System.currentTimeMillis());
        } while (false);
        OnRenderFrameStatListener listener = mOnRenderFrameStatListener;
        if (listener != null)
        {
            listener.onRenderFrame(SystemClock.elapsedRealtime() - t0);
        }
        return i_err;
    }

    public void setSurface(Surface i_surface)
    {
        SyncQueueEvent(MSG_SETSURFACE, new Runnable() {
            @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
            @Override
            public void run() {
                if (m_eglSurface != null)
                {
                    mEglCore.releaseSurface(m_eglSurface);
                    m_eglSurface = null;
                }
                if (i_surface != null)
                {
                    m_eglSurface = (EGLSurface) mEglCore.createWindowSurface(i_surface);
                }
            }
        });
    }
    public void release()
    {
        int i_err = 0;
        do
        {
            HJLog.i("test", "20170427 sync release render enter");

            {
                SyncQueueEvent(MSG_CLOSE, new Runnable()
                {
                    @Override
                    public void run()
                    {
                        if (m_gl_timer != null)
                        {
                            m_gl_timer.release();
                            m_gl_timer = null;
                        }

                        for (int i = 0; i < m_callback_list.size(); i++)
                        {
                            glDrawCallback cb = m_callback_list.get(i);
                            cb.release();
                        }
                        m_callback_list.clear();

                        if (m_eglSurface != null)
                        {
                            mEglCore.releaseSurface(m_eglSurface);
                            //m_eglSurface.release();
                            m_eglSurface = null;
                        }
                        if (mEglCore != null)
                        {
                            mEglCore.release();
                            mEglCore = null;
                        }
                        HJLog.i("test", "20170427 sync release render run end");
                    }
                });
                super.release();
            }
        } while (false);
    }

    public final static int SCREEN_SCALE_FIT  = 0;
    public final static int SCREEN_SCALE_CLIP = 1;

    private int m_outWidth  = 0;
    private int m_outHeight = 0;

    public int getOutOffX() {
        return m_outOffX;
    }
    public void setOutOffX(int m_outOffX) {
        this.m_outOffX = m_outOffX;
    }

    public int getOutOffY() {
        return m_outOffY;
    }

    public void setOutOffY(int m_outOffY) {
        this.m_outOffY = m_outOffY;
    }

    public float getVertexMatScaleX() {
        return m_vertexMatScaleX;
    }

    public void setVertexMatScaleX(float m_vertexMatScaleX) {
        this.m_vertexMatScaleX = m_vertexMatScaleX;
    }

    public float getVertexMatScaleY() {
        return m_vertexMatScaleY;
    }

    public void setVertexMatScaleY(float m_vertexMatScaleY) {
        this.m_vertexMatScaleY = m_vertexMatScaleY;
    }

    private int m_outOffX   = 0;
    private int m_outOffY   = 0;
    private float m_vertexMatScaleX = 1.f;
    private float m_vertexMatScaleY = 1.f;

    public int getOutWidth()
    {
        return m_outWidth;
    }
    public int getOutHeight()
    {
        return m_outHeight;
    }

    public void ScreenDisplay(int i_codecStyle,
                               int dw,
                               int dh,
                               int width,
                               int height)
    {
        if (height == 0 || dh == 0)
        {
            return;
        }

        int screenW = dw;
        int screenH = dh;

        double sar = (double) width / (double) height;
        double dar = (double) dw / (double) dh;

        switch (i_codecStyle) {
            case SCREEN_SCALE_FIT:
                if (dar < sar) {
                    dh = (int) (dw / sar);
                } else {
                    dw = (int) (dh * sar);
                }
                break;
            case SCREEN_SCALE_CLIP:
                if (dar < sar) {
                    dw = (int) (dh * sar);
                } else {
                    dh = (int) (dw / sar);
                }
                break;
        }
        m_outWidth  = dw;
        m_outHeight = dh;
        m_outOffX   = (screenW - m_outWidth) / 2;
        m_outOffY   = (screenH - m_outHeight) / 2;
        m_vertexMatScaleX = (float)m_outWidth / screenW;
        m_vertexMatScaleY = (float)m_outHeight / screenH;
    }
    public int ScreenDisplayMap(int i_codecStyle,
                              int dw,
                              int dh,
                              int width,
                              int height,
                              int i_value)
    {
        int value = 0;
        if (height == 0 || dh == 0)
        {
            return 0;
        }

        int screenW = dw;
        int screenH = dh;

        double sar = (double) width / (double) height;
        double dar = (double) dw / (double) dh;

        switch (i_codecStyle) {
            case SCREEN_SCALE_FIT:
                if (dar < sar) {
                    dh = (int) (dw / sar);
                    value = (int)(i_value / sar);
                } else {
                    dw = (int) (dh * sar);
                    value = (int)(i_value * sar);
                }
                break;
            case SCREEN_SCALE_CLIP:
                if (dar < sar) {
                    dw = (int) (dh * sar);
                    value = (int)(i_value * sar);
                } else {
                    dh = (int) (dw / sar);
                    value = (int)(i_value / sar);
                }
                break;
        }
        return value;
    }

    public void ScreenDisplayRatio(int i_codecStyle,
                              int i_targetW,
                              int i_targetH,
                              int i_srcW,
                              int i_srcH)
    {
        if (i_srcH == 0 || i_targetH == 0)
        {
            return;
        }

        int screenW = i_targetW;
        int screenH = i_targetH;

        int dw = i_targetW;
        int dh = i_targetH;

        int width = i_srcW;
        int height = i_srcH;

        switch (i_codecStyle) {
            case SCREEN_SCALE_FIT:
                if (dw < ((dh *width) / height))
                {
                    dh = (dw * height / width);
                }
                else
                {
                    dw = (dh * width) / height;
                }
                break;
            case SCREEN_SCALE_CLIP:
                if (dw < ((dh *width) / height))
                {
                    dw = (dh * width) / height;
                }
                else
                {
                    dh = (dw * height) / width;
                }
                break;
        }
        m_outWidth  = dw;
        m_outHeight = dh;
        m_outOffX   = (screenW - m_outWidth) / 2;
        m_outOffY   = (screenH - m_outHeight) / 2;
        m_vertexMatScaleX = (float)m_outWidth / screenW;
        m_vertexMatScaleY = (float)m_outHeight / screenH;
    }

    public int ScreenDisplayRatioMap(int i_codecStyle,
                                   int i_targetW,
                                   int i_targetH,
                                   int i_srcW,
                                   int i_srcH,
                                   int i_value)
    {
        if (i_srcH == 0 || i_targetH == 0)
        {
            return 0;
        }
        if (i_value == 0)
        {
            return 0;
        }

        int value = 0;

        int dw = i_targetW;
        int dh = i_targetH;

        int width = i_srcW;
        int height = i_srcH;

        switch (i_codecStyle) {
            case SCREEN_SCALE_FIT:
                if (dw < ((dh *width) / height))
                {
                    value = (dw * i_value / width);
                }
                else
                {
                    value = (dh * i_value) / height;
                }
                break;
            case SCREEN_SCALE_CLIP:
                if (dw < ((dh *width) / height))
                {
                    value = (dh * i_value) / height;
                }
                else
                {
                    value = (dw * i_value) / width;
                }
                break;
        }
        return value;
    }
   public interface glDrawCallback
   {
       int init();
       int draw();
       int nodraw();
       int release();
   }
   ArrayList<glDrawCallback> m_callback_list = new ArrayList<>();
   public void addCallback(glDrawCallback i_back)
   {
       AsyncQueueEvent(0, new Runnable()
       {
           @Override
           public void run()
           {
               if (null != i_back)
               {
                   i_back.init();
                   m_callback_list.add(i_back);
               }
           }
       });
   }
   public void removeCallback(glDrawCallback i_back)
   {
       AsyncQueueEvent(0, new Runnable()
       {
           @Override
           public void run()
           {
               if (null != i_back)
               {
                   i_back.release();
                   m_callback_list.remove(i_back);
               }
           }
       });
   }

   private synchronized int priCallbackDraw()
   {
       int i_err = 0;
       do
       {
           for (int i = 0; i < m_callback_list.size(); i++)
           {
                i_err = m_callback_list.get(i).draw();
                if (i_err < 0)
                {
                    break;
                }
           }
       } while(false);
       return i_err;
   }
    private synchronized int priCallbackNoDraw()
    {
        int i_err = 0;
        do
        {
            for (int i = 0; i < m_callback_list.size(); i++)
            {
                i_err = m_callback_list.get(i).nodraw();
                if (i_err < 0)
                {
                    break;
                }
            }
        } while(false);
        return i_err;
    }


   private SurfaceTextureCb m_surfaceTextureCb;
   public interface SurfaceTextureCb
   {
       void textureUpdate(int textureName, float[] mat);
   }
   public void setSurfaceTextureCb(SurfaceTextureCb i_cb)
   {
       m_surfaceTextureCb = i_cb;
   }


}
