//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJApplication.h"
#include "HJFLog.h"
#include "HJImguiUtils.h"
//#include "IconFontCppHeaders/IconsFontAwesome5.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJApplication::HJApplication()
{
}

HJApplication::~HJApplication()
{
    done();
}

int HJApplication::init()
{
	int res = HJ_OK;
	HJLogi("init entry");
	do
	{
		glfwSetErrorCallback(HJApplication::glfw_error_callback);
		if (!glfwInit()) {
			HJLoge("glfw init error");
			return HJErrFatal;
		}
        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        //ImGui::StyleColorsDark();
        ImGui::StyleColorsClassic();

        //main view
        m_mainWindow = std::make_shared<HJWindow>();
        res = m_mainWindow->init();
        if (HJ_OK != res) {
            HJLoge("error, main view create failed");
            break;
        }
#if defined(HJ_OS_WINDOWS)
        m_mainWindow->setWindowSize(HJ_SIZE_2K);
#elif defined(HJ_OS_MACOS)
        m_mainWindow->setWindowSize(HJ_SIZE_1080P);
#endif
#ifdef __EMSCRIPTEN__
        ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
        ImGui_ImplOpenGL3_Init(glsl_version);
        //
        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        
        HJSizei monitorSize = HJWindow::gettPrimaryMonitorSize();
        float fontSize = 15.0f * monitorSize.w / 1920.f;
        //std::string fontName = HJContext::Instance().getResDir() + "/fonts/fa-solid-900.ttf";
        std::string fontName = HJContext::Instance().getResDir() + "/fonts/ProggyTiny.ttf";
#if defined(HJ_OS_WINDOWS)
        fontName = "c:/Windows/Fonts/msyh.ttc";
#elif defined(HJ_OS_MACOS)
        fontName = "/System/Library/Fonts/Times.ttc"; //Songti.ttc PingFang.ttc"
#endif
        //static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
        //ImFontConfig icons_config;
        //icons_config.MergeMode = true;
        //icons_config.PixelSnapH = true;
        //icons_config.GlyphMinAdvanceX = fontSize;
        //
        //ImFont* font = io.Fonts->AddFontFromFileTTF(fontName.c_str()/*FONT_ICON_FILE_NAME_FAS*/, fontSize, &icons_config, icons_ranges);
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontName.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        if(!font) {
            HJLoge("error, get font failed, fontName:" + fontName);
            res = HJErrFatal;
            break;
        }
        //ImFont* font = io.Fonts->AddFontFromFileTTF(fontName.c_str(), 30.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != nullptr);
        //
        res = onInit();
        if (HJ_OK != res) {
            HJLoge("onInit error");
        }
        m_runState = HJRun_Init;
	} while (false);

	return HJ_OK;
}

void HJApplication::done()
{
    if (m_runState >= HJRun_Done) {
        return;
    }
    HJLogi("done entry");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    if (m_mainWindow) {
        m_mainWindow->done();
        m_mainWindow = nullptr;
    }
    glfwTerminate();
    //
    m_runState = HJRun_Done;
}

int HJApplication::run()
{
    if (!m_mainWindow) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    HJLogi("run entry");
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!m_mainWindow->isClose())
#endif
    {
        m_runState = HJRun_Running;
        //uint64_t t0 = HJUtils::getCurrentMillisecond();
        onRunBegin();
        executeTasks();
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //
        onRunning();
        //m_mainWindow->draw();
        
        // Rendering
        ImGui::Render();
        m_mainWindow->renderClear();
        //
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        //
        m_mainWindow->renderPresent();
        
        //glfwWaitEvents();
        onRunEnd();
        //HJLogi("render time:" + HJ2STR(HJUtils::getCurrentMillisecond() - t0));
    };
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    HJLogi("run end");

	return res;
}

void HJApplication::glfw_error_callback(int error, const char* description)
{
	HJFLogi("glfw error:{}, description:{}", error, description);
}


NS_HJ_END
