#pragma once

#include "HJPrerequisites.h"
#include <string>

#if defined(HarmonyOS)

#include <GLES3/gl3.h>

NS_HJ_BEGIN

class HJOGShaderProgram 
{
public:
    HJ_DEFINE_CREATE(HJOGShaderProgram);
    HJOGShaderProgram();
    virtual ~HJOGShaderProgram() noexcept;

    // disallow copy and move
    HJOGShaderProgram(const HJOGShaderProgram &other) = delete;
    void operator=(const HJOGShaderProgram &other) = delete;
    HJOGShaderProgram(HJOGShaderProgram &&other) = delete;
    void operator=(HJOGShaderProgram &&other) = delete;

    int init(const std::string &vertexShader, const std::string &fragShader);
    bool Valid() const
    {
        return id_ > 0;
    }
    void Use() const
    {
        glUseProgram(id_);
    }

    void SetBool(const std::string &name, bool value);
    static void SetInt(GLint i_handle, int value);
    void SetInt(const std::string &name, int value);
    void SetFloat(const std::string &name, float value);
    void SetFloat4v(const std::string &name, float *values, int cnt);
    void SetMatrix4v(const std::string &name, float *matrix, int cnt, bool transpose = false);
    static void SetMatrix4v(GLint i_handle, float *matrix, int cnt, bool transpose = false);
    GLint GetAttribLocation(const std::string &name);
    GLint GetUniformLocation(const std::string &name);
private:
    int priCheckCompileErrors(GLuint shader, const std::string &shaderType);
    GLuint id_ = 0;
};

NS_HJ_END

#endif

