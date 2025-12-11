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
#include "HJLocalServer.h"
#include "HJGlobalSettings.h"
#include "HJMov2Live.h"
#include "HJUUIDTools.h"
#include "HJPackerManager.h"
#include "HJDataFuse.h"

#if defined(HJ_OS_WIN32)
#include <crtdbg.h>
#elif defined(HJ_OS_MACOS)
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#endif

NS_HJ_USING
//***********************************************************************************//
class HJApplicationImpl : public HJApplication, public HJServerDelegate
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
    void onTestClient();
    void onTestSever();
    void onTestLocalServer();
    void onTestCloseServer();
    void onMov2Live();
    void onTestUUID();
    void onTestPacker();
    void onTestDataFuse();

private:
    virtual int onLocalServerNotify(HJNotification::Ptr ntfy);
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

	HJDownloader::Ptr     m_downloader{ nullptr };
	HJHTTPServer::Ptr     m_httpServer{ nullptr };
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
                    "TestClient",
                    "StartServer",
                    "CloseServer",
                    "Mov2Live",
                    "TestJson", 
                    "TestUUID",
                    "TestPacker"
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

            } else if ("TestClient" == name) {
                onTestClient();
            } else if ("StartServer" == name) {
                onTestLocalServer();
            } else if ("CloseServer" == name) {
                onTestCloseServer();
            }
            else if ("Mov2Live" == name) {
                onMov2Live();
            } else if ("TestJson" == name) {
                //onTestJson();
            } else if ("TestUUID" == name) {
                onTestUUID();
            } else if ("TestPacker" == name) {
                //onTestPacker();
                onTestDataFuse();
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

void HJApplicationImpl::onTestClient()
{
    if (m_downloader) {
		return;
    }
    std::string url = "http://static.s3.huajiao.com/Object.access/hj-video/Y2p6cG1mamwubXA0";
    auto localUrl = HJMediaUtils::makeLocalUrl("E:/movies/blob/", url);
    m_downloader = std::make_shared<HJDownloader>(url, localUrl);
    m_downloader->setOnProgress([this](int64_t current, int64_t total) {
        HJLogi("downloading current:{}, total:{}", current, total);
    });
    m_downloader->start();

    return;
}

void HJApplicationImpl::onTestSever()
{
    if (m_httpServer){
        return;
    }
 //   HJServerParams params
	//m_httpServer = std::make_shared<HJHTTPServer>(params);
 //   m_httpServer->start();

	return;
}

void HJApplicationImpl::onTestLocalServer()
{
    HJParams::Ptr params = HJCreates<HJParams>();
    (*params)["cache_dir"] = HJConcateDirectory(HJUtilitys::exeDir(), "cache_dir");
    (*params)["cache_size"] = 600;
    (*params)["media_dir"] = "E:/movies/blob/server";

    HJLocalServer::getInstance()->init(params, [&](const HJNotification::Ptr ntfy) -> int {
        HJFLogi("local server notify:{},{}", ntfy->getID(), ntfy->getMsg());
        return HJ_OK;
    });

    return;
}

void HJApplicationImpl::onTestCloseServer()
{
    HJLocalServer::getInstance()->done();
}

void HJApplicationImpl::onMov2Live()
{
    auto mov2live = std::make_shared<HJMov2Live>();
    mov2live->init("E:/movies/blob/server/zzqc.mp4");
    mov2live->toLive("E:/movies/blob/server/zzqc_live.mp4");

    return;
}

void HJApplicationImpl::onTestUUID()
{ 
    auto uuid0_opt = HJUUIDTools::generate_uuid();
    HJUUID uuid0 = uuid0_opt.value();
    HJFLogi("uuid0:0x{:x}", uuid0.data[0]);
    HJFLogi("uuid0:{}", uuid0.toString());
    const int64_t number = 123456789;

    auto uuid0_opt2 = HJUUIDTools::generate_uuid();
    HJUUID uuid2 = uuid0_opt2.value();

    // 2. 测试成功的派生和验证（正常路径）
    HJUUID uuid1 = HJUUIDTools::derive_uuid(uuid0, number);
    HJFLogi("uuid1:{}", uuid1.toString());

    HJUUID uuid3 = HJUUIDTools::derive_uuid(uuid0);
    HJFLogi("uuid3:{}", uuid3.toString());

    HJUUID uuid4 = HJUUIDTools::derive_uuid(uuid0);
    if (uuid3.data == uuid4.data) {
        HJFLogi("uuid3 == uuid4");
    }

    auto isOK = HJUUIDTools::verify_uuid(uuid1, uuid0, number);
    HJFLogi("verify_uuid:{}, number:{}", isOK, number);

    isOK = HJUUIDTools::verify_uuid(uuid1, uuid0, 123);
    HJFLogi("verify_uuid:{}, number:{}", isOK, 123);

    isOK = HJUUIDTools::verify_uuid(uuid1, uuid0, 0);
    HJFLogi("verify_uuid:{}, number:{}", isOK, 0);

    isOK = HJUUIDTools::verify_uuid(uuid2, uuid1);
    HJFLogi ("verify_uuid2:{}, number:{}", isOK, 0);

    isOK = HJUUIDTools::verify_uuid(uuid3, uuid2);
    HJFLogi("verify_uuid3:{}, number:{}", isOK, 0);

    return;
}

void HJApplicationImpl::onTestPacker()
{
    HJPackerManager::Ptr packerManager = HJCreates<HJPackerManager>();
    packerManager->registerPackers<HJLZ4Packer, HJSegPacker>();
    //HJParams::Ptr params;
    //packerManager->registerPackers<
    //    HJLZ4Packer,
    //    std::make_pair<HJSegPacker, std::tuple<size_t>>(1024)>();
    //
    std::vector<uint8_t> data;
    HJVibeData::Ptr vibeData = HJCreates<HJVibeData>(data);
    auto outData = packerManager->pack(vibeData);


    return;
}

void HJApplicationImpl::onTestDataFuse()
{
    HJDataFuse::Ptr dataFuse = HJCreates<HJDataFuse>();

    std::string msg = "TOOL CFG: width:720 height:1280 fps:25:1 timebase:1:25 vfr:0 vui:1:1:0:5:1:1:1:5:format:1 preset:7 tune:meetingCamera level:50 repeat-header:1 ref:3 long-term:0 open-gop:0 keyint:50:5 rc:1 cqp:32 cbr/abr:1548 crf:0:0:0 vbv-max-bitrate:2012:1 vbv-buf-size:2012:4 vbv-buf-init:0.9 max-frame-ratio:100 ip-factor:2 rc-peakratetol:5 qp:40:22:35:22 aq:5:1 wpp:1 pool-threads:2:64 svc:0 fine-tune:-1 roi:1:1 TOOL CFG: qpInitMin:22 qpInitMax:35 MaxRatePeriod:1 MaxBufPeriod:4 HAD:1 FDM:1 RQT:1 TransformSkip:0 SAO:1 TMVPMode:1 SignBitHidingFlag:1 MvStart:4 BiRefNum:0 FastSkipSub:1 EstimateCostSkipSub0:0 EstimateCostSkipSub1:0 EstimateCostSkipSub2:0 SkipCurFromSplit0:1 SkipCurFromSplit1:2 SkipCurFromSplit2:3 SubDiffThresh0:18 SubDiffThresh1:10 SubDiffThresh2:6 FastMergeSkip:1 SkipUV:1 RefineSkipUV:0 MergeSkipTh0:30000 MergeSkipTh1:30000 MergeSkipTh2:40 MergeSkipTh3:40 MergeSkipTh:0 MergeSkipDepth:0 MergeSkipEstopTh0:350 MergeSkipEstopTh1:250 MergeSkipEstopTh2:150 MergeSkipEstopTh3:100 CbfSkipIntra:1 neiIntraSkipIntra:6 SkipFatherIntra:0 SkipIntraFromRDCost:3 StopIntraFromDist0:11 StopIntraFromDist1:12 StopIntraFromDist2:14 StopIntraFromDist3:16 TopdownContentTh0:32 TopdownContentTh1:32 TopdownContentTh2:32 TopdownContentTh3:32 TopdownContentTh4:32 BottomUpContentTh0:0 BottomUpContentTh1:14 BottomUpContentTh2:12 BottomUpContentTh3:0 BottomUpContentTh4:0 BottomUpContentTh5:12 madth4merge:128 DepthSkipCur:0 CheckCurFromSubSkip:0 DepthSkipSub:3 StopCurFromNborCost:5 SkipFatherBySubmode:0 SkipL1ByNei:0 CheckCurFromNei:0 EarlySkipAfterSkipMerge:2 sccIntra8distTh:5000 sccIntraNxNdistTh:120 TuEarlyStopPu:18 FastSkipInterRdo:1 IntraCheckInInterFastSkipNxN0:17 IntraCheckInInterFastSkipNxN1:40 IntraCheckInIntraFastSkipNxN:0 IntraCheckInIntraSkipNxN:0 AmpSkipAmp:1 Skip2NAll:0 Skip2NFromSub0:600 Skip2NFromSub1:600 Skip2NFromSub2:650 Skip2NRatio0:450 Skip2NRatio1:450 Skip2NRatio2:450 SkipSubFromSkip2n:1 AdaptMergeNum:5 TuStopPu:18 qp2qstepbetter:610 qp2qstepfast:600 InterCheckMerge:1 skipIntraFromPreAnalysis0:18 skipIntraFromPreAnalysis1:18 skipIntraFromPreAnalysis2:18 DisNxNLevel:3 SubdiffSkipSub0:260 SubdiffSkipSub1:200 SubdiffSkipSub2:120 SubdiffSkipSub:0 SubdiffSkipSubRatio0:10 SubdiffSkipSubRatio1:10 SubdiffSkipSubRatio2:60 SubdiffSkipSubRatio:0 StopSubMaxCosth0:2048 StopSubMaxCosth1:0 StopSubMaxCosth2:0 StopSubMaxCosth3:0 CostSkipSub0:12 CostSkipSub1:6 CostSkipSub2:3 CostSkipSub:1 DistSkipSub0:12 DistSkipSub1:8 DistSkipSub2:4 DistSkipSub3:1 SkipSubNoCbf:1 SaoLambdaRatio0:7 SaoLambdaRatio1:8 SaoLambdaRatio2:8 SaoLambdaRatio3:9 AdaptiveSaoLambda[0]:60 AdaptiveSaoLambda[1]:90 AdaptiveSaoLambda[2]:120 AdaptiveSaoLambda[3]:150 SecModeInInter0:6 SecModeInInter1:0 SecModeInInter2:0 SecModeInInter3:4 SecModeInInter4:1 SecModeInIntra0:6 SecModeInIntra1:0 SecModeInIntra2:0 SecModeInIntra3:3 SecModeInIntra4:1 ChromaModeOptInInter0:0 ChromaModeOptInInter1:0 ChromaModeOptInInter2:0 IntraCheckInInterFastSkipUseNborCu:1 disframesao:1 skipTuSplit:16 FastQuantInter0:95 FastQuantInter1:95 FastQuantInterChroma:20 MeInterpMethod:0 MultiRef:1 BiMultiRef:0 BiMultiRefThr:0 MultiRefTh:0 MultiRefFast2nx2nTh:0 MvStart:4 StopSubFromNborCost:0 StopSubFromNborCost2:80 StopSubFromNborCount:0 imecorrect:1 fmecorrect:1 qmeguidefast:1 unifmeskip:3 fasthme:1 FasterFME:1 AdaptHmeRatio0:0 AdaptHmeRatio1:0 AdaptHmeRatio2:0 skipqme0:0 skipqme1:0 skipqme2:0 skipqme3:23 fastqmenumbaseframetype0:3 fastqmenumbaseframetype1:2 fastqmenumbaseframetype2:2 FastSubQme:12 adaptiveFmeAlg0:0 adaptiveFmeAlg1:0 adaptiveFmeAlg2:0 adaptiveFmeAlg3:0 GpbSkipL1Me:0 BackLongTermReference:1 InterSatd2Sad:2 qCompress:0.6 qCompressFirst: 0.5 CutreeLambdaPos: 2 CutreeLambdaNeg: 100 CutreeLambdaFactor: 0 AdaptPSNRSSIM:40 LHSatdscale:0 PMESearchRange:1 LookAheadHMVP:0 hmvp-thres0 LookAheadNoEarlySkip:1 LHSatdScaleTh:80 LHSatdScaleTh2:180 FreqDomainCostIntra0:-1 FreqDomainCostIntra1:-1 FreqDomainCostIntra2:-1 deblock(tc:beta):0, 0 RCWaitRatio1: 0.5 RCWaitRatio2: 0.2 RCConsist:0 QPstep:4 RoundKeyint:0 VbvPredictorSimple:0.5 VbvPredictorSimpleMin:0.5 VbvPredictorNormal:1 VbvPredictorNormalMin:1 VbvPredictorComplex:1 VbvPredictorComplexMin:1 DpbErrorResilience: 0 aud:0 lookahead-use-sad:1 intra-sad0:1 intra-sad1:1 intra-sad2:0 intra-sad3:0 intra-sad4:0 skip-sao-freq:0 skip-sao-den:0 skip-sao-dist:0 adaptive-quant:0 wpp2CachedTuCoeffRows:0 disablemergemode:0 merge-mode-depth:0 skipCudepth0:0 faster-code-coeff:0 rqt-split-first0 intra-fine-search-opt:0 sao-use-merge-mode0:0 sao-use-merge-mode1:0 sao-use-merge-mode2:0 enable-sameblock:0 sameblock-dqp:255 scclock-th:0 skip-child-by-sameblock:0 skip-no-zero-mv-by-sameblock:0 sameblock-disable-merge:0 tzsearchscc-adaptive-stepsize:0 skip-intra-fine-search-rd-thr2:0 skip-sub-from-father-skip:0 prune-split-from-rdcost:0 check-cur-from-subcost:0 skip-dct:0 list1-fast-ref-cost-thr:0 list1-fast-ref-cost-thr2:0 char-aq-cost-min:0 char-target-qp:0 content-early-stop-thresh0:0 content-early-stop-thresh1:0 content-early-stop-thresh2:0 content-early-stop-thresh3:0 sub-diff-skip-sub-chroma-weight:-1 skip-inter-by-intra:0 enable-contrast-enhancement:0 lowdelay-skip-ref:0 enable-adaptive-me:0 search-thresh:0 search-offset:0 skip-l1-by-sub:0 skip-dct-inter4x4:0 same-ctu-skip-sao:0 skip-l1-by-skip-dir:0 skip-bi-by-father:0 skip-l1-by-father:0 skip-l1-by-nei:0 enable-hmvp:0 check-cur-from-nei:0 skip-sub-from-skip-2n-cudepth:5 lp-single-ref-list-cudepth:-1 ime-round:16 camera-subjective-opt:1 enable-chroma-weight:0 disable-tu-early-stop-for-subjective:0 enable-subjective-loss:0 enable-contrast-enhancement:0 go-down-qp-for-subjective:51 skip-2n-from-sub-qp-for-subjective:51 skip-2n-from-sub-chroma-weight-for-subjective:0 skip-2n-from-sub-chroma-weight4-for-subjective:0 complex-block-thr-factor1-for-subjective:3 complex-block-thr-factor2-for-subjective:0 opt-try-go-up-for-subjective:0 large-qp-large-size-intra-use-4-angle:0 aq-adjust-for-subjective:0 enable-subjective-char-qp:0 hyperfast-tune-subjective:0 hyperfast-tune-detail:0 scenecut-dqp:0 skip-dct-offset0:0 skip-dct-offset1:0 skip-dct-offset2:0 skip-lowres-inter:0 lp-single-ref-list-cu-depth:-1 lp-single-ref-frame-cu-depth:-1 slice-depth0-fast-ref:0 fast-bi-pattern-search:0 skip-intranxn-in-bslice:0 intra-stride-inter-frame:0 topdown-father-skip:0 skip-father-by-submode:0 skip-coarse-intra-pred-mode-by-cost:0 skip-inter64x64:0 skip-sub-from-skip-2n-cudepth:5 inter-search-lowres-mv:0 skip-dct:0 stop-split-by-neibor-skip:0 intra-non-std-dct:0 pool-mask:0 max-merge:3 tu-inter-depth:1 tu-intra-depth:1 sr:256 me:1 pme:0 max-dqp-depth:1 cb-qp-offset:4 cr-qp-offset:4 lookahead-threads:1 vbv-adapt:0 vbv-control:0 roi2-strength:0 roi2-uv-delta:0 roi2-height-up-scale:0.25 roi2-height-down-scale:0.25 roi2-margin-left-scale:0 roi2-margin-right-scale:0";
    auto packets = dataFuse->pack(msg);

    auto raw = dataFuse->unpack(packets);
    std::string outmsg = std::string(raw.begin(), raw.end());
    if (msg != outmsg) {
        HJFLogi("outmsg:{}", outmsg);
    }

    return;
}

int HJApplicationImpl::onLocalServerNotify(HJNotification::Ptr ntfy)
{
    return HJ_OK;
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
    //HJFileUtil::delete_file((cfg.mLogDir + "HJLog.txt"));
    HJContext::Instance().init(cfg);

    HJGlobalSettings settings;
    settings.useTLSPool = true;
    settings.useHTTPPool = true;
    HJGlobalSettingsManager::init(&settings);

    HJNetManager::registerAllNetworks();

    //HJFileUtil::delete_file((cfg.mLogDir + "jpublisher/jsdk_JLog.txt"));
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

