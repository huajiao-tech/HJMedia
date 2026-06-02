package com.HJMediasdk.utils;

public interface HJAudioRawPcmListener
{
    int notify(int i_sampleRate,
               int i_channels,
               int i_sampleFmt,
               int i_bytesPerSample,
               long i_data,
               int i_nSize,
               int i_nFlag,
               long i_ptsMs);
}
