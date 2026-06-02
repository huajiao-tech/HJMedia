package com.HJMediasdk.utils;

import java.util.ArrayDeque;
import java.util.Deque;

/**
 * Java version of src/utils/base/HJAsyncCache.h
 * Thread-safe cache queue.
 */
public class HJAsyncCache<T extends AutoCloseable> {
    private static final String TAG = HJAsyncCache.class.getSimpleName();
    private final Object mLock = new Object();
    private final Deque<T> mDeque = new ArrayDeque<>();

    public T acquire() {
        synchronized (mLock) {
            if (mDeque.isEmpty()) {
                return null;
            }
            return mDeque.pollFirst();
        }
    }

    /**
     * Keep C++ behavior: only push back when queue is empty.
     */
    public boolean recovery(T data) {
        synchronized (mLock) {
            if (mDeque.isEmpty()) {
                mDeque.addLast(data);
                return true;
            }
            return false;
        }
    }

    /**
     * Keep C++ behavior: clear old value, keep latest only.
     */
    public void enqueue(T data) {
        Deque<T> tmpDeque = null;
        synchronized (mLock) {
            if (!mDeque.isEmpty())
            {
                tmpDeque = new ArrayDeque<>(mDeque);
                mDeque.clear();
            }
            mDeque.addLast(data);
        }
        if (tmpDeque != null)
        {
            for (T item : tmpDeque)
            {
                try
                {
                    item.close();
                }
                catch (Exception e)
                {
                    HJLog.i(TAG,"exception");
                }

            }
            tmpDeque.clear();
        }
    }

    /**
     * Additional helper for normal queue usage (no clear).
     */
    public void enqueueKeep(T data) {
        synchronized (mLock) {
            mDeque.addLast(data);
        }
    }

    public int size() {
        synchronized (mLock) {
            return mDeque.size();
        }
    }

    public void clear() {
        synchronized (mLock) {
            mDeque.clear();
        }
    }
}

