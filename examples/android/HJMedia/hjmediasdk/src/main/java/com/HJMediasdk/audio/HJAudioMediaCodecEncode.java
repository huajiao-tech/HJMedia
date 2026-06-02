package com.HJMediasdk.audio;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;

import com.HJMediasdk.utils.HJLog;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class HJAudioMediaCodecEncode
{
    private static final String TAG = HJAudioMediaCodecEncode.class.getSimpleName();
    private static final String MIME_AAC = MediaFormat.MIMETYPE_AUDIO_AAC;
    private static final int DEFAULT_SAMPLE_RATE = 44100;
    private static final int DEFAULT_CHANNELS = 2;
    private static final int DEFAULT_BITRATE = 128000;
    private static final int DEFAULT_AAC_PROFILE = MediaCodecInfo.CodecProfileLevel.AACObjectLC;
    private static final long DEFAULT_CODEC_TIMEOUT_US = 10000L;

    private final Object mStateLock = new Object();
    private final Object mListenerLock = new Object();

    private HJAudioListener mListener;

    private MediaCodec mMediaCodec;
    private MediaCodec.BufferInfo mBufferInfo;
    private int mSampleRate = DEFAULT_SAMPLE_RATE;
    private int mChannels = DEFAULT_CHANNELS;
    private int mBitrate = DEFAULT_BITRATE;
    private int mAacProfile = DEFAULT_AAC_PROFILE;
    private boolean mAddAdtsHeader = true;
    private long mCodecTimeoutUs = DEFAULT_CODEC_TIMEOUT_US;

    public HJAudioMediaCodecEncode()
    {
    }

    public void setListener(HJAudioListener i_listener)
    {
        synchronized (mListenerLock)
        {
            mListener = i_listener;
        }
    }

    public void setSampleRate(int i_nSampleRate)
    {
        mSampleRate = Math.max(8000, i_nSampleRate);
    }

    public void setChannels(int i_nChannels)
    {
        mChannels = i_nChannels >= 2 ? 2 : 1;
    }

    public void setBitrate(int i_nBitrate)
    {
        mBitrate = Math.max(32000, i_nBitrate);
    }

    public void setAacProfile(int i_nAacProfile)
    {
        mAacProfile = i_nAacProfile;
    }

    public void setAddAdtsHeader(boolean i_bAddAdtsHeader)
    {
        mAddAdtsHeader = i_bAddAdtsHeader;
    }

    public void setCodecTimeoutUs(long i_nCodecTimeoutUs)
    {
        mCodecTimeoutUs = Math.max(1000L, i_nCodecTimeoutUs);
    }

    public int init()
    {
        synchronized (mStateLock)
        {
            priReleaseLocked();

            try
            {
                MediaFormat format = MediaFormat.createAudioFormat(MIME_AAC, mSampleRate, mChannels);
                format.setInteger(MediaFormat.KEY_AAC_PROFILE, mAacProfile);
                format.setInteger(MediaFormat.KEY_BIT_RATE, mBitrate);
                format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, priFrameBytes() * 4);

                MediaCodec codec = MediaCodec.createEncoderByType(MIME_AAC);
                codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
                codec.start();

                mMediaCodec = codec;
                mBufferInfo = new MediaCodec.BufferInfo();
                return 0;
            }
            catch (Exception e)
            {
                HJLog.e(TAG, "encoder init failed " + e.getMessage());
                priNotifyErr(HJAudioListener.CAP_OTHER_ERR);
                priReleaseLocked();
                return -1;
            }
        }
    }

    public void release()
    {
        synchronized (mStateLock)
        {
            priReleaseLocked();
        }
    }

    public int encode(ByteBuffer i_buffer, int i_nSize, long i_nPtsMs)
    {
        if (i_buffer == null || i_nSize <= 0)
        {
            return 0;
        }

        synchronized (mStateLock)
        {
            if (mMediaCodec == null)
            {
                return -1;
            }

            try
            {
                int inputIndex = mMediaCodec.dequeueInputBuffer(mCodecTimeoutUs);
                if (inputIndex < 0)
                {
                    return 0;
                }

                ByteBuffer inputBuffer = mMediaCodec.getInputBuffer(inputIndex);
                if (inputBuffer == null)
                {
                    priNotifyErr(HJAudioListener.CAP_OTHER_ERR);
                    return -1;
                }

                ByteBuffer sourceBuffer = i_buffer.duplicate();
                int writeSize = Math.min(i_nSize, Math.min(sourceBuffer.remaining(), inputBuffer.remaining()));
                if (writeSize <= 0)
                {
                    mMediaCodec.queueInputBuffer(inputIndex, 0, 0, priMsToUs(i_nPtsMs), 0);
                    return 0;
                }

                inputBuffer.clear();
                sourceBuffer.limit(sourceBuffer.position() + writeSize);
                inputBuffer.put(sourceBuffer);
                mMediaCodec.queueInputBuffer(inputIndex, 0, writeSize, priMsToUs(i_nPtsMs), 0);

                priDrainEncoderLocked(false);
                return 0;
            }
            catch (Exception e)
            {
                HJLog.e(TAG, "encode failed " + e.getMessage());
                priNotifyErr(HJAudioListener.CAP_OTHER_ERR);
                return -1;
            }
        }
    }

    private void priReleaseLocked()
    {
        if (mMediaCodec == null)
        {
            mBufferInfo = null;
            return;
        }

        try
        {
            priDrainEncoderLocked(true);
        }
        catch (Exception e)
        {
            HJLog.w(TAG, "drain encoder on release failed " + e.getMessage());
        }

        try
        {
            mMediaCodec.stop();
        }
        catch (Exception e)
        {
            HJLog.w(TAG, "stop encoder failed " + e.getMessage());
        }

        try
        {
            mMediaCodec.release();
        }
        catch (Exception e)
        {
            HJLog.w(TAG, "release encoder failed " + e.getMessage());
        }

        mMediaCodec = null;
        mBufferInfo = null;
    }

    private void priDrainEncoderLocked(boolean i_bEndOfStream)
    {
        if (mMediaCodec == null || mBufferInfo == null)
        {
            return;
        }

        if (i_bEndOfStream)
        {
            int inputIndex = mMediaCodec.dequeueInputBuffer(mCodecTimeoutUs);
            if (inputIndex >= 0)
            {
                mMediaCodec.queueInputBuffer(inputIndex,
                                             0,
                                             0,
                                             priMsToUs(System.currentTimeMillis()),
                                             MediaCodec.BUFFER_FLAG_END_OF_STREAM);
            }
        }

        while (true)
        {
            int outputIndex = mMediaCodec.dequeueOutputBuffer(mBufferInfo, mCodecTimeoutUs);
            if (outputIndex == MediaCodec.INFO_TRY_AGAIN_LATER)
            {
                if (!i_bEndOfStream)
                {
                    break;
                }
                continue;
            }

            if (outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED)
            {
                priDispatchOutputFormatExtra(mMediaCodec.getOutputFormat());
                continue;
            }

            if (outputIndex < 0)
            {
                break;
            }

            ByteBuffer outputBuffer = mMediaCodec.getOutputBuffer(outputIndex);
            if (outputBuffer != null && mBufferInfo.size > 0)
            {
                outputBuffer.position(mBufferInfo.offset);
                outputBuffer.limit(mBufferInfo.offset + mBufferInfo.size);

                if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0)
                {
                    priDispatchExtraReady(outputBuffer.slice(), mBufferInfo.size);
                }
                else
                {
                    priDispatchCapAac(outputBuffer.slice(),
                                      mBufferInfo.size,
                                      priUsToMs(mBufferInfo.presentationTimeUs),
                                      HJAudioListener.AAC_MDAT);
                }
            }

            mMediaCodec.releaseOutputBuffer(outputIndex, false);

            if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0)
            {
                break;
            }
        }
    }

    private void priDispatchOutputFormatExtra(MediaFormat i_format)
    {
        if (i_format == null)
        {
            return;
        }

        ByteBuffer extra = i_format.getByteBuffer("csd-0");
        if (extra == null || !extra.hasRemaining())
        {
            return;
        }
        priDispatchExtraReady(extra.duplicate(), extra.remaining());
    }

    private void priDispatchExtraReady(ByteBuffer i_buffer, int i_nSize)
    {
        if (i_buffer == null || i_nSize <= 0)
        {
            return;
        }

        ByteBuffer extraBuffer = priCopyDirectBuffer(i_buffer, i_nSize);
        synchronized (mListenerLock)
        {
            if (mListener == null)
            {
                return;
            }
            ByteBuffer notifyBuffer = extraBuffer.duplicate().order(ByteOrder.nativeOrder());
            notifyBuffer.position(0);
            notifyBuffer.limit(extraBuffer.limit());
            int ret = mListener.onExtraReady(notifyBuffer, notifyBuffer.remaining());
            if (ret < 0)
            {
                HJLog.e(TAG, "onExtraReady failed ret=" + ret);
            }
        }
    }

    private void priDispatchCapAac(ByteBuffer i_payload, int i_nPayloadSize, long i_nPtsUs, int i_nFlag)
    {
        if (i_payload == null || i_nPayloadSize <= 0)
        {
            return;
        }

        int frameSize = mAddAdtsHeader ? i_nPayloadSize + 7 : i_nPayloadSize;
        ByteBuffer frameBuffer = ByteBuffer.allocateDirect(frameSize).order(ByteOrder.nativeOrder());
        if (mAddAdtsHeader)
        {
            priAddAdtsHeader(frameBuffer, frameSize, mSampleRate, mChannels);
        }

        ByteBuffer payloadBuffer = i_payload.duplicate();
        payloadBuffer.limit(payloadBuffer.position() + i_nPayloadSize);
        frameBuffer.put(payloadBuffer);
        frameBuffer.flip();

        synchronized (mListenerLock)
        {
            if (mListener == null)
            {
                return;
            }
            ByteBuffer notifyBuffer = frameBuffer.duplicate().order(ByteOrder.nativeOrder());
            notifyBuffer.position(0);
            notifyBuffer.limit(frameBuffer.limit());
            int ret = mListener.onCapAAC(notifyBuffer,
                                         notifyBuffer.remaining(),
                                         i_nPtsUs,
                                         i_nFlag);
            if (ret < 0)
            {
                HJLog.e(TAG, "onCapAAC failed ret=" + ret);
            }
        }
    }

    private void priNotifyErr(int i_nErr)
    {
        synchronized (mListenerLock)
        {
            if (mListener != null)
            {
                mListener.onErr(i_nErr);
            }
        }
    }

    private ByteBuffer priCopyDirectBuffer(ByteBuffer i_source, int i_nSize)
    {
        ByteBuffer sourceBuffer = i_source.duplicate();
        int copySize = Math.min(i_nSize, sourceBuffer.remaining());
        ByteBuffer copyBuffer = ByteBuffer.allocateDirect(copySize).order(ByteOrder.nativeOrder());
        sourceBuffer.limit(sourceBuffer.position() + copySize);
        copyBuffer.put(sourceBuffer);
        copyBuffer.flip();
        return copyBuffer;
    }

    private void priAddAdtsHeader(ByteBuffer i_packet, int i_nPacketLen, int i_nSampleRate, int i_nChannels)
    {
        int freqIdx = priSampleRateToAdtsIndex(i_nSampleRate);
        int chanCfg = i_nChannels >= 2 ? 2 : 1;
        int profile = 2;

        i_packet.put((byte) 0xFF);
        i_packet.put((byte) 0xF1);
        i_packet.put((byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2)));
        i_packet.put((byte) (((chanCfg & 3) << 6) + (i_nPacketLen >> 11)));
        i_packet.put((byte) ((i_nPacketLen & 0x7FF) >> 3));
        i_packet.put((byte) (((i_nPacketLen & 7) << 5) + 0x1F));
        i_packet.put((byte) 0xFC);
    }

    private int priSampleRateToAdtsIndex(int i_nSampleRate)
    {
        switch (i_nSampleRate)
        {
            case 96000:
                return 0;
            case 88200:
                return 1;
            case 64000:
                return 2;
            case 48000:
                return 3;
            case 44100:
                return 4;
            case 32000:
                return 5;
            case 24000:
                return 6;
            case 22050:
                return 7;
            case 16000:
                return 8;
            case 12000:
                return 9;
            case 11025:
                return 10;
            case 8000:
                return 11;
            case 7350:
                return 12;
            default:
                return 4;
        }
    }

    private int priFrameBytes()
    {
        return 1024 * mChannels * 2;
    }

    private long priMsToUs(long i_nPtsMs)
    {
        return i_nPtsMs * 1000L;
    }

    private long priUsToMs(long i_nPtsUs)
    {
        return i_nPtsUs / 1000L;
    }


}
