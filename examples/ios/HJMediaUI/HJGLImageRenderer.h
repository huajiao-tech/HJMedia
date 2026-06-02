#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>

class HJGLImageRenderer {
public:
    HJGLImageRenderer();
    ~HJGLImageRenderer();

    bool setup();
    void resize(int width, int height);
    bool uploadImage(const std::vector<uint8_t>& data, int width, int height);
    void draw();
    void draw(GLuint i_texture);
    bool isReady() const { return m_ready; }

private:
    void destroy();
    void updateMatrix();
    GLuint compileShader(GLenum type, const std::string& source);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
    std::string composeVertexShader() const;
    std::string composeFragmentShader() const;

    GLuint m_program = 0;
    GLuint m_vertexBuffer = 0;
    GLuint m_texture = 0;
    GLint m_samplerLocation = -1;
    GLint m_positionLocation = -1;
    GLint m_uvLocation = -1;
    GLint m_matrixLocation = -1;
    int m_viewWidth = 0;
    int m_viewHeight = 0;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
    std::array<float, 16> m_mvpMatrix {};
    bool m_ready = false;
};
