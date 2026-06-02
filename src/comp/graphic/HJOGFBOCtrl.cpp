#include "HJOGFBOCtrl.h"
#include "HJFLog.h"

NS_HJ_BEGIN

thread_local std::atomic<int64_t> HJOGFBOCtrl::m_fboCreateCnt{0};
thread_local std::atomic<int64_t> HJOGFBOCtrl::m_fboDestroyCnt{0};

HJOGFBOCtrl::HJOGFBOCtrl()
{
    const int64_t create_cnt = m_fboCreateCnt.fetch_add(1, std::memory_order_relaxed) + 1;
    const int64_t destroy_cnt = m_fboDestroyCnt.load(std::memory_order_relaxed);
	HJFLogi("HJOGFBOCtrl::HJOGFBOCtrl() create m_fboCreateCnt:{} m_fboDestroyCnt:{} diff:{} obj:{}",
            create_cnt, destroy_cnt, (create_cnt - destroy_cnt), size_t(this));
}
HJOGFBOCtrl::~HJOGFBOCtrl()
{
    const int64_t destroy_cnt = m_fboDestroyCnt.fetch_add(1, std::memory_order_relaxed) + 1;
    const int64_t create_cnt = m_fboCreateCnt.load(std::memory_order_relaxed);
	HJFLogi("HJOGFBOCtrl::~HJOGFBOCtrl() destroy m_fboCreateCnt:{} m_fboDestroyCnt:{} diff:{}",
            create_cnt, destroy_cnt, (create_cnt - destroy_cnt));
    done();
}
void HJOGFBOCtrl::done()
{
	if (m_bInit)
	{
		glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
		glDeleteTextures(1, &m_texture);
        m_texture = 0;
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
bool HJOGFBOCtrl::isTransparency()
{
    return m_bTransparency;
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

          // check FBO complete
          GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
          if (status != GL_FRAMEBUFFER_COMPLETE)
          {
              HJFLoge("HJOGFBOCtrl::init error fbo incomplete, status:{} width:{} height:{} obj:{}", status, m_width, m_height, size_t(this));
              i_err = -1;
              glBindFramebuffer(GL_FRAMEBUFFER, preBindingFramebuffer);
              break;
          }

          glBindTexture(GL_TEXTURE_2D, 0);
          glBindFramebuffer(GL_FRAMEBUFFER, preBindingFramebuffer);

          HJFLogi("HJOGFBOCtrl::HJOGFBOCtrl() init success: width:{} height:{} obj:{}", m_width, m_height, size_t(this));
          m_bInit = true;
      } while (false);

      if (i_err < 0)
      {
          if (m_framebuffer != 0)
          {
              glDeleteFramebuffers(1, &m_framebuffer);
              m_framebuffer = 0;
          }
          if (m_texture != 0)
          {
              glDeleteTextures(1, &m_texture);
              m_texture = 0;
          }
      }
      return i_err;
  }


GLuint HJOGFBOCtrl::attach()
{
	int i_err = 0;
	if (!m_bInit)
	{
		return 0;
	}
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_preFramebuffer);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		HJFLoge("glGetIntegerv glGetError:{} obj:{} width:{} height:{}", err, size_t(this), m_width, m_height);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		HJFLoge("bind glGetError:{} obj:{} width:{} height:{}", err, size_t(this), m_width, m_height);
	}
	float alpha = m_bTransparency ? 0.f : 1.f;
	glViewport(0, 0, m_width, m_height);
	glClearColor(0.f, 0.f, 0.f, alpha);
	glClear(GL_COLOR_BUFFER_BIT);

	return m_texture;
}

void HJOGFBOCtrl::detach()
{
	if (!m_bInit)
	{
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, m_preFramebuffer);
}

NS_HJ_END
