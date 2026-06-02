package com.example.ui;

import android.util.Log;

import com.HJMediasdk.entry.player.HJAudioMixerJni;

import java.nio.ByteBuffer;
import java.util.Arrays;

final class MixerAudioPlaybackRouter
{
    private static final String TAG = MixerAudioPlaybackRouter.class.getSimpleName();
    interface PcmPlayer
    {
        int play(int sampleRate, int channels, int sampleFmt, ByteBuffer buffer);
    }

    interface AacDecoder
    {
        int configure(int sampleRate, int channels, ByteBuffer codecConfig);

        int decode(ByteBuffer encodedData);

        void release();
    }

    interface AacDecoderFactory
    {
        AacDecoder create(PcmPlayer pcmPlayer, StatusListener statusListener);
    }

    interface StatusListener
    {
        void onStatus(String status);
    }

    private final PcmPlayer mPcmPlayer;
    private final AacDecoderFactory mAacDecoderFactory;
    private final StatusListener mStatusListener;

    private AacDecoder mAacDecoder;
    private byte[] mCodecConfig;
    private int mAacSampleRate;
    private int mAacChannels;

    MixerAudioPlaybackRouter(PcmPlayer pcmPlayer,
                             AacDecoderFactory aacDecoderFactory,
                             StatusListener statusListener)
    {
        mPcmPlayer = pcmPlayer;
        mAacDecoderFactory = aacDecoderFactory;
        mStatusListener = statusListener;
    }

    int handle(int sampleRate,
               int channels,
               int sampleFmt,
               int bytesPerSample,
               ByteBuffer buffer,
               int flag)
    {
        if (buffer == null)
        {
            return 0;
        }
        Log.i(TAG, "hand out mix data:" + buffer.slice() + ", flag：" + flag);
        if ((flag & HJAudioMixerJni.HJAudioListenerFlag_PCM) != 0)
        {
            return mPcmPlayer.play(sampleRate, channels, sampleFmt, buffer.duplicate());
        }

        int ret = 0;
        if ((flag & HJAudioMixerJni.HJAudioListenerFlag_AACHEADER) != 0)
        {
            ret = handleAacHeader(sampleRate, channels, buffer.duplicate());
        }
        if (ret < 0)
        {
            return ret;
        }
        if ((flag & HJAudioMixerJni.HJAudioListenerFlag_AACDat) != 0)
        {
            return handleAacData(buffer.duplicate());
        }
        return ret;
    }

    void release()
    {
        releaseDecoder();
        mCodecConfig = null;
        mAacSampleRate = 0;
        mAacChannels = 0;
    }

    private int handleAacHeader(int sampleRate, int channels, ByteBuffer codecConfig)
    {
        byte[] newCodecConfig = copyBytes(codecConfig);
        if (newCodecConfig.length == 0)
        {
            postStatus("aac codec config is empty");
            return 0;
        }

        boolean codecConfigChanged =
                sampleRate != mAacSampleRate ||
                channels != mAacChannels ||
                !Arrays.equals(mCodecConfig, newCodecConfig);
        if (!codecConfigChanged && mAacDecoder != null)
        {
            return 0;
        }

        releaseDecoder();
        mCodecConfig = newCodecConfig;
        mAacSampleRate = sampleRate;
        mAacChannels = channels;
        if (mAacDecoderFactory == null)
        {
            postStatus("aac decoder factory is unavailable");
            return -1;
        }

        mAacDecoder = mAacDecoderFactory.create(mPcmPlayer, mStatusListener);
        if (mAacDecoder == null)
        {
            postStatus("create aac decoder failed");
            return -1;
        }

        int ret = mAacDecoder.configure(sampleRate,
                                        channels,
                                        ByteBuffer.wrap(mCodecConfig));
        if (ret < 0)
        {
            postStatus("configure aac decoder failed ret=" + ret);
            releaseDecoder();
            return ret;
        }
        return 0;
    }

    private int handleAacData(ByteBuffer encodedData)
    {
        if (mAacDecoder == null)
        {
            postStatus("drop aac data before codec config");
            return 0;
        }
        return mAacDecoder.decode(encodedData);
    }

    private void releaseDecoder()
    {
        if (mAacDecoder != null)
        {
            mAacDecoder.release();
            mAacDecoder = null;
        }
    }

    private void postStatus(String status)
    {
        if (mStatusListener != null)
        {
            mStatusListener.onStatus(status);
        }
    }

    private static byte[] copyBytes(ByteBuffer buffer)
    {
        ByteBuffer copy = buffer.duplicate();
        byte[] data = new byte[copy.remaining()];
        copy.get(data);
        return data;
    }
}
