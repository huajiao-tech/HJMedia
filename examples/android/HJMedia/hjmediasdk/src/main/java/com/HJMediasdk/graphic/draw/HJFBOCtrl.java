package com.HJMediasdk.graphic.draw;

import android.opengl.GLES20;

import com.HJMediasdk.utils.HJLog;

public class HJFBOCtrl
{
	private static final String TAG = HJFBOCtrl.class.getSimpleName();
	
	private int m_nListCnt = 2;
	
	private int[] mFrameBuffers;
	private int[] mFrameBufferTextures;
	private int[] m_pre = new int[1];	
	private int[] mDepthRender;

	private int m_width = 0;
	private int m_height = 0;
	
	public final static int FBO_STYLE_2D     = 1;
	public final static int FBO_STYLE_3D     = 2;
	public final static int FBO_OPACITY      = 4;
	public final static int FBO_TRANSPARENCY = 8;
	
	private int m_fbo_style = FBO_STYLE_2D | FBO_TRANSPARENCY;

	public HJFBOCtrl(int style)
	{
		m_fbo_style = style;
		m_pre[0] = 0;
	}
	public HJFBOCtrl()
	{
		m_pre[0] = 0;
	}
	
	private int proc_3d(int i_nIdx, int width, int height)
	{
		int i_err = 0;
		do
		{
			 if ((m_fbo_style & FBO_STYLE_3D) == 0)
			 {
				 break;
			 }
			 
		    // Create a depth buffer and bind it. ==========================
            GLES20.glGenRenderbuffers(1, mDepthRender, i_nIdx);
            i_err = HJGlUtil.checkGlError("glGenRenderbuffers");
            if (i_err < 0)
            {
            	break;
            }
            GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, mDepthRender[i_nIdx]);
            i_err = HJGlUtil.checkGlError("glBindRenderbuffer " + mDepthRender[i_nIdx]);
            if (i_err < 0)
            {
            	break;
            }
            
            // Allocate storage for the depth buffer.
            GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER, GLES20.GL_DEPTH_COMPONENT16,
                    width, height);
            i_err = HJGlUtil.checkGlError("glRenderbufferStorage");
            if (i_err < 0)
            {
            	break;
            }
            // Attach the depth buffer and the texture (color buffer) to the framebuffer object.
            GLES20.glFramebufferRenderbuffer(GLES20.GL_FRAMEBUFFER, GLES20.GL_DEPTH_ATTACHMENT,
                    GLES20.GL_RENDERBUFFER, mDepthRender[i_nIdx]);
            i_err = HJGlUtil.checkGlError("glFramebufferRenderbuffer");
            if (i_err < 0)
            {
            	break;
            }
            // depth. ==========================

		} while (false);
		return i_err;
	}
	
	private int createTexture(int i_nIdx, int width, int height)
	{
		int i_err = 0;
		do			
		{
			GLES20.glGenFramebuffers(1, mFrameBuffers, i_nIdx);
			i_err = HJGlUtil.checkGlError("glGenFramebuffers");
			if (i_err < 0)
		    {
			    break;
			}
			
			GLES20.glGenTextures(1, mFrameBufferTextures, i_nIdx);
			i_err = HJGlUtil.checkGlError("glGenTextures");
			if (i_err < 0)
		    {
			    break;
			}
			    
	        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mFrameBufferTextures[i_nIdx]);
	        
	        i_err = HJGlUtil.checkGlError("glBindTexture");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
	        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, width, height, 0,
	                GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
	        
	        i_err = HJGlUtil.checkGlError("glTexImage2D");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
	        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
	                GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
	        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
	                GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
	        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
	                GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
	        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
	                GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
	        
	        
	        i_err = HJGlUtil.checkGlError("glTexParameterf");
		    if (i_err < 0)
		    {
		    	break;
		    }

		    // _GS_ 20210601
			int[] bindingFramebuffer = new int[1];
			GLES20.glGetIntegerv(GLES20.GL_FRAMEBUFFER_BINDING, bindingFramebuffer, 0);
			GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffers[i_nIdx]);
	        
	        i_err = HJGlUtil.checkGlError("glBindFramebuffer");
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
		    i_err = proc_3d(i_nIdx, width, height);
		    if (i_err < 0)
		    {
		    	break;
		    }
		    
	        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
	                GLES20.GL_TEXTURE_2D, mFrameBufferTextures[i_nIdx], 0);
	        
	        i_err = HJGlUtil.checkGlError("glFramebufferTexture2D");
		    if (i_err < 0)
		    {
		    	break;
		    }

	        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
			// _GS_ 20210601
//	        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
			GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, bindingFramebuffer[0]);
	        HJLog.i(TAG, "20161011, createTexture w " + width + " height " + height + " i_nIdx " + i_nIdx + " fbuffer " + mFrameBuffers[i_nIdx] + " fbtexture " + mFrameBufferTextures[i_nIdx]);
		} while (false);
		return i_err;
	}
	
	public int init(int i_nListCnt, int i_nWidth, int i_nHeight)
	{
		int i_err = 0;
		do
		{
			m_nListCnt = i_nListCnt;
			if (m_nListCnt <= 0)
			{
				i_err = -1;
				break;
			}
			
			mFrameBuffers = new int[m_nListCnt];
			if (null == mFrameBuffers)
			{
				i_err = -1;
				break;
			}

			mFrameBufferTextures = new int[m_nListCnt];
			if (null == mFrameBufferTextures)
			{
				i_err = -1;
				break;
			}
			
			if ((m_fbo_style & FBO_STYLE_3D) != 0)
			{
				mDepthRender = new int[m_nListCnt];
				if (null == mDepthRender)
				{
					i_err = -1;
					break;
				}
			}

			m_width = i_nWidth;
			m_height = i_nHeight;
			
			for (int i = 0; i < m_nListCnt; i++)
			{
				i_err = createTexture(i, m_width, m_height);
				if (i_err < 0)
				{
					return i_err;
				}
			}
						
		} while (false);
		return i_err;
	}
	
	public int width()
	{
		return m_width;
	}
	public int height()
	{
		return m_height;
	}
	
	public int get_tex_id(int i_nIdx)
	{
		return mFrameBufferTextures[i_nIdx];
	}

	public int attach(int i_nIdx)
	{
		int i_err = 0;
		GLES20.glGetIntegerv(GLES20.GL_FRAMEBUFFER_BINDING, m_pre, 0);
		
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffers[i_nIdx]);
		i_err = HJGlUtil.checkGlError("glBindFramebuffer fail");
	    if (i_err < 0)
	    {
	    	return -1;
	    }
	    
	    float alpha = 0.f;
	    if ((m_fbo_style & FBO_OPACITY) != 0)
	    {
	    	alpha = 1.f;
	    }
	    else if ((m_fbo_style & FBO_TRANSPARENCY) != 0)
	    {
	    	alpha = 0.f;
	    }
	    GLES20.glViewport(0, 0, width(), height());
		GLES20.glClearColor(0.f, 0.f, 0.f, alpha);

		int clear = GLES20.GL_COLOR_BUFFER_BIT;
		if ((m_fbo_style & FBO_STYLE_3D) != 0)
	    {
			clear |= GLES20.GL_DEPTH_BUFFER_BIT;
	    }
		GLES20.glClear(clear);
		
		//LogDebug.i(TAG, "attach fbo pre " + m_pre[0] + " cur " + mFrameBuffers[i_nIdx] + " texture id " + mFrameBufferTextures[i_nIdx]);
		
		return mFrameBufferTextures[i_nIdx];
	}
	
	public int get_frame_buffer(int i_nIdx)
	{
		return mFrameBuffers[i_nIdx];
	}
	
	public int get_fbo_texture(int i_nIdx)
	{
		return mFrameBufferTextures[i_nIdx];
	}
	
	public void detach()
	{
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, m_pre[0]);
		//LogDebug.i(TAG, "detach restore " + m_pre[0]);
	}
	
	public void release()
	{
		if (mFrameBuffers != null) 
		{
		    GLES20.glDeleteFramebuffers(mFrameBuffers.length, mFrameBuffers, 0);
		    mFrameBuffers = null;
		}
		        
		if (mFrameBufferTextures != null)
		{
		    GLES20.glDeleteTextures(mFrameBufferTextures.length, mFrameBufferTextures, 0);
		    mFrameBufferTextures = null;
		}
		
		if (mDepthRender != null)
		{
			GLES20.glDeleteRenderbuffers(mDepthRender.length, mDepthRender, 0);
			mDepthRender = null;
		}
	}
	
}
	