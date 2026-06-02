package com.HJMediasdk.entry.player;

import androidx.annotation.Keep;

@Keep
public class HJVideoEnhanceOption
{
    public float denoiseStrength = 1.0f;
    public float sharpness = 1.0f;
    public boolean enableExtraSharpen = true;
    public float extraSharpenBoost = 0.55f;
    public float match = 1.0f;
    public boolean enableDenoise = true;
    public boolean enableSR = true;

    public HJVideoEnhanceOption()
    {
    }
}

