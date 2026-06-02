#pragma once

#include "HJPrerequisites.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJTransferMediaData.h"

NS_HJ_BEGIN


class HJMediaDataDraw
{
public:  
	HJ_DEFINE_CREATE(HJMediaDataDraw);
    
	HJMediaDataDraw();
	virtual ~HJMediaDataDraw();
	void update(HJTransferMediaData::Ptr i_data);
	GLuint getTextureId() const;
private:
	bool m_bCreateTexture = false;
	GLuint m_textureId = 0;
	int m_width = 0;
	int m_height = 0;
	std::shared_ptr<HJSPBuffer> m_RGBBuffer = nullptr;
};

NS_HJ_END



