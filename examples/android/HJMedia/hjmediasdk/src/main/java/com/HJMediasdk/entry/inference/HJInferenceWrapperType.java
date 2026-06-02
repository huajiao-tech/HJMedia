package com.HJMediasdk.entry.inference;

import androidx.annotation.Keep;

@Keep
public final class HJInferenceWrapperType
{
    private HJInferenceWrapperType()
    {
    }

    public static final class FaceDetect
    {
        private FaceDetect()
        {
        }

        public static final int HJFaceDetectWrapperType_Unknown = -1;
        public static final int HJFaceDetectWrapperType_TNNBLAZE = 0;
        public static final int HJFaceDetectWrapperType_TNNALIGNER = 1;
        public static final int HJFaceDetectWrapperType_NCNNRETINAFACE = 3;
        public static final int HJFaceDetectWrapperType_NCNNSCRFD = 4;
    }

    public static final class VideoSR
    {
        private VideoSR()
        {
        }

        public static final int HJVideoSRWrapperType_Unknown = -1;
        public static final int HJVideoSRWrapperType_NCNNREALESRGAN = 0;
        public static final int HJVideoSRWrapperType_NCNNREALCUGAN = 1;
        public static final int HJVideoSRWrapperType_NCNNPLAINUSR = 2;
        public static final int HJVideoSRWrapperType_TEXTUREFSR = 6;
    }
}
