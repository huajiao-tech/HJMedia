//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <filesystem>
#include "HJImage.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJImguiUtils.h"
#include "HJJson.h"
#include "stb_image.h"

#define GL_CLAMP_TO_EDGE	0x812F
#define GL_BGRA				0x80E1

NS_HJ_BEGIN
//***********************************************************************************//
HJImage::HJImage()
{

}
HJImage::~HJImage()
{
	done();
}

int HJImage::init(const std::string& filename)
{
	if (filename.empty()) {
		return HJErrInvalidUrl;
	}
	std::filesystem::path imagePath(filename);
	if (!imagePath.has_extension()) {
		HJLoge("not support image format");
		return HJErrNotSupport;
	}
	std::string ext = imagePath.extension().u8string();
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
		uint8_t* imageData = (uint8_t *)stbi_load(imagePath.u8string().c_str(), &m_width, &m_height, &m_nrChannels, STBI_rgb_alpha);
		if (!imageData || !m_width || !m_height) {
			return HJErrFatal;
		}
		m_data = imageData;
	} else
	{
		HJFLoge("not support image format ext:{}", ext);
		return HJErrNotSupport;
	}
	return HJ_OK;
}

void HJImage::done()
{
	if (m_data) {
		stbi_image_free(m_data);
		m_data = NULL;
	}
	unbind();
}

uint64_t HJImage::bind()
{
	if (m_texID) {
		glBindTexture(GL_TEXTURE_2D, (GLuint)m_texID);
		return m_texID;
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	if (!tex) {
		return 0;
	}
	m_texID = tex;
	//
	int fmt = 1;
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, m_data);
	//glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	//
	if (m_data) {
		stbi_image_free(m_data);
		m_data = NULL;
	}
	return m_texID;
}

void HJImage::unbind()
{
	GLuint texID = (GLuint)(m_texID);
	if (0 != texID) {
		glDeleteTextures(1, &texID);
		texID = 0;
	}
}

NS_HJ_END