package com.HJMediasdk.entry.render;

import com.HJMediasdk.entry.HJBaseNativeInstance;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.lang.ref.WeakReference;

public class HJRenderFaceuEntryJni extends HJBaseNativeInstance implements HJBaseListener
{
    private static final String TAG = HJRenderFaceuEntryJni.class.getSimpleName();

    public static final int HJ_FACEU_NOTIFY_UNKNOWN  = -1;
    public static final int HJ_FACEU_NOTIFY_COMPLETE = 1;

    private WeakReference<HJBaseListener> m_listenerWtr;

    public HJRenderFaceuEntryJni()
    {
    }

    public int faceuInit(HJBaseListener i_listener) {
        m_listenerWtr = new WeakReference<>(i_listener);
        return nativeFaceuInit(this);
    }

    public int faceuAdd(String i_uniqueKey, String i_faceuUrl, boolean i_bDebugPoints)
    {
        return nativeFaceuAdd(i_uniqueKey, i_faceuUrl, i_bDebugPoints);
    }

    public int faceuRemove(String i_uniqueKey)
    {
        return nativeFaceuRemove(i_uniqueKey);
    }

    public int faceuRemoveAll()
    {
        return nativeFaceuRemoveAll();
    }

    public int faceuRender(HJFacePointInfo i_facePointInfo)
    {
        return nativeFaceuRender(i_facePointInfo);
    }

    public void faceuDone()
    {
        nativeFaceuDone();
    }

    @Override
    public int onNotify(final int id, final long value, final String desc)
    {
        int i_err = 0;
        HJLog.i(TAG, "onNotify id:" + id + " value:" + value + " desc:" + desc);

        HJBaseListener listener = m_listenerWtr.get();
        if (listener != null)
        {
            i_err = listener.onNotify(id, value, desc);
        }
        else
        {
            i_err = -1;
            HJLog.e(TAG, "listener is null");
        }
        return i_err;
    }

    private native int nativeFaceuInit(HJBaseListener i_listener);
    private native int nativeFaceuAdd(String i_uniqueKey, String i_faceuUrl, boolean i_bDebugPoints);
    private native int nativeFaceuRemove(String i_uniqueKey);
    private native int nativeFaceuRemoveAll();
    private native int nativeFaceuRender(HJFacePointInfo i_facePointInfo);
    private native void nativeFaceuDone();
}