#include "HJOGBaseShader.h"
#include "HJFLog.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN

//////////////////////////////////                                                                                               
const std::string HJOGShaderFilterGray::s_fragmentShader =                                                                       
std::string(HJOGCommon::HJ_GLSL_VERSION) +                                                                                                    
HJOGCommon::HJ_GLSL_PRECISION +                                                                                                  
R"(                                                                                                                              
in vec2  v_texcood;
uniform sampler2D sTexture;
out vec4 FragColor;
void main()
{
    vec4 color = texture(sTexture,v_texcood);
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(gray), color.a);    
}
)";

//////////////////////////////////                                                                                               
const std::string HJOGShaderFilterBlurHori::s_fragmentShader =                                                                   
std::string(HJOGCommon::HJ_GLSL_VERSION) +                                                                                                    
HJOGCommon::HJ_GLSL_PRECISION +                                                                                                  
R"(                                                                                                                              
in vec2  v_texcood;
uniform sampler2D sTexture;
uniform vec2 uStride;
out vec4 FragColor;
void main()
{
   vec4 result = vec4(0.0);
					 vec2 tex_offset = uStride; 
					 result += texture(sTexture, v_texcood) * 0.227027;	
					 result += texture(sTexture, v_texcood + vec2(tex_offset.x, 0.0)) * 0.1945946;	
					 result += texture(sTexture, v_texcood + vec2(2.0 * tex_offset.x, 0.0)) * 0.1216216;	
					 result += texture(sTexture, v_texcood + vec2(3.0 * tex_offset.x, 0.0)) * 0.054054;	
					 result += texture(sTexture, v_texcood + vec2(4.0 * tex_offset.x, 0.0)) * 0.016216;	
					 result += texture(sTexture, v_texcood - vec2(tex_offset.x, 0.0)) * 0.1945946;
					 result += texture(sTexture, v_texcood - vec2(2.0 * tex_offset.x, 0.0)) * 0.1216216;
					 result += texture(sTexture, v_texcood - vec2(3.0 * tex_offset.x, 0.0)) * 0.054054;
					 result += texture(sTexture, v_texcood - vec2(4.0 * tex_offset.x, 0.0)) * 0.016216;	
					 FragColor = result; 
}

)";
const std::string HJOGShaderFilterBlurVert::s_fragmentShader =                                                                   
std::string(HJOGCommon::HJ_GLSL_VERSION) +                                                                                                    
HJOGCommon::HJ_GLSL_PRECISION +                                                                                                  
R"(                                                                                                                              
in vec2  v_texcood;
uniform sampler2D sTexture;
uniform vec2 uStride;
out vec4 FragColor;
void main()
{
 vec4 result = vec4(0.0);
					 vec2 tex_offset = uStride; 
					 result += texture(sTexture, v_texcood) * 0.227027;	
					 result += texture(sTexture, v_texcood + vec2(0.0, tex_offset.y)) * 0.1945946;	
					 result += texture(sTexture, v_texcood + vec2(0.0, 2.0 * tex_offset.y)) * 0.1216216;
					 result += texture(sTexture, v_texcood + vec2(0.0, 3.0 * tex_offset.y)) * 0.054054;	
					 result += texture(sTexture, v_texcood + vec2(0.0, 4.0 * tex_offset.y)) * 0.016216;
					 result += texture(sTexture, v_texcood - vec2(0.0, tex_offset.y)) * 0.1945946;	
					 result += texture(sTexture, v_texcood - vec2(0.0, 2.0 * tex_offset.y)) * 0.1216216;
					 result += texture(sTexture, v_texcood - vec2(0.0, 3.0 * tex_offset.y)) * 0.054054;	
					 result += texture(sTexture, v_texcood - vec2(0.0, 4.0 * tex_offset.y)) * 0.016216;	
					 FragColor = result;
}

)";

///////////////////////////////////////////////////////////////////                                                              
const std::string HJOGShaderFilterSplitScreenLROES::s_fragmentShader =                                                           
std::string(HJOGCommon::HJ_GLSL_VERSION) +                                                                                                    
R"(                                                                                                                              
#extension GL_OES_EGL_image_external_essl3 :require 
precision mediump float;
in vec2  v_texcood;    
uniform samplerExternalOES sTexture;
out vec4 FragColor;
void main() 
{
    vec4 planeAlpha = texture(sTexture,vec2(v_texcood.x * 0.5, v_texcood.y));
    vec4 planeColor = texture(sTexture,vec2(v_texcood.x * 0.5 + 0.5, v_texcood.y));
    FragColor = vec4(planeColor.rgb * planeAlpha.r, planeAlpha.r);    
}
)";

///////////////////////////////////////////////////////////////////                                                              
const std::string HJOGShaderFilterSplitScreenLR2D::s_fragmentShader =                                                            
std::string(HJOGCommon::HJ_GLSL_VERSION) +                                                                                                    
HJOGCommon::HJ_GLSL_PRECISION +                                                                                                  
R"(                                                                                                                              
in vec2  v_texcood;
uniform sampler2D sTexture;
out vec4 FragColor;
void main() 
{
    vec4 planeAlpha = texture(sTexture,vec2(v_texcood.x * 0.5, v_texcood.y));
    vec4 planeColor = texture(sTexture,vec2(v_texcood.x * 0.5 + 0.5, v_texcood.y));
    FragColor = vec4(planeColor.rgb * planeAlpha.r, planeAlpha.r);    
}
)";
///////////////////////////////////////////////////////////////////

HJOGBaseShader::~HJOGBaseShader()
{
    //HJFLogi("{} ~ this:{}", getInsName(), size_t(this));    
}
HJOGBaseShader::Ptr HJOGBaseShader::createShader(int i_shaderType, bool i_bForceXMirror, bool i_bForceYMirror)
{
    HJOGBaseShader::Ptr shader = nullptr;
    int i_err = HJ_OK;
    switch (i_shaderType)
    {
    case HJOGBaseShaderType_PreMul_Copy_OES:
        shader = HJOGCopyShaderStrip::Create();
        shader->setInsName("HJOGBaseShaderType_PreMul_Copy_OES");
        i_err = shader->init(OGCopyShaderStripFlag_OES);
        break;
    case HJOGBaseShaderType_PreMul_Copy_2D:
        shader = HJOGCopyShaderStrip::Create();
        shader->setInsName("HJOGBaseShaderType_PreMul_Copy_2D");
        i_err = shader->init(OGCopyShaderStripFlag_2D);    
        break;

    case HJOGBaseShaderType_Copy_OES:
        shader = HJOGCopyShaderStrip::Create();
        shader->setInsName("HJOGBaseShaderType_Copy_OES");
        i_err = shader->init(OGCopyShaderStripFlag_OES, false);
        break;
    case HJOGBaseShaderType_Copy_2D:
        shader = HJOGCopyShaderStrip::Create();
        shader->setInsName("HJOGBaseShaderType_Copy_2D");
        i_err = shader->init(OGCopyShaderStripFlag_2D, false);   
        break;
    case HJOGBaseShaderType_Gray:
        shader = HJOGShaderFilterGray::Create();
        shader->setInsName("HJOGBaseShaderType_Gray");
        i_err = shader->init();    
        break;
    case HJOGBaseShaderType_BlurHori:
        shader = HJOGShaderFilterBlurHori::Create();
        shader->setInsName("HJOGBaseShaderType_Hori");
        i_err = shader->init();    
        break;
    case HJOGBaseShaderType_BlurVert:
        shader = HJOGShaderFilterBlurVert::Create();
        shader->setInsName("HJOGBaseShaderType_Vert");
        i_err = shader->init();    
        break;
    case HJOGBaseShaderType_SplitScreenLR_OES:
        shader = HJOGShaderFilterSplitScreenLROES::Create();
        shader->setInsName("HJOGBaseShaderType_SplitScreenLR_OES");
        i_err = shader->init(OGCopyShaderStripFlag_OES);    
        break;
    case HJOGBaseShaderType_SplitScreenLR_2D:
		shader = HJOGShaderFilterSplitScreenLR2D::Create();
		shader->setInsName("HJOGBaseShaderType_SplitScreenLR_2D");
		i_err = shader->init(OGCopyShaderStripFlag_2D);
        break;
    default:
        break;
    }
    if (i_err < 0)
    {
        shader = nullptr;
    }
    else
    {
        HJOGCopyShaderStrip::Ptr copyShader = std::dynamic_pointer_cast<HJOGCopyShaderStrip>(shader);
        if (copyShader)
        {
            copyShader->setForceXMirror(i_bForceXMirror);
            copyShader->setForceYMirror(i_bForceYMirror);
        }
    }
    return shader;
}


NS_HJ_END

