package com.HJMediasdk.entry.player;

import com.HJMediasdk.entry.HJBaseNativeInstance;
import com.HJMediasdk.utils.HJAudioListener;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class HJAudioMixerJni extends HJBaseNativeInstance implements HJBaseListener, HJAudioListener
{
    private static final String TAG = HJAudioMixerJni.class.getSimpleName();

    public static final int AV_SAMPLE_FMT_S16 = 1;
    public static final int HJAudioListenerFlag_PCM = 0x01;
    public static final int HJAudioListenerFlag_AACHEADER = 0x02;
    public static final int HJAudioListenerFlag_AACDat = 0x04;

    private WeakReference<HJBaseListener> mListenerWtr;
    private WeakReference<HJAudioListener.UI> mAudioListenerUIWtr;
    private ByteBuffer mReusablePcmBuffer;

    public HJAudioMixerJni()
    {
    }

    public int init(HJAudioMixerConfig config,
                    HJBaseListener listener,
                    HJAudioListener.UI audioListener)
    {
        mListenerWtr = listener == null ? null : new WeakReference<>(listener);
        mAudioListenerUIWtr = audioListener == null ? null : new WeakReference<>(audioListener);
        mReusablePcmBuffer = null;
        return nativeInit(config, this, this);
    }

    public void done()
    {
        mListenerWtr = null;
        mAudioListenerUIWtr = null;
        mReusablePcmBuffer = null;
        nativeDone();
    }

    public int addInput(HJAudioMixerInputDesc inputDesc)
    {
        return nativeAddInput(inputDesc);
    }

    public int setInputVolume(String inputId, float volume)
    {
        return nativeSetInputVolume(inputId, volume);
    }

    public int setMasterMute(boolean muted)
    {
        return nativeSetMasterMute(muted);
    }

    public boolean isMasterMuted()
    {
        return nativeIsMasterMuted();
    }

    public int flushInput(String inputId)
    {
        return nativeFlushInput(inputId);
    }

    public int removeInput(String inputId, boolean drain)
    {
        return nativeRemoveInput(inputId, drain);
    }

    public int pushNativePcm(String inputId,
                             long dataPtr,
                             int size,
                             int samples,
                             int channels,
                             int sampleRate,
                             int sampleFmt,
                             long ptsMs)
    {
        return nativePushPcmPtr(inputId, dataPtr, size, samples, channels, sampleRate, sampleFmt, ptsMs);
    }

    public int pushPcm(String inputId,
                       ByteBuffer buffer,
                       int size,
                       int samples,
                       int channels,
                       int sampleRate,
                       int sampleFmt,
                       long ptsMs)
    {
        return nativePushPcmBuffer(inputId, buffer, size, samples, channels, sampleRate, sampleFmt, ptsMs);
    }

    public void release()
    {
        done();
    }

    @Override
    public int onNotify(int id, long value, String desc)
    {
        HJLog.i(TAG, "onNotify id:" + id + " value:" + value + " desc:" + desc);
        if (mListenerWtr != null)
        {
            HJBaseListener listener = mListenerWtr.get();
            if (listener != null)
            {
                return listener.onNotify(id, value, desc);
            }
        }
        return 0;
    }

    @Override
    public int notify(int i_sampleRate,
                      int i_channels,
                      int i_sampleFmt,
                      int i_bytesPerSample,
                      long i_data,
                      int i_nSize,
                      int i_nFlag)
    {
        if (mAudioListenerUIWtr == null || i_nSize <= 0)
        {
            return 0;
        }

        HJAudioListener.UI audioListener = mAudioListenerUIWtr.get();
        if (audioListener == null)
        {
            return 0;
        }

        ByteBuffer buffer = ensurePcmBufferCapacity(i_nSize);
        int i_err = nativeAcquirePCM(i_data, i_nSize, buffer);
        if (i_err == 0)
        {
            buffer.position(0);
            buffer.limit(i_nSize);
            i_err = audioListener.notify(i_sampleRate,
                                         i_channels,
                                         i_sampleFmt,
                                         i_bytesPerSample,
                                         buffer,
                                         i_nFlag);
        }
        return i_err;
    }

    private ByteBuffer ensurePcmBufferCapacity(int size)
    {
        if (mReusablePcmBuffer == null || mReusablePcmBuffer.capacity() < size)
        {
            mReusablePcmBuffer = ByteBuffer.allocateDirect(size).order(ByteOrder.nativeOrder());
        }
        mReusablePcmBuffer.clear();
        return mReusablePcmBuffer;
    }

    private native int nativeAcquirePCM(long i_data, int i_nSize, ByteBuffer i_buffer);
    private native int nativeInit(HJAudioMixerConfig config, HJBaseListener listener, HJAudioListener audioListener);
    private native void nativeDone();
    private native int nativeAddInput(HJAudioMixerInputDesc inputDesc);
    private native int nativeSetInputVolume(String inputId, float volume);
    private native int nativeSetMasterMute(boolean muted);
    private native boolean nativeIsMasterMuted();
    private native int nativeFlushInput(String inputId);
    private native int nativeRemoveInput(String inputId, boolean drain);
    private native int nativePushPcmPtr(String inputId,
                                        long dataPtr,
                                        int size,
                                        int samples,
                                        int channels,
                                        int sampleRate,
                                        int sampleFmt,
                                        long ptsMs);
    private native int nativePushPcmBuffer(String inputId,
                                           ByteBuffer buffer,
                                           int size,
                                           int samples,
                                           int channels,
                                           int sampleRate,
                                           int sampleFmt,
                                           long ptsMs);
}
