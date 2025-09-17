#include "HJOGFBOCtrl.h"

NS_HJ_BEGIN

HJOGFBOCtrl::HJOGFBOCtrl()
{
}
HJOGFBOCtrl::~HJOGFBOCtrl()
{
    done();
}
void HJOGFBOCtrl::done()
{
	if (m_bInit)
	{
		glDeleteFramebuffers(1, &m_framebuffer);
		glDeleteTextures(1, &m_texture);
        m_bInit = false;
	}    
}
int HJOGFBOCtrl::getWidth()
{
	return m_width;
}
int HJOGFBOCtrl::getHeight()
{
	return m_height;
}
GLuint HJOGFBOCtrl::getTextureId()
{
	return m_texture;
}
int HJOGFBOCtrl::init(int i_nWidth, int i_nHeight, bool i_bTransparency)
{
	int i_err = 0;
	do
	{
		m_width = i_nWidth;
		m_height = i_nHeight;
		m_bTransparency = i_bTransparency;
		
		glGenFramebuffers(1, &m_framebuffer);
		glGenTextures(1, &m_texture);
		
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLint preBindingFramebuffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &preBindingFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, preBindingFramebuffer);
		
		m_bInit = true;
	} while (false);
	return i_err;
}

GLuint HJOGFBOCtrl::attach()
{
	int i_err = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_preFramebuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	
	float alpha = m_bTransparency ? 0.f : 1.f;
	glViewport(0, 0, m_width, m_height);
	glClearColor(0.f, 0.f, 0.f, alpha);
	glClear(GL_COLOR_BUFFER_BIT);

	//LogDebug.i(TAG, "attach fbo pre " + m_pre[0] + " cur " + mFrameBuffers[i_nIdx] + " texture id " + mFrameBufferTextures[i_nIdx]);
	return m_texture;
}

void HJOGFBOCtrl::detach()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_preFramebuffer);
}

NS_HJ_END
