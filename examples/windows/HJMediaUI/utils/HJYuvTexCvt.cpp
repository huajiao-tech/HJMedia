#pragma once

#include "HJFLog.h"
#include "HJYuvTexCvt.h"
#include "HJSPBuffer.h"
#include "libyuv.h"

NS_HJ_BEGIN


HJYuvTexCvt::HJYuvTexCvt()
{

}

HJYuvTexCvt::~HJYuvTexCvt()
{
	if (m_bCreateTexture)
	{
		glDeleteTextures(1, &m_textureId);
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
		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RGBBuffer->getBuf());
}

NS_HJ_END



