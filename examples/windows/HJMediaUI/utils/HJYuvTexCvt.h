#pragma once

#include "HJPrerequisites.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;

NS_HJ_BEGIN

class HJSPBuffer;

class HJYuvInfo
{
public:
	HJ_DEFINE_CREATE(HJYuvInfo);

	HJYuvInfo()
	{

	}

	int m_type = 0;

	uint8_t* m_y = nullptr;
	uint8_t* m_u = nullptr;
	uint8_t* m_v = nullptr;

	int m_yLineSize = 0;
	int m_uLineSize = 0;
	int m_vLineSize = 0;

	int m_width = 0;
	int m_height = 0;	
};

class HJYuvTexCvt
{
public:  
	HJ_DEFINE_CREATE(HJYuvTexCvt);
    
	HJYuvTexCvt();
	virtual ~HJYuvTexCvt();
	void update(HJYuvInfo::Ptr i_yuvInfo);
	GLuint getTextureId() const;
private:
	bool m_bCreateTexture = false;
	GLuint m_textureId = 0;
	int m_width = 0;
	int m_height = 0;
	std::shared_ptr<HJSPBuffer> m_RGBBuffer = nullptr;
};

NS_HJ_END



