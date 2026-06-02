#include "App.h"
#include "HJFLog.h"
#include "HJRenderContextExport.h"
#include "HJFaceRenderUI.h"

NS_HJ_BEGIN
class ImPlotDemo : public App 
{
public:
    using App::App;
    void Update() override 
    {
        bool bOpen = true;

		if (ImGui::Begin("CreateUI"))
		{
            if (!m_faceRenderUI)
            {
                m_faceRenderUI = HJFaceRenderUI::Create();
            }
            m_faceRenderUI->run();
		}
        ImGui::End();
        //if (ImGui::Button("open"))
        //{

        //}
        //ImGui::Begin("menu_0", &bOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);
        //ImGui::SetWindowPos(ImVec2(0, 0));
        //if (ImGui::BeginMenuBar())
        //{
        //    if (ImGui::BeginMenu("Items"))
        //    {

        //        ImGui::EndMenu();
        //    }
        //    ImGui::EndMenuBar();
        //}
        //ImGui::End();
    }
private:
    HJFaceRenderUI::Ptr m_faceRenderUI = nullptr;
};
NS_HJ_END

int main(int argc, char const *argv[])
{
    HJ::HJLog::Instance().init(true, "e:/hjdemolog/", 2, ((1 << 1) | (1 << 2)), true, 1024 * 1024 * 5, 5);
    HJEntryContextInfo info;
    info.logIsValid = true;
    info.logDir = "e:/";
    info.logLevel = HJLOGLevel_INFO;
    info.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
    info.logMaxFileSize = 5 * 1024 * 1024;
    info.logMaxFileNum = 5;
    renderContextInit(info);

    HJ::ImPlotDemo app("Demo", 1560,1360,argc,argv);
    app.Run(30);

    renderContextUnInit();
    return 0;
}
