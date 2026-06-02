#include "App.h"
#include "HJUIBaseItem.h"
#include "HJFLog.h"

NS_HJ_BEGIN
class ImPlotDemo : public App 
{
public:
    using App::App;

	virtual int RenderEveryStart()
	{
        int i_err = 0;
		for (auto& item : m_items)
		{
			i_err = item->renderEveryStart();
			if (i_err < 0)
			{
				break;
			}
		}
        return i_err;
	}
	virtual int RenderEveryEnd()
	{
		int i_err = 0;
		for (auto& item : m_items)
		{
			i_err = item->renderEveryEnd();
			if (i_err < 0)
			{
				break;
			}
		}
		return i_err;
	}
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

#if !defined(HJ_MEDIA_GRAPHIC_UI)
				if (ImGui::MenuItem("SR"))
				{
					type = HJUIItemType_SR;
				}
                if (ImGui::MenuItem("NodeEditor"))
                {
                    type = HJUIItemType_NodeEditor;
                }
                if (ImGui::MenuItem("RendGraphWrapper"))
                {
                    type = HJUIItemType_RendGraphWrapper;
                }
                                                
                if (ImGui::MenuItem("PlayerCom"))
				{
					type = HJUIItemType_PlayerCom;
				}
				if (ImGui::MenuItem("Test"))
				{
					type = HJUIItemType_Test;
				}

                if (ImGui::MenuItem("SharedMemory"))
                {
                    type = HJUIItemType_SharedMemory;
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
#endif

				if (ImGui::MenuItem("FaceuTool"))
				{
					type = HJUIItemType_FaceuTool;
				}

                if (type != HJUIItemType_None)
                {
                    m_items.clear();
                    HJUIBaseItem::Ptr item = HJUIBaseItem::createItem(type, getWindow());
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

            bool bUpdate = item->updateTitle();
            App::updateTitle(!bUpdate);
        }
    }

private:
    std::vector<HJUIBaseItem::Ptr> m_items;
};
NS_HJ_END

int main(int argc, char const *argv[])
{
    HJ::HJLog::Instance().init(true, "e:/", 2, ((1 << 1) | (1 << 2)), true, 1024 * 1024 * 5, 5);

    HJ::ImPlotDemo app("Demo", 1020,1400,argc,argv);
    app.Run(30);

    return 0;
}
