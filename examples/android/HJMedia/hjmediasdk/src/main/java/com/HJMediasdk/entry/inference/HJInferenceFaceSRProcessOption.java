package com.HJMediasdk.entry.inference;

import androidx.annotation.Keep;

@Keep
public class HJInferenceFaceSRProcessOption
{
    public static final int HJFaceSRProcessMode_Full = 0;
    public static final int HJFaceSRProcessMode_FaceOrigin = 1;
    public static final int HJFaceSRProcessMode_FaceScale = 2;
    public static final int HJFaceSRProcessMode_FullScale = 3;

    public int mode = HJFaceSRProcessMode_FaceScale;
    public int preScaleWidth = 0;
    public int preScaleHeight = 0;
    public int composeCanvasWidth = 0;
    public int composeCanvasHeight = 0;
    public boolean bFeather = true;
    public boolean bEnablePostSRDisplayResize = true;
    public boolean bMixedEnable = true;
    public float mixAlphaRatio = 0.8f;
    public int policy = 3;

    public HJInferenceFaceSRProcessOption()
    {
    }
}
