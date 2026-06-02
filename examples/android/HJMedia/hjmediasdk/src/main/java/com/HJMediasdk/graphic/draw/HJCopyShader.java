package com.HJMediasdk.graphic.draw;

import java.nio.FloatBuffer;

import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;

public class HJCopyShader
{	
	private static final String TAG = HJCopyShader.class.getSimpleName();
	
	private static final String COPY_VERTEX_SHADER =
            "uniform mat4 uMVPMatrix;\n" +
            "uniform mat4 uSTMatrix;\n" +
            "attribute vec4 aPosition;\n" +
            "attribute vec4 aTextureCoord;\n" +
            "varying vec2 vTextureCoord;\n" +
            "void main() {\n" +
            "  gl_Position = uMVPMatrix * aPosition;\n" +
            "  vTextureCoord = (uSTMatrix * aTextureCoord).xy;\n" +
            "}\n";

    private static final String COPY_FRAGMENT_OES_SHADER =
            "#extension GL_OES_EGL_image_external : require\n" +
            "precision mediump float;\n" +      // highp here doesn't seem to matter
            "varying vec2 vTextureCoord;\n" +
            "uniform samplerExternalOES sTexture;\n" +
            "void main() {\n" +
            "  gl_FragColor = texture2D(sTexture, vTextureCoord);\n" +
            "}\n";
	
    private static final String COPY_FRAGMENT_2D_SHADER =
            "precision mediump float;\n" +      // highp here doesn't seem to matter
            "varying vec2 vTextureCoord;\n" +
            "uniform sampler2D sTexture;\n" +
            "void main() {\n" +
            "  gl_FragColor = texture2D(sTexture, vTextureCoord);\n" +
            "}\n";
    
    private HJShaderCommon m_shader_info = null;
    
    public static final int COPY_DIRECT                = 1;
    public static final int COPY_BLEND                 = 2;
    public static final int COPY_BLEND_SRC_ONEMINUSSRC = 4;
    public static final int COPY_BLEND_ONE_ONEMINUSSRC = 8;
    public static final int COPY_BLEND_ONE             = 16;

	private HJDrawChangeImgCb m_draw_img_cb = null;
    private int m_copy_style   = COPY_DIRECT;
    private int m_shader_style = HJShaderCommon.SHADER_OES;

    private int maPositionHandle = 0;
    private int maTextureHandle = 0;
    private int mSampleHandle = 0;
    private int muMVPMatrixHandle = 0;
    private int muSTMatrixHandle = 0;
    private int mProgram = 0;
    
	private float[] mMVPMatrix = new float[16];
    private float[] mSTMatrix = new float[16];
    
    private float[] mTmpVertaxMat;
    private float[] mTmpTextureMat;
    
    private HJShaderCb m_shader_cb = null;
    
	public HJCopyShader(int coord_style)
	{
		m_shader_info = new HJShaderCommon(coord_style);
	}
	
	public void setShaderListener(HJShaderCb cb)
	{
		m_shader_cb = cb;
	} 
	
	public int release()
	{
		if (mProgram > 0)
		{
			GLES20.glDeleteProgram(mProgram);
			mProgram = 0;
		}
		if (m_shader_info != null)
		{
			m_shader_info.release();
			m_shader_info = null;
		}
		return 0;
	}
	
	public void setVertexMat(float[] mat)
	{
		mTmpVertaxMat = mat;
	}
	public void setTextureMat(float[] mat)
	{
		mTmpTextureMat = mat;
	}

	public void setDrawChangeImgCb(HJDrawChangeImgCb i_cb)
	{
		m_draw_img_cb = i_cb;
	}

	public int init(int i_shader_style, int i_copy_style) 
    {
    	int i_err = 0;
    	do
    	{ 
    		Matrix.setIdentityM(mMVPMatrix, 0);
    		Matrix.setIdentityM(mSTMatrix, 0);
    		
    	    mTmpVertaxMat  = mMVPMatrix;
    	    mTmpTextureMat = mSTMatrix;
    		
    		m_copy_style   = i_copy_style;
    		m_shader_style = i_shader_style;
    				
 	        i_err = ConfigParam();
 	        if (i_err < 0)
 	        {
 	        	break;
 	        }

    	} while (false);
    	
    	return i_err;
    }
	
	private int ConfigParam()
	{
		int i_err = 0;
		do
		{	
			String vertexShader   = "";
			String fragmentShader = "";
		
			if (m_shader_cb != null)
			{
				vertexShader = m_shader_cb.getVertexShader();
				fragmentShader = m_shader_cb.getFragmentShader(m_shader_style);
			}
			
			if (vertexShader.equals(""))
			{
				vertexShader = COPY_VERTEX_SHADER;
			}
			
			if (fragmentShader.equals(""))
			{
				if (m_shader_style == HJShaderCommon.SHADER_OES)
				{
					fragmentShader = COPY_FRAGMENT_OES_SHADER;
				}
				else
				{
					fragmentShader = COPY_FRAGMENT_2D_SHADER;
				}
			}
			
			
			mProgram = HJGlUtil.createProgram(vertexShader, fragmentShader);
 	        if (mProgram <= 0)
 	        {
 	        	i_err = -1;
 	        	break;
 	        }
			
			maPositionHandle = GLES20.glGetAttribLocation(mProgram, "aPosition");
			i_err = HJGlUtil.checkGlError("glGetAttribLocation aPosition");
			if (i_err < 0)
			{
				break;
			}
 	        if (maPositionHandle == -1) {
 	           	i_err = -1;
 	           	break;
 	        }
 	        maTextureHandle = GLES20.glGetAttribLocation(mProgram, "aTextureCoord");
 	        i_err = HJGlUtil.checkGlError("glGetAttribLocation aTextureCoord");
 	        if (i_err < 0)
 	        {
 	        	break;
 	        }
 	        if (maTextureHandle == -1) {
 	        	i_err = -1;
 	        	break;
 	        }

 	        muMVPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMVPMatrix");
 	        i_err = HJGlUtil.checkGlError("glGetUniformLocation uMVPMatrix");
 	        if (i_err < 0)
 	        {
 	        	break;
 	        }
 	        if (muMVPMatrixHandle == -1) {
 	        	i_err = -1;
 	        	break;
 	        }

 	        muSTMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uSTMatrix");
 	        i_err = HJGlUtil.checkGlError("glGetUniformLocation uSTMatrix");
 	        if (i_err < 0)
 	        {
 	        	break;
 	        }
 	        if (muSTMatrixHandle == -1) {
 	        	i_err = -1;
 	        	break;
 	        }
 	        
 	        mSampleHandle = GLES20.glGetUniformLocation(mProgram, "sTexture");
 	        i_err = HJGlUtil.checkGlError("glGetUniformLocation mSampleHandle");
 	        if (i_err < 0)
 	        {
 	        	break;
 	        }
	        if (mSampleHandle == -1) {
	        	i_err = -1;
	        	break;
	        }
	        
	        if (m_shader_cb != null)
	        {
	        	i_err = m_shader_cb.init_additional_parame(mProgram);
	        	if (i_err < 0)
	        	{
	        		break;
	        	}
	        }
        
		} while (false);
		return i_err;
	}
	
	public int draw(int texture_id)
	{
		int i_err = 0;
		do
		{
			int target = 0;
			
			GLES20.glUseProgram(mProgram);
		    i_err = HJGlUtil.checkGlError("glUseProgram");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
		    if ((m_copy_style & COPY_BLEND) != 0)
		    {
		    	GLES20.glEnable(GLES20.GL_BLEND);
				i_err = HJGlUtil.checkGlError("glEnable");
			    if (i_err < 0)
			    {
			    	break;
			    }
			    
			    int src = 0;
			    int dst = 0;
			    if ((m_copy_style & COPY_BLEND_SRC_ONEMINUSSRC) != 0)
			    {
			    	src = GLES20.GL_SRC_ALPHA;
			    	dst = GLES20.GL_ONE_MINUS_SRC_ALPHA;
			    }
			    else if ((m_copy_style & COPY_BLEND_ONE_ONEMINUSSRC) != 0)
			    {
			    	src = GLES20.GL_ONE;
			    	dst = GLES20.GL_ONE_MINUS_SRC_ALPHA;
			    }
			    else if ((m_copy_style & COPY_BLEND_ONE) != 0)
				{
					src = GLES20.GL_ONE;
					dst = GLES20.GL_ONE;
				}
				GLES20.glBlendFunc(src, dst);
		    }
			
		    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		    i_err = HJGlUtil.checkGlError("glActiveTexture");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
		    if (m_shader_style == HJShaderCommon.SHADER_OES)
		    {
		    	target = GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
		    }
		    else if (m_shader_style == HJShaderCommon.SHADER_2D)
		    {
		    	target = GLES20.GL_TEXTURE_2D;
		    }
			else if (m_shader_style == HJShaderCommon.SHADER_CUBE)
			{
				target = GLES20.GL_TEXTURE_CUBE_MAP;
			}
		    GLES20.glBindTexture(target, texture_id);
		    i_err = HJGlUtil.checkGlError("glBindTexture");
		    if (i_err < 0)
		    {
		    	break;
		    }

		    //no use
//		    if (m_draw_img_cb != null)
//		    {
//		    	m_draw_img_cb.onDrawChangeImg();
//		    }
		    FloatBuffer buffer = m_shader_info.get_buffer();
		    i_err = m_shader_info.setPointer(buffer, maPositionHandle, maTextureHandle);
		    if (i_err < 0)
		    {
		    	break;
		    }
		    		    		    		   	    
		    GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mTmpVertaxMat, 0);
		    GLES20.glUniformMatrix4fv(muSTMatrixHandle, 1, false, mTmpTextureMat, 0);
		        
		    if (m_shader_cb != null)
		    {
		    	i_err = m_shader_cb.set_additional_parame();
		    	if (i_err < 0)
		    	{
		    		break;
		    	}
		    }
		    
		    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);		       
		    i_err = HJGlUtil.checkGlError("glDrawArrays");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
		    GLES20.glFlush();
		    
		    if ((m_copy_style & COPY_BLEND) != 0)
		    {
		    	GLES20.glDisable(GLES20.GL_BLEND);	
		    }
		    
		    if (m_shader_cb != null)
		    {
		    	m_shader_cb.draw_end();
		    }
		    
		    GLES20.glDisableVertexAttribArray(maPositionHandle);
	        GLES20.glDisableVertexAttribArray(maTextureHandle);
	        GLES20.glBindTexture(target, 0);
	        GLES20.glUseProgram(0);
		} while (false);
		return i_err;
	}
	
}