//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN         //��ֹ windows.h ���� winsock.h, ������WinSock2.h��ͻ
#endif
#include "HJUtils.h"
#include "HJMedias.h"
#include "HJCores.h"
#include "HJGuis.h"
#include "HJImguiUtils.h"
#include "HJPlayerView.h"
#include "HJFLog.h"
#include "HJNetManager.h"
#include "HJGraphPlayerView.h"

#if defined(HJ_OS_WIN32)
#include <crtdbg.h>
#elif defined(HJ_OS_MACOS)
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#endif

NS_HJ_USING
//***********************************************************************************//
class HJApplicationImpl : public HJApplication
{
public:
    using Ptr = std::shared_ptr<HJApplicationImpl>;

    virtual int onInit() override;
    virtual int onRunBegin() override;
    virtual int onRunning() override;
    virtual int onRunEnd() override;
    virtual void onDone() override;
    // media tools
    void onFileDialog();
    void onPlayer();
    void onGraphPlayer();
    void onStreamAnalyzer();
    void onTransMuxer();
    // net tools
    void onMQMonitorServer();
    void onMQMonitorClient();
    //
private:
    HJAppMainMenu::Ptr         m_menu = nullptr;
    HJOverlayView::Ptr         m_overlayView = nullptr;
    //HJPopupView::Ptr         m_popupView = nullptr;
    HJPlayerView::Ptr          m_playerView = nullptr;
    HJGraphPlayerView::Ptr     m_jplayerView = nullptr;
    //HJStreamAnalyzerView::Ptr  m_streamAnalyzerView = nullptr;    
    //HJQMonitorSeverView::Ptr   m_monitorSeverView = nullptr;
    //HJQMonitorClientView::Ptr  m_monitorClientView = nullptr;
    HJFileDialogView::Ptr      m_fileDialog = nullptr;
    //
    HJExecutor::Ptr            m_executor{ nullptr };
    HJList<HJTask::Ptr>       m_timers;
    uint64_t                    m_timerCount{0};
    std::mutex                  m_timerMutex;
	HJNetworkSimulator::Ptr   m_networkSimulator{ nullptr };
};

int HJApplicationImpl::onInit()
{
    int res = HJApplication::onInit();

    //onProcessDevices();

    m_menu = std::make_shared<HJAppMainMenu>();
    std::string menuInfo = R"(
    {
        "menu":[
            {
                "title":"MediaTools",
                "menu":[
                    "Player",
                    "GraphPlayer",
                    "Stream Analyzer",
                    "Trans Muxer",
                    "Trans Codec",
                    "---",
                    "Test",
                    "TestDone",
                    "TestTime",
                    "TestJson"
                ]
            },
            {
                "title":"NetTools",
                "menu":[
                    "MQMonitor"
                ]
            }
        ],
        "toolbar":[
            {
                "icon":"icon_setting.png",
                "tips":"setting"
            },
            {
                "icon":"icon_play.png",
                "tips":"play"
            }
        ]
    }
    )";
    res = m_menu->init(HJAnsiToUtf8(menuInfo));
    if (HJ_OK != res) {
        HJLoge("error, menu init failed");
        return res;
    }
    m_menu->onMenuClick = [&](const std::string& title, const std::string& name) {
        //HJLogi("click menu title:" + title + ", name:" + name);
        if ("MediaTools" == title)
        {
            if ("Player" == name) {
                onPlayer();
            }
            else if ("GraphPlayer" == name) {
                onGraphPlayer();
            }
            else if ("Stream Analyzer" == name) {
                onStreamAnalyzer();
            } else if ("Trans Muxer" == name) {

            } else if ("Test" == name) {
                //onTest();
            } else if ("TestDone" == name) {
                //onTestDone();
            } else if ("TestTime" == name) {
                //onTestTime();
            } else if ("TestJson" == name) {
                //onTestJson();
            }
        }
        else if ("NetTools" == title)
        {
            if ("MQMonitor" == name) {
                onMQMonitorClient();
                onMQMonitorServer();
            }
        }
    };
    //
    m_overlayView = std::make_shared<HJOverlayView>();
    res = m_overlayView->init("");
    if (HJ_OK != res) {
        HJLoge("error, overlay view init failed");
        return res;
    }

    //m_popupView = std::make_shared<HJPopupView>();
    //m_popupView->init("");
    //
    HJFileDialogView::setGLReady();

    return res;
}

int HJApplicationImpl::onRunBegin()
{
    int res = HJApplication::onRunBegin();

    return res;
}
//
//void Demo_FilledLinePlots() {
//    static double xs1[101], ys1[101], ys2[101], ys3[101];
//    srand(0);
//    for (int i = 0; i < 101; ++i) {
//        xs1[i] = (float)i;
//        ys1[i] = HJUtils::randomf(400.0, 450.0);
//        ys2[i] = HJUtils::randomf(275.0, 350.0);
//        ys3[i] = HJUtils::randomf(150.0, 225.0);
//    }
//    static bool show_lines = true;
//    static bool show_fills = true;
//    static float fill_ref = 0;
//    static int shade_mode = 0;
//    static ImPlotShadedFlags flags = 0;
//    ImGui::Checkbox("Lines", &show_lines); ImGui::SameLine();
//    ImGui::Checkbox("Fills", &show_fills);
//    if (show_fills) {
//        ImGui::SameLine();
//        if (ImGui::RadioButton("To -INF", shade_mode == 0))
//            shade_mode = 0;
//        ImGui::SameLine();
//        if (ImGui::RadioButton("To +INF", shade_mode == 1))
//            shade_mode = 1;
//        ImGui::SameLine();
//        if (ImGui::RadioButton("To Ref", shade_mode == 2))
//            shade_mode = 2;
//        if (shade_mode == 2) {
//            ImGui::SameLine();
//            ImGui::SetNextItemWidth(100);
//            ImGui::DragFloat("##Ref", &fill_ref, 1, -100, 500);
//        }
//    }
//
//    if (ImPlot::BeginPlot("Stock Prices")) {
//        ImPlot::SetupAxes("Days", "Price");
//        ImPlot::SetupAxesLimits(0, 100, 0, 500);
//        if (show_fills) {
//            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
//            ImPlot::PlotShaded("Stock 1", xs1, ys1, 101, shade_mode == 0 ? -INFINITY : shade_mode == 1 ? INFINITY : fill_ref, flags);
//            ImPlot::PlotShaded("Stock 2", xs1, ys2, 101, shade_mode == 0 ? -INFINITY : shade_mode == 1 ? INFINITY : fill_ref, flags);
//            ImPlot::PlotShaded("Stock 3", xs1, ys3, 101, shade_mode == 0 ? -INFINITY : shade_mode == 1 ? INFINITY : fill_ref, flags);
//            ImPlot::PopStyleVar();
//        }
//        if (show_lines) {
//            ImPlot::PlotLine("Stock 1", xs1, ys1, 101);
//            ImPlot::PlotLine("Stock 2", xs1, ys2, 101);
//            ImPlot::PlotLine("Stock 3", xs1, ys3, 101);
//        }
//        ImPlot::EndPlot();
//    }
//}

int HJApplicationImpl::onRunning()
{
    int res = HJApplication::onRunning();

    if (m_menu) {
        m_menu->draw();
    }
    //if (m_streamAnalyzerView && m_menu) {
    //    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    //    auto mvsize = m_menu->getVSize();
    //    HJRectf viewRect = { 0, mvsize.h, viewport->Size.x, viewport->Size.y };
    //    m_streamAnalyzerView->draw(viewRect);
    //    if (m_streamAnalyzerView->isDone()) {
    //        m_streamAnalyzerView = nullptr;
    //    }
    //}
    if (m_playerView && m_menu) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        auto mvsize = m_menu->getVSize();
        HJRectf viewRect = { 0, mvsize.h, viewport->Size.x, viewport->Size.y };
        m_playerView->draw(viewRect);
        if (m_playerView->isDone()) {
            m_playerView = nullptr;
        }
    }
   if (m_jplayerView && m_menu) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        auto mvsize = m_menu->getVSize();
        HJRectf viewRect = { 0, mvsize.h, viewport->Size.x, viewport->Size.y };
        m_jplayerView->draw(viewRect);
        if (m_jplayerView->isDone()) {
            m_jplayerView = nullptr;
        }
    }
   /*
    if (m_monitorSeverView && m_menu) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        auto mvsize = m_menu->getVSize();
        HJRectf viewRect = { 0, mvsize.h, viewport->Size.x / 2, viewport->Size.y };
        m_monitorSeverView->draw(viewRect);
        if (m_monitorSeverView->isDone()) {
            m_monitorSeverView = nullptr;
        }
    }
    if (m_monitorClientView && m_menu) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        auto mvsize = m_menu->getVSize();
        HJRectf viewRect = { viewport->Size.x / 2, mvsize.h, viewport->Size.x / 2, viewport->Size.y };
        m_monitorClientView->draw(viewRect);
        if (m_monitorClientView->isDone()) {
            m_monitorClientView = nullptr;
        }
    }*/
    if (m_overlayView) {
        m_overlayView->draw();
    }



    //bool m_isDone = false;
    //if (ImGui::Begin("Analyzer", &m_isDone, 1024))
    //{
    //    //Demo_FilledLinePlots();
    //    static int shade_mode = 0;
    //    if (ImGui::RadioButton("Video", shade_mode == 0)) {
    //        shade_mode = 0;
    //    }
    //    ImGui::SameLine();
    //    if (ImGui::RadioButton("Audio", shade_mode == 1)) {
    //        shade_mode = 1;
    //    }

    //    ImGui::End();
    //}

    //bool m_isDone = false;
    //if (ImGui::Begin("Analyzer", &m_isDone, 1024))
    //{
    //    static ImVector<int> selection;
    //    static float row_min_height = 0.0f; // Auto
    //    static ImGuiTableFlags flags =
    //        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
    //        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
    //        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
    //        | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
    //        | ImGuiTableFlags_SizingFixedFit;
    //    static ImGuiTableColumnFlags columns_base_flags = ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;
    //    ImVec2 outer_size_value = ImVec2(HJ_MIN(1280.f, ImGui::GetWindowSize().x), HJ_MIN(ImGui::GetWindowSize().y, 720.f));
    //    float init_width_or_weight = 0.0;
    //    ImGui::NewLine();
    //    if (ImGui::BeginTable("MFrameTables", 8, flags, { 1280, 720 }, 0.0f))
    //    {
    //        // Declare columns
    //        ImGui::TableSetupColumn("  ID  ", columns_base_flags, init_width_or_weight, 0);
    //        ImGui::TableSetupColumn("Media Type", columns_base_flags, init_width_or_weight, 1);

    //        ImGui::TableSetupScrollFreeze(1, 1);
    //        //
    //        ImGui::TableHeadersRow();

    //        ImGui::PushButtonRepeat(true);

    //        ImGuiListClipper clipper;
    //        clipper.Begin(3);
    //        while (clipper.Step())
    //        {
    //            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
    //            {
    //                const bool item_is_selected = selection.contains(row_n);
    //                ImGui::PushID(row_n);
    //                {
    //                    ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);

    //                    if (ImGui::TableSetColumnIndex(0))
    //                    {
    //                        ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
    //                        if (ImGui::Selectable(HJ2STR(row_n).c_str(), item_is_selected, selectable_flags, ImVec2(0, row_min_height)))
    //                        {
    //                            if (ImGui::GetIO().KeyCtrl)
    //                            {
    //                                if (item_is_selected)
    //                                    selection.find_erase_unsorted(row_n);
    //                                else
    //                                    selection.push_back(row_n);
    //                            }
    //                            else
    //                            {
    //                                selection.clear();
    //                                selection.push_back(row_n);
    //                            }
    //                        }
    //                    }
    //
    //                    if (ImGui::TableSetColumnIndex(1)) {
    //                        ImGui::TextUnformatted("xxx");
    //                    }

    //                }
    //                ImGui::PopID();
    //            }
    //        }
    //        ImGui::PopButtonRepeat();

    //        ImGui::EndTable();
    //    }
    //    ImGui::End();
    //}


    //top overlay
    if (m_fileDialog) {
        res = m_fileDialog->draw([&](const std::vector<std::filesystem::path>& files) {
            HJLogi("file dialog");
            });
        if (m_fileDialog->isDone()) {
            m_fileDialog = nullptr;
        }
    }

    //if (m_popupView) {
    //    res = m_popupView->draw();
    //    if (m_popupView->isDone()) {
    //        m_popupView = nullptr;
    //    }
    //}

    //{
    //    static float f = 0.0f;
    //    static int counter = 0;

    //    ImGui::Begin("operator");                          // Create a window called "Hello, world!" and append into it.
    //    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    //    ImGui::SameLine();
    //    ImGui::Text("counter = %d", counter);

    //    if (ImGui::Button("Open file")) {
    //        m_fileDialog = std::make_shared<HJFileDialogView>();
    //        m_fileDialog->init("Open a media", HJFileDialogView::KEY_MEDIA_FILTER, true, "E:/movies");
    //    }
    //    //ifd::FileDialog::Instance().Open("ShaderOpenDialog", "Open a shader", "Image file (*.png;*.jpg;*.jpeg;*.bmp;*.tga){.png,.jpg,.jpeg,.bmp,.tga},.*", true);
    //    if (ImGui::Button("Open directory"))
    //        ifd::FileDialog::Instance().Open("DirectoryOpenDialog", "Open a directory", "");
    //    if (ImGui::Button("Save file"))
    //        ifd::FileDialog::Instance().Save("ShaderSaveDialog", "Save a shader", "*.sprj {.sprj}");



    //    //if (ifd::FileDialog::Instance().IsDone("ShaderOpenDialog")) {
    //    //    if (ifd::FileDialog::Instance().HasResult()) {
    //    //        const std::vector<std::filesystem::path>& res = ifd::FileDialog::Instance().GetResults();
    //    //        for (const auto& r : res) // ShaderOpenDialog supports multiselection
    //    //            printf("OPEN[%s]\n", r.u8string().c_str());
    //    //    }
    //    //    ifd::FileDialog::Instance().Close();
    //    //}
    //    if (ifd::FileDialog::Instance().IsDone("DirectoryOpenDialog")) {
    //        if (ifd::FileDialog::Instance().HasResult()) {
    //            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
    //            printf("DIRECTORY[%s]\n", res.c_str());
    //        }
    //        ifd::FileDialog::Instance().Close();
    //    }
    //    if (ifd::FileDialog::Instance().IsDone("ShaderSaveDialog")) {
    //        if (ifd::FileDialog::Instance().HasResult()) {
    //            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
    //            printf("SAVE[%s]\n", res.c_str());
    //        }
    //        ifd::FileDialog::Instance().Close();
    //    }

        //if (ImGui::Button("ChooseFileDlgKey")) {
        //    IGFD::FileDialogConfig config;
        //    config.path = ".";
        //    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose a Directory", nullptr, config);
        //    //IGFD::FileDialogConfig config;
        //    //config.path = ".";
        //    //config.countSelectionMax = 1;
        //    //config.flags = ImGuiFileDialogFlags_Modal;
        //    //ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".cpp,.h,.hpp", config);

        //}
        //// display
        //if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
        //{
        //    // action if OK
        //    if (ImGuiFileDialog::Instance()->IsOk())
        //    {
        //        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
        //        std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
        //        // action
        //    }

        //    // close
        //    ImGuiFileDialog::Instance()->Close();
        //}

    //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS) slide:%.3f", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, f);
    //    ImGui::End();
    //}

    return res;
}

int HJApplicationImpl::onRunEnd()
{
    int res = HJApplication::onRunEnd();

    return res;
}

void HJApplicationImpl::onDone()
{
    HJApplication::onDone();

    HJ_AUTOU_LOCK(m_timerMutex);
    if (m_executor) {
        m_executor = nullptr;
    }
}

void HJApplicationImpl::onFileDialog()
{
    if (!m_fileDialog) {
        m_fileDialog = std::make_shared<HJFileDialogView>();
        m_fileDialog->init("Open a media", HJFileDialogView::KEY_MEDIA_FILTER, true, "E:/movies");
    }
}

void HJApplicationImpl::onStreamAnalyzer()
{
    //if (m_streamAnalyzerView) {
    //    return;
    //}
    //m_streamAnalyzerView = std::make_shared<HJStreamAnalyzerView>();
    //int res = m_streamAnalyzerView->init("");
    //if (HJ_OK != res) {

    //}
    return;
}

void HJApplicationImpl::onPlayer()
{
    if (m_playerView) {
        return;
    }
    m_playerView = std::make_shared<HJPlayerView>();
    int res = m_playerView->init("");
    if (HJ_OK != res) {

    }
    return;
}

void HJApplicationImpl::onGraphPlayer()
{
    if (m_jplayerView) {
        return;
    }
    m_jplayerView = std::make_shared<HJGraphPlayerView>();
    int res = m_jplayerView->init("");
    if (HJ_OK != res) {

    }
    return;
}

void HJApplicationImpl::onTransMuxer()
{

    return;
}

void HJApplicationImpl::onMQMonitorServer()
{
    //if (m_monitorSeverView) {
    //    return;
    //}
    //m_monitorSeverView = std::make_shared<HJQMonitorSeverView>();
    //int res = m_monitorSeverView->init("");
    //if (HJ_OK != res) {

    //}
    return;
}

void HJApplicationImpl::onMQMonitorClient()
{
    //if (m_monitorClientView) {
    //    return;
    //}
    //m_monitorClientView = std::make_shared<HJQMonitorClientView>();
    //int res = m_monitorClientView->init("");
    //if (HJ_OK != res) {

    //}
    return;
}

#if defined(HJ_OS_MACOS)
//-----------------------------------------------------------------------------------
// AppDelegate
//-----------------------------------------------------------------------------------
@interface AppDelegate : NSObject <NSApplicationDelegate>
//@property (nonatomic, readonly) NSWindow* window;
@end

@implementation AppDelegate
//@synthesize window = _window;

-(instancetype)init
{
    if (self = [super init])
    {
    }
    return self;
}

-(void)dealloc
{
//    _window = nil;
}

-(void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Make the application a foreground application (else it won't receive keyboard events)
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);

    return;
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}
@end
#endif
//***********************************************************************************//
int main(int argc, const char* argv[])
{
#if defined(HJ_OS_WIN32)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#elif defined(HJ_OS_MACOS)
//    @autoreleasepool
//    {
//        NSApp = [NSApplication sharedApplication];
//        AppDelegate* delegate = [[AppDelegate alloc] init];
//        [[NSApplication sharedApplication] setDelegate:delegate];
//    }
#endif
    HJContextConfig cfg;
    cfg.mIsLog = true;
    cfg.mLogMode = HJLOGMODE_FILE | HJLOGMODE_CONSOLE;
    cfg.mLogDir = HJUtilitys::logDir() + R"(xmediatools/)";
    cfg.mResDir = HJConcateDirectory(HJCurrentDirectory(), R"(resources)");
#if defined(HJ_OS_WIN32)
    cfg.mMediaDir = "E:/movies";
    //cfg.mResDir = HJUtils::exeDir() + "resources";
    //cfg.mMediaDir = "C:";
#elif defined(HJ_OS_MACOS)
    //cfg.mResDir = "/Users/zhiyongleng/works/MediaTools/SLMedia_OW/examples/Windows/XMediaTools/resources";
    cfg.mMediaDir = "/Users/zhiyongleng/works/movies";
#endif
    cfg.mThreadNum = 0;
    //HJFileUtil::delete_file((cfg.mLogDir + "HJLog.txt").c_str());
    HJContext::Instance().init(cfg);

    HJNetManager::registerAllNetworks();

    //HJFileUtil::delete_file((cfg.mLogDir + "jpublisher/jsdk_JLog.txt").c_str());
    //publish_context_init((cfg.mLogDir + "jpublisher/").c_str(), 2, cfg.mLogMode, 1024 * 1024 * 5, 5); //JLOG_INFO 2
    ////jplayer
    //int i_err = ContextInit((cfg.mLogDir + "jplayer/").c_str(), 2, cfg.mLogMode, 1024 * 1024 * 10, 5);
    //
    //
    HJApplicationImpl::Ptr g_gui = std::make_shared<HJApplicationImpl>();
    int res = g_gui->init();
    if (HJ_OK != res) {
        return res;
    }
    g_gui->run();

    //
    g_gui = nullptr;

    //publish_context_done();
    HJContext::Instance().done();
#if defined(HJ_OS_MACOS)
    return NSApplicationMain(argc, argv);
#else
    return HJ_OK;
#endif
}

//***********************************************************************************//

