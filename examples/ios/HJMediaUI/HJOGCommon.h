#pragma once

//#include "HJPrerequisites.h"
//#include "HJOGUtils.h"

//NS_HJ_BEGIN
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
class HJOGCommon
{
public:
    static GLuint textureCreate(uint32_t target = GL_TEXTURE_2D);
    static void textureDestroy(GLuint texture);
    static void textureUploadRGBA(GLuint texture, GLsizei width, GLsizei height, unsigned char* data);
    static void textureUpload(GLuint texture, GLsizei width, GLsizei height, int nComponents, unsigned char* data);
    static void textureUpload(GLuint texture, GLenum internalFormat, GLsizei width, GLsizei height, GLenum dataFormat, unsigned char* data);
   
    static float IdentifyMatrix[16];
    static float TextureMatrixYFlip[16];

    static const char* HJ_GLSL_VERSION;
    static const char* HJ_GLSL_PRECISION;
};	

//NS_HJ_END
