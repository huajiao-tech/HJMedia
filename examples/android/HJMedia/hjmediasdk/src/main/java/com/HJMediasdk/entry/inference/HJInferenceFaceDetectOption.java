package com.HJMediasdk.entry.inference;

import androidx.annotation.Keep;

@Keep
public class HJInferenceFaceDetectOption
{
    private static final String TAG = HJInferenceFaceDetectOption.class.getSimpleName();

    public float tnnFaceAlignThreshold = 0.75f;
    public int tnnFaceAlignMinFaceSize = 20;

    public float ncnnRetinaFaceProbThreshold = 0.8f;
    public float ncnnRetinaFaceNmsThreshold = 0.4f;
    public int ncnnRetinaFaceThreadNums = 1; // 1 or 2
    public int retinaFaceTargetSize = 320;
    public boolean ncnnRetinaFaceUseGPU = false;

    // Fit+padding gives better face rect/landmark quality in most cases.
    public boolean ncnnScrfdEqualScale = true;
    public int ncnnScrfdTargetSize = 320;
    public float ncnnScrfdProbThreshold = 0.3f;
    public float ncnnScrfdNmsThreshold = 0.45f;
    public int ncnnScrfdThreadNums = 1; // 1-2
    public boolean ncnnScrfdUseGPU = false;

    public int coreMLRetinaFaceComputeMode = 0;
    public int visionRectTargetSize = 0;
    public int visionRectComputeMode = 0;

    public HJInferenceFaceDetectOption()
    {
    }

}
