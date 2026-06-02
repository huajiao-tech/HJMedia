#pragma once

#include "HJFLog.h"
#include "HJMediaDataDraw.h"
#include "HJSPBuffer.h"
#include "libyuv.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN


HJMediaDataDraw::HJMediaDataDraw()
{

}

HJMediaDataDraw::~HJMediaDataDraw()
{
	if (m_bCreateTexture)
	{
		HJOGCommon::textureDestroy(m_textureId);
		m_textureId = 0;
		m_bCreateTexture = false;
	}
}
GLuint HJMediaDataDraw::getTextureId() const
{
	return m_textureId;
}
void HJMediaDataDraw::update(HJTransferMediaData::Ptr i_data)
{
	int w = i_data->getWidth();
	int h = i_data->getHeight();
	if (!m_bCreateTexture)
	{
		m_bCreateTexture = true;
		m_textureId = HJOGCommon::textureCreate();
	}

	bool bUpdate = false;
	unsigned char* rgba = nullptr;
	if (m_width != w || m_height != h)
	{
		m_width = w;
		m_height = h;
		bUpdate = true;
	}
	HJTransferMediaDataYUVI420::Ptr i_yuvInfo = std::dynamic_pointer_cast<HJTransferMediaDataYUVI420>(i_data);
	if (i_yuvInfo)
	{
		if (bUpdate)
        {
            m_RGBBuffer = HJSPBuffer::create(w * h * 4);
        }
		libyuv::I420ToABGR(i_yuvInfo->getData(0), i_yuvInfo->getStride(0), 
			i_yuvInfo->getData(1), i_yuvInfo->getStride(1),
			i_yuvInfo->getData(2), i_yuvInfo->getStride(2),
			m_RGBBuffer->getBuf(), w * 4,
			w, h);

		rgba = m_RGBBuffer->getBuf();
	}
	else
	{
		HJTransferMediaDataRGBA::Ptr i_rgbInfo = std::dynamic_pointer_cast<HJTransferMediaDataRGBA>(i_data);
        if (i_rgbInfo)
        {
			rgba = i_rgbInfo->getBuffer()->getBuf();
        }
	}
	glBindTexture(GL_TEXTURE_2D, m_textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glBindTexture(GL_TEXTURE_2D, 0);
}

NS_HJ_END



