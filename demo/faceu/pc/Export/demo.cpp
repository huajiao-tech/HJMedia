#include "App.h"
#include "HJRenderContextExport.h"
#include "HJFaceRenderUI.h"
#include <memory>


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
                m_faceRenderUI = std::make_shared<HJFaceRenderUI>();
            }
            m_faceRenderUI->run();
		}
        ImGui::End();
    }
private:
    std::shared_ptr<HJFaceRenderUI> m_faceRenderUI = nullptr;
};

int main(int argc, char const *argv[])
{
    HJEntryContextInfo info;
    info.logIsValid = true;
    info.logDir = "";
    info.logLevel = HJLOGLevel_INFO;
    info.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
    info.logMaxFileSize = 5 * 1024 * 1024;
    info.logMaxFileNum = 5;
    renderContextInit(info);

    ImPlotDemo app("Demo", 1560,1360,argc,argv);
    app.Run(30);

    renderContextUnInit();
    return 0;
}
