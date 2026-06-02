#include "HJGLImageRenderer.h"

#include <cstdio>

#include "HJOGCommon.h"

namespace {
struct QuadVertex {
    GLfloat position[2];
    GLfloat uv[2];
};

constexpr QuadVertex kQuadVertices[] = {
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
};
}

HJGLImageRenderer::HJGLImageRenderer() {
    m_mvpMatrix = {1.f, 0.f, 0.f, 0.f,
                   0.f, 1.f, 0.f, 0.f,
                   0.f, 0.f, 1.f, 0.f,
                   0.f, 0.f, 0.f, 1.f};
}

HJGLImageRenderer::~HJGLImageRenderer() {
    destroy();
}

bool HJGLImageRenderer::setup() {
    destroy();

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, composeVertexShader());
    if (!vertexShader) {
        return false;
    }
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, composeFragmentShader());
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return false;
    }

    m_program = linkProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!m_program) {
        return false;
    }

    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_texture = HJOGCommon::textureCreate(GL_TEXTURE_2D);

    m_positionLocation = glGetAttribLocation(m_program, "aPosition");
    m_uvLocation = glGetAttribLocation(m_program, "aTexCoord");
    m_samplerLocation = glGetUniformLocation(m_program, "uTexture");
    m_matrixLocation = glGetUniformLocation(m_program, "uMVP");

    m_ready = (m_positionLocation >= 0 && m_uvLocation >= 0 && m_samplerLocation >= 0 && m_matrixLocation >= 0);
    if (!m_ready) {
        destroy();
    }
    return m_ready;
}

void HJGLImageRenderer::resize(int width, int height) {
    m_viewWidth = width;
    m_viewHeight = height;
    updateMatrix();
}

bool HJGLImageRenderer::uploadImage(const std::vector<uint8_t>& data, int width, int height) {
    if (!m_ready || !m_texture) {
        return false;
    }
    if (data.empty() || width <= 0 || height <= 0) {
        return false;
    }
    m_imageWidth = width;
    m_imageHeight = height;
    updateMatrix();
    HJOGCommon::textureUploadRGBA(m_texture, width, height, const_cast<unsigned char*>(data.data()));
    return true;
}
void HJGLImageRenderer::draw(GLuint i_texture)
{
    if (!m_ready || !m_program || !i_texture) {
        return;
    }

    glUseProgram(m_program);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glEnableVertexAttribArray(m_positionLocation);
    glVertexAttribPointer(m_positionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(m_uvLocation);
    glVertexAttribPointer(m_uvLocation, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<const void*>(sizeof(GLfloat) * 2));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, i_texture);
    glUniform1i(m_samplerLocation, 0);
    glUniformMatrix4fv(m_matrixLocation, 1, GL_FALSE, m_mvpMatrix.data());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(m_positionLocation);
    glDisableVertexAttribArray(m_uvLocation);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void HJGLImageRenderer::draw() {
    draw(m_texture);
}

void HJGLImageRenderer::destroy() {
    if (m_vertexBuffer != 0) {
        glDeleteBuffers(1, &m_vertexBuffer);
        m_vertexBuffer = 0;
    }
    if (m_texture != 0) {
        HJOGCommon::textureDestroy(m_texture);
        m_texture = 0;
    }
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_ready = false;
}

void HJGLImageRenderer::updateMatrix() {
    m_mvpMatrix = {1.f, 0.f, 0.f, 0.f,
                   0.f, 1.f, 0.f, 0.f,
                   0.f, 0.f, 1.f, 0.f,
                   0.f, 0.f, 0.f, 1.f};

    if (m_viewWidth <= 0 || m_viewHeight <= 0 || m_imageWidth <= 0 || m_imageHeight <= 0) {
        return;
    }

    float viewAspect = static_cast<float>(m_viewWidth) / static_cast<float>(m_viewHeight);
    float imageAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (imageAspect > viewAspect) {
        scaleY = viewAspect / imageAspect;
    } else {
        scaleX = imageAspect / viewAspect;
    }
    m_mvpMatrix[0] = scaleX;
    m_mvpMatrix[5] = scaleY;
}

GLuint HJGLImageRenderer::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<char> log(length);
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            std::fprintf(stderr, "Shader compile error: %s\n", log.data());
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint HJGLImageRenderer::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::vector<char> log(length);
            glGetProgramInfoLog(program, length, nullptr, log.data());
            std::fprintf(stderr, "Program link error: %s\n", log.data());
        }
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

std::string HJGLImageRenderer::composeVertexShader() const {
    std::string shader = HJOGCommon::HJ_GLSL_VERSION;
    shader += HJOGCommon::HJ_GLSL_PRECISION;
    shader += R"(
        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec2 aTexCoord;
        uniform mat4 uMVP;
        out vec2 vTexCoord;
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uMVP * vec4(aPosition, 0.0, 1.0);
        }
    )";
    return shader;
}

std::string HJGLImageRenderer::composeFragmentShader() const {
    std::string shader = HJOGCommon::HJ_GLSL_VERSION;
    shader += HJOGCommon::HJ_GLSL_PRECISION;
    shader += R"(
        in vec2 vTexCoord;
        layout(location = 0) out vec4 fragColor;
        uniform sampler2D uTexture;
        void main() {
            fragColor = texture(uTexture, vTexCoord);
        }
    )";
    return shader;
}
