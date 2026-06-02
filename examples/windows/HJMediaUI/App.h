#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui.h>
#include <implot.h>
#include <string>
#include <map>

typedef struct GLFWwindow GLFWwindow;

#if 0
#include "Native.h"
#include "Fonts/Fonts.h"
#include "Helpers.h"

#include "cxxopts.hpp"
#endif

/// Macro to disable console on Windows
#if defined(_WIN32) && defined(APP_NO_CONSOLE)
    #pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

// Barebones Application Framework
struct App 
{
    // Constructor.
    App(std::string title, int w, int h, int argc, char const *argv[]);
    // Destructor.
    virtual ~App();
    // Called at top of run
    virtual void Start() { }
    // Update, called once per frame.
    virtual void Update() { /*implement me*/ }
    // Runs the app.
    void Run();
    // Runs the app with FPS cap (fps<=0 means uncapped)
    void Run(int fps);

    void buildFonts();
    void updateTitle(bool i_bUpdate);
    virtual int RenderEveryStart()
    {
        return 0;
    }
    virtual int RenderEveryEnd()
    {
        return 0;
    }
    void* getWindow() const
    {
        return Window;
    }

    // Get window size
    ImVec2 GetWindowSize() const;

    ImVec4 ClearColor;                    // background clear color
    GLFWwindow* Window;                   // GLFW window handle
    std::map<std::string,ImFont*> Fonts;  // font map
    bool UsingDGPU;                       // using discrete gpu (laptops only)

private:
    void priAlignWindow();

    ImFont* m_DefaultFont = nullptr;
    ImFont* m_HeaderFont = nullptr;
};
