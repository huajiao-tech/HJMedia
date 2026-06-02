package com.HJMediasdk.audio;

import java.nio.ByteBuffer;

import android.Manifest;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Build;

import androidx.annotation.RequiresPermission;

import com.HJMediasdk.utils.HJLog;

public class HJAudioCapture
{
	private static final String TAG = HJAudioCapture.class.getSimpleName();
	private int m_bufferSize = 0;
	private AudioRecord mAudioRecord = null;
	private long m_startTime = 0;		
	private int s_nSampleRate = 44100;
	private int s_nChannelStyle = AudioFormat.CHANNEL_IN_MONO;
	
	private boolean m_bAuthorityFlag = false;
	
	public HJAudioCapture()
	{	
		
	}

	private void pri_stop()
	{
		    	
    	if (mAudioRecord != null)
    	{
    		if (!m_bAuthorityFlag)
    		{
    			try
        		{
        			mAudioRecord.stop();
        		}
        		catch (IllegalStateException e)
        		{
        			e.printStackTrace();
        		}
    		}   		
    		mAudioRecord.release();
    		mAudioRecord = null;
    	}
	}
	public void reset()
	{
		pri_stop();
	}
	
    public void release()
    {
    	pri_stop();
    }
    
	@RequiresPermission(Manifest.permission.RECORD_AUDIO)
    public int init(int i_samplerate, int i_nInChannels)
	{
		int i_err = 0;

		do 
		{					
			if (Build.VERSION.SDK_INT < 16)
			{
				i_err = -1;
				HJLog.e(TAG, "version is low not support aac encode " + Build.VERSION.SDK_INT);
				break;
			}

			if (i_nInChannels == 1)
			{
				s_nChannelStyle = AudioFormat.CHANNEL_IN_MONO;
			}
			else
			{
				s_nChannelStyle = AudioFormat.CHANNEL_IN_STEREO;
			}
			s_nSampleRate = i_samplerate;
			
			m_bufferSize = AudioRecord.getMinBufferSize(s_nSampleRate, s_nChannelStyle, AudioFormat.ENCODING_PCM_16BIT)*2;
			if (m_bufferSize < 0)
			{
				i_err = -1;
				break;
			}
            HJLog.i(TAG, "audiorec s_nChannelStyle " + s_nChannelStyle);
			try
			{
				mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, s_nSampleRate, s_nChannelStyle, AudioFormat.ENCODING_PCM_16BIT, m_bufferSize);
				if (mAudioRecord == null)
				{
                    HJLog.e(TAG, "AudioRecord failed");
					i_err = -1;
					break;
				}
				if (mAudioRecord.getState() != AudioRecord.STATE_INITIALIZED)
				{
                    HJLog.e(TAG, "AudioRecord state is not STATE_INITIALIZED, no authority");
					i_err = 0;
					m_bAuthorityFlag = true;
					break;
				}
			}
			catch (IllegalArgumentException e)
			{
				e.printStackTrace();
				i_err = -1;
				break;
			}

		}while (false);
        HJLog.e(TAG, "audiorecroder init i_Err" + i_err);
		return i_err;
	}
	
	public int getPCMSize()
	{
		int nChannel;
		if (s_nChannelStyle == AudioFormat.CHANNEL_IN_MONO)
		{
			nChannel = 1;
		}
		else
		{
			nChannel = 2;
		}

		int size = 1024 * 2 * nChannel;
		HJLog.i(TAG, "audiorec get pcm size " + size);
		return size;
	}
	
	public int open() 
	{
		int i_err = 0;
		do
		{		
			if (m_bAuthorityFlag)
			{
                HJLog.i(TAG, "authority err not start recording");
				break;
			}
			
			m_startTime = System.currentTimeMillis();
			
			try
			{
                HJLog.e(TAG, "audiorecord audio startRecoding begin " + m_startTime);
				mAudioRecord.startRecording();
				long curTime = System.currentTimeMillis();
                HJLog.e(TAG, "audiorecord audio end timediff " + (curTime - m_startTime) + " curTime " + curTime);
			}
			catch (IllegalStateException e)
			{
				e.printStackTrace();
				i_err = -1;
				break;
			}

            HJLog.e(TAG, "the record time diff " + (System.currentTimeMillis() - m_startTime));
					
		} while (false);
		return i_err;			
	}
	
	public int read(byte[] buf, int size) 
	{
		int i_err = 0;
		do
		{		
			try
			{
				if (m_bAuthorityFlag)
				{
					i_err = -1;
					break;
				}
				
				i_err = mAudioRecord.read(buf, 0, size);
				
				//LogDebug.e(TAG, " read ret " + i_err);
				if (i_err < 0)
				{
                    HJLog.e(TAG, " read fail ret " + i_err);
				}			
			}
			catch (IllegalStateException e)
			{
				e.printStackTrace();
				i_err = -1;
				break;
			}
			
			//LogDebug.e(TAG, "the record time diff " + (System.currentTimeMillis() - m_startTime));
					
		} while (false);
		return i_err;			
	}
	
	public int read(ByteBuffer buf, int size) 
	{
		int i_err = 0;
		do
		{		
			try
			{
				if (m_bAuthorityFlag)
				{
					i_err = -1;
					break;
				}
				
				i_err = mAudioRecord.read(buf, size);
				
				//LogDebug.e(TAG, " read ret " + i_err);
				if (i_err < 0)
				{
                    HJLog.e(TAG, " read fail ret " + i_err);
				}			
			}
			catch (IllegalStateException e)
			{
				e.printStackTrace();
				i_err = -1;
				break;
			}
			
			//LogDebug.e(TAG, "the record time diff " + (System.currentTimeMillis() - m_startTime));
					
		} while (false);
		return i_err;			
	}
}
