#include "HJFLog.h"
#include "HJYuvTexCvt.h"
#include "HJSPBuffer.h"
#include "libyuv.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN


HJYuvTexCvt::HJYuvTexCvt()
{

}

HJYuvTexCvt::~HJYuvTexCvt()
{
	if (m_bCreateTexture)
	{
		HJOGCommon::textureDestroy(m_textureId);
		m_textureId = 0;
		m_bCreateTexture = false;
	}
}
GLuint HJYuvTexCvt::getTextureId() const
{
	return m_textureId;
}
void HJYuvTexCvt::update(HJYuvInfo::Ptr i_yuvInfo)
{
	int w = i_yuvInfo->m_width;
	int h = i_yuvInfo->m_height;
	if (!m_bCreateTexture)
	{
		m_bCreateTexture = true;
		m_textureId = HJOGCommon::textureCreate();
	}
	
	if (m_width!= w || m_height!= h)
	{
		m_width = w;
		m_height = h;
		m_RGBBuffer = HJSPBuffer::create(w * h * 4);
	}

	libyuv::I420ToABGR(i_yuvInfo->m_y, i_yuvInfo->m_yLineSize, 
		i_yuvInfo->m_u, i_yuvInfo->m_uLineSize,
		i_yuvInfo->m_v, i_yuvInfo->m_vLineSize,
		m_RGBBuffer->getBuf(), w * 4,
		w, h);
	glBindTexture(GL_TEXTURE_2D, m_textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RGBBuffer->getBuf());
	glBindTexture(GL_TEXTURE_2D, 0);
}

NS_HJ_END



