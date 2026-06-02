package com.HJMediasdk.utils;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;

public class HJBaseToast
{
	public static void toast(Context context, String str, long time)
	{
		Looper looper = Looper.getMainLooper();
		Handler handler = new Handler(looper);
		final Toast toast_info = Toast.makeText(context, str, Toast.LENGTH_LONG);
		toast_info.show();
		handler.postDelayed(new Runnable() {
			@Override
			public void run() {
				toast_info.cancel();
			}
		}, time);
	}
}