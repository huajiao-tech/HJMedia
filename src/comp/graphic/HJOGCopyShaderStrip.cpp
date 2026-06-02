#include "HJOGBaseShader.h"
#include "HJFLog.h"
#include "linmath.h"
#include "HJOGShaderCommon.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

HJOGCopyShaderStrip::HJOGCopyShaderStrip()
{
    HJ_SetInsName(HJOGCopyShaderStrip);
}
HJOGCopyShaderStrip::~HJOGCopyShaderStrip()
{
    release();
}

int HJOGCopyShaderStrip::init(int i_nFlag, bool i_bPreMultipleShader)
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
                fragShader = i_bPreMultipleShader ? HJOGShaderCommon::s_fragmentCopyPreMultipleShader : HJOGShaderCommon::s_fragmentCopyShader;
            }
            else if (m_nFlag & OGCopyShaderStripFlag_OES)
            {
                fragShader = i_bPreMultipleShader ? HJOGShaderCommon::s_fragmentCopyPreMultipleShaderOES : HJOGShaderCommon::s_fragmentCopyShaderOES;
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

        // Init VBO and VAO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), HJOGShaderCommon::s_rectangleSTRIPVertexs, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(maPositionHandle, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(maPositionHandle);

        // Texture coord attribute
        glVertexAttribPointer(maTextureHandle, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(maTextureHandle);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        HJFLogi("{} shader init end i_err:{}", getInsName(), i_err);
    } while (false);
    return i_err;
}

int HJOGCopyShaderStrip::draw(GLuint textureId, float *vertexMat, float *texMat)
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

        glBindVertexArray(VAO);

        glActiveTexture(GL_TEXTURE0);
        if (m_textureStyle == TEXTURE_TYPE_2D)
        {
            glBindTexture(GL_TEXTURE_2D, textureId);
        }
        else
        {
#if defined(HarmonyOS)
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
#endif
        }
        HJOGShaderProgram::SetInt(mSampleHandle, 0);

        HJOGShaderProgram::SetMatrix4v(muMVPMatrixHandle, vertexMat, 16, false);
        HJOGShaderProgram::SetMatrix4v(muSTMatrixHandle, texMat, 16, false);

        shaderDrawUpdate();
        // HJFLogi("Matrix");
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glBindVertexArray(0);

        glDisable(GL_BLEND);

        if (m_textureStyle == TEXTURE_TYPE_2D)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
#if defined(HarmonyOS)
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
#endif
        }

        shaderDrawEnd();
        //
        glUseProgram(0);
    } while (false);
    return i_err;
}

int HJOGCopyShaderStrip::draw(GLuint textureId, int i_mode, int srcw, int srch, int dstw, int dsth, float *texMat, bool i_bYFlip, bool i_bXFlip)
{
    int i_err = 0;
    do
    {
        if (!m_shaderProgram)
        {
            i_err = -1;
            break;
        }

        bool bXFlip = i_bXFlip;
        if (m_bForceXMirror)
        {
            bXFlip = true;
        }
        bool bYFlip = i_bYFlip;
        if (m_bForceYMirror)
        {
            bYFlip = true;
        }

        m_shaderProgram->Use();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(VAO);

        glActiveTexture(GL_TEXTURE0);
        if (m_textureStyle == TEXTURE_TYPE_2D)
        {
            glBindTexture(GL_TEXTURE_2D, textureId);
        }
        else
        {
#if defined(HarmonyOS)
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
#endif
        }
        HJOGShaderProgram::SetInt(mSampleHandle, 0);

        mat4x4 mat, mvp;
        mat4x4_identity(mat);
        if (bYFlip)
        {
            mat[1][1] = -1;
        }
        if (bXFlip)
        {
            mat[0][0] = -1;
        }


        float xscale = 1.f;
        float yscale = 1.f;

        HJOGShaderCommon::GetScaleFromMode(i_mode, srcw, srch, dstw, dsth, xscale, yscale);
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
        
        glBindVertexArray(0);

        glDisable(GL_BLEND);

        if (m_textureStyle == TEXTURE_TYPE_2D)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
#if defined(HarmonyOS)
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
#endif
        }

        shaderDrawEnd();
        //
        glUseProgram(0);
    } while (false);
    return i_err;
}

void HJOGCopyShaderStrip::release()
{
    if (m_shaderProgram)
    {
        m_shaderProgram = nullptr;
        HJFLogi("{} release this:{}", getInsName(), size_t(this));
    }
    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
}

NS_HJ_END
