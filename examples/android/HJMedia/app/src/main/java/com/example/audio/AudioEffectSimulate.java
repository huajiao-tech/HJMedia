package com.example.audio;

import com.HJMediasdk.utils.HJLog;

import java.nio.ByteBuffer;

public class AudioEffectSimulate {
    private static final String TAG = AudioEffectSimulate.class.getSimpleName();
    private Object m_sync_obj = new Object();
    private int m_effect_style = AudioEffect.ORIGINAL_SOUND;
    private AudioEffect mAudioEffectEx = null;
    private byte[] m_effect_buffer = null;

    public void release()
    {
        synchronized(m_sync_obj)
        {
            uninitEffect();
        }
    }
    public int audSetEffect(int i_nEffectVal, float param)
    {
        int i_err = 0;
        do
        {
            synchronized(m_sync_obj)
            {
                m_effect_style = i_nEffectVal;

                if (m_effect_style == AudioEffect.ORIGINAL_SOUND)
                {
                    return 0;
                }

                initEffect();

                if (i_nEffectVal > AudioEffect.V_DAMP)
                {
                    i_err = -1;
                    uninitEffect();
                    HJLog.e(TAG, "set effect err val " + i_nEffectVal);
                    break;
                }


                if (mAudioEffectEx != null)
                {
                    if (i_nEffectVal <= AudioEffect.ORIGINAL_SOUND)
                    {
                        mAudioEffectEx.setEffectParam(i_nEffectVal);
                        HJLog.i(TAG, "audioeffect set val " + i_nEffectVal);
                    }
                    else
                    {
                        String str = map_effect(i_nEffectVal);
                        if (!str.equals(""))
                        {
                            mAudioEffectEx.setEffectParamByName(str, param);
                            HJLog.i(TAG, "audioeffect set val " + i_nEffectVal + " param " + param);
                        }
                    }
                }
            }
        } while (false);
        return i_err;
    }

    public int doEffect(ByteBuffer buffer, int samples, int sampleBytes, int channels)
    {
        int ret = 0;
        do
        {
            synchronized(m_sync_obj)
            {
                if (m_effect_style == AudioEffect.ORIGINAL_SOUND)
                {
                    return 0;
                }

                if (buffer == null || samples <= 0 || sampleBytes <= 0 || channels <= 0)
                {
                    ret = -1;
                    break;
                }

                int dataSize = samples * sampleBytes * channels;
                if (dataSize <= 0 || buffer.remaining() < dataSize)
                {
                    ret = -1;
                    break;
                }

                if (m_effect_buffer == null || m_effect_buffer.length < dataSize)
                {
                    m_effect_buffer = new byte[dataSize];
                }

                ByteBuffer readBuffer = buffer.duplicate();
                readBuffer.get(m_effect_buffer, 0, dataSize);

                if (mAudioEffectEx != null)
                {
                    //long t0 = System.currentTimeMillis();//1-2ms
                    ret = mAudioEffectEx.doEffect(m_effect_buffer, samples, sampleBytes, channels);
                    //LogDebug.i(TAG, "doEffect time diff " + (System.currentTimeMillis() - t0));
                    //test_out_pcm(m_effect_buffer, samples * sampleBytes * channels);
                    if (ret != samples)
                    {
                        ret = -1;
                        break;
                    }
                }

                ByteBuffer writeBuffer = buffer.duplicate();
                writeBuffer.put(m_effect_buffer, 0, dataSize);
            }
        } while (false);

        return ret;
    }

    private void initEffect()
    {
        if(mAudioEffectEx == null)
        {
            mAudioEffectEx = new AudioEffect();
            mAudioEffectEx.attachEffect();
            HJLog.i(TAG, "audioeffect initEffect");
        }
    }

    private void uninitEffect()
    {
        if(mAudioEffectEx != null)
        {
            mAudioEffectEx.detachEffect();
            mAudioEffectEx = null;
            HJLog.i(TAG, "audioeffect uninitEffect");
        }
    }
    private String map_effect(int i_nVal)
    {
        String str = "";
        switch(i_nVal)
        {
            case AudioEffect.V_ROOM_SIZE:
                str = AudioEffect.ROOM_SIZE;
                break;
            case AudioEffect.V_WET:
                str = AudioEffect.WET;
                break;
            case AudioEffect.V_DRY:
                str = AudioEffect.DRY;
                break;
            case AudioEffect.V_WIDTH:
                str = AudioEffect.WIDTH;
                break;
            case AudioEffect.V_DAMP:
                str = AudioEffect.DAMP;
                break;

            default:
                break;
        }
        return str;
    }

	

}
