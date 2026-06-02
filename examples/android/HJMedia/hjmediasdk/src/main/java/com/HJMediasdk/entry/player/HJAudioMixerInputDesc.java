package com.HJMediasdk.entry.player;

public class HJAudioMixerInputDesc
{
    public String inputId = "";
    public int sampleRate = 48000;
    public int channels = 2;
    public int sampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
    public float volume = 1.0f;

    public HJAudioMixerInputDesc()
    {
    }

    public HJAudioMixerInputDesc(String inputId, int sampleRate, int channels, int sampleFmt)
    {
        this.inputId = inputId;
        this.sampleRate = sampleRate;
        this.channels = channels;
        this.sampleFmt = sampleFmt;
    }
}
