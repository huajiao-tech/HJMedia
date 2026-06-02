package com.HJMediasdk.entry.render;

import com.HJMediasdk.entry.HJBaseNativeInstance;
import com.HJMediasdk.utils.HJLog;

public class HJRenderContextJni
{
    private static final String TAG = HJRenderContextJni.class.getSimpleName();
    private static final Object s_syncObject = new Object();
    private static boolean s_bContextInit = false;

    static {
        try {
            System.loadLibrary("HJRender");
        }catch (Exception e){
            HJLog.e(TAG, "loadLibrary failed");
        }
    }

    public HJRenderContextJni()
    {
    }

    public static int contextInit(String logDir, int logLevel, int logMode, int max_size, int max_files) {
        HJLog.i(TAG, "renderContextInit enter");
        int i_err = 0;
        synchronized (s_syncObject)
        {
            if (!s_bContextInit)
            {
                i_err = renderContextInit(logDir, logLevel, logMode, max_size, max_files);
                if (i_err == 0)
                {
                    HJLog.i(TAG, "renderContextInit success");
                    s_bContextInit = true;
                }
                else
                {
                    HJLog.e(TAG, "renderContextInit failed");
                }
            }
        }
        return i_err;
    }
    public static int contextUnInit()
    {
        HJLog.i(TAG, "contextUnInit enter");
        synchronized (s_syncObject)
        {
            if (s_bContextInit)
            {
                renderContextUnInit();
                HJLog.i(TAG, "renderContextUnInit");
                s_bContextInit = false;
            }
        }
        return 0;
    }

    private static native int renderContextInit(String logDir, int logLevel, int logMode, int max_size, int max_files);
    private static native int renderContextUnInit();
}