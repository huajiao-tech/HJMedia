
#include "HJOGShaderProgram.h"
#include "HJOGShaderCommon.h"
#include "HJOGCommon.h"


const std::string HJOGShaderCommon::s_vertexCopyShader = 
std::string(HJOGCommon::HJ_GLSL_VERSION) +
R"(
uniform mat4 uMVPMatrix;
uniform mat4 uSTMatrix;
in vec4 a_position;
in vec4 a_texcood;
out vec2 v_texcood;
void main()
{
    gl_Position = uMVPMatrix * a_position;
    v_texcood = (uSTMatrix * a_texcood).xy;
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyPreMultipleShader =
std::string(HJOGCommon::HJ_GLSL_VERSION) +
std::string(HJOGCommon::HJ_GLSL_PRECISION) +
R"(
in vec2  v_texcood; 
uniform sampler2D sTexture;
out vec4 FragColor;
void main()
{
    vec4 color = texture(sTexture,v_texcood);
    FragColor = vec4(color.rgb * color.a, color.a);    
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyPreMultipleShaderOES = 
std::string(HJOGCommon::HJ_GLSL_VERSION) +
R"(
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;
in vec2  v_texcood;    
uniform samplerExternalOES sTexture;
out vec4 FragColor;
void main() 
{
    vec4 color = texture(sTexture,v_texcood);
    FragColor = vec4(color.rgb * color.a, color.a);    
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyShader = 
std::string(HJOGCommon::HJ_GLSL_VERSION) +
std::string(HJOGCommon::HJ_GLSL_PRECISION) +
R"(
in vec2  v_texcood; 
uniform sampler2D sTexture;
out vec4 FragColor;
void main()
{
    vec4 color = texture(sTexture,v_texcood);
    FragColor = vec4(color.rgb, color.a);    
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyShaderOES = 
std::string(HJOGCommon::HJ_GLSL_VERSION) +
R"(
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;
in vec2  v_texcood;    
uniform samplerExternalOES sTexture;
out vec4 FragColor;
void main() 
{
    vec4 color = texture(sTexture,v_texcood);
    FragColor = vec4(color.rgb, color.a);    
}
)";



const GLfloat HJOGShaderCommon::s_rectangleSTRIPVertexs[] = {
    1.0f, -1.0f, 0.f, 1.f, 0.f,   
    -1.0f, -1.0f,0.f, 0.f, 0.f,  
    1.0f, 1.0f,  0.f, 1.f, 1.f,  
    -1.0f, 1.0f, 0.f, 0.f, 1.f };

HJOGShaderCommon::HJOGShaderCommon()
{
    
}
HJOGShaderCommon::~HJOGShaderCommon()
{
    
}

void HJOGShaderCommon::GetScaleFromMode(int i_mode, int i_srcW, int i_srcH, int i_screenW, int i_screenH, float& o_scalex, float& o_scaley)
{
	int width = i_srcW;
	int height = i_srcH;
	int dw = i_screenW;
	int dh = i_screenH;
	double sar = (double)width / (double)height;
	double dar = (double)dw / (double)dh;
	if (i_mode == HJWindowRenderMode_FIT)
	{
		if (dar < sar)
		{
			dh = (int)(dw / sar);
		}
		else
		{
			dw = (int)(dh * sar);
		}
	}
	else if (i_mode == HJWindowRenderMode_ORIGIN)
	{
		dw = width;
		dh = height;
	}
	else if (i_mode == HJWindowRenderMode_CLIP)
	{
		if (dar < sar)
		{
			dw = (int)(dh * sar);
		}
		else
		{
			dh = (int)(dw / sar);
		}
	}
	else if (i_mode == HJWindowRenderMode_FIT)
	{
		dw = i_screenW;
		dh = i_screenH;
	}

	o_scalex = (float)dw / i_screenW;
	o_scaley = (float)dh / i_screenH;
}

