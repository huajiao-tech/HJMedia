#define GLAD_GL_IMPLEMENTATION  //only implemntation once, another use gl function, not use GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "App.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "HJFLog.h"

#define SHOW_DEBUG_INFO 0


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

/// Macro to request high performance GPU in systems (usually laptops) with both
/// dedicated and discrete GPUs
#if defined(_WIN32)
    extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0;
    extern "C" __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 0;
#endif

void StyeColorsApp()
{
    static const ImVec4 bg_dark = ImVec4(0.15f, 0.16f, 0.21f, 1.00f);
    static const ImVec4 bg_mid = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
    static const ImVec4 accent_dark = ImVec4(0.292f, 0.360f, 0.594f, 1.000f);
    static const ImVec4 accent_light = ImVec4(0.409f, 0.510f, 0.835f, 1.000f);
    static const ImVec4 active = ImVec4(0.107f, 0.118f, 0.157f, 1.000f);
    static const ImVec4 attention = ImVec4(0.821f, 1.000f, 0.000f, 1.000f);

    auto &style = ImGui::GetStyle();
    style.WindowPadding = {6, 6};
    style.FramePadding = {6, 3};
    style.CellPadding = {6, 3};
    style.ItemSpacing = {6, 6};
    style.ItemInnerSpacing = {6, 6};
    style.ScrollbarSize = 16;
    style.GrabMinSize = 8;
    style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = style.TabBorderSize = 0;
    style.FrameBorderSize = 1;
    style.WindowRounding = style.ChildRounding = style.PopupRounding = style.ScrollbarRounding = style.GrabRounding = style.TabRounding = 4;

    ImVec4 *colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.89f, 0.89f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f);
    colors[ImGuiCol_WindowBg] = bg_mid;
    colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.21f, 0.27f, 0.00f);
    colors[ImGuiCol_PopupBg] = bg_mid;
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    colors[ImGuiCol_FrameBgHovered] = accent_light;
    colors[ImGuiCol_FrameBgActive] = active;
    colors[ImGuiCol_TitleBg] = accent_dark;
    colors[ImGuiCol_TitleBgActive] = accent_dark;
    colors[ImGuiCol_TitleBgCollapsed] = accent_dark;
    colors[ImGuiCol_MenuBarBg] = accent_dark;
    colors[ImGuiCol_ScrollbarBg] = bg_mid;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.89f, 0.89f, 0.93f, 0.27f);
    colors[ImGuiCol_ScrollbarGrabHovered] = accent_light;
    colors[ImGuiCol_ScrollbarGrabActive] = active;
    colors[ImGuiCol_CheckMark] = accent_dark;
    colors[ImGuiCol_SliderGrab] = accent_dark;
    colors[ImGuiCol_SliderGrabActive] = accent_light;
    colors[ImGuiCol_Button] = accent_dark;
    colors[ImGuiCol_ButtonHovered] = accent_light;
    colors[ImGuiCol_ButtonActive] = active;
    colors[ImGuiCol_Header] = accent_dark;
    colors[ImGuiCol_HeaderHovered] = accent_light;
    colors[ImGuiCol_HeaderActive] = active;
    colors[ImGuiCol_Separator] = accent_dark;
    colors[ImGuiCol_SeparatorHovered] = accent_light;
    colors[ImGuiCol_SeparatorActive] = active;
    colors[ImGuiCol_ResizeGrip] = accent_dark;
    colors[ImGuiCol_ResizeGripHovered] = accent_light;
    colors[ImGuiCol_ResizeGripActive] = active;
    colors[ImGuiCol_Tab] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    colors[ImGuiCol_TabHovered] = accent_light;
    colors[ImGuiCol_TabActive] = accent_dark;
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = active;
    colors[ImGuiCol_PlotLines] = accent_light;
    colors[ImGuiCol_PlotLinesHovered] = active;
    colors[ImGuiCol_PlotHistogram] = accent_light;
    colors[ImGuiCol_PlotHistogramHovered] = active;
    colors[ImGuiCol_TableHeaderBg] = accent_dark;
    colors[ImGuiCol_TableBorderStrong] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    colors[ImGuiCol_TextSelectedBg] = accent_light;
    colors[ImGuiCol_DragDropTarget] = attention;
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
#ifdef IMGUI_HAS_DOCK
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
#endif

    ImPlot::StyleColorsAuto();

    ImVec4 *pcolors = ImPlot::GetStyle().Colors;
    pcolors[ImPlotCol_PlotBg] = ImVec4(0, 0, 0, 0);
    pcolors[ImPlotCol_PlotBorder] = ImVec4(0, 0, 0, 0);
    pcolors[ImPlotCol_Selection] = attention;
    pcolors[ImPlotCol_Crosshairs] = colors[ImGuiCol_Text];

    ImPlot::GetStyle().DigitalBitHeight = 20;

    auto &pstyle = ImPlot::GetStyle();
    pstyle.PlotPadding = pstyle.LegendPadding = {12, 12};
    pstyle.LabelPadding = pstyle.LegendInnerPadding = {6, 6};
    pstyle.LegendSpacing = {10, 2};
    pstyle.AnnotationPadding = {4,2};

    const ImU32 Dracula[]  = {4288967266, 4285315327, 4286315088, 4283782655, 4294546365, 4287429361, 4291197439, 4294830475, 4294113528, 4284106564                        };
    ImPlot::GetStyle().Colormap = ImPlot::AddColormap("Dracula",Dracula,10);
}


static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    HJFLoge("GLFW Error {} {}", error, description);
}
void App::priAlignWindow()
{
    // Position main window: align outer top-left corner with primary monitor work area top-left
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (monitor)
	{
		int workX = 0, workY = 0, workW = 0, workH = 0;
		glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
		if (workW <= 0 || workH <= 0)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if (mode)
			{
				workX = 0;
				workY = 0;
				workW = mode->width;
				workH = mode->height;
			}
		}

		int frameLeft = 0, frameTop = 0, frameRight = 0, frameBottom = 0;
		glfwGetWindowFrameSize(Window, &frameLeft, &frameTop, &frameRight, &frameBottom);

		int posX = workX + frameLeft;
		int posY = workY + frameTop;
		glfwSetWindowPos(Window, posX, posY);
	}
}

App::App(std::string title, int w, int h, int argc, char const *argv[])
{

    //lfs
    bool use_msaa = false;
    bool no_vsync = false;
    bool im_style = false;

#ifdef _DEBUG
    title += " - OpenGL - Debug";
#else
    title += " - OpenGL";
#endif

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        abort();

        // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);          // 3.0+ only
#endif
    //lfs
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);// GLFW_OPENGL_COMPAT_PROFILE);  // 3.2+ only

    if (use_msaa) {
        title += " - 4X MSAA";
        glfwWindowHint(GLFW_SAMPLES, 4);
    }

    // Create window with graphics context
    Window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);
    if (Window == NULL)
    {
        fprintf(stderr, "Failed to initialize GLFW window!\n");
        abort();
    }

    priAlignWindow();

    glfwMakeContextCurrent(Window);
    glfwSwapInterval(no_vsync ? 0 : 1);

    // Initialize OpenGL loader
    bool err = gladLoadGL(glfwGetProcAddress) == 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        abort();
    } 

    const GLubyte* vendor = glGetString(GL_VENDOR); 
    const GLubyte* renderer = glGetString(GL_RENDERER); 

    title +=  " - ";
    title += reinterpret_cast< char const * >(renderer);
    glfwSetWindowTitle(Window, title.c_str());

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    if (use_msaa)
        glEnable(GL_MULTISAMPLE); 

    if (im_style) {
        ImGui::StyleColorsDark();
        ImPlot::StyleColorsDark();
    }
    else {
        ClearColor = ImVec4(0.15f, 0.16f, 0.21f, 1.00f);
        StyeColorsApp();
    }

    ImGuiIO &io = ImGui::GetIO();

    // add fonts
    io.Fonts->Clear();

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;

    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 14.0f;
    icons_config.GlyphOffset = ImVec2(0, 0);
    icons_config.OversampleH = 1;
    icons_config.OversampleV = 1;
    icons_config.FontDataOwnedByAtlas = false;
    //lfs
    ImGui::StyleColorsDark();
 }

App::~App()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(Window);
    glfwTerminate();
}

void App::Run()
{
    Run(0);
}

void App::Run(int fps)
{
    Start();
    const bool cap_fps = fps > 0;
    const double target_sec = cap_fps ? (1.0 / static_cast<double>(fps)) : 0.0;

    // Main loop
    while (!glfwWindowShouldClose(Window))
    {
        auto frame_start = std::chrono::high_resolution_clock::now();

        RenderEveryStart();

        glfwMakeContextCurrent(Window);
        glfwPollEvents();

		ImGuiIO& io = ImGui::GetIO();
        std::string pos = "Mouse: <INVALID>";
        if (ImGui::IsMousePosValid())
        {
            pos = HJFMT("Mouse: ({}, {})", io.MousePos.x, io.MousePos.y);
        }
		glfwSetWindowTitle(Window, pos.c_str());

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        Update();
        glfwMakeContextCurrent(Window);
        {
            GLenum err = glGetError();
            if (err != GL_NO_ERROR)
            {
                fprintf(stderr, "glGetError %d\n", err);
                abort();
            }
        }
#if 0
        {
            static float ball_offset = 0.0f;
            static float ball_speed = 3.0f;
            ball_offset += ball_speed;
            if (ball_offset > 300.0f || ball_offset < 0.0f) ball_speed *= -1.0f;

            ImDrawList* fg_list = ImGui::GetForegroundDrawList();
            fg_list->AddRectFilled(ImVec2(20, 20), ImVec2(350, 80), IM_COL32(50, 50, 50, 200)); 
            fg_list->AddText(ImVec2(30, 30), IM_COL32(255, 255, 0, 255), "Render Check: If this moves, Context is OK");
            fg_list->AddCircleFilled(ImVec2(50.0f + ball_offset, 60.0f), 10.0f, IM_COL32(0, 255, 0, 255));
        }
#endif

#if SHOW_DEBUG_INFO
        ImGui::ShowMetricsWindow();
#endif
        // Rendering
        ImGui::Render();
        {
            GLenum err = glGetError();
            if (err != GL_NO_ERROR)
            {
                fprintf(stderr, "glGetError %d\n", err);
                abort();
            }
        }
        int display_w, display_h;
        glfwGetFramebufferSize(Window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        {
            GLenum err = glGetError();
            if (err != GL_NO_ERROR)
            {
                fprintf(stderr, "glGetError %d\n", err);
                abort();
            }
        }


        glfwSwapBuffers(Window);

        RenderEveryEnd();

        if (cap_fps)
        {
            auto frame_end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = frame_end - frame_start;
            double remaining = target_sec - elapsed.count();
            if (remaining > 0.0)
            {
                std::this_thread::sleep_for(std::chrono::duration<double>(remaining));
            }
        }
    }
}

ImVec2 App::GetWindowSize() const
{
    int w, h;
    glfwGetWindowSize(Window, &w, &h);
    return ImVec2(w, h);
}
