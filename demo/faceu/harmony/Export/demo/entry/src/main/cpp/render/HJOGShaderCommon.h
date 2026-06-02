#pragma once


#include <string>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

typedef enum HJOGGLTextureStyle 
{
    TEXTURE_TYPE_2D = 0,
    TEXTURE_TYPE_OES,
} HJOGGLTextureStyle;
typedef enum HJWindowRenderMode
{
	HJWindowRenderMode_CLIP,
	HJWindowRenderMode_FIT,
	HJWindowRenderMode_FULL,
	HJWindowRenderMode_ORIGIN,
} HJWindowRenderMode;

class HJOGShaderCommon
{
public:
    
    HJOGShaderCommon();
    virtual ~HJOGShaderCommon();
    
    const static std::string s_vertexCopyShader;
    const static std::string s_fragmentCopyShader;
    const static std::string s_fragmentCopyShaderOES;
    
    const static std::string s_fragmentCopyPreMultipleShader;
    const static std::string s_fragmentCopyPreMultipleShaderOES;
            
    const static GLfloat s_rectangleSTRIPVertexs[];
    
    static void GetScaleFromMode(int i_mode, int i_srcW, int i_srcH, int i_screenW, int i_screenH, float& o_scalex, float& o_scaley);
};



