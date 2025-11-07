#if defined(HarmonyOS)
#include "HJPrioComFBOBlur.h"
#include "HJFLog.h"

NS_HJ_BEGIN

class HJPrioComBlurBaseShader : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJPrioComBlurBaseShader);
    HJPrioComBlurBaseShader()
    {}
    virtual ~HJPrioComBlurBaseShader()
    {}
    virtual int shaderGetHandle(GLuint i_program) override
    {
        int i_err = HJ_OK;
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
    int m_stride = 6;
    int m_width = 0;
    int m_height = 0;
};

///////////////////////////////////////////////////////////////////

class HJPrioComBlurHoriShader : public HJPrioComBlurBaseShader
{
public:
    HJ_DEFINE_CREATE(HJPrioComBlurHoriShader);
    HJPrioComBlurHoriShader()
    {}
    virtual ~HJPrioComBlurHoriShader()
    {}
    virtual std::string shaderGetFragment() override
    {
        return s_shader_hori;
    }
private:
    const static std::string s_shader_hori;
};

const std::string HJPrioComBlurHoriShader::s_shader_hori = R"(
precision mediump float;
varying vec2  v_texcood; 
uniform sampler2D sTexture;
uniform vec2 uStride;
void main()
{
   vec4 result = vec4(0.0);
					 vec2 tex_offset = uStride; 
					 result += texture2D(sTexture, v_texcood) * 0.227027;	
					 result += texture2D(sTexture, v_texcood + vec2(tex_offset.x, 0.0)) * 0.1945946;	
					 result += texture2D(sTexture, v_texcood + vec2(2.0 * tex_offset.x, 0.0)) * 0.1216216;	
					 result += texture2D(sTexture, v_texcood + vec2(3.0 * tex_offset.x, 0.0)) * 0.054054;	
					 result += texture2D(sTexture, v_texcood + vec2(4.0 * tex_offset.x, 0.0)) * 0.016216;	
					 result += texture2D(sTexture, v_texcood - vec2(tex_offset.x, 0.0)) * 0.1945946;
					 result += texture2D(sTexture, v_texcood - vec2(2.0 * tex_offset.x, 0.0)) * 0.1216216;
					 result += texture2D(sTexture, v_texcood - vec2(3.0 * tex_offset.x, 0.0)) * 0.054054;
					 result += texture2D(sTexture, v_texcood - vec2(4.0 * tex_offset.x, 0.0)) * 0.016216;	
					 gl_FragColor = result; 
}
)";

//////////////////////////////////////////////////////////////

class HJPrioComBlurVertShader : public HJPrioComBlurBaseShader
{
public:
    HJ_DEFINE_CREATE(HJPrioComBlurVertShader);
    HJPrioComBlurVertShader()
    {}
    virtual ~HJPrioComBlurVertShader()
    {}    
    virtual std::string shaderGetFragment() override
    {
        return s_shader_vert;
    }
private:
    const static std::string s_shader_vert;
};

const std::string HJPrioComBlurVertShader::s_shader_vert = R"(
precision mediump float;
varying vec2  v_texcood; 
uniform sampler2D sTexture;
uniform vec2 uStride;
void main()
{
 vec4 result = vec4(0.0);
					 vec2 tex_offset = uStride; 
					 result += texture2D(sTexture, v_texcood) * 0.227027;	
					 result += texture2D(sTexture, v_texcood + vec2(0.0, tex_offset.y)) * 0.1945946;	
					 result += texture2D(sTexture, v_texcood + vec2(0.0, 2.0 * tex_offset.y)) * 0.1216216;
					 result += texture2D(sTexture, v_texcood + vec2(0.0, 3.0 * tex_offset.y)) * 0.054054;	
					 result += texture2D(sTexture, v_texcood + vec2(0.0, 4.0 * tex_offset.y)) * 0.016216;
					 result += texture2D(sTexture, v_texcood - vec2(0.0, tex_offset.y)) * 0.1945946;	
					 result += texture2D(sTexture, v_texcood - vec2(0.0, 2.0 * tex_offset.y)) * 0.1216216;
					 result += texture2D(sTexture, v_texcood - vec2(0.0, 3.0 * tex_offset.y)) * 0.054054;	
					 result += texture2D(sTexture, v_texcood - vec2(0.0, 4.0 * tex_offset.y)) * 0.016216;	
					 gl_FragColor = result;
}
)";

////////////////////////////////////////////////////////////////////////////////

HJPrioComBlurHori::HJPrioComBlurHori()
{
    HJ_SetInsName(HJPrioComBlurHori);
    setPriority(HJPrioComType_VideoBlur);
}

HJPrioComBlurHori::~HJPrioComBlurHori()
{

}

int HJPrioComBlurHori::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        HJPrioComBaseFBOInfo::Ptr info = nullptr;
        HJ_CatchMapGetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
        if (!info)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }    
        
        HJPrioComBlurHoriShader::Ptr draw = std::dynamic_pointer_cast<HJPrioComBlurHoriShader>(m_draw);
        if (draw)
        {
            draw->setWidth(info->m_width);
            draw->setHeight(info->m_height);
        }    

        HJPrioComFBOBase::check(info->m_width, info->m_height);
#endif
    } while (false);
    return i_err;
}
int HJPrioComBlurHori::render(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        if (!HJPrioComFBOBase::IsReady())
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)
        if (m_draw)
        {
            HJPrioComBaseFBOInfo::Ptr info = nullptr;
            HJ_CatchMapGetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
            if (!info)
            {
                i_err = HJ_WOULD_BLOCK;
                break;
            } 
            i_err = HJPrioComFBOBase::draw([this, info]()
            {
                return m_draw->draw(info->m_texture, "crop", info->m_width, info->m_height, info->m_width, info->m_height, info->m_matrix, false);
            });
        }
#endif
    } while (false);
    return i_err;
}

int HJPrioComBlurHori::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJPrioComFBOBase::init(i_param);
        if (i_err < 0)
        {
            break;
        }
#if defined(HarmonyOS)       
        m_draw = HJPrioComBlurHoriShader::Create();
        i_err = m_draw->init();
        if (i_err < 0)
        {
            break;
        }
#endif
    } while (false);
    return i_err;
}
void HJPrioComBlurHori::done() 
{
#if defined(HarmonyOS)
    if (m_draw)
    {
        m_draw->release();
        m_draw = nullptr;
    }    
#endif
    HJPrioComFBOBase::done();
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HJPrioComFBOBlur::HJPrioComFBOBlur()
{
    HJ_SetInsName(HJPrioComFBOBlur);
    setPriority(HJPrioComType_VideoBlur);
}

HJPrioComFBOBlur::~HJPrioComFBOBlur()
{

}

int HJPrioComFBOBlur::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
//        HJPrioComBaseFBOInfo::Ptr info = nullptr;
//        HJ_CatchMapGetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
//        if (!info)
//        {
//            i_err = HJ_WOULD_BLOCK;
//            break;
//        }
        i_err = m_blurHori->update(i_param);
        if (i_err < 0)
        {
            break;
        }    
        i_err = m_blurHori->render(i_param);
        if (i_err < 0)
        {
            break;
        }     
        i_param->remove(HJ_CatchName(HJPrioComBaseFBOInfo::Ptr));
        HJPrioComBaseFBOInfo::Ptr info = HJPrioComBaseFBOInfo::Create<HJPrioComBaseFBOInfo>(m_blurHori->width(),m_blurHori->height(), m_blurHori->getMatrix(), m_blurHori->texture());
        HJ_CatchMapSetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
        
        HJPrioComBlurVertShader::Ptr draw = std::dynamic_pointer_cast<HJPrioComBlurVertShader>(m_draw);
        if (draw)
        {
            draw->setWidth(info->m_width);
            draw->setHeight(info->m_height);
        }
        HJPrioComFBOBase::check(info->m_width, info->m_height);
#endif
    } while (false);
    return i_err;
}
int HJPrioComFBOBlur::render(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        if (!HJPrioComFBOBase::IsReady())
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)
        if (m_draw)
        {
            HJPrioComBaseFBOInfo::Ptr info = nullptr;
            HJ_CatchMapGetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
            if (!info)
            {
                i_err = HJ_WOULD_BLOCK;
                break;
            } 
            i_err = HJPrioComFBOBase::draw([this, info]()
            {
                return m_draw->draw(info->m_texture, "crop", info->m_width, info->m_height, info->m_width, info->m_height, info->m_matrix, false);
            });
        }
#endif
    } while (false);
    return i_err;
}

int HJPrioComFBOBlur::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJPrioComFBOBase::init(i_param);
        if (i_err < 0)
        {
            break;
        }
#if defined(HarmonyOS)    
        m_blurHori = HJPrioComBlurHori::Create();
        i_err = m_blurHori->init(i_param);
        if (i_err < 0)
        {
            break;
        }
        
        m_draw = HJPrioComBlurVertShader::Create();
        i_err = m_draw->init();
        if (i_err < 0)
        {
            break;
        }
#endif
    } while (false);
    return i_err;
}
void HJPrioComFBOBlur::done() 
{
#if defined(HarmonyOS)
    if (m_draw)
    {
        m_draw->release();
        m_draw = nullptr;
    }   
    
    if (m_blurHori)
    {
        m_blurHori->done();
        m_blurHori = nullptr;
    }
#endif
    HJPrioComFBOBase::done();
}

NS_HJ_END

#endif