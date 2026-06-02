package com.HJMediasdk.graphic.draw;

import static javax.microedition.khronos.opengles.GL10.GL_RGBA;
import static javax.microedition.khronos.opengles.GL10.GL_UNSIGNED_BYTE;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources.NotFoundException;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLES31;
import android.opengl.GLUtils;
import android.os.Environment;

import com.HJMediasdk.utils.HJLog;

@SuppressLint("InlinedApi")
public class HJShaderCommon
{
	private static final String TAG = HJShaderCommon.class.getSimpleName();

    public static final int SHADER_OES = 0;
    public static final int SHADER_2D  = 1;
    public static final int SHADER_CUBE = 2;

    private FloatBuffer mVerticalMir = null;   
    private FloatBuffer mNormal = null;
	public static final int FLOAT_SIZE_BYTES = 4;
    public static final  int TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
    public static final  int TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
    public static final  int TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;
    
    //1. use surfacetexture matrix, for example, camera, player(rotate)
    //2. FBO texture is positive, identity matrix;
    public static final int COORD_VERTEX_TEXTURE_MAP  = 1; 
    
    //texture is reversal(vertical flip), but not use matrix, for example, player(no rotate), bitmap
    public static final int COORD_VERTEX_TEXTURE_FLIP = 2; 
    
    private int m_style = COORD_VERTEX_TEXTURE_MAP;
    public HJShaderCommon(int style)
    {  	
    	m_style = style;
    	create_vertex();		
    }
    
    private static final float[] mNormalData = 
    {
        // X, Y, Z, U, V
        -1.0f, -1.0f, 0, 0.f, 0.f,
        1.0f, -1.0f, 0,  1.f, 0.f,
        -1.0f,  1.0f, 0, 0.f, 1.f,
        1.0f,  1.0f, 0,  1.f, 1.f,
    };
    
    private static final float[] mVerticalMirData = 
    {
       // X, Y, Z, U, V, vertex reflection(up-down) 
       -1.0f, -1.0f, 0,  0.f, 1.f,
       1.0f,  -1.0f, 0,  1.f, 1.f,
       -1.0f, 1.0f,  0,  0.f, 0.f,
       1.0f,  1.0f,  0,  1.f, 0.f,		
    }; 

    private void create_vertex()
    {
    	if (m_style == COORD_VERTEX_TEXTURE_MAP)
    	{
        	mNormal = ByteBuffer
    				.allocateDirect(
    						mNormalData.length * FLOAT_SIZE_BYTES)
    				.order(ByteOrder.nativeOrder()).asFloatBuffer();
    		mNormal.put(mNormalData).position(0);
    	}
    	else if (m_style == COORD_VERTEX_TEXTURE_FLIP)
    	{
    		mVerticalMir = ByteBuffer
    				.allocateDirect(mVerticalMirData.length * FLOAT_SIZE_BYTES)
    				.order(ByteOrder.nativeOrder()).asFloatBuffer();
    		mVerticalMir.put(mVerticalMirData).position(0); 	 
    	}	   	
    }
         
    public int setPointer(FloatBuffer buffer, int maPositionHandle, int maTextureHandle)
    {
    	int i_err = 0;
    	do
    	{
    		GLES20.glEnableVertexAttribArray(maPositionHandle);
		    i_err = HJGlUtil.checkGlError("glEnableVertexAttribArray maPositionHandle");
		    if (i_err < 0)
		    {
		    	break;
		    }
    		
    		buffer.position(TRIANGLE_VERTICES_DATA_POS_OFFSET); 		
		    GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false,
		    		TRIANGLE_VERTICES_DATA_STRIDE_BYTES, buffer);
		    
		    i_err = HJGlUtil.checkGlError("glVertexAttribPointer maPosition");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
		    
		    GLES20.glEnableVertexAttribArray(maTextureHandle);
		    i_err = HJGlUtil.checkGlError("glEnableVertexAttribArray maTextureHandle");
		    if (i_err < 0)
		    {
		    	break;
		    }

		    buffer.position(TRIANGLE_VERTICES_DATA_UV_OFFSET);
		    GLES20.glVertexAttribPointer(maTextureHandle, 2, GLES20.GL_FLOAT, false,
		    		TRIANGLE_VERTICES_DATA_STRIDE_BYTES, buffer);
		    
		    i_err = HJGlUtil.checkGlError("glVertexAttribPointer maTextureHandle");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
    	} while (false);
    	return i_err;
    }

    public FloatBuffer get_buffer()
    {
    	FloatBuffer buffer = null;
    	do
    	{
    		if (m_style == COORD_VERTEX_TEXTURE_MAP)
    		{
    			buffer = mNormal;
    		}
    		else if (m_style == COORD_VERTEX_TEXTURE_FLIP)
    		{
    			buffer = mVerticalMir;
    		}
    	} while (false);
    	return buffer;
    }
    
    public static int textureUpload(int shader_style, int id, int width, int height, IntBuffer buf)
    {
    	int i_err = 0;
    	do
    	{
    		//glBindTexture must before glTexImage2D;
			//gpu know upload the data to the dst texture;
    		i_err = bindTexture(shader_style, id);
    		if (i_err < 0)
    		{
    			break;
    		}
    		GLES20.glPixelStorei(GLES20.GL_UNPACK_ALIGNMENT, 1);   
			GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA,
					width, height, 0,
					GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE,
					buf);
    		
    	} while (false);
    	return i_err;
    }
    
    public static int textureUpload(int shader_style, int id, Bitmap bitmap)
    {
    	int i_err = 0;
    	do
    	{
    		//glBindTexture must before glTexImage2D;
			//gpu know upload the data to the dst texture;
    		i_err = bindTexture(shader_style, id);
    		if (i_err < 0)
    		{
    			break;
    		}
    		
    		try
    		{
    			GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);	 
    		} 
    		catch(NullPointerException e)
    		{
    			i_err = -1;
    			break;
    		}
    		catch (IllegalArgumentException e)
    		{
    			i_err = -1;
    			break;
    		}
            bindTexture(shader_style, 0);
    	} while (false);
    	return i_err;
    }

	private static int getTarget(int shader_style)
	{
		int target = 0;
		if (shader_style == SHADER_OES)
		{
			target = GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
		}
		else if (shader_style == SHADER_2D)
		{
			target = GLES20.GL_TEXTURE_2D;
		}
		else if (shader_style == SHADER_CUBE)
		{
			target = GLES20.GL_TEXTURE_CUBE_MAP;
		}
		return target;
	}

	/*
	must know the texture, bind the texture
	1. if use HWConstant.SHADER_2D, must after GLES20.glBindTexture(target, texture_id);
	2. if use HWConstant.SHADER_OES, must after surfacetexture.updateTexImage();
	 */
	public static void getTextureWidthAndHeight(int shader_style, int[] texDims)
	{
		int target = 0;
		if (shader_style == SHADER_OES)
		{
			target = GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
		}
		else if (shader_style == SHADER_2D)
		{
			target = GLES31.GL_TEXTURE_2D;
		}

		//long t0 = System.currentTimeMillis();
		GLES31.glGetTexLevelParameteriv(target, 0, GLES31.GL_TEXTURE_WIDTH, texDims, 0);
		GLES31.glGetTexLevelParameteriv(target, 0, GLES31.GL_TEXTURE_HEIGHT, texDims, 1);
		//HJLog.i(TAG, "get param wh time " + (System.currentTimeMillis() - t0));
	}

    public static int bindTexture(int shader_style, int id)
    {
    	int i_err = 0;
    	int target = getTarget(shader_style);
        GLES20.glBindTexture(target, id); 
        i_err = HJGlUtil.checkGlError("glBindTexture 2d");
        return i_err;
    }

	public static void viewport(int x, int y, int width, int height) {
		GLES20.glViewport(x, y, width, height);
	}

	public static void clear(float red, float green, float blue, float alpha) {
		GLES20.glClearColor(red, green, blue, alpha);
		GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT/* | GLES20.GL_DEPTH_BUFFER_BIT*/);
	}
    public static void textureDestory(int[] texture)
    {
        GLES20.glDeleteTextures(1, texture, 0);
    }
    public static int textureCreate(int shader_style, int[] texture)
    {
    	int i_err = 0;
    	int id = 0;
    	int target = getTarget(shader_style);
    	
        GLES20.glGenTextures(1, texture, 0);  
        
        i_err = HJGlUtil.checkGlError("glGenTextures");
        if (i_err < 0)
        {
        	id = -1;
        }
        
        id = texture[0];    

        GLES20.glBindTexture(target, id); 
        i_err = HJGlUtil.checkGlError("glBindTexture 2d");
        if (i_err < 0)
        {
        	id = -1;
        }

        GLES20.glTexParameterf(target, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(target, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(target, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(target, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

        GLES20.glBindTexture(target, 0);

        return id;
    }

    public int release()
    {
    	mNormal = null;
    	
    	return 0;
    }

    //left_bottom(x1,y1), right_top(x2,y2);
    public void set_user_array(float x1, float y1, float x2, float y2)
    {    	
    	mVerticalMir.put(5 * 0 + 3, x1);
    	mVerticalMir.put(5 * 0 + 4, y2);
    	
    	mVerticalMir.put(5 * 1 + 3, x2);
    	mVerticalMir.put(5 * 1 + 4, y2);    	
    	
    	mVerticalMir.put(5 * 2 + 3, x1);
    	mVerticalMir.put(5 * 2 + 4, y1);
    	
    	mVerticalMir.put(5 * 3 + 3, x2);
    	mVerticalMir.put(5 * 3 + 4, y1);   
    	
    	mVerticalMir.position(0);
    }
    
    
    public static String s_query_str_from_raw(Context context, int id)
	{
		String str = "";
		do
		{
			int len = 0;
			
			if (context == null)
			{
				break;
			}
			
			try 
			{
				InputStream in = context.getResources().openRawResource(id);
				if (in == null)
				{
					break;
				}
				len = in.available();
				if (len <= 0)
				{
					break;
				}
				byte b[] = new byte[len];
				str = in.read(b) == len ? new String(b, "UTF-8") : null;
			} 
			catch (NotFoundException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			catch (IOException e) 
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		} while (false);
		
		return str;
	}

    public static final int SCALE_CROP = 0;
    public static final int SCALE_FIT  = 1;
    public static final int SCALE_FULL = 2;
    public static final int SCALE_ORG  = 3;
    
    public static void s_get_window_scale(int i_scale_style, 
			int i_nViewW,
			int i_nViewH, 
			int i_nRealW, 
			int i_nRealH,
			PointF o_pt) 
	{
		int width  = i_nRealW;
		int height = i_nRealH;

		// calculate pixel aspect ration
		int dw = i_nViewW;
		int dh = i_nViewH;

		// calculate aspect ratio
		double sar = (double) width  / (double) height;
		// calculate display aspect ratio
		double dar = (double) dw / (double) dh;

		switch (i_scale_style) {
		case SCALE_CROP:
			if (dar < sar) {
				dw = (int) (dh * sar);
			} else {
				dh = (int) (dw / sar);
			}
			break;

		case SCALE_FIT:
			if (dar < sar) {
				dh = (int) (dw / sar);
			} else {
				dw = (int) (dh * sar);
			}
			break;

		case SCALE_FULL:
			dw = i_nViewW;
			dh = i_nViewH;
			break;
		case SCALE_ORG:
			dw = width;
			dh = height;
			break;

		default:
			if (dar < sar) {
				dw = (int) (dh * sar);
			} else {
				dh = (int) (dw / sar);
			}
			break;
		}

		o_pt.x = (float) dw / (float) i_nViewW;
		o_pt.y = (float) dh / (float) i_nViewH;
	}
    
    public static void s_get_window_scale(int i_scale_style, 
			int i_nViewW,
			int i_nViewH, 
			float i_realScale,
			PointF o_pt) 
	{

		// calculate pixel aspect ration
		int dw = i_nViewW;
		int dh = i_nViewH;

		// calculate aspect ratio
		double sar = (double)i_realScale;
		// calculate display aspect ratio
		double dar = (double) dw / (double) dh;

		switch (i_scale_style) {
		case SCALE_CROP:
			if (dar < sar) {
				dw = (int) (dh * sar);
			} else {
				dh = (int) (dw / sar);
			}
			break;

		case SCALE_FIT:
			if (dar < sar) {
				dh = (int) (dw / sar);
			} else {
				dw = (int) (dh * sar);
			}
			break;

		case SCALE_FULL:
			dw = i_nViewW;
			dh = i_nViewH;
			break;

		default:
			if (dar < sar) {
				dw = (int) (dh * sar);
			} else {
				dh = (int) (dw / sar);
			}
			break;
		}

		o_pt.x = (float) dw / (float) i_nViewW;
		o_pt.y = (float) dh / (float) i_nViewH;
	}
    
    public static float s_getRotation(double dx, double dy) 
	{
		if (dx == 0.0 || dy == 0.0)
		{
			HJLog.i(TAG, "getRotation == 0dx " + dx + " dy " + dy);
		}
		
		if ((dx + 0.001) == 0.0)
		{
			HJLog.i(TAG, "getRotation (dx + 0.001) == 0.0 ");
			dx += 0.002;
		}
		else
		{
			dx += 0.001;
		}
		
		if ((dy + 0.001) == 0.0)
		{
			HJLog.i(TAG, "getRotation (dy + 0.001) == 0.0 ");
			dy += 0.002;
		}
		else
		{
			dy += 0.001;
		}
		
		double radians = Math.atan2(Math.abs(dy), Math.abs(dx));
		double degrees = Math.toDegrees(radians);

		if (dx > 0 && dy > 0)
		{
			//degrees = degrees;
		} 
		else if (dx < 0 && dy > 0)
		{
			degrees = 180 - degrees;
		} 
		else if (dx < 0 && dy < 0)
		{
			degrees = 180 + degrees;
		}
		else if (dx > 0 && dy < 0)
		{
			degrees = 360 - degrees;
		}
		return (float) degrees;
	}
    
    public static void s_rotate_point(PointF[] i_src, 
    		PointF i_anchor,
    		double i_nAngle,  
    		PointF i_tmpPot,
    		PointF[] i_dst)
	{
		float r = 0.f;
		double degrees = 0.0;
		double d = 0.0;
		
		float mx = i_anchor.x;
		float my = i_anchor.y;
		
		HJLog.i(TAG, "rotate_point rotate " + i_nAngle);
		
		for (int i = 0; i < i_src.length; i++)
		{			
			i_tmpPot.x = i_src[i].x - mx;
			i_tmpPot.y = i_src[i].y - my;
					
			r = i_tmpPot.length();

			degrees = HJShaderCommon.s_getRotation((double)i_tmpPot.x, (double)i_tmpPot.y);
			
			d = (degrees - i_nAngle) * (float)(Math.PI / 180.0f); 
			
			i_dst[i].x = r * (float)Math.cos(d) + mx;
			i_dst[i].y = r * (float)Math.sin(d) + my;
		}	
	}
    
	public static boolean s_justInnter(PointF[] i_pt, PointF pos)
	{
	    int nn = i_pt.length;
	    int prevIndex = nn - 1;
	    boolean inside = false;
	    PointF vert = null;
	    PointF prev = null;
	    
	    for (int ii = 0; ii < nn; ii ++) 
	    {
	        vert = i_pt[ii];
	        prev = i_pt[prevIndex];
	        if ((vert.y < pos.y && prev.y >= pos.y) || (prev.y < pos.y && vert.y >= pos.y)) {
	            if (vert.x + (pos.y - vert.y) / (prev.y - vert.y) * (prev.x - vert.x) < pos.x) inside = !inside;
	        }
	        prevIndex = ii;
	    }
	    return inside;
	}
	public static Bitmap readPixelToBitmap(int mWidth, int mHeight)
	{
		int[] iat = new int[mWidth * mHeight];
		IntBuffer ib = IntBuffer.allocate(mWidth * mHeight);

		GLES20.glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, ib);

		int[] ia = ib.array();

		//Stupid !
		// Convert upside down mirror-reversed image to right-side up normal
		// image.
		for (int i = 0; i < mHeight; i++) {
			for (int j = 0; j < mWidth; j++) {
				iat[(mHeight - i - 1) * mWidth + j] = ia[i * mWidth + j];
			}
		}

		Bitmap mBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
		mBitmap.copyPixelsFromBuffer(IntBuffer.wrap(iat));

		return mBitmap;
	}
	private static void saveFrameExNoFlip(File file, int mWidth, int mHeight)
	{
		long t4 = 0, t5 = 0, t6 = 0, t7 = 0, t8 = 0, t9 = 0;

		IntBuffer ib = IntBuffer.allocate(mWidth * mHeight);

		t4 = System.currentTimeMillis();

		GLES20.glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, ib);

		t5 = System.currentTimeMillis();

		int[] ia = ib.array();

		t6 = System.currentTimeMillis();

		Bitmap mBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
		mBitmap.copyPixelsFromBuffer(IntBuffer.wrap(ia));

		t7 = System.currentTimeMillis();

		String filename = file.toString();

		BufferedOutputStream bos = null;
		try {

			try {
				bos = new BufferedOutputStream(new FileOutputStream(filename));
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			mBitmap.compress(Bitmap.CompressFormat.PNG, 100, bos);
			//mBitmap.compress(Bitmap.CompressFormat.JPEG, 75, bos);
			t8 = System.currentTimeMillis();

			mBitmap.recycle();

			t9 = System.currentTimeMillis();
		}

		finally {
			if (bos != null)
			{
				try {
					bos.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
		HJLog.e(TAG, "glReadPixels " + (t5 - t4) + " convert " + (t6 - t5) + " copyPixelsFromBuffer " + (t7 - t6) + " compress " + (t8 - t7) + " recycle " + (t9 - t8));
	}
    private static void saveFrameEx(File file, int mWidth, int mHeight)
    {
    	long t4 = 0, t5 = 0, t6 = 0, t7 = 0, t8 = 0, t9 = 0;
    	
    	int[] iat = new int[mWidth * mHeight];
        IntBuffer ib = IntBuffer.allocate(mWidth * mHeight);
        
        t4 = System.currentTimeMillis();
        
        GLES20.glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, ib);
        
        t5 = System.currentTimeMillis();
        
        int[] ia = ib.array();

        //Stupid !
        // Convert upside down mirror-reversed image to right-side up normal
        // image.
        for (int i = 0; i < mHeight; i++) {
            for (int j = 0; j < mWidth; j++) {
                iat[(mHeight - i - 1) * mWidth + j] = ia[i * mWidth + j];
            }
        }
        
        t6 = System.currentTimeMillis();
        
        Bitmap mBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
        mBitmap.copyPixelsFromBuffer(IntBuffer.wrap(iat));
        
        t7 = System.currentTimeMillis();
        
        String filename = file.toString();
        
        BufferedOutputStream bos = null;
        try {
        	
        	try {
				bos = new BufferedOutputStream(new FileOutputStream(filename));
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}	
        	
        	mBitmap.compress(Bitmap.CompressFormat.PNG, 100, bos);
			//mBitmap.compress(Bitmap.CompressFormat.JPEG, 75, bos);
            t8 = System.currentTimeMillis();
            
            mBitmap.recycle();
            
            t9 = System.currentTimeMillis();                    
        } 
        
        finally {
            if (bos != null)
            {
				try {
					bos.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
            }
        }
        HJLog.e(TAG, "glReadPixels " + (t5 - t4) + " convert " + (t6 - t5) + " copyPixelsFromBuffer " + (t7 - t6) + " compress " + (t8 - t7) + " recycle " + (t9 - t8));
    }
    
    public static void s_savebitmap(String strUrl, int width, int height)
    {
    	saveFrameEx(new File(strUrl), width, height);		
    }
	public static void s_savebitmapNoFlip(String strUrl, int width, int height)
	{
		saveFrameExNoFlip(new File(strUrl), width, height);
	}
	public static void s_saveBitmapRGBToGary(String strUrl, ByteBuffer i_buf, int width, int height)
	{
		Bitmap mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		byte[] iat = new byte[width * height * 4];
		int k = 0;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				int idx = i * width * 4 + 4 * j;
				int r = i_buf.get(idx) & 0xff;
				int g = i_buf.get(idx + 1) & 0xff;
				int b = i_buf.get(idx + 2) & 0xff;
				byte gray = (byte)(((int)(0.299f * r + 0.587f * g + 0.114f * b)) & 0xff);

				iat[k++] = gray;
				iat[k++] = gray;
				iat[k++] = gray;
				iat[k++] = (byte)255;
			}
		}
		mBitmap.copyPixelsFromBuffer(ByteBuffer.wrap(iat));

		BufferedOutputStream bos = null;
		try {

			try {
				bos = new BufferedOutputStream(new FileOutputStream(strUrl));
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			mBitmap.compress(Bitmap.CompressFormat.PNG, 100, bos);
			//mBitmap.compress(Bitmap.CompressFormat.JPEG, 75, bos);

			mBitmap.recycle();
		}

		finally {
			if (bos != null)
			{
				try {
					bos.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}
	public static void s_saveBitmapGary(String strUrl, ByteBuffer i_buf, int width, int height)
	{
		i_buf.position(0);
		Bitmap mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		byte[] iat = new byte[width * height * 4];
		int k = 0;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				int idx = i * width + j;
				iat[k++] = i_buf.get(idx);
				iat[k++] = i_buf.get(idx);
				iat[k++] = i_buf.get(idx);
				iat[k++] = -1;
			}
		}
		mBitmap.copyPixelsFromBuffer(ByteBuffer.wrap(iat));

		BufferedOutputStream bos = null;
		try {

			try {
				bos = new BufferedOutputStream(new FileOutputStream(strUrl));
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			mBitmap.compress(Bitmap.CompressFormat.PNG, 100, bos);
			//mBitmap.compress(Bitmap.CompressFormat.JPEG, 75, bos);

			mBitmap.recycle();
		}

		finally {
			if (bos != null)
			{
				try {
					bos.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}

	public static void s_saveBitmapIntBuffer(String strUrl, IntBuffer i_buf, int width, int height)
	{
		i_buf.position(0);
		int[] ia = i_buf.array();
		Bitmap mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		mBitmap.copyPixelsFromBuffer(IntBuffer.wrap(ia));

		BufferedOutputStream bos = null;
		try {

			try {
				bos = new BufferedOutputStream(new FileOutputStream(strUrl));
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			mBitmap.compress(Bitmap.CompressFormat.PNG, 100, bos);
			//mBitmap.compress(Bitmap.CompressFormat.JPEG, 75, bos);

			mBitmap.recycle();
		}

		finally {
			if (bos != null)
			{
				try {
					bos.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}

//	public static void s_saveBitmapRGBA(String strUrl, ByteBuffer i_buf, int width, int height)
//	{
//		Bitmap mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
//
//		mBitmap.copyPixelsFromBuffer(i_buf);
//
//		BufferedOutputStream bos = null;
//		try {
//
//			try {
//				bos = new BufferedOutputStream(new FileOutputStream(strUrl));
//			} catch (FileNotFoundException e) {
//				// TODO Auto-generated catch block
//				e.printStackTrace();
//			}
//
//			mBitmap.compress(Bitmap.CompressFormat.PNG, 100, bos);
//			//mBitmap.compress(Bitmap.CompressFormat.JPEG, 75, bos);
//
//			mBitmap.recycle();
//		}
//
//		finally {
//			if (bos != null)
//			{
//				try {
//					bos.close();
//				} catch (IOException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				}
//			}
//		}
//	}
    private static boolean m_bIsTestUpload = true;
	private static FileOutputStream m_outputStream = null;
	private static String m_strFile = "";
	private static byte[] m_upload_data = null;
	private static int m_upload_size = 0;

	public static void s_test_save(ByteBuffer outputFrame, int i_nSize) 
	{
		try 
		{
			if (!m_bIsTestUpload) 
			{
				return;
			}
			if (null == m_outputStream)
			{
				m_strFile = Environment.getExternalStorageDirectory() + "/"
						+ System.currentTimeMillis() + ".obj";
				m_outputStream = new FileOutputStream(m_strFile);
			}
			if (m_outputStream != null) 
			{
				if (i_nSize > m_upload_size) 
				{
					m_upload_data = null;
					m_upload_data = new byte[i_nSize];
					m_upload_size = i_nSize;
					HJLog.i(TAG, "upload size " + i_nSize);
				}
				HJLog.i(TAG, "get m_upload_data " + i_nSize + " capability "
						+ outputFrame.capacity());
				outputFrame.get(m_upload_data, 0, i_nSize);
				try 
				{
					m_outputStream.write(m_upload_data, 0, i_nSize);
				}
				catch (IOException ioe) 
				{
					throw new RuntimeException(ioe);
				}
				outputFrame.rewind();
			}

		} 
		catch (IOException ioe) 
		{
			throw new RuntimeException(ioe);
		}
	}
    
}