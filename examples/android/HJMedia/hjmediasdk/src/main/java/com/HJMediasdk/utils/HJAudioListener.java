package com.HJMediasdk.utils;

import java.nio.ByteBuffer;

public interface HJAudioListener
{
    int notify(int i_sampleRate, int i_channels, int i_sampleFmt, int i_bytesPerSample, long i_data, int i_nSize, int i_nFlag);

    interface UI
    {
        int notify(int i_sampleRate, int i_channels, int i_sampleFmt, int i_bytesPerSample, ByteBuffer i_buffer, int i_nFlag);
    }
}
