#include "HJOGCopyShaderStrip.h"
#include "HJFLog.h"
#include "linmath.h"
#include "HJOGShaderCommon.h"
#include "HJComUtils.h"

#if defined(HarmonyOS)

NS_HJ_BEGIN

HJOGCopyShaderStrip::HJOGCopyShaderStrip()
{
    HJ_SetInsName(HJOGCopyShaderStrip);
}
HJOGCopyShaderStrip::~HJOGCopyShaderStrip()
{
}

int HJOGCopyShaderStrip::init(int i_nFlag)
{
    int i_err = 0;
    do
    {
        m_nFlag = i_nFlag;
        
        if (m_nFlag & OGCopyShaderStripFlag_2D)
        {
            m_textureStyle = TEXTURE_TYPE_2D;
        }
        else if (m_nFlag & OGCopyShaderStripFlag_OES)
        {
            m_textureStyle = TEXTURE_TYPE_OES;
        }

        std::string vertexShader = shaderGetVertex();
        if (vertexShader.empty())
        {
            vertexShader = HJOGShaderCommon::s_vertexCopyShader;
        }
        std::string fragShader = shaderGetFragment();
        if (fragShader.empty())
        {
            if (m_nFlag & OGCopyShaderStripFlag_2D)
            {
                fragShader = HJOGShaderCommon::s_fragmentCopyShader;
            }
            else if (m_nFlag & OGCopyShaderStripFlag_OES)
            {
                fragShader = HJOGShaderCommon::s_fragmentCopyShaderOES;
            }
        }

        HJFLogi("{} Texture Style: {}", getInsName(), m_textureStyle == TEXTURE_TYPE_2D ? "2D" : "OES");

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
        maTextureHandle = m_shaderProgram->GetAttribLocation("a_texcood");
        if (maTextureHandle == -1)
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

        muSTMatrixHandle = m_shaderProgram->GetUniformLocation("uSTMatrix");
        if (muSTMatrixHandle == -1)
        {
            i_err = -1;
            break;
        }

        mSampleHandle = m_shaderProgram->GetUniformLocation("sTexture");
        if (mSampleHandle == -1)
        {
            i_err = -1;
            break;
        }
        
        i_err = shaderGetHandle(m_shaderProgram->getProgramId());
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
int HJOGCopyShaderStrip::draw(GLuint textureId, const std::string &i_fitMode, int srcw, int srch, int dstw, int dsth, float *texMat, bool i_bYFlip)
{
    int i_err = 0;
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

        glVertexAttribPointer(maPositionHandle, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), HJOGShaderCommon::s_rectangleSTRIPVertexs);
        glEnableVertexAttribArray(maPositionHandle);

        glVertexAttribPointer(maTextureHandle, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), &HJOGShaderCommon::s_rectangleSTRIPVertexs[3]);
        glEnableVertexAttribArray(maTextureHandle);

        glActiveTexture(GL_TEXTURE0);
        if (m_textureStyle == TEXTURE_TYPE_2D)
        {
            glBindTexture(GL_TEXTURE_2D, textureId);
        }
        else
        {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
        }
        HJOGShaderProgram::SetInt(mSampleHandle, 0);

        mat4x4 mat, mvp;
        mat4x4_identity(mat);
        if (i_bYFlip)
        {
            mat[1][1] = -1;
        }
        float xscale = 1.f;
        float yscale = 1.f;

        HJOGShaderCommon::GetScaleFromMode(i_fitMode, srcw, srch, dstw, dsth, xscale, yscale);
        mat4x4_scale_aniso(mvp, mat, xscale, yscale, 1.f);

        if (!texMat)
        {
            HJOGShaderProgram::SetMatrix4v(muSTMatrixHandle, m_texMatrix, 16, false);
        } 
        else 
        {
            HJOGShaderProgram::SetMatrix4v(muSTMatrixHandle, texMat, 16, false);
        }    
        
        HJOGShaderProgram::SetMatrix4v(muMVPMatrixHandle, (float *)&mvp, 16, false);

        shaderDrawUpdate();
        // HJFLogi("Matrix");
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(maPositionHandle);
        glDisableVertexAttribArray(maTextureHandle);

        glDisable(GL_BLEND);

        if (m_textureStyle == TEXTURE_TYPE_2D)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        }
        
        shaderDrawEnd();
        //
        glUseProgram(0);
    } while (false);
    return i_err;
}
void HJOGCopyShaderStrip::release()
{
    m_shaderProgram = nullptr;
    HJFLogi("{} release", getInsName());
}

// the camera mvp transform; the effect is equal texture matrix
#if 0
            //every switch camera; back-front
			int result = 0;
            int degrees = (360 - m_transformInfo.m_display) % 360; //m_transformInfo.m_display=display.getDefaultDisplaySync().rotation * 90
            int orientation = m_transformInfo.m_cameraRotate;   //m_transformInfo.m_cameraRotate=this.cameraDevices[position].cameraOrientation;
            if (m_transformInfo.m_bFront)  //camera_position_back camera_position_front
            {
                result = (orientation + degrees) % 360;
                result = (360 - result) % 360;  // compensate the mirror
            } 
            else 
            {  // back-facing
                result = (orientation - degrees + 360) % 360;
            }
            m_result = result;
			

            //every draw frame
			mat4x4 mvp;
			mat4x4_identity(mvp);
			
			mat4x4 A;
            mat4x4_identity(A);
            if (m_transformInfo.m_bFront)
            {
                mat4x4_scale_aniso(mvp, A, -1.0f, -1.0f, 1.0f);
            } 
            else 
            {
                mat4x4_scale_aniso(mvp, A, 1.0f, -1.0f, 1.0f);
            }
            float a = 0.f;
            switch(m_result)
            {
            case 0:
                a = 0.f;
                break;
            case 90:
                a = M_PI / 2.f;
                break;
            case 180:
                a = M_PI;
                break;
            case 270:
                a = M_PI * 3.f / 2.f;
                break;
            }
            
            HJFLogi("m_transformInfo: result {} bfront {} a {}", m_result , m_transformInfo.m_bFront, a);
                        
            mat4x4 rotateA;
            mat4x4_identity(rotateA);
            mat4x4_rotate(rotateA, rotateA, 0, 0, 1, -a);
            mat4x4_mul(mvp, rotateA, mvp);
            //mat4x4_rotate_Z(mvp, mvp, -a); //error, first rotate, after flip
            glUniformMatrix4fv(muMVPMatrixHandle, 1, GL_FALSE, (const GLfloat*)&mvp);
            
            mat4x4 newTexMat;
            mat4x4_identity(newTexMat);
            glUniformMatrix4fv(muSTMatrixHandle,  1, GL_FALSE, (const GLfloat*)&newTexMat);

#endif

NS_HJ_END

#endif