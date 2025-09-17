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

} HJOGCopyShaderStripFlag;

class HJOGCopyShaderStripInterf
{
public:
    HJOGCopyShaderStripInterf()
    {
        
    }
    virtual ~HJOGCopyShaderStripInterf()
    {
        
    }
    virtual std::string shaderGetVertex()
    {
        return "";
    }
    virtual std::string shaderGetFragment()
    {
        return "";
    }
    virtual int shaderGetHandle(GLuint i_program)
    {
        return 0;
    }
    virtual void shaderDrawUpdate()
    {
        
    }
    virtual void shaderDrawEnd()
    {
        
    }
    
    void setInsName(const std::string &i_name)
    {
        m_insName = i_name;
    }
    const std::string &getInsName() const 
    {
        return m_insName;
    }
    
private:
    std::string m_insName = "";    
};

class HJOGCopyShaderStrip : public HJOGCopyShaderStripInterf
{
public:
    HJ_DEFINE_CREATE(HJOGCopyShaderStrip);

	HJOGCopyShaderStrip();
	virtual ~HJOGCopyShaderStrip();
    
    int init(int i_nFlag = OGCopyShaderStripFlag_2D);
    int draw(GLuint textureId, const std::string &i_fitMode, int srcw, int srch, int dstw, int dsth, float *texMat = nullptr, bool i_bYFlip = false);
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
    
    float m_texMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; 
};

NS_HJ_END

#endif