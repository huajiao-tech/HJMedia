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
#ifndef NATIVE_XCOMPONENT_EGL_CORE_H
#define NATIVE_XCOMPONENT_EGL_CORE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <string>

namespace NativeXComponentSample {
class EGLCore {
public:
    explicit EGLCore(const std::string& id) { this->m_id = id; };
    ~EGLCore() {}
    bool EglContextInit(void* window, int width, int height);
    bool CreateEnvironment();
    void Draw(int& hasDraw);
    void Background();
    void ChangeColor(int& hasChangeColor);
    void Release();
    void UpdateSize(int width, int height);
    void LoadYuv(const char * path);

private:
    GLuint LoadShader(GLenum type, const char* shaderSrc);
    GLuint CreateProgram(const char* vertexShader, const char* fragShader);
    GLint PrepareDraw();
    bool ExecuteDraw(GLint position, const GLfloat* color, const GLfloat shapeVertices[], unsigned long vertSize);
    bool ExecuteDrawStar(GLint position, const GLfloat* color, const GLfloat shapeVertices[], unsigned long vertSize);
    bool ExecuteDrawNewStar(GLint position, const GLfloat* color, const GLfloat shapeVertices[], unsigned long vertSize);
    void Rotate2d(GLfloat centerX, GLfloat centerY, GLfloat* rotateX, GLfloat* rotateY, GLfloat theta);
    bool FinishDraw();
    bool ReadYuvFile(const char *datasource, int width, int height);
    bool PrepareLoad();
    void YuvRender(int width, int height);
    bool FinishLoad();

private:
    EGLNativeWindowType m_eglWindow;
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLConfig m_eglConfig = EGL_NO_CONFIG_KHR;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLContext m_eglContext = EGL_NO_CONTEXT;
    GLuint m_program;
    bool m_flag = false;
    int m_width;
    int m_height;
    GLfloat m_widthPercent;
    unsigned char *m_buf[3] = { 0 };
    std::string m_id;
};
} // namespace NativeXComponentSample
#endif // NATIVE_XCOMPONENT_EGL_CORE_H
