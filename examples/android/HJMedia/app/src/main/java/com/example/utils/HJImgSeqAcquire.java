package com.example.utils;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;

import com.HJMediasdk.utils.HJAsyncCache;
import com.HJMediasdk.utils.HJBitmap;
import com.HJMediasdk.utils.HJTimerThread;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/**
 * Acquire image sequence bitmaps from a directory.
 * Reads all image files, decodes to Bitmap, and pushes into queue.
 */
public class HJImgSeqAcquire {

    private static final List<String> IMAGE_EXT = Arrays.asList(
            ".jpg", ".jpeg", ".png", ".bmp", ".webp"
    );

    private final Object mStateLock = new Object();
    private final HJAsyncCache<HJBitmap> mBitmapQueue = new HJAsyncCache<>();
    private final HJTimerThread mTimerThread = new HJTimerThread();

    private final BitmapFactory.Options mDecodeOptions = new BitmapFactory.Options();
    private List<File> mImageFiles = new ArrayList<>();
    private int mDecodeIndex = 0;
    private boolean mRunning = false;
    private boolean mFinished = false;
    private boolean mLoop = true;

    public HJImgSeqAcquire() {
        mDecodeOptions.inPreferredConfig = Bitmap.Config.ARGB_8888;
    }

    public int start(String directoryPath, long periodMs, boolean i_bLoop) {
        if (TextUtils.isEmpty(directoryPath)) {
            return -1;
        }
        stop();

        List<File> files = listImageFiles(directoryPath);
        if (files.isEmpty()) {
            return -2;
        }

        synchronized (mStateLock) {
            mImageFiles = files;
            mDecodeIndex = 0;
            mFinished = false;
            mRunning = true;
            mLoop = i_bLoop;
        }

        int ret = mTimerThread.start();
        if (ret < 0) {
            synchronized (mStateLock) {
                mRunning = false;
            }
            return ret;
        }
        long usePeriod = Math.max(1, periodMs);
        mTimerThread.startPeriodic(0, usePeriod, this::decodeNextAndEnqueue);
        return 0;
    }

    public void stop() {
        synchronized (mStateLock) {
            mRunning = false;
            mFinished = true;
            mDecodeIndex = 0;
            mImageFiles.clear();
        }
        mTimerThread.release();
        clearQueueAndRecycle();
    }

    public HJBitmap acquire() {
        return mBitmapQueue.acquire();
    }

    public boolean recovery(HJBitmap bitmap) {
        return mBitmapQueue.recovery(bitmap);
    }

    public int size() {
        return mBitmapQueue.size();
    }

    public boolean isFinished() {
        synchronized (mStateLock) {
            return mFinished;
        }
    }

    private void decodeNextAndEnqueue() {
        File fileToDecode;
        synchronized (mStateLock) {
            if (!mRunning) {
                return;
            }
            if (mDecodeIndex >= mImageFiles.size()) {
                if (mLoop) {
                    mDecodeIndex = 0;
                } else {
                    mFinished = true;
                    mRunning = false;
                    mTimerThread.stopTimerOnly();
                    return;
                }
            }
            fileToDecode = mImageFiles.get(mDecodeIndex++);
        }

        Bitmap bmp = BitmapFactory.decodeFile(fileToDecode.getAbsolutePath(), mDecodeOptions);
        if (bmp != null) {
            mBitmapQueue.enqueue(new HJBitmap(bmp));
        }
    }

    private void clearQueueAndRecycle() {
        HJBitmap bmp;
        while ((bmp = mBitmapQueue.acquire()) != null) {
            bmp.close();
        }
    }

    private List<File> listImageFiles(String directoryPath) {
        File dir = new File(directoryPath);
        if (!dir.exists() || !dir.isDirectory()) {
            return new ArrayList<>();
        }
        File[] files = dir.listFiles();
        if (files == null || files.length == 0) {
            return new ArrayList<>();
        }

        List<File> out = new ArrayList<>();
        for (File f : files) {
            if (f == null || !f.isFile()) {
                continue;
            }
            String name = f.getName().toLowerCase(Locale.US);
            for (String ext : IMAGE_EXT) {
                if (name.endsWith(ext)) {
                    out.add(f);
                    break;
                }
            }
        }
        out.sort((a, b) -> naturalCompare(a.getName(), b.getName()));
        return out;
    }

    private static int naturalCompare(String a, String b) {
        if (a == null && b == null) {
            return 0;
        }
        if (a == null) {
            return -1;
        }
        if (b == null) {
            return 1;
        }

        int ia = 0;
        int ib = 0;
        final int na = a.length();
        final int nb = b.length();
        while (ia < na && ib < nb) {
            char ca = a.charAt(ia);
            char cb = b.charAt(ib);
            boolean da = Character.isDigit(ca);
            boolean db = Character.isDigit(cb);

            if (da && db) {
                int sa = ia;
                int sb = ib;
                while (ia < na && Character.isDigit(a.charAt(ia))) {
                    ia++;
                }
                while (ib < nb && Character.isDigit(b.charAt(ib))) {
                    ib++;
                }

                String ra = a.substring(sa, ia);
                String rb = b.substring(sb, ib);

                String ta = trimLeadingZeros(ra);
                String tb = trimLeadingZeros(rb);

                if (ta.length() != tb.length()) {
                    return ta.length() < tb.length() ? -1 : 1;
                }
                int numCmp = ta.compareTo(tb);
                if (numCmp != 0) {
                    return numCmp;
                }
                // same numeric value: shorter raw digits first (e.g. 2 before 02)
                if (ra.length() != rb.length()) {
                    return ra.length() < rb.length() ? -1 : 1;
                }
            } else {
                char la = Character.toLowerCase(ca);
                char lb = Character.toLowerCase(cb);
                if (la != lb) {
                    return la < lb ? -1 : 1;
                }
                ia++;
                ib++;
            }
        }
        if (ia == na && ib == nb) {
            return 0;
        }
        return ia == na ? -1 : 1;
    }

    private static String trimLeadingZeros(String s) {
        int i = 0;
        while (i < s.length() - 1 && s.charAt(i) == '0') {
            i++;
        }
        return s.substring(i);
    }
}
