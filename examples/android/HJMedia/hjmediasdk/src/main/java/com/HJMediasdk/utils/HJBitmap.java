package com.HJMediasdk.utils;

import android.graphics.Bitmap;

/**
 * Bitmap wrapper with explicit lifecycle control.
 * Java has no deterministic destructor like C++, so use close() explicitly.
 */
public class HJBitmap implements AutoCloseable {

    private Bitmap mBitmap;

    public HJBitmap(Bitmap bitmap) {
        mBitmap = bitmap;
    }

    public Bitmap get() {
        return mBitmap;
    }

    public boolean isValid() {
        return mBitmap != null && !mBitmap.isRecycled();
    }

    @Override
    public void close() {
        if (mBitmap != null && !mBitmap.isRecycled()) {
            mBitmap.recycle();
        }
        mBitmap = null;
    }
}

