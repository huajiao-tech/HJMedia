/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "egl_core.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>
#include <hilog/log.h>

#include "../common/common.h"
#include "plugin_render.h"

namespace NativeXComponentSample {

bool EGLCore::EglContextInit(void* window, int width, int height) {
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "EglContextInit execute");
    if ((window == nullptr) || (width <= 0) || (height <= 0)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "EglContextInit: param error");
        return false;
    }

    UpdateSize(width, height);
    m_eglWindow = static_cast<EGLNativeWindowType>(window);

    // Init display.
    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglGetDisplay: unable to get EGL display");
        return false;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    if (!eglInitialize(m_eglDisplay, &majorVersion, &minorVersion)) {
        OH_LOG_Print(
            LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglInitialize: unable to get initialize EGL display");
        return false;
    }

    // Select configuration.
    const EGLint maxConfigSize = 1;
    EGLint numConfigs;
    if (!eglChooseConfig(m_eglDisplay, ATTRIB_LIST, &m_eglConfig, maxConfigSize, &numConfigs)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglChooseConfig: unable to choose configs");
        return false;
    }

    return CreateEnvironment();
}

bool EGLCore::CreateEnvironment() {
    // Create surface.
    if (m_eglWindow == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglWindow_ is null");
        return false;
    }
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_eglWindow, NULL);
    if (m_eglSurface == nullptr) {
        OH_LOG_Print(
            LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglCreateWindowSurface: unable to create surface");
        return false;
    }
    // Create context.
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, CONTEXT_ATTRIBS);
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglMakeCurrent failed");
        return false;
    }
    // Create program.
    m_program = CreateProgram(this->m_id == OPENGL_XCOMPONENT_ID ? VERTEX_SHADER: VERTEX_SHADER_YUV,
        this->m_id == OPENGL_XCOMPONENT_ID ? FRAGMENT_SHADER: FRAGMENT_SHADER_YUV);
    if (m_program == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "CreateProgram: unable to create program");
        return false;
    }
    return true;
}

void EGLCore::Background() {
    GLint position = PrepareDraw();
    if (position == POSITION_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Background get position failed");
        return;
    }

    if (!ExecuteDraw(position, BACKGROUND_COLOR,
                     BACKGROUND_RECTANGLE_VERTICES, sizeof(BACKGROUND_RECTANGLE_VERTICES))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Background execute draw failed");
        return;
    }

    if (!FinishDraw()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Background FinishDraw failed");
        return;
    }
}

void EGLCore::Draw(int& hasDraw) {
    m_flag = false;
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "Draw");
    GLint position = PrepareDraw();
    if (position == POSITION_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw get position failed");
        return;
    }

    if (!ExecuteDraw(position, BACKGROUND_COLOR,
                     BACKGROUND_RECTANGLE_VERTICES, sizeof(BACKGROUND_RECTANGLE_VERTICES))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw execute draw background failed");
        return;
    }

    // Divided into five quadrilaterals and calculate one of the quadrilateral's Vertices.
    GLfloat rotateX = 0;
    GLfloat rotateY = FIFTY_PERCENT * m_height;
    GLfloat centerX = 0;
    // Convert DEG(54° & 18°) to RAD.
    GLfloat centerY = -rotateY * (M_PI / 180 * 54) * (M_PI / 180 * 18);
    // Convert DEG(18°) to RAD.
    GLfloat leftX = -rotateY * (M_PI / 180 * 18);
    GLfloat leftY = 0;
    // Convert DEG(18°) to RAD.
    GLfloat rightX = rotateY * (M_PI / 180 * 18);
    GLfloat rightY = 0;

    const GLfloat shapeVertices[] = { centerX / m_width, centerY / m_height, leftX / m_width, leftY / m_height,
        rotateX / m_width, rotateY / m_height, rightX / m_width, rightY / m_height };

    if (!ExecuteDrawStar(position, DRAW_COLOR, shapeVertices, sizeof(shapeVertices))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw execute draw shape failed");
        return;
    }

    // Convert DEG(72°) to RAD.
    GLfloat rad = M_PI / 180 * 72;
    // Rotate four times
    for (int i = 0; i < NUM_4; ++i) {
        Rotate2d(centerX, centerY, &rotateX, &rotateY, rad);
        Rotate2d(centerX, centerY, &leftX, &leftY, rad);
        Rotate2d(centerX, centerY, &rightX, &rightY, rad);

        const GLfloat shapeVertices[] = { centerX / m_width, centerY / m_height, leftX / m_width, leftY / m_height,
            rotateX / m_width, rotateY / m_height, rightX / m_width, rightY / m_height };

        if (!ExecuteDrawStar(position, DRAW_COLOR, shapeVertices, sizeof(shapeVertices))) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw execute draw shape failed");
            return;
        }
    }

    if (!FinishDraw()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw FinishDraw failed");
        return;
    }
    hasDraw = 1;

    m_flag = true;
}

void EGLCore::ChangeColor(int& hasChangeColor) {
    if (!m_flag) {
        return;
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "ChangeColor");
    GLint position = PrepareDraw();
    if (position == POSITION_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "ChangeColor get position failed");
        return;
    }

    if (!ExecuteDraw(position, BACKGROUND_COLOR,
                     BACKGROUND_RECTANGLE_VERTICES, sizeof(BACKGROUND_RECTANGLE_VERTICES))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "ChangeColor execute draw background failed");
        return;
    }

    // Divided into five quadrilaterals and calculate one of the quadrilateral's Vertices
    GLfloat rotateX = 0;
    GLfloat rotateY = FIFTY_PERCENT * m_height;
    GLfloat centerX = 0;
    // Convert DEG(54° & 18°) to RAD
    GLfloat centerY = -rotateY * (M_PI / 180 * 54) * (M_PI / 180 * 18);
    // Convert DEG(18°) to RAD
    GLfloat leftX = -rotateY * (M_PI / 180 * 18);
    GLfloat leftY = 0;
    // Convert DEG(18°) to RAD
    GLfloat rightX = rotateY * (M_PI / 180 * 18);
    GLfloat rightY = 0;

    const GLfloat shapeVertices[] = { centerX / m_width, centerY / m_height, leftX / m_width, leftY / m_height,
        rotateX / m_width, rotateY / m_height, rightX / m_width, rightY / m_height };

    if (!ExecuteDrawNewStar(0, CHANGE_COLOR, shapeVertices, sizeof(shapeVertices))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw execute draw shape failed");
        return;
    }

    // Convert DEG(72°) to RAD
    GLfloat rad = M_PI / 180 * 72;
    // Rotate four times
    for (int i = 0; i < NUM_4; ++i) {
        Rotate2d(centerX, centerY, &rotateX, &rotateY, rad);
        Rotate2d(centerX, centerY, &leftX, &leftY, rad);
        Rotate2d(centerX, centerY, &rightX, &rightY, rad);
        const GLfloat shapeVertices[] = { centerX / m_width, centerY / m_height, leftX / m_width, leftY / m_height,
            rotateX / m_width, rotateY / m_height, rightX / m_width, rightY / m_height };

        if (!ExecuteDrawNewStar(position, CHANGE_COLOR, shapeVertices, sizeof(shapeVertices))) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Draw execute draw shape failed");
            return;
        }
    }

    if (!FinishDraw()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "ChangeColor FinishDraw failed");
    }
    hasChangeColor = 1;
}

GLint EGLCore::PrepareDraw() {
    if ((m_eglDisplay == nullptr) || (m_eglSurface == nullptr) || (m_eglContext == nullptr) ||
        (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "PrepareDraw: param error");
        return POSITION_ERROR;
    }

    // The gl function has no return value.
    glViewport(DEFAULT_X_POSITION, DEFAULT_Y_POSITION, m_width, m_height);
    glClearColor(GL_RED_DEFAULT, GL_GREEN_DEFAULT, GL_BLUE_DEFAULT, GL_ALPHA_DEFAULT);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_program);

    return glGetAttribLocation(m_program, POSITION_NAME);
}

bool EGLCore::ExecuteDraw(GLint position, const GLfloat* color, const GLfloat shapeVertices[], unsigned long vertSize) {
    if ((position > 0) || (color == nullptr) || (vertSize / sizeof(shapeVertices[0])) != SHAPE_VERTICES_SIZE) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "ExecuteDraw: param error");
        return false;
    }

    // The gl function has no return value.
    glVertexAttribPointer(position, POINTER_SIZE, GL_FLOAT, GL_FALSE, 0, shapeVertices);
    glEnableVertexAttribArray(position);
    glVertexAttrib4fv(1, color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, TRIANGLE_FAN_SIZE);
    glDisableVertexAttribArray(position);

    return true;
}

bool EGLCore::ExecuteDrawStar( GLint position, const GLfloat* color, const GLfloat shapeVertices[],
    unsigned long vertSize) {
    if ((position > 0) || (color == nullptr) || (vertSize / sizeof(shapeVertices[0])) != SHAPE_VERTICES_SIZE) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "ExecuteDraw: param error");
        return false;
    }

    // The gl function has no return value.
    glVertexAttribPointer(position, POINTER_SIZE, GL_FLOAT, GL_FALSE, 0, shapeVertices);
    glVertexAttribPointer(1, POINTER_SIZE, GL_FLOAT, GL_FALSE, 0, color);
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(1);
    glVertexAttrib4fv(1, color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, TRIANGLE_FAN_SIZE);
    glDisableVertexAttribArray(position);
    glDisableVertexAttribArray(1);

    return true;
}

bool EGLCore::ExecuteDrawNewStar(GLint position, const GLfloat* color, const GLfloat shapeVertices[],
    unsigned long vertSize) {
    if ((position > 0) || (color == nullptr) || (vertSize / sizeof(shapeVertices[0])) != SHAPE_VERTICES_SIZE) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "ExecuteDraw: param error");
        return false;
    }

    // The gl function has no return value.
    glVertexAttribPointer(position, POINTER_SIZE, GL_FLOAT, GL_FALSE, 0, shapeVertices);
    glEnableVertexAttribArray(position);
    glVertexAttrib4fv(1, color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, TRIANGLE_FAN_SIZE);
    glDisableVertexAttribArray(position);

    return true;
}

void EGLCore::Rotate2d(GLfloat centerX, GLfloat centerY, GLfloat* rotateX, GLfloat* rotateY, GLfloat theta) {
    GLfloat tempX = cos(theta) * (*rotateX - centerX) - sin(theta) * (*rotateY - centerY);
    GLfloat tempY = sin(theta) * (*rotateX - centerX) + cos(theta) * (*rotateY - centerY);
    *rotateX = tempX + centerX;
    *rotateY = tempY + centerY;
}

bool EGLCore::FinishDraw() {
    // The gl function has no return value.
    glFlush();
    glFinish();
    return eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

GLuint EGLCore::LoadShader(GLenum type, const char* shaderSrc) {
    if ((type <= 0) || (shaderSrc == nullptr)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCreateShader type or shaderSrc error");
        return PROGRAM_ERROR;
    }

    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCreateShader unable to load shader");
        return PROGRAM_ERROR;
    }

    // The gl function has no return value.
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != 0) {
        return shader;
    }

    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen <= 1) {
        glDeleteShader(shader);
        return PROGRAM_ERROR;
    }

    char* infoLog = (char*)malloc(sizeof(char) * (infoLen + 1));
    if (infoLog != nullptr) {
        memset(infoLog, 0, infoLen + 1);
        glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCompileShader error = %s", infoLog);
        free(infoLog);
        infoLog = nullptr;
    }
    glDeleteShader(shader);
    return PROGRAM_ERROR;
}

GLuint EGLCore::CreateProgram(const char* vertexShader, const char* fragShader) {
    if ((vertexShader == nullptr) || (fragShader == nullptr)) {
        OH_LOG_Print(
            LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram: vertexShader or fragShader is null");
        return PROGRAM_ERROR;
    }

    GLuint vertex = LoadShader(GL_VERTEX_SHADER, vertexShader);
    if (vertex == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram vertex error");
        return PROGRAM_ERROR;
    }

    GLuint fragment = LoadShader(GL_FRAGMENT_SHADER, fragShader);
    if (fragment == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram fragment error");
        return PROGRAM_ERROR;
    }

    GLuint program = glCreateProgram();
    if (program == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram program error");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return PROGRAM_ERROR;
    }

    // The gl function has no return value.
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return program;
    }

    OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram linked error");
    GLint infoLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
        char* infoLog = (char*)malloc(sizeof(char) * (infoLen + 1));
        memset(infoLog, 0, infoLen + 1);
        glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glLinkProgram error = %s", infoLog);
        free(infoLog);
        infoLog = nullptr;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(program);
    return PROGRAM_ERROR;
}

void EGLCore::UpdateSize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_width > 0) {
        m_widthPercent = FIFTY_PERCENT * m_height / m_width;
    }
}

void EGLCore::Release() {
    if ((m_eglDisplay == nullptr) || (m_eglSurface == nullptr) || (!eglDestroySurface(m_eglDisplay, m_eglSurface))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release eglDestroySurface failed");
    }

    if ((m_eglDisplay == nullptr) || (m_eglContext == nullptr) || (!eglDestroyContext(m_eglDisplay, m_eglContext))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release eglDestroyContext failed");
    }

    if ((m_eglDisplay == nullptr) || (!eglTerminate(m_eglDisplay))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release eglTerminate failed");
    }
}

void EGLCore::LoadYuv(const char * path) {
    if (!PrepareLoad()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "unable to prepareLoad");
    }
    // 根据yuv图片的宽高设置
    int width = 320;
    int height = 240;

    if (!ReadYuvFile(path, width, height)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "unable to readYuvFile");
    }
    YuvRender(width, height);
    FinishLoad();
}

void EGLCore::YuvRender(int width, int height) {
    // Texture ID.
    GLuint texts[3] = { 0 };
    // Create several texture objects and get the texture ID.
    glGenTextures(3, texts);

    // Bound texture. The following settings and loadings all apply to the currently bound texture object.
    // GL_TEXTURE0, GL_TEXTURE1, and GL_TEXTURE2 are texture units.
    // GL_Texture_1D, GL_Texture_2D, and CUBE_MAP are texture targets.
    // After the texture target is bound to the texture through the glBindTexture function, 
    // the operations performed on the texture target are reflected on the texture.
    glBindTexture(GL_TEXTURE_2D, texts[0]);
    // Shrinking filters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // A magnified filter.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D,
                 0,                // Details are basically 0 by default.
                 GL_LUMINANCE,     // GPU internal format luminance, grayscale image.
                 width,            // Width of the loaded texture.
                 height,           // Height of the loaded texture.
                 0,                // Textured border.
                 GL_LUMINANCE,     // Data pixel format luminance, grayscale image.
                 GL_UNSIGNED_BYTE, // Data type for storing pixels.
                 NULL              // Data for the texture.
    );

    // Bind the texture.
    glBindTexture(GL_TEXTURE_2D, texts[1]);
    // Shrinking filters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Sets the format and size of the texture.
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 width / 2,        // The amount of u data is 1/4 of the screen.
                 height / 2,
                 0,                // Borders.
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 NULL
    );

    // Bind the texture.
    glBindTexture(GL_TEXTURE_2D, texts[2]);
    // Shrinking filters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Sets the format and size of the texture.
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 width / 2,        //  The amount of v data is 1/4 of the screen.
                 height / 2,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 NULL
    );

    // Activates the first layer of texture, bound to the created texture.
    glActiveTexture(GL_TEXTURE0);
    // Binds the texture corresponding to y.
    glBindTexture(GL_TEXTURE_2D, texts[0]);
    // Replace the texture.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, // Offset of the original texture.
                    width, height,          // Width and height of the loaded texture.
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, m_buf[0]);

    // Activates the second layer of texture, which is bound to the created texture.
    glActiveTexture(GL_TEXTURE1);
    // Binds the texture corresponding to u.
    glBindTexture(GL_TEXTURE_2D, texts[1]);
    // Replace the texture.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_buf[1]);
    // Activates the third layer of texture, which is bound to the created texture.
    glActiveTexture(GL_TEXTURE2);
    // Binds the texture corresponding to v.
    glBindTexture(GL_TEXTURE_2D, texts[2]);
    // Replace the texture.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_buf[2]);
}

bool EGLCore::ReadYuvFile(const char *data_source, int width, int height) {
    FILE *fp = fopen(data_source, "rb");
    if (!fp) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "file", "openFileErr");
        return false;
    }
    m_buf[0] = new unsigned char[width * height];     // y
    m_buf[1] = new unsigned char[width * height / 4]; // u
    m_buf[2] = new unsigned char[width * height / 4]; // v

    if (feof(fp) == 0) {
        fread(m_buf[0], 1, width * height, fp);
        fread(m_buf[1], 1, width * height / 4, fp);
        fread(m_buf[2], 1, width * height / 4, fp);
        fclose(fp);
        return true;
    }
    fclose(fp);
    return false;
}

bool EGLCore::PrepareLoad() {
    if ((nullptr == m_eglDisplay) || (nullptr == m_eglSurface) || (nullptr == m_eglContext) ||
        (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "PrepareDraw: param error");
        return false;
    }
    glUseProgram(m_program);
    // Add 3D vertex data.
    static float ver[] = {
        1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f
    };

    GLuint apos = static_cast<GLuint>(glGetAttribLocation(m_program, "aPosition"));
    glEnableVertexAttribArray(apos);
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 0, ver);

    // Add texture coordinate data.
    static float fragment[] = {
        1.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    GLuint aTex = static_cast<GLuint>(glGetAttribLocation(m_program, "aTextCoord"));
    glEnableVertexAttribArray(aTex);
    glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 0, fragment);

    // Texture initialization.
    GLint textureUniformY = glGetUniformLocation(m_program, "SamplerY");
    GLint textureUniformU = glGetUniformLocation(m_program, "SamplerU");
    GLint textureUniformV = glGetUniformLocation(m_program, "SamplerV");
    // Set several texture sampler variables.
    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformU, 1);
    glUniform1i(textureUniformV, 2);

    glUniform1i(glGetUniformLocation(m_program, "yTexture"), 0);
    glUniform1i(glGetUniformLocation(m_program, "uTexture"), 1);
    glUniform1i(glGetUniformLocation(m_program, "vTexture"), 2);
    return true;
}

bool EGLCore::FinishLoad() {
    // The gl function has no return value.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFlush();
    glFinish();
    // The window displays, swapping double buffers.
    bool swap = eglSwapBuffers(m_eglDisplay, m_eglSurface);
    m_flag = true;
    return swap;
}
} // namespace NativeXComponentSample
