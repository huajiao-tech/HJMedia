package com.HJMediasdk.entry.player;

import com.HJMediasdk.entry.HJBaseNativeInstance;
import com.HJMediasdk.utils.HJAudioListener;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class HJMusicPlayerJni extends HJBaseNativeInstance implements HJBaseListener, HJAudioListener
{
    private static final String TAG = HJMusicPlayerJni.class.getSimpleName();
    public static final String EVT_STREAM_OPENED = "EVENT_GRAPH_STREAM_OPENED_ID";
    public static final String EVT_EOF = "EVENT_GRAPH_EOF_ID";
    private WeakReference<HJBaseListener> mListenerWtr;
    private WeakReference<HJAudioListener.UI> mAudioListenerUIWtr;
    private ByteBuffer mReusablePcmBuffer;

    public HJMusicPlayerJni()
    {
    }

    public int init(int repeats,
                    long prerollDurationMs,
                    long pcmCallbackIntervalMs,
                    int sampleRate,
                    int channels,
                    HJBaseListener listener,
                    HJAudioListener.UI audioListener)
    {
        return initInternal(repeats,
                prerollDurationMs,
                pcmCallbackIntervalMs,
                sampleRate,
                channels,
                listener,
                audioListener);
    }

    private int initInternal(int repeats,
                             long prerollDurationMs,
                             long pcmCallbackIntervalMs,
                             int sampleRate,
                             int channels,
                             HJBaseListener listener,
                             HJAudioListener.UI audioListener)
    {
        mListenerWtr = listener == null ? null : new WeakReference<>(listener);
        mAudioListenerUIWtr = audioListener == null ? null : new WeakReference<>(audioListener);
        return nativeInit(repeats, prerollDurationMs, pcmCallbackIntervalMs, sampleRate, channels, this, this);
    }

    public void done()
    {
        mListenerWtr = null;
        mAudioListenerUIWtr = null;
        mReusablePcmBuffer = null;
        nativeDone();
    }

    public int open(String url)
    {
        return open(url, 0L);
    }

    public int open(String url, long startTimestamp)
    {
        return nativeOpen(url, startTimestamp);
    }

    public int pause()
    {
        return nativePause();
    }

    public int resume()
    {
        return nativeResume();
    }

    public int seek(long timestamp)
    {
        return nativeSeek(timestamp);
    }

    public int setMute(boolean mute)
    {
        return nativeSetMute(mute);
    }

    public boolean isMuted()
    {
        return nativeIsMuted();
    }

    public int setVolume(float volume)
    {
        return nativeSetVolume(volume);
    }

    public float getVolume()
    {
        return nativeGetVolume();
    }

    public long getCurrentTimestamp()
    {
        return nativeGetCurrentTimestamp();
    }

    public long getDuration()
    {
        return nativeGetDuration();
    }

    public int[] getAudioTrackIds()
    {
        return nativeGetAudioTrackIds();
    }

    public int switchAudioTrack(int trackId)
    {
        return nativeSwitchAudioTrack(trackId);
    }

    public int setRepeats(int repeats)
    {
        return nativeSetRepeats(repeats);
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
    private native int nativeInit(int repeats,
                                  long prerollDurationMs,
                                  long pcmCallbackIntervalMs,
                                  int sampleRate,
                                  int channels,
                                  HJBaseListener listener,
                                  HJAudioListener audioListener);
    private native void nativeDone();
    private native int nativeOpen(String url, long startTimestamp);
    private native int nativePause();
    private native int nativeResume();
    private native int nativeSeek(long timestamp);
    private native int nativeSetMute(boolean mute);
    private native boolean nativeIsMuted();
    private native int nativeSetVolume(float volume);
    private native float nativeGetVolume();
    private native long nativeGetCurrentTimestamp();
    private native long nativeGetDuration();
    private native int[] nativeGetAudioTrackIds();
    private native int nativeSwitchAudioTrack(int trackId);
    private native int nativeSetRepeats(int repeats);
}
