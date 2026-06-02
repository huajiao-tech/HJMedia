package com.HJMediasdk.utils;

import java.io.File;
import android.util.Log;


public class HJLog
{
	private static boolean m_bEnable = false;
	private static String TAG = HJLog.class.getSimpleName();
    
	public static void setEnable(boolean i_bEnable)
	{
        m_bEnable = i_bEnable;
	}

    private static boolean IsEnable()
    {
        return m_bEnable;
    }
	public static boolean BoolReadFlagFromFd(String str)
	{	
		File file = new File(str);
		return file.exists();
	}
	
	public static int IntReadFlagFromFd(String str)
	{	
		int nFlag = 0;
		File file = new File(str);
		if (file.exists())
		{
			nFlag = 1;
		}
		else 
		{
			nFlag = 0;
		}
		return nFlag;
	}	

	private static String pri_add_tid(String msg)
	{
		return "tid:" + Thread.currentThread().getId() + " " + Thread.currentThread().getName() + " " + msg;
	}

    public static int v(String tag, String msg) 
    {
    	if (!IsEnable())
    	{
    		return 0;
    	}

        return Log.v(tag, pri_add_tid(msg));
    }
    
    public static int d(String tag, String msg) 
    {
    	if (!IsEnable())
    	{
    		return 0;
    	}

        return Log.d(tag, pri_add_tid(msg));
    }
    
    public static int i(String tag, String msg) 
    {
    	if (!IsEnable())
    	{
    		return 0;
    	}

        return Log.i(tag, pri_add_tid(msg));
    }
    
    public static int w(String tag, String msg) 
    {
    	if (!IsEnable())
    	{
    		return 0;
    	}

        return Log.w(tag, pri_add_tid(msg));
    }

	public static int w(String tag, String msg, Exception exception)
	{
		if (!IsEnable())
		{
			return 0;
		}

		return Log.w(tag, pri_add_tid(msg), exception);
	}

    public static int e(String tag, String msg) 
    {
    	if (!IsEnable())
    	{
    		return 0;
    	}

        return Log.e(tag, pri_add_tid(msg));
    }
    
    
    
    private static String strPriority[];
    
    static 
    {
        strPriority = new String[8];
        strPriority[0] = "";
        strPriority[1] = "";
        strPriority[2] = "verbose=";
        strPriority[3] = "debug===";
        strPriority[4] = "info====";
        strPriority[5] = "warn====";
        strPriority[6] = "error===";
        strPriority[7] = "ASSERT==";
    }
}