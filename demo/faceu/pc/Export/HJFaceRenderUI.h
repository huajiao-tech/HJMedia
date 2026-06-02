#pragma once

#include <string>
#include <vector>
#include <glad/gl.h>

class HJFaceRenderUI
{
public:
	HJFaceRenderUI();
	virtual ~HJFaceRenderUI();
	int run();
private:
	//void priHJFaceuCallback(const char* i_uniqueKey, int i_type);
	std::vector<std::string> parseConfig(const std::string& i_path);
	void priDrawOneAndAlpha(int i_width, int i_height);
	GLuint textureCreate(uint32_t target = GL_TEXTURE_2D);
	void textureDestroy(GLuint texture);
	void textureUpload(GLuint texture, GLenum internalFormat, GLsizei width, GLsizei height, GLenum dataFormat, unsigned char* data);
	void textureUploadRGBA(GLuint texture, GLsizei width, GLsizei height, unsigned char* data);

	char m_urls[3][512] = { 0 };
	bool m_bDraw = false;

	static std::string m_imgSeqUrl;
	static std::string m_imgSeqUrlPrefix;
	static std::string m_faceuUrl0;
	static std::string m_faceuUrl1;
	static std::string m_faceuUrl2;
	bool m_bContainFace = true;
	bool m_bDebugPoint = false;
	bool m_bDrawImgSeq = false;
	bool m_bCreateTexture = false;
	GLuint m_textureFaceuId = 0;
	GLuint m_textureImgSeqId = 0;
	std::vector<std::string> m_imgSeqPaths;
	void *m_handle = nullptr;
};




