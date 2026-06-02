#include "HJOGCommon.h"

//NS_HJ_BEGIN

float HJOGCommon::IdentifyMatrix[16]     = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
float HJOGCommon::TextureMatrixYFlip[16] = {1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f};

#if defined(MACOS_LIB)

    const char* HJOGCommon::HJ_GLSL_VERSION = "#version 150 core\n";

#elif defined(WIN32_LIB)

    const char* HJOGCommon::HJ_GLSL_VERSION = "#version 330 core\n";

#else

    const char* HJOGCommon::HJ_GLSL_VERSION = "#version 300 es\n";

#endif



const char* HJOGCommon::HJ_GLSL_PRECISION = "precision mediump float; \n";

GLuint HJOGCommon::textureCreate(uint32_t target)
{
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    
    glBindTexture(target, textureId);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(target, 0);
    return textureId;
}

void HJOGCommon::textureDestroy(GLuint texture)
{
    glDeleteTextures(1, &texture);  
}

void HJOGCommon::textureUpload(GLuint texture, GLenum internalFormat, GLsizei width, GLsizei height, GLenum dataFormat, unsigned char* data)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HJOGCommon::textureUploadRGBA(GLuint texture, GLsizei width, GLsizei height, unsigned char* data)
{
    HJOGCommon::textureUpload(texture, GL_RGBA, width, height, GL_RGBA, data);
}

void HJOGCommon::textureUpload(GLuint texture, GLsizei width, GLsizei height, int nComponents, unsigned char* data)
{
    GLenum internalFormat;
    GLenum dataFormat;
    if (nComponents == 3)
    {
        internalFormat = GL_RGB;
        dataFormat = GL_RGB;
    }
    else if (nComponents == 4)
    {
        internalFormat = GL_RGBA;
        dataFormat = GL_RGBA;
    }
    HJOGCommon::textureUpload(texture, internalFormat, width, height, dataFormat, data);
}
//NS_HJ_END

