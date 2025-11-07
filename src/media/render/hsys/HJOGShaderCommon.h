#pragma once

#include "HJPrerequisites.h"
#include <string>

#if defined(HarmonyOS)

#include <GLES3/gl3.h>

NS_HJ_BEGIN

typedef enum HJOGGLTextureStyle 
{
    TEXTURE_TYPE_2D = 0,
    TEXTURE_TYPE_OES,
} HJOGGLTextureStyle;

class HJOGShaderCommon
{
public:
    HJ_DEFINE_CREATE(HJOGShaderCommon);
    
    HJOGShaderCommon();
    virtual ~HJOGShaderCommon();
    
    const static std::string s_vertexCopyShader;
    const static std::string s_fragmentCopyShader;
    const static std::string s_fragmentCopyShaderOES;
    
    const static std::string s_fragmentCopyPreMultipleShader;
    const static std::string s_fragmentCopyPreMultipleShaderOES;
            
    const static GLfloat s_rectangleSTRIPVertexs[];
    
    static std::string s_render_mode_fit;
    static std::string s_render_mode_clip;
    static std::string s_render_mode_origin;
    static std::string s_render_mode_full;   
    
    static void GetScaleFromMode(const std::string& i_mode, int i_srcW, int i_srcH, int i_screenW, int i_screenH, float& o_scalex, float& o_scaley);
};

NS_HJ_END

#endif

