package com.HJMediasdk.entry.inference;

import com.HJMediasdk.utils.HJLog;

public class HJInferenceContextJni
{
    private static final String TAG = HJInferenceContextJni.class.getSimpleName();
    private static final Object s_syncObject = new Object();
    private static boolean s_bContextInit = false;

    static {
        try {
            System.loadLibrary("HJInference");
        }catch (Exception e){
            HJLog.e(TAG, "loadLibrary failed");
        }
    }

    public HJInferenceContextJni()
    {
    }

    public static int contextInit(String logDir, int logLevel, int logMode, int max_size, int max_files) {
        HJLog.i(TAG, "contextInit enter");
        int i_err = 0;
        synchronized (s_syncObject)
        {
            if (!s_bContextInit)
            {
                i_err = inferenceContextInit(logDir, logLevel, logMode, max_size, max_files);
                if (i_err == 0)
                {
                    HJLog.i(TAG, "contextInit success");
                    s_bContextInit = true;
                }
                else
                {
                    HJLog.e(TAG, "contextInit failed");
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
                inferenceContextUnInit();
                HJLog.i(TAG, "contextUnInit end");
                s_bContextInit = false;
            }
        }
        return 0;
    }

    private static native int inferenceContextInit(String logDir, int logLevel, int logMode, int max_size, int max_files);
    private static native int inferenceContextUnInit();
}