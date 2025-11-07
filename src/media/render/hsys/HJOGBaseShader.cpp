#include "HJOGBaseShader.h"
#include "HJFLog.h"
#include "HJOGBaseShader.h"

NS_HJ_BEGIN

//////////////////////////////////
const std::string HJOGShaderFilterGray::s_fragmentShader = R"(
precision mediump float;
varying vec2  v_texcood; 
uniform sampler2D sTexture;
void main()
{
    vec4 color = texture2D(sTexture,v_texcood);
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    gl_FragColor = vec4(vec3(gray), color.a);    
}
)";

//////////////////////////////////
const std::string HJOGShaderFilterBlurHori::s_fragmentShader = R"(
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
const std::string HJOGShaderFilterBlurVert::s_fragmentShader = R"(
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
///////////////////////////////////////////////////////////////////

HJOGBaseShader::~HJOGBaseShader()
{
    HJFLogi("{} ~ this:{}", getInsName(), size_t(this));    
}
HJOGBaseShader::Ptr HJOGBaseShader::createShader(int i_shaderType)
{
    HJOGBaseShader::Ptr shader = nullptr;
    int i_err = HJ_OK;
    switch (i_shaderType)
    {
    case HJOGBaseShaderType_Copy_OES:
        shader = HJOGCopyShaderStrip::Create();
        shader->setInsName("HJOGBaseShaderType_Copy_OES");
        i_err = shader->init(OGCopyShaderStripFlag_OES);
        break;
    case HJOGBaseShaderType_Copy_2D:
        shader = HJOGCopyShaderStrip::Create();
        shader->setInsName("HJOGBaseShaderType_Copy_2D");
        i_err = shader->init(OGCopyShaderStripFlag_2D);    
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
    default:
        break;
    }
    if (i_err < 0)
    {
        shader = nullptr;
    }    
    return shader;
}


NS_HJ_END

