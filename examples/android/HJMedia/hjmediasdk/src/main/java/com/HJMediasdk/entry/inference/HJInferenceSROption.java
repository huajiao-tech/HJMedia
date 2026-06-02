package com.HJMediasdk.entry.inference;

import androidx.annotation.Keep;

@Keep
public class HJInferenceSROption
{
    private static final String TAG = HJInferenceSROption.class.getSimpleName();

    public String ncnnRealESRGANType = "realesr-general-x4v3";
    public float ncnnRealESRGANDenose = 0.5f;
    public String ncnnRealCUGANType = "conservative";
    public boolean ncnnUseGPU = true;
    public int ncnnThreadNums = 1;
    public int ncnnScale = 2;
    public float textureFsrDenoiseStrength = 0.2f;
    public float textureFsrSharpness = 1.0f;
    public boolean textureFsrEnableExtraSharpen = true;
    public float textureFsrExtraSharpenBoost = 0.55f;
    public int textureFsrScale = 2;

    public HJInferenceSROption()
    {
    }
    
}
