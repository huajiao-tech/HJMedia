package com.HJMediasdk.utils;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class HJBaseThread
{
	private static final String TAG = HJBaseThread.class.getSimpleName();
	
	private InnerThread mThread = null;

	private final static int MSG_SYNC = 1;
	private final static int MSG_ASYNC = 0;

	private final static int SYNC_FLAG = 0x1; 
    
	public HJBaseThread()
	{
		
	}
	
	public int start()
	{
		int i_err = 0;
		do
		{
			if (mThread != null) {
				i_err = -1;
				break;
			}
			mThread = new InnerThread();
			mThread.start();

			while (!mThread.is_started()) {
				try {
					Thread.sleep(1);
				} catch (InterruptedException e) {
					e.printStackTrace();
					break;
				}
			}
		} while (false);
		return i_err;
	}
	
	public void AsyncQueueEvent(int msgId, Runnable r) 
	{
		if (mThread != null) {
			mThread.AsyncQueueEvent(msgId, r);
		}
	}
	
	public void AsyncQueueClearEvent(int msgId, Runnable r)
	{
		if (mThread != null) {
			mThread.AsyncQueueClearEvent(msgId, r);
		}
	}
	
	public void SyncQueueEvent(int msgId, Runnable r) 
	{
		if (mThread != null) {
			mThread.SyncQueueEvent(msgId, r);
		}
	}

	public void clearAllMsgs()
	{
		if (mThread != null)
		{
			mThread.clearAllMsgs();
		}
	}

	public Handler getHandler()
	{
		Handler handler = null;
		if (mThread != null)
		{
			handler = mThread.getHandler();
		}
		return handler;
	}

	public void release()
	{
		if (mThread != null) {
			mThread.msg_quit();

			try {
				mThread.interrupt();
				mThread.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			HJLog.i(TAG, "thread join end");
			mThread = null;
		}
	}
		
	public class InnerThread extends Thread
	{
		private static final String TAG = "InnerThread";
    	
		private boolean m_bIsStarted = false;
		
		private Handle_ex m_handler = null;
		private boolean m_bIsQuit = false;
		
		public boolean is_started()
		{
			return m_bIsStarted;
		}
		public Handler getHandler(){return m_handler;}
		public void msg_quit()
		{
			m_bIsQuit = true;
			if (m_handler != null)
			{
				m_handler.getLooper().quit();
			}			
		}

		public void clearAllMsgs()
		{
			m_handler.removeCallbacksAndMessages(null);
		}
		
		public void AsyncQueueClearEvent(int msgId, Runnable r)
		{
			m_handler.removeMessages(msgId);
			if (r != null) {
				Message msg = m_handler.obtainMessage();
				msg.what = msgId;
				msg.obj = (Object) r;
				msg.arg1 = MSG_ASYNC;
				m_handler.sendMessage(msg);
			}
		}
		
    	public void AsyncQueueEvent(int msgId, Runnable r) 
    	{ 	
			Message msg = m_handler.obtainMessage();
			msg.what = msgId;	
			msg.obj  = (Object)r;
			msg.arg1 = MSG_ASYNC;
			m_handler.sendMessage(msg);
    	}

		class MsgEvent
		{
			Runnable r;
			int flag;
		}
    	
    	public void SyncQueueEvent(int msgId, Runnable r)
    	{
    		if (m_handler != null)
            {
				MsgEvent info = new MsgEvent();
				info.flag = 0;
				info.flag |= SYNC_FLAG;
				info.r = r;

            	Message msg = m_handler.obtainMessage();
    			msg.what = msgId;	
    			msg.obj  = (Object)info;
    			msg.arg1 = MSG_SYNC;
                //HJLog.i("test", "20170419 sync msg, msgId: " + msgId);
    			m_handler.sendMessage(msg);

            	while (!m_bIsQuit)
    			{
            		if ((info.flag & SYNC_FLAG) == 0)
            		{
            			//HJLog.i(TAG, "20170419 sync msg end, event queue empty");
            			break;
            		}

    				try 
        			{
        				Thread.sleep(1);
        			} 
        			catch (InterruptedException e) 
        			{
        				e.printStackTrace();
        			}
    			}           	
            }        		
    	}

		@Override
		public void run() 
	    {  	        
			Looper.prepare(); 
			Looper looper = Looper.myLooper();
			if (looper == null) {
				HJLog.e(TAG, "Looper is null");
				return;
			}
			m_handler = new Handle_ex(looper);
			
	        m_bIsStarted = true;
	        
	        Looper.loop();  
	                
	        HJLog.i(TAG, "20170427 Looper end!=====");
	    } 

		private class Handle_ex extends Handler
	    {
			Handle_ex(Looper looper)
			{
				super(looper);
			}
			
	    	@Override
			public void handleMessage(Message msg)
	        {  
				//HJLog.i(TAG, "20170706 handlemessage msg " + msg.what);
				Runnable r = null;

				if (msg.arg1 == MSG_SYNC)
				{
					MsgEvent info = (MsgEvent)msg.obj;
					if (info != null)
					{
						r = info.r;
						if (r != null)
						{
							r.run();
						}
						info.flag = 0;
					}
				}
				else
				{
					r = (Runnable)msg.obj;
					if (r != null)
					{
						r.run();
					}
				}
	         }  
	     }; 
	}
}
