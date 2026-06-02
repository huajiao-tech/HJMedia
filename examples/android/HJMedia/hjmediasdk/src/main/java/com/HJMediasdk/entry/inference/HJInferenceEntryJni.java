package com.HJMediasdk.entry.inference;

import com.HJMediasdk.entry.HJBaseNativeInstance;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.lang.ref.WeakReference;

public class HJInferenceEntryJni extends HJBaseNativeInstance
{
    private static final String TAG = HJInferenceEntryJni.class.getSimpleName();

    private WeakReference<HJBaseListener> m_listenerWtr;

    public HJInferenceEntryJni()
    {
    }

    public int process(String i_modelPath, String i_modeType, boolean i_bCpu, int i_threadNum, int i_nScale, String i_url, String o_url)
    {
        return nativeProcess(i_modelPath, 1, i_modeType, i_bCpu, i_threadNum, i_nScale, i_url, o_url);
    }

    public int process(String i_modelPath, int i_srType, String i_modeType, boolean i_bCpu, int i_threadNum, int i_nScale, String i_url, String o_url)
    {
        return nativeProcess(i_modelPath, i_srType, i_modeType, i_bCpu, i_threadNum, i_nScale, i_url, o_url);
    }

    public int initSR(String i_modelPath, int i_srType, String i_modeType, boolean i_bCpu, int i_threadNum, int i_nScale)
    {
        return nativeInitSR(i_modelPath, i_srType, i_modeType, i_bCpu, i_threadNum, i_nScale);
    }

    public int initSR(String i_modelPath, String i_modeType, boolean i_bCpu, int i_threadNum, int i_nScale)
    {
        return initSR(i_modelPath, 1, i_modeType, i_bCpu, i_threadNum, i_nScale);
    }

    public int processSR(String i_url, String o_url)
    {
        return nativeProcessSR(i_url, o_url);
    }

    private native int nativeInitSR(String i_modelPath, int i_srType, String i_modeType, boolean i_bCpu, int i_threadNum, int i_nScale);
    private native int nativeProcessSR(String i_url, String o_url);
    private native int nativeProcess(String i_modelPath, int i_srType, String i_modeType, boolean i_bCpu, int i_threadNum, int i_nScale, String i_url, String o_url);

}
