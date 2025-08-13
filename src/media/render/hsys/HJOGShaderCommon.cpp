
#include "HJOGShaderProgram.h"
#include "HJFLog.h"
#include "HJOGShaderCommon.h"

#if defined(HarmonyOS)

NS_HJ_BEGIN

std::string HJOGShaderCommon::s_render_mode_fit = "fit";
std::string HJOGShaderCommon::s_render_mode_clip = "clip";
std::string HJOGShaderCommon::s_render_mode_origin = "origin";
std::string HJOGShaderCommon::s_render_mode_full = "full";  

const std::string HJOGShaderCommon::s_vertexCopyShader = R"(
uniform mat4 uMVPMatrix;
uniform mat4 uSTMatrix;
attribute vec4 a_position;
attribute vec4 a_texcood;
varying vec2 v_texcood;
void main()
{
    gl_Position = uMVPMatrix * a_position;
    v_texcood = (uSTMatrix * a_texcood).xy;
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyShader = R"(
precision mediump float;
varying vec2  v_texcood; 
uniform sampler2D sTexture;
void main()
{
    vec4 color = texture2D(sTexture,v_texcood);
    gl_FragColor = vec4(color.rgb * color.a, color.a);    
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyShaderOES = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2  v_texcood;    
uniform samplerExternalOES sTexture;
void main() 
{
    vec4 color = texture2D(sTexture,v_texcood);
    gl_FragColor = vec4(color.rgb * color.a, color.a);    
}
)"; 

const std::string HJOGShaderCommon::s_fragmentCopyShaderAlphaLeftRight = R"(
precision mediump float;
varying vec2  v_texcood; 
uniform sampler2D sTexture;
void main()
{
    vec4 planeAlpha = texture2D(sTexture,vec2(v_texcood.x * 0.5, v_texcood.y));
    vec4 planeColor = texture2D(sTexture,vec2(v_texcood.x * 0.5 + 0.5, v_texcood.y));
    gl_FragColor = vec4(planeColor.rgb * planeAlpha.r, planeAlpha.r);    
}
)";

const std::string HJOGShaderCommon::s_fragmentCopyShaderAlphaLeftRightOES = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2  v_texcood;    
uniform samplerExternalOES sTexture;
void main() 
{
    vec4 planeAlpha = texture2D(sTexture,vec2(v_texcood.x * 0.5, v_texcood.y));
    vec4 planeColor = texture2D(sTexture,vec2(v_texcood.x * 0.5 + 0.5, v_texcood.y));
    gl_FragColor = vec4(planeColor.rgb * planeAlpha.r, planeAlpha.r);    
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

void HJOGShaderCommon::GetScaleFromMode(const std::string& i_mode, int i_srcW, int i_srcH, int i_screenW, int i_screenH, float& o_scalex, float& o_scaley)
{
	int width = i_srcW;
	int height = i_srcH;
	int dw = i_screenW;
	int dh = i_screenH;
	double sar = (double)width / (double)height;
	double dar = (double)dw / (double)dh;
	if (i_mode == s_render_mode_fit)
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
	else if (i_mode == s_render_mode_origin)
	{
		dw = width;
		dh = height;
	}
	else if (i_mode == s_render_mode_clip)
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
	else if (i_mode == s_render_mode_full)
	{
		dw = i_screenW;
		dh = i_screenH;
	}

	o_scalex = (float)dw / i_screenW;
	o_scaley = (float)dh / i_screenH;
}


NS_HJ_END

#endif
