package com.HJMediasdk.graphic.draw;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.HJMediasdk.utils.HJLog;

import java.io.File;

public class HJBitmapTexture
{	
	private static final String TAG = "BitmapTexture";
	private int mTextureID = -12345;
	private final int[] m_texture_id = new int[1];
	private final static int SHADER_STYLE = HJShaderCommon.SHADER_2D;
	private int m_width = 0;
	private int m_height = 0;
	public int getTextureId()
	{
		return mTextureID;
	}

	public int init(String i_url)
	{
		int i_err = 0;
		do {
            long t0 = System.currentTimeMillis();
			File file = new File(i_url);
			if (!file.exists())
			{
				i_err = -1;
				break;
			}

			mTextureID = HJShaderCommon.textureCreate(SHADER_STYLE, m_texture_id);
			if (mTextureID < 0)
			{
				i_err = -1;
				break;
			}
            long t1 = System.currentTimeMillis();
			Bitmap bitmap = BitmapFactory.decodeFile(i_url, null);
            long t2 = System.currentTimeMillis();
			i_err = HJShaderCommon.textureUpload(SHADER_STYLE, mTextureID, bitmap);
			if (i_err < 0)
			{
				break;
			}
            long t3 = System.currentTimeMillis();

            HJLog.i(TAG, "init time: " + (t1 - t0) + "ms" + " decode time: " + (t2 - t1) + "ms" + " texImage2D time: " + (t3 - t2) + "ms");

			m_width = bitmap.getWidth();
			m_height = bitmap.getHeight();
			bitmap.recycle();
		} while(false);
		return i_err;
	}
	public int getWidth()
	{
		return m_width;
	}
	public int getHeight()
	{
		return m_height;
	}

	public void release()
	{
		if (mTextureID > 0)
		{
            HJShaderCommon.textureDestory(m_texture_id);
		}
	}
	
}