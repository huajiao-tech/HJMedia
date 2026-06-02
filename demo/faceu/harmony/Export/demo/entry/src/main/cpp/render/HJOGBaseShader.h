#pragma once


#include "linmath.h"
#include "HJOGShaderProgram.h"
#include "HJOGShaderCommon.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>


typedef enum HJOGBaseShaderFlag
{
    OGCopyShaderStripFlag_UNKNOWN        = 0,
    OGCopyShaderStripFlag_2D             = 1 << 1,
    OGCopyShaderStripFlag_OES            = 1 << 2,
} HJOGBaseShaderFlag;

typedef enum HJOGBaseShaderType
{
    HJOGBaseShaderType_Copy_2D,
    HJOGBaseShaderType_Copy_OES,
    
    HJOGBaseShaderType_PreMul_Copy_2D,
    HJOGBaseShaderType_PreMul_Copy_OES,

    HJOGBaseShaderType_Gray,

    HJOGBaseShaderType_BlurHori,
    HJOGBaseShaderType_BlurVert,
    
    HJOGBaseShaderType_SplitScreenLR_2D,
    HJOGBaseShaderType_SplitScreenLR_OES,

} HJOGBaseShaderType;

class HJOGCopyShaderStrip
{
public:

    HJOGCopyShaderStrip();
    virtual ~HJOGCopyShaderStrip();

    int init(int i_nFlag = OGCopyShaderStripFlag_2D, bool i_bPreMultipleShader = true);
    int draw(GLuint textureId, int i_mode, int srcw, int srch, int dstw, int dsth, float *texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false);
    int draw(GLuint textureId, float *vertexMat, float *texMat);
    void release();
    
	void setForceXMirror(bool i_bForceXMirror)
    {
        m_bForceXMirror = i_bForceXMirror;
    }
    bool getForceXMirror() const
    {
        return m_bForceXMirror;
    }
	void setForceYMirror(bool i_bForceYMirror)
	{
        m_bForceYMirror = i_bForceYMirror;
	}
	bool getForceYMirror() const
	{
		return m_bForceYMirror;
	}
private:
    int m_nFlag = OGCopyShaderStripFlag_UNKNOWN;
    HJOGGLTextureStyle m_textureStyle = TEXTURE_TYPE_2D;
    std::shared_ptr<HJOGShaderProgram> m_shaderProgram = nullptr;
    GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;
    
    GLint maPositionHandle = 0;
    GLint maTextureHandle = 0;
    GLint mSampleHandle = 0;
    GLint muMVPMatrixHandle = 0;
    GLint muSTMatrixHandle = 0;
    
    bool m_bForceXMirror = false;
    bool m_bForceYMirror = false;
    float m_texMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};
};
