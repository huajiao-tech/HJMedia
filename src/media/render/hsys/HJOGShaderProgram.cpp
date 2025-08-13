
#include "HJOGShaderProgram.h"
#include "HJFLog.h"

#if defined(HarmonyOS)

NS_HJ_BEGIN

HJOGShaderProgram::HJOGShaderProgram()
{

}

HJOGShaderProgram::~HJOGShaderProgram() noexcept
{
    if (Valid()) {
        glDeleteProgram(id_);
    }
}

int HJOGShaderProgram::init(const std::string &vertexShader, const std::string &fragShader)
{
    int i_err = 0;
    do 
    {
        auto vShaderCode = vertexShader.c_str();
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        i_err = priCheckCompileErrors(vertex, "VERTEX");
        if (i_err < 0)
        {
            break;
        }

        auto fShaderCode = fragShader.c_str();
        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        i_err = priCheckCompileErrors(fragment, "FRAGMENT");
        if (i_err < 0)
        {
            break;
        }
        id_ = glCreateProgram();
        if (id_ == 0)
        {
            i_err = -1;
            HJFLoge("create program error");
            break;
        }    
        glAttachShader(id_, vertex);
        glAttachShader(id_, fragment);
        glLinkProgram(id_);
        i_err = priCheckCompileErrors(id_, "PROGRAM");
        if (i_err < 0)
        {
            break;
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        
    } while (false);
    return i_err;
}

int HJOGShaderProgram::priCheckCompileErrors(GLuint shader, const std::string &shaderType)
{
    int i_err = 0;
	do
	{
		int success;
		char infoLog[1024];
		if (shaderType != "PROGRAM") 
        {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				HJFLoge("ShaderProgram ERROR::SHADER_COMPILATION_ERROR of type: {}, infoLog is:{}",shaderType.c_str(), infoLog);
                i_err = -1;
                break;
			}
		}
		else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                HJFLoge("ShaderProgram ERROR::PROGRAM_LINKING_ERROR of type: {}, infoLog is: {}", shaderType.c_str(), infoLog);
                i_err = -1;
                break;
			}
		}
	} while (false);
    return i_err;
    
}

void HJOGShaderProgram::SetBool(const std::string &name, bool value)
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), static_cast<GLint>(value));
}

void HJOGShaderProgram::SetInt(const std::string &name, int value)
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), static_cast<GLint>(value));
}
void HJOGShaderProgram::SetInt(GLint i_handle, int value)
{
    glUniform1i(i_handle, value);
}
void HJOGShaderProgram::SetFloat(const std::string &name, float value)
{
    glUniform1f(glGetUniformLocation(id_, name.c_str()), static_cast<GLfloat>(value));
}

void HJOGShaderProgram::SetFloat4v(const std::string &name, float *values, int cnt)
{
    if (cnt != 4 || values == nullptr) {
        HJFLoge("ShaderProgram::SetFloat4v: invalid arguments");
        return;
    }
    glUniform4fv(glGetUniformLocation(id_, name.c_str()), 1, values);
}

void HJOGShaderProgram::SetMatrix4v(const std::string &name, float *matrix, int cnt, bool transpose)
{
    if (cnt != 16 || matrix == nullptr) {
        HJFLoge("ShaderProgram::SetFloat4v: invalid arguments.");
        return;
    }
    GLboolean glTranspose = transpose ? GL_TRUE : GL_FALSE;
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, glTranspose, matrix);
}

void HJOGShaderProgram::SetMatrix4v(GLint i_handle, float *matrix, int cnt, bool transpose)
{
    if (cnt != 16 || matrix == nullptr) {
        HJFLoge("ShaderProgram::SetFloat4v: invalid arguments.");
        return;
    }
    GLboolean glTranspose = transpose ? GL_TRUE : GL_FALSE;
    glUniformMatrix4fv(i_handle, 1, glTranspose, matrix);    
}

GLint HJOGShaderProgram::GetAttribLocation(const std::string &name)
{
    return glGetAttribLocation(id_, name.c_str());
}

GLint HJOGShaderProgram::GetUniformLocation(const std::string &name)
{
    return glGetUniformLocation(id_, name.c_str());
}

NS_HJ_END

#endif
