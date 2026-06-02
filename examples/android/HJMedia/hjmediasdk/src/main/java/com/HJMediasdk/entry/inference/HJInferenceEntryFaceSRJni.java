package com.HJMediasdk.entry.inference;

import android.graphics.Bitmap;

import com.HJMediasdk.entry.HJBaseNativeInstance;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.lang.ref.WeakReference;

public class HJInferenceEntryFaceSRJni extends HJBaseNativeInstance
{
    private static final String TAG = HJInferenceEntryFaceSRJni.class.getSimpleName();

    private WeakReference<HJBaseListener> m_listenerWtr;

    public int getFaceDetectTime() {
        return m_faceDetectTime;
    }

    public int getSRTime() {
        return m_srTime;
    }



    private int m_faceDetectTime = 0;
    private int m_srTime = 0;

    public int getSROriginX() {
        return m_srOriginX;
    }

    public int getSROriginY() {
        return m_srOriginY;
    }

    public int getSROriginW() {
        return m_srOriginW;
    }

    public int getSROriginH() {
        return m_srOriginH;
    }

    public int getSROriginScaleW() {
        return m_srOriginScaleW;
    }

    public int getSROriginScaleH() {
        return m_srOriginScaleH;
    }

    public int getSRPadLeft() {
        return m_srPadLeft;
    }

    public int getSRPadRight() {
        return m_srPadRight;
    }

    public int getSRPadTop() {
        return m_srPadTop;
    }

    public int getSRPadBottom() {
        return m_srPadBottom;
    }

    public int getSRTargetDisplayW() {
        return m_srTargetDisplayW;
    }

    public int getSRTargetDisplayH() {
        return m_srTargetDisplayH;
    }

    private int m_srOriginX = 0;
    private int m_srOriginY = 0;
    private int m_srOriginW = 0;
    private int m_srOriginH = 0;
    private int m_srOriginScaleW = 0;
    private int m_srOriginScaleH = 0;
    private int m_srTargetDisplayW = 0;
    private int m_srTargetDisplayH = 0;
    private int m_srPadLeft = 0;
    private int m_srPadRight = 0;
    private int m_srPadTop = 0;
    private int m_srPadBottom = 0;

    public HJInferenceEntryFaceSRJni()
    {
    }
    public int initFaceSR(String i_modelPath, int i_faceDetectType, HJInferenceFaceDetectOption i_faceDetectOption, int i_srType, HJInferenceSROption i_srOption)
    {
        return nativeInitFaceSR(i_modelPath, i_faceDetectType, i_faceDetectOption, i_srType, i_srOption);
    }

    public void done()
    {
        nativeDone();
    }

    public int process(Bitmap i_bitmap, HJInferenceFaceSRProcessOption i_processOption)
    {
        return nativeProcess(i_bitmap, i_processOption);
    }

    public Bitmap getLastSRBitmap()
    {
        return nativeGetLastSRBitmap();
    }

    public int processTexture(int inputTextureId, int inputWidth, int inputHeight)
    {
        return nativeProcessTexture(inputTextureId, inputWidth, inputHeight);
    }

    public int getLastSRTextureId()
    {
        return nativeGetLastSRTextureId();
    }

    public int getLastSRTextureWidth()
    {
        return nativeGetLastSRTextureWidth();
    }

    public int getLastSRTextureHeight()
    {
        return nativeGetLastSRTextureHeight();
    }

    public void release()
    {
        done();
    }

    private native int nativeInitFaceSR(String i_modelPath, int i_faceDetectType, HJInferenceFaceDetectOption i_faceDetectOption, int i_srType, HJInferenceSROption i_srOption);
    private native int nativeProcess(Bitmap i_bitmap, HJInferenceFaceSRProcessOption i_processOption);
    private native int nativeProcessTexture(int inputTextureId, int inputWidth, int inputHeight);
    private native Bitmap nativeGetLastSRBitmap();
    private native int nativeGetLastSRTextureId();
    private native int nativeGetLastSRTextureWidth();
    private native int nativeGetLastSRTextureHeight();
    private native void nativeDone();
}
