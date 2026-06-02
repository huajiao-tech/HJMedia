package com.HJMediasdk.entry.player;

import java.util.ArrayList;
import java.util.List;

public class HJAudioMixerConfig
{
    public static final int OUT_TYPE_AAC = 0;
    public static final int OUT_TYPE_PCM = 1;

    public static final int AAC_TYPE_RAW = 0;
    public static final int AAC_TYPE_ADIF = 1;
    public static final int AAC_TYPE_ADTS = 2;

    public int outputSampleRate = 48000;
    public int outputChannels = 2;
    public int outputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
    public int maxInputs = 8;
    public int frameSamples = 1024;
    public int syncWindowMs = 42;
    public int lateThresholdMs = 150;
    public boolean enableCompand = true;
    public boolean enableLimiter = true;
    public int outType = OUT_TYPE_AAC;
    public int aacType = AAC_TYPE_RAW;
    public List<HJAudioMixerInputDesc> inputs = new ArrayList<>();
}
