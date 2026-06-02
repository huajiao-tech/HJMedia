package com.HJMediasdk.audio;

import java.nio.ByteBuffer;



public interface HJAudioListener
{
    public static final int AAC_HEADER = 1;
    public static final int AAC_MDAT   = 2;

    int PCM_READ_ERR    = -1;
    int CAP_OTHER_ERR   = -2;

    int  onExtraReady(ByteBuffer extra, int i_nSize);
    int  onCapAAC(ByteBuffer outputFrame, int i_nSize, long i_nSysTime, int i_nFlag);
    int  onErr(int extra);
}