#include "HJOGPointShader.h"
#include "HJFLog.h"
#include "HJComUtils.h"

#if defined(HarmonyOS)

NS_HJ_BEGIN
    
const std::string HJOGPointShader::s_vertexShader = R"(
uniform mat4 uMVPMatrix;
uniform vec4 u_Color;
uniform float u_PointSize;
attribute vec4 a_position;
varying vec4 v_Color;
void main() 
{
    gl_Position = uMVPMatrix * a_position;
    gl_PointSize = u_PointSize;
    v_Color = u_Color;
}
)";

const std::string HJOGPointShader::s_fragmentShader = R"(
precision mediump float;
varying vec4 v_Color;
void main() 
{
    gl_FragColor = v_Color;
}
)";

int HJOGPointShader::init(int i_pointCnt, float i_pointSize, HJColor i_color)
{
    int i_err = HJ_OK;
    do 
    {
        std::string vertexShader = s_vertexShader;
        std::string fragShader = s_fragmentShader;
        
        m_shaderProgram = HJOGShaderProgram::Create();
        i_err = m_shaderProgram->init(vertexShader, fragShader);
        if (i_err < 0)
        {
            break;
        }

        maPositionHandle = m_shaderProgram->GetAttribLocation("a_position");
        if (maPositionHandle == -1)
        {
            i_err = -1;
            break;
        }

        muMVPMatrixHandle = m_shaderProgram->GetUniformLocation("uMVPMatrix");
        if (muMVPMatrixHandle == -1)
        {
            i_err = -1;
            break;
        }

        mColorHandle = m_shaderProgram->GetUniformLocation("u_Color");
        if (mColorHandle == -1)
        {
            i_err = -1;
            break;
        }
        
        mPointSizeHandle = m_shaderProgram->GetUniformLocation("u_PointSize");
        if (mPointSizeHandle == -1)
        {
            i_err = -1;
            break;
        }
        
        for (int i = 0; i < i_pointCnt; i++)
        {
            m_points.push_back(HJPointf{0.f, 0.f});
        }
        
        glGenBuffers(1, &VBO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(HJPointf), (const void *)0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        
        m_pointCnt = i_pointCnt;
        m_pointSize = i_pointSize;
        m_color = i_color;
    } while (false);
    HJFLogi("shader init end i_err:{}", i_err);
    return i_err;
}
int HJOGPointShader::update(std::vector<HJPointf> i_points)
{
    int i_err = HJ_OK;
    do 
    {
        if (i_points.size() != m_pointCnt)
        {
            break;
        }    
        m_points = std::move(i_points);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_points.size() * sizeof(HJPointf), m_points.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
    } while (false);
    return i_err;
    
}
int HJOGPointShader::draw()
{
    int i_err = HJ_OK;
    do 
    {
        if (!m_shaderProgram)
        {
            i_err = -1;
            break;
        }
        m_shaderProgram->Use();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        
        glEnableVertexAttribArray(maPositionHandle);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(maPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0); //only x, y
           
        glUniform4f(mColorHandle, m_color.r, m_color.g, m_color.b, m_color.a);
            
        HJOGShaderProgram::SetMatrix4v(muMVPMatrixHandle, m_mvpMatrix, 16, false);

        glUniform1f(mPointSizeHandle, m_pointSize);

        glDrawArrays(GL_POINTS, 0, m_pointCnt);
        glDisableVertexAttribArray(maPositionHandle);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisable(GL_BLEND);
        glUseProgram(0);  
    } while (false);
    return i_err;
}
void HJOGPointShader::release()
{
    m_shaderProgram = nullptr;    
}
HJPointf HJOGPointShader::convert(HJPointf i_src, int i_width, int i_height, bool i_bTopLeft)
{
    int y = i_bTopLeft ? i_height - i_src.y : i_src.y;
    int x = i_src.x;
    
    return HJPointf{x * 2.f / i_width - 1.f, y * 2.f / i_height - 1.f};
}

NS_HJ_END

#endif