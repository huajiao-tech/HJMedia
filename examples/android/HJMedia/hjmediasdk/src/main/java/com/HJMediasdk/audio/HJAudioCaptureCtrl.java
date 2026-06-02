package com.HJMediasdk.audio;

import android.Manifest;

import androidx.annotation.RequiresPermission;

import com.HJMediasdk.utils.HJAudioListener;
import com.HJMediasdk.utils.HJLog;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class HJAudioCaptureCtrl
{
    private static final String TAG = HJAudioCaptureCtrl.class.getSimpleName();

    public static final int AV_SAMPLE_FMT_S16 = 1;
    public static final int HJAudioListenerFlag_PCM = 0x01;

    private final Object m_sync_obj = new Object();

    private WeakReference<HJAudioListener.UI> m_audioListenerUIWtr;
    private HJAudioCapture m_cap = null;
    private CapThreadPool m_cap_thread = null;
    private int m_nSampleRate = 44100;
    private int m_nChannes = 1;
    private int m_nBitsPerSample = 16;

    public HJAudioCaptureCtrl()
    {
    }

    public void setAudioListener(HJAudioListener.UI listener)
    {
        m_audioListenerUIWtr = listener == null ? null : new WeakReference<>(listener);
    }

    public void registerAudioListener(HJAudioListener.UI listener)
    {
        setAudioListener(listener);
    }

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    public int init(int i_nSampleRate, int i_nChannels)
    {
        synchronized (m_sync_obj)
        {
            releaseLocked();

            HJAudioCapture audioCapture = new HJAudioCapture();
            try
            {
                int initRet = audioCapture.init(i_nSampleRate, i_nChannels);
                if (initRet < 0)
                {
                    HJLog.e(TAG, "audio capture init failed ret=" + initRet);
                    audioCapture.release();
                    return initRet;
                }

                int openRet = audioCapture.open();
                if (openRet < 0)
                {
                    HJLog.e(TAG, "audio capture open failed ret=" + openRet);
                    audioCapture.release();
                    return openRet;
                }
            }
            catch (SecurityException e)
            {
                HJLog.e(TAG, "audio capture permission denied " + e.getMessage());
                audioCapture.release();
                return -1;
            }

            m_cap = audioCapture;
            m_nSampleRate = i_nSampleRate > 0 ? i_nSampleRate : 44100;
            m_nChannes = i_nChannels >= 2 ? 2 : 1;
            m_nBitsPerSample = 16;

            m_cap_thread = new CapThreadPool(true, audioCapture.getPCMSize());
            m_cap_thread.start();
            return 0;
        }
    }

    public void release()
    {
        synchronized (m_sync_obj)
        {
            releaseLocked();
        }
    }

    private void releaseLocked()
    {
        CapThreadPool capThreadPool = m_cap_thread;
        m_cap_thread = null;
        if (capThreadPool != null)
        {
            capThreadPool.join_cap();
        }

        HJAudioCapture audioCapture = m_cap;
        m_cap = null;
        if (audioCapture != null)
        {
            audioCapture.release();
        }

        if (capThreadPool != null && capThreadPool != Thread.currentThread())
        {
            capThreadPool.interrupt();
            try
            {
                capThreadPool.join();
            }
            catch (InterruptedException e)
            {
                Thread.currentThread().interrupt();
            }
        }
    }

    public class CapThreadPool extends Thread
    {
        private boolean m_bStart = false;
        private boolean m_bIsQuit = false;
        private boolean m_bIsEof = false;
        private byte[] m_pcm_data = null;
        private int m_pcmSize = 0;
        private int m_samplesByte = 2;
        private int m_samples = 1024;
        private boolean m_bIsReadErr = false;
        private long m_err_aac_idx = 0;
        private long m_err_start_time = 0;
        private ByteBuffer mBuffer = null;
        private int mBufferSize = 0;

        public CapThreadPool(boolean i_bIsJavaCap, int pcmSize)
        {
            m_pcmSize = pcmSize;
            m_pcm_data = new byte[m_pcmSize];

            m_samples = 1024;
            if (m_nChannes > 0)
            {
                m_samples = m_pcmSize / (m_nChannes * m_samplesByte);
            }
        }

        public boolean is_start()
        {
            return m_bStart;
        }

        public void join_cap()
        {
            m_bIsQuit = true;
        }

        public boolean is_eof()
        {
            return m_bIsEof;
        }

        private int read(byte[] buf, int i_nSize)
        {
            int nRetSize = 0;
            do
            {
                if (m_bIsReadErr)
                {
                    nRetSize = i_nSize;
                    for (int i = 0; i < i_nSize; i++)
                    {
                        buf[i] = 0;
                    }

                    long aacTime = m_err_aac_idx * 1024L * 1000L / Math.max(1, m_nSampleRate);
                    long sysTime = System.currentTimeMillis() - m_err_start_time;
                    long diff = aacTime - sysTime;
                    m_err_aac_idx++;

                    if (diff > 0)
                    {
                        try
                        {
                            Thread.sleep(diff);
                        }
                        catch (InterruptedException e)
                        {
                            Thread.currentThread().interrupt();
                            nRetSize = -1;
                            break;
                        }
                    }
                }
                else
                {
                    HJAudioCapture audioCapture = m_cap;
                    if (audioCapture == null)
                    {
                        nRetSize = -1;
                        break;
                    }

                    nRetSize = audioCapture.read(buf, i_nSize);
                    if (nRetSize < 0)
                    {
                        HJLog.i(TAG, "pcm read fail");
                        m_bIsReadErr = true;
                        m_err_aac_idx = 1;
                        m_err_start_time = System.currentTimeMillis();
                        nRetSize = i_nSize;
                        for (int i = 0; i < i_nSize; i++)
                        {
                            buf[i] = 0;
                        }
                    }
                }

            } while (false);
            return nRetSize;
        }

        @Override
        public void run()
        {
            int nRetSize = 0;
            m_bStart = true;

            while (!m_bIsQuit)
            {
                nRetSize = read(m_pcm_data, m_pcmSize);
                if (nRetSize < 0)
                {
                    if (Thread.currentThread().isInterrupted() || m_bIsQuit)
                    {
                            break;
                    }
                    continue;
                }

                HJAudioListener.UI listener = m_audioListenerUIWtr == null ? null : m_audioListenerUIWtr.get();
                if (listener != null)
                {
                    if (mBuffer == null || mBufferSize < nRetSize)
                    {
                        mBuffer = ByteBuffer.allocateDirect(nRetSize).order(ByteOrder.nativeOrder());
                        mBufferSize = nRetSize;
                    }

                    mBuffer.clear();
                    mBuffer.put(m_pcm_data, 0, nRetSize);
                    mBuffer.position(0);
                    mBuffer.limit(nRetSize);

                    int ret = listener.notify(m_nSampleRate,
                                                  m_nChannes,
                                                  AV_SAMPLE_FMT_S16,
                                                  m_samplesByte,
                                                  mBuffer.duplicate(),
                                                  HJAudioListenerFlag_PCM);
                    if (ret < 0)
                    {
                        HJLog.e(TAG, "audio listener notify failed ret=" + ret);
                    }
                }

                try
                {
                    Thread.sleep(2L);
                }
                catch (InterruptedException e)
                {
                    Thread.currentThread().interrupt();
                    break;
                }
            }

            m_bIsEof = true;
        }
    }
}
