#pragma once

#include "HJPrerequisites.h"
#include "linmath.h"
#include "HJOGShaderProgram.h"
#include "HJOGShaderCommon.h"

#if defined(HarmonyOS)

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

NS_HJ_BEGIN

typedef enum HJOGCopyShaderStripFlag
{
    OGCopyShaderStripFlag_UNKNOWN        = 0,
    OGCopyShaderStripFlag_2D             = 1 << 1,
    OGCopyShaderStripFlag_OES            = 1 << 2,
    OGCopyShaderStripFlag_AlphaLeftRight = 1 << 3,

} HJOGCopyShaderStripFlag;

class HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGCopyShaderStrip);

	HJOGCopyShaderStrip();
	virtual ~HJOGCopyShaderStrip();
    int init(int i_nFlag);
    int draw(GLuint textureId, const std::string &i_fitMode, int srcw, int srch, int dstw, int dsth, float *texMat, bool i_bYFlip);
    void release();
	
private:
    int m_nFlag = OGCopyShaderStripFlag_UNKNOWN;
    HJOGGLTextureStyle m_textureStyle = TEXTURE_TYPE_2D;
    HJOGShaderProgram::Ptr m_shaderProgram = nullptr;
    //GLuint m_program = 0;
    GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;
    
    GLint maPositionHandle = 0;
    GLint maTextureHandle = 0;
    GLint mSampleHandle = 0;
    GLint muMVPMatrixHandle = 0;
    GLint muSTMatrixHandle = 0;
};

NS_HJ_END

#endif