package com.HJMediasdk.entry.player;

import androidx.annotation.Keep;

import com.HJMediasdk.entry.HJBaseNativeInstance;

@Keep
public class HJVideoEnhanceJni extends HJBaseNativeInstance
{
    private int m_lastOutputTextureId = 0;
    private long m_lastElapsedMs = 0L;

    public HJVideoEnhanceJni()
    {
    }

    public int init()
    {
        return init(new HJVideoEnhanceOption());
    }

    public int init(HJVideoEnhanceOption option)
    {
        return nativeInit(option);
    }

    public int setOption(HJVideoEnhanceOption option)
    {
        return nativeSetOption(option);
    }

    public int process(int inputTextureId,
                       int inputWidth,
                       int inputHeight,
                       int outputWidth,
                       int outputHeight)
    {
        return nativeProcess(inputTextureId, inputWidth, inputHeight, outputWidth, outputHeight);
    }

    public int getLastOutputTextureId()
    {
        return m_lastOutputTextureId;
    }

    public long getLastElapsedMs()
    {
        return m_lastElapsedMs;
    }

    public void done()
    {
        nativeDone();
    }

    public void release()
    {
        done();
    }

    private native int nativeInit(HJVideoEnhanceOption option);
    private native int nativeSetOption(HJVideoEnhanceOption option);
    private native int nativeProcess(int inputTextureId,
                                     int inputWidth,
                                     int inputHeight,
                                     int outputWidth,
                                     int outputHeight);
    private native void nativeDone();
}
