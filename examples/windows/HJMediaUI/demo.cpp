#include "App.h"
#include "HJUIBaseItem.h"
#include "HJFLog.h"

NS_HJ_BEGIN
class ImPlotDemo : public App 
{
public:
    using App::App;
    void Update() override 
    {
        bool bOpen = true;

        //ImGui::SetNextWindowSize(ImVec2(32.0 * 25, 32.0 * 15), ImGuiCond_FirstUseEver);
        ImGui::Begin("menu_0", &bOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowPos(ImVec2(0, 0));
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Items"))
            {
                HJUIItemType type = HJUIItemType_None;

				if (ImGui::MenuItem("Test"))
				{
					type = HJUIItemType_Test;
				}

                if (ImGui::MenuItem("SharedMemory"))
                {
                    type = HJUIItemType_SharedMemory;
                }   



                if (ImGui::MenuItem("PlayerCom"))
                {
                    type = HJUIItemType_PlayerCom;
                }     
                if (ImGui::MenuItem("PlayerPlugin"))
                {
                    type = HJUIItemType_PlayerPlugin;
                }

                if (ImGui::MenuItem("Yuv"))
                {
                    type = HJUIItemType_YuvRender;
                }
                
                if (ImGui::MenuItem("Pusher"))
                {
                    int a = 0;
                }



                if (type != HJUIItemType_None)
                {
                    m_items.clear();
                    HJUIBaseItem::Ptr item = HJUIBaseItem::createItem(type);
                    m_items.emplace_back(std::move(item));
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        for (auto& item : m_items)
        {
            int i_err = item->run();
            if (i_err < 0)
            {
                break;
            }
        }
    }

private:
    std::vector<HJUIBaseItem::Ptr> m_items;
};
NS_HJ_END

int main(int argc, char const *argv[])
{
    HJ::HJLog::Instance().init(true, "e:/", 2, ((1 << 1) | (1 << 2)), true, 1024 * 1024 * 5, 5);

    HJ::ImPlotDemo app("Demo", 1920,1080,argc,argv);
    app.Run();

    return 0;
}
