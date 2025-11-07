//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJWindow.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJImguiUtils.h"

NS_HJ_BEGIN
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
//***********************************************************************************//
HJWindow::HJWindow()
{
    //setName(HJMakeGlobalName(HJ_TYPE_NAME(HJWindow)));
    setName("HJMediaTools");
}

HJWindow::~HJWindow()
{
    done();
}

int HJWindow::init(const HJSizei wsize)
{
    int res = HJ_OK;
    HJLogi("init entry, name:" + m_name);
    do
    {
        m_wsize = wsize;
        //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_DEPTH_BITS, 16);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        // Create window with graphics context
        m_window = glfwCreateWindow(m_wsize.w, m_wsize.h, m_name.c_str(), nullptr, nullptr);
        if (!m_window) {
            HJLoge("error, glfw create window failed");
            return HJErrNewObj;
        }
        glfwSetWindowUserPointer(m_window, this);
        setIcon("logo.png");

        glfwSetFramebufferSizeCallback(m_window, HJWindow::onFramebufferSizeCallback);
        glfwSetWindowRefreshCallback(m_window, HJWindow::onWindowRefreshCallback);
        glfwSetKeyCallback(m_window, HJWindow::onKeyCallback);
        glfwSetMouseButtonCallback(m_window, HJWindow::onMouseButtonCallback);
        glfwSetCursorPosCallback(m_window, HJWindow::onCursorPositionCallback);
 /*       glfwSetScrollCallback(m_window, Viewer::ScrollWheelCallback);
        glfwSetDropCallback(m_window, Viewer::FileDropCallback);
        glfwSetWindowFocusCallback(m_window, Viewer::FocusCallback);
        glfwSetWindowIconifyCallback(m_window, Viewer::IconifyCallback);*/

        if (m_initBackends)
        {
            glfwMakeContextCurrent(m_window);
            //gladLoadGL(glfwGetProcAddress);
            //glfwSetWindowAspectRatio(m_window, 1, 1);
            glfwSwapInterval(1); // Enable vsync

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOpenGL(m_window, true);
            //
            glfwSetTime(0.0);
        }
    } while (false);

    return HJ_OK;
}

void HJWindow::done()
{
    m_rightClickMenu = nullptr;
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = NULL;
    }
}

int HJWindow::draw()
{
    int res = HJ_OK;
    do
    {
        if (m_rightClickMenu) {
            res = m_rightClickMenu->draw();
            if (m_rightClickMenu->isDone()) {
                m_rightClickMenu = nullptr;
            }
        }
    } while (false);

    return res;
}

bool HJWindow::isClose()
{
    if (!m_window) {
        return true;
    }
    return glfwWindowShouldClose(m_window);
}

void HJWindow::makeCurrent()
{
    glfwMakeContextCurrent(m_window);
}

void HJWindow::renderClear()
{
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(m_bgColor.x * m_bgColor.w, m_bgColor.y * m_bgColor.w, m_bgColor.z * m_bgColor.w, m_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void HJWindow::renderPresent()
{
    glfwSwapBuffers(m_window);
}

void HJWindow::setWindowSize(const HJSizei size)
{
    HJSizei realSize = size;
    HJPostioni pos = HJ_POS_ZERO;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (realSize.w > mode->width || realSize.h > mode->height) {
            HJLogw("warning, set window size beyond mode size");
            realSize = {mode->width, mode->height};
//            return;
        }
        pos.x = (mode->width - realSize.w) / 2;
        pos.y = (mode->height - realSize.h) / 2;
    }
    //
    if (pos.x < 0 || pos.y < 0) {
        HJFLogw("waring, set window size:{},{}", pos.x, pos.y);
        return;
    }
    HJPostioni oldPos = HJ_POS_ZERO;
    HJSizei oldSize = HJ_SIZE_ZERO;
    glfwGetWindowPos(m_window, &oldPos.x, &oldPos.y);
    glfwGetWindowSize(m_window, &oldSize.w, &oldSize.h);
    if (oldPos.x != pos.x || oldPos.y != pos.y ||
        oldSize.w != realSize.w || oldSize.h != realSize.h) {
        glfwSetWindowPos(m_window, pos.x, pos.y);
        glfwSetWindowSize(m_window, realSize.w, realSize.h);
        HJFLogi("set window pos:({}, {}), size:({},{})", pos.x, pos.y, realSize.w, realSize.h);
    }
    return;
}

const HJRecti HJWindow::getWindowRect()
{
    HJRecti rect = HJ_RECT_ZERO;
    glfwGetWindowPos(m_window, &rect.x, &rect.y);
    glfwGetWindowSize(m_window, &rect.w, &rect.h);
    return rect;
}

void HJWindow::setIcon(const std::string& iconName)
{
    std::string resDir = HJContext::Instance().getResDir();
    if (resDir.empty()) {
        return;
    }
    std::string filename = resDir + "/icon/" + iconName;
    HJImage::Ptr image = std::make_shared<HJImage>();
    int res = image->init(filename);
    if (HJ_OK != res) {
        return;
    }
    GLFWimage img = { image->getWidth(), image->getHeight(), image->getData() };
    glfwSetWindowIcon(m_window, 1, &img);
    return;
}

const HJSizei HJWindow::gettPrimaryMonitorSize()
{
    HJSizei size = HJ_SIZE_ZERO;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        size = { mode->width, mode->height };
    }
    return size;
}

HJWindow* HJWindow::getWSelf(GLFWwindow* window) {
    return  (HJWindow *)(window ? glfwGetWindowUserPointer(window) : NULL);
}

bool HJWindow::changeTheme(GLFWwindow* window, const bool isDark)
{
#if defined(HJ_OS_WINDOWS)
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) {
        return false;
    }
    BOOL USE_DARK_MODE = isDark;
    //DwmSetWindowAttribute(
    //    hwnd,
    //    DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
    //    &USE_DARK_MODE,
    //    sizeof(USE_DARK_MODE)
    //);
    BOOL pre_dark_mode = false;
    DwmGetWindowAttribute(
        hwnd,
        DWMWA_USE_IMMERSIVE_DARK_MODE,
        &pre_dark_mode,
        sizeof(pre_dark_mode) );
    if (pre_dark_mode != USE_DARK_MODE) {
        DwmSetWindowAttribute(
            hwnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE,
            &USE_DARK_MODE,
            sizeof(USE_DARK_MODE) );
        return true;
    }
#endif
    return false;
}

HJHND HJWindow::getHandle()
{
    if (!m_window) {
        return NULL;
    }
#if defined(HJ_OS_WINDOWS)
    return (HJHND)glfwGetWin32Window(m_window);
#else
    return (HJHND)NULL;
#endif
}

void HJWindow::onFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    HJFLogi("on framebuffer size change width:{}, height:{}", width, height);
    glViewport(0, 0, width, height);
}

void HJWindow::onWindowRefreshCallback(GLFWwindow* window)
{
    //HJFLogi("on window refresh callback, ");
    bool isOK = changeTheme(window, true);
    if (isOK) {
        glfwSwapBuffers(window);
    }
}

void HJWindow::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    HJFLogi("on key callback, key:{}, scancode:{}, action:{}, mods:{}", key, scancode, action, mods);
    if (action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_ESCAPE && mods == 0)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if ((key == GLFW_KEY_ENTER && mods == GLFW_MOD_ALT) ||
        (key == GLFW_KEY_F11 && mods == GLFW_MOD_ALT))
    {
        static int windowed_xpos, windowed_ypos, windowed_width, windowed_height;
        if (glfwGetWindowMonitor(window))
        {
            glfwSetWindowMonitor(window, NULL,
                windowed_xpos, windowed_ypos,
                windowed_width, windowed_height, 0);
        }
        else
        {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            if (monitor)
            {
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwGetWindowPos(window, &windowed_xpos, &windowed_ypos);
                glfwGetWindowSize(window, &windowed_width, &windowed_height);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
        }
    }
}

void HJWindow::onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    //HJFLogi("on mouse button callback, button:{}, action:{}, mods:{}", button, action, mods);
    HJWindow* wself = HJWindow::getWSelf(window);
    if (GLFW_MOUSE_BUTTON_RIGHT == button && !action)
    {
        //if (!wself->m_rightClickMenu) {
        //    HJRightClickMenu::Ptr menu = std::make_shared<HJRightClickMenu>();
        //    int res = menu->init("");
        //    if (HJ_OK != res) {
        //        HJFLoge("error, create right click menu failed");
        //        return;
        //    }
        //    //
        //    wself->m_rightClickMenu = menu;
        //}
        if (wself->m_rightClickMenu)
        {
            double posx, posy;
            glfwGetCursorPos(window, &posx, &posy);
            wself->m_rightClickMenu->setVPos({(float)posx, (float)posy});
        }
    }
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        //HJFLogw("warning, mouse button is not left, button:{}", button);
        return;
    }
    //wself->m_rightClickMenu = nullptr;

    if (action == GLFW_PRESS)
    {
    }
}

void HJWindow::onCursorPositionCallback(GLFWwindow* window, double x, double y)
{
    //HJFLogi("on cursor position callback, x:{}, y:{}", x, y);

}


NS_HJ_END
