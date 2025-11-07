#pragma once

#include "HJMediaData.h"
#include "HJMediaUtils.h"
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

class HJOGPointShader
{
public:
    HJ_DEFINE_CREATE(HJOGPointShader);

	HJOGPointShader() = default;
	virtual ~HJOGPointShader() = default;
    
    int init(int i_pointCnt, float i_pointSize = 10.f, HJColor i_color = {0.f, 1.f, 0.f, 1.f});
    int update(std::vector<HJPointf> i_points);
    int draw();
    void release();
    static HJPointf convert(HJPointf i_src, int i_width, int i_height, bool i_bTopLeft = true); 
	
private:
    const static std::string s_vertexShader;
    const static std::string s_fragmentShader;
    
    HJOGShaderProgram::Ptr m_shaderProgram = nullptr;
    GLint maPositionHandle = 0;
    GLint muMVPMatrixHandle = 0;
    GLint mColorHandle = 0;
    GLint mPointSizeHandle = 0;

    GLuint VBO = 0;
    std::vector<HJPointf> m_points;
    
    int m_pointCnt = 0;
    float m_pointSize = 10.f;
    HJColor m_color{0.f, 1.f, 0.f, 1.f};
    float m_mvpMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; 
};

NS_HJ_END

#endif