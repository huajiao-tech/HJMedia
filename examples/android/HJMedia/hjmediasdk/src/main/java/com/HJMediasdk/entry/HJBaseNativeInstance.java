package com.HJMediasdk.entry;

public class HJBaseNativeInstance
{
    private final static String TAG = HJBaseNativeInstance.class.getSimpleName();

    public long mHandle = 0;

    protected static int m_instanceIdx = 0;

    public boolean isHandleValid()
    {
        //HJLog.i(TAG, "this handle " + mHandle + " hashcode " + this.hashCode());
        return (mHandle != 0);
    }

    public long getHandle()
    {
        return mHandle;
    }

    protected static int getInstanceIdx()
    {
        return m_instanceIdx++;
    }
}
