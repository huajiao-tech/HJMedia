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

typedef enum HJOGBaseShaderFlag
{
    OGCopyShaderStripFlag_UNKNOWN        = 0,
    OGCopyShaderStripFlag_2D             = 1 << 1,
    OGCopyShaderStripFlag_OES            = 1 << 2,
} HJOGBaseShaderFlag;

typedef enum HJOGBaseShaderType
{
    HJOGBaseShaderType_Copy_OES,
    HJOGBaseShaderType_Copy_2D,
    HJOGBaseShaderType_Gray,
    HJOGBaseShaderType_BlurHori,
    HJOGBaseShaderType_BlurVert,
} HJOGBaseShaderType;


class HJOGBaseShader
{
public:
    HJ_DEFINE_CREATE(HJOGBaseShader);
    HJOGBaseShader()
    {
        
    }
    virtual ~HJOGBaseShader();
    virtual int init(int i_nFlag = OGCopyShaderStripFlag_2D, bool i_bPreMultipleShader = true)
    {
        return 0;
    }
    virtual int draw(GLuint textureId, const std::string &i_fitMode, int srcw, int srch, int dstw, int dsth, float *texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false)
    {
        return 0;
    }
    virtual void release()
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
    
    static HJOGBaseShader::Ptr createShader(int i_shaderType);
    
private:
    std::string m_insName = "";    
};

class HJOGCopyShaderStrip : public HJOGBaseShader
{
public:
    HJ_DEFINE_CREATE(HJOGCopyShaderStrip);

	HJOGCopyShaderStrip();
	virtual ~HJOGCopyShaderStrip();
    
    virtual int init(int i_nFlag = OGCopyShaderStripFlag_2D, bool i_bPreMultipleShader = true) override;
    virtual int draw(GLuint textureId, const std::string &i_fitMode, int srcw, int srch, int dstw, int dsth, float *texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false) override;
    int draw(GLuint textureId, float *vertexMat, float *texMat);
    virtual void release() override;
	
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

class HJOGShaderFilterGray : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterGray);
    HJOGShaderFilterGray() = default;
    virtual ~HJOGShaderFilterGray() = default;
    virtual std::string shaderGetFragment() override 
    {
        return s_fragmentShader;
    }
private:
   const static std::string s_fragmentShader;
};

class HJOGBaseBlurShader : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGBaseBlurShader);
    HJOGBaseBlurShader() = default;
    virtual ~HJOGBaseBlurShader() = default;
    virtual int shaderGetHandle(GLuint i_program) override
    {
        int i_err = 0;
        do 
        {
            m_strideHandle = glGetUniformLocation(i_program, "uStride");     
            if (m_strideHandle == -1)
            {
                i_err = -1;
                break;
            }
        } while (false);
        return i_err;
    }
    virtual void shaderDrawUpdate() override
    {
        glUniform2f(m_strideHandle, (float)m_stride / m_width, (float)m_stride / m_height);
    }
    
    void setWidth(int i_width)
    {
        m_width = i_width;
    }
    void setHeight(int i_height)
    {
        m_height = i_height;
    }
    void setStride(int i_stride)
    {
        m_stride = i_stride;
    }
private:
   
    GLint m_strideHandle = 0;
    int m_stride = 4;
    int m_width = 0;
    int m_height = 0;
};
class HJOGShaderFilterBlurHori : public HJOGBaseBlurShader
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterBlurHori);
    HJOGShaderFilterBlurHori() = default;
    virtual ~HJOGShaderFilterBlurHori() = default;
    virtual std::string shaderGetFragment() override 
    {
        return s_fragmentShader;
    }
private:
   const static std::string s_fragmentShader;    
};
class HJOGShaderFilterBlurVert : public HJOGBaseBlurShader
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterBlurVert);
    HJOGShaderFilterBlurVert() = default;
    virtual ~HJOGShaderFilterBlurVert() = default;
    virtual std::string shaderGetFragment() override 
    {
        return s_fragmentShader;
    }
private:
   const static std::string s_fragmentShader;    
};

NS_HJ_END

#endif