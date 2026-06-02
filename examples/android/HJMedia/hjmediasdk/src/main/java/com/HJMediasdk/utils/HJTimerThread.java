package com.HJMediasdk.utils;

/**
 * Wrapper of HJBaseThread + HJBaseTimer.
 */
public class HJTimerThread {

    private static final int MSG_TIMER = 0x6A6A;

    private final HJBaseThread mThread = new HJBaseThread();
    private HJBaseTimer mTimer = null;
    private boolean mStarted = false;

    public int start() {
        if (mStarted) {
            return 0;
        }
        int ret = mThread.start();
        if (ret < 0) {
            return ret;
        }
        mStarted = true;
        return 0;
    }

    public void post(int msgId, Runnable runnable) {
        if (!mStarted || runnable == null) {
            return;
        }
        mThread.AsyncQueueEvent(msgId, runnable);
    }

    public void postClear(int msgId, Runnable runnable) {
        if (!mStarted || runnable == null) {
            return;
        }
        mThread.AsyncQueueClearEvent(msgId, runnable);
    }

    public void postSync(int msgId, Runnable runnable) {
        if (!mStarted || runnable == null) {
            return;
        }
        mThread.SyncQueueEvent(msgId, runnable);
    }

    public void startPeriodic(long delayMs, long periodMs, final Runnable runnable) {
        if (!mStarted || runnable == null || periodMs <= 0) {
            return;
        }
        stopTimerOnly();
        mTimer = new HJBaseTimer("HJTimerThreadTimer", true);
        mTimer.schedule(new HJBaseTimerTask() {
            @Override
            public void run() {
                mThread.AsyncQueueEvent(MSG_TIMER, runnable);
            }
        }, Math.max(0, delayMs), periodMs);
    }

    public void startOnce(long delayMs, final Runnable runnable) {
        if (!mStarted || runnable == null) {
            return;
        }
        stopTimerOnly();
        mTimer = new HJBaseTimer("HJTimerThreadTimerOnce", true);
        mTimer.schedule(new HJBaseTimerTask() {
            @Override
            public void run() {
                mThread.AsyncQueueEvent(MSG_TIMER, runnable);
            }
        }, Math.max(0, delayMs));
    }

    public void stopTimerOnly() {
        if (mTimer != null) {
            mTimer.release();
            mTimer = null;
        }
    }

    public void release() {
        stopTimerOnly();
        mThread.release();
        mStarted = false;
    }
}

