package com.HJMediasdk.entry.render;

import androidx.annotation.Keep;

import com.HJMediasdk.utils.HJLog;

import java.util.Vector;

@Keep
public class HJFacePointInfo
{
    private static final String TAG = HJFacePointInfo.class.getSimpleName();

    public HJFacePointInfo()
    {
    }

    public int width = 0;
    public int height = 0;
    public int faceCount = 0;
    public Vector<HJSingleFaceInfo> faces = new Vector<HJSingleFaceInfo>();

    public void setSize(int width, int height)
    {
        this.width = width;
        this.height = height;
    }
    public void clear()
    {
        faces.clear();
        faceCount = 0;
    }

    public void add(HJSingleFaceInfo face)
    {
        faces.add(face);
        faceCount++;
    }
    @Keep
    public static class HJSingleFaceInfo
    {
        public HJFaceRect rect;
        public Vector<HJFacePoint> points = new Vector<HJFacePoint>();

        public void setRect(float x, float y, float w, float h)
        {
            rect = new HJFaceRect(x, y, w, h);
        }
        public void add(float x, float y)
        {
            points.add(new HJFacePoint(x, y));
        }
        @Keep
        public static class HJFaceRect
        {
            public float x = 0.f;
            public float y = 0.f;
            public float w = 0.f;
            public float h = 0.f;

            public HJFaceRect(float x, float y, float w, float h)
            {
                this.x = x;
                this.y = y;
                this.w = w;
                this.h = h;
            }
        }
        @Keep
        public static class HJFacePoint
        {
            public float x = 0.f;
            public float y = 0.f;
            public HJFacePoint(float x, float y)
            {
                this.x = x;
                this.y = y;
            }
        }
    }
}