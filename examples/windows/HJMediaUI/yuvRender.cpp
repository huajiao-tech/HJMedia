#include "yuvRender.h"
#include "imgui.h"
#include "HJFileData.h"
#include "libYuv.h"
#include "HJSPBuffer.h"

NS_HJ_BEGIN

yuvRender::yuvRender()
{

}
yuvRender::~yuvRender()
{

}
int yuvRender::init(const std::string& i_url, int i_width, int i_height)
{
	int i_err = 0;
	do
	{
		m_yuvReader = HJYuvReader::Create();
		i_err = m_yuvReader->init(i_url, i_width, i_height);
		if (i_err)
		{
			break;
		}
		if (!m_bCreateTexture)
		{
			m_bCreateTexture = true;
			glGenTextures(1, &m_textureId);
			glBindTexture(GL_TEXTURE_2D, m_textureId);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		m_RGBBuffer = HJSPBuffer::create(i_width * i_height * 4);
	} while (false);
	return i_err;
}
void yuvRender::draw()
{
	bool bShow = false;
	bool bOpen=true;
	ImGui::Begin("menu_0", &bOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);
	ImGui::SetWindowPos(ImVec2(0, 0));     
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Menu"))
		{
			/*IMGUI_DEMO_MARKER("Menu/File");
			ShowExampleMenuFile();*/
			if (ImGui::MenuItem("Main menu bar"))
			{
				int a = 0;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Examples"))
		{
			/*IMGUI_DEMO_MARKER("Menu/Examples");
			ImGui::MenuItem("Main menu bar", NULL, &demo_data->ShowMainMenuBar);

			ImGui::SeparatorText("Mini apps");
			ImGui::MenuItem("Assets Browser", NULL, &demo_data->ShowAppAssetsBrowser);
			ImGui::MenuItem("Console", NULL, &demo_data->ShowAppConsole);
			ImGui::MenuItem("Custom rendering", NULL, &demo_data->ShowAppCustomRendering);
			ImGui::MenuItem("Documents", NULL, &demo_data->ShowAppDocuments);
			ImGui::MenuItem("Log", NULL, &demo_data->ShowAppLog);
			ImGui::MenuItem("Property editor", NULL, &demo_data->ShowAppPropertyEditor);
			ImGui::MenuItem("Simple layout", NULL, &demo_data->ShowAppLayout);
			ImGui::MenuItem("Simple overlay", NULL, &demo_data->ShowAppSimpleOverlay);

			ImGui::SeparatorText("Concepts");
			ImGui::MenuItem("Auto-resizing window", NULL, &demo_data->ShowAppAutoResize);
			ImGui::MenuItem("Constrained-resizing window", NULL, &demo_data->ShowAppConstrainedResize);
			ImGui::MenuItem("Fullscreen window", NULL, &demo_data->ShowAppFullscreen);
			ImGui::MenuItem("Long text display", NULL, &demo_data->ShowAppLongText);
			ImGui::MenuItem("Manipulating window titles", NULL, &demo_data->ShowAppWindowTitles);*/

			ImGui::EndMenu();
		}
		//if (ImGui::MenuItem("MenuItem")) {} // You can also use MenuItem() inside a menu bar!
		if (ImGui::BeginMenu("Tools"))
		{
//			IMGUI_DEMO_MARKER("Menu/Tools");
//			ImGuiIO& io = ImGui::GetIO();
//#ifndef IMGUI_DISABLE_DEBUG_TOOLS
//			const bool has_debug_tools = true;
//#else
//			const bool has_debug_tools = false;
//#endif
//			ImGui::MenuItem("Metrics/Debugger", NULL, &demo_data->ShowMetrics, has_debug_tools);
//			if (ImGui::BeginMenu("Debug Options"))
//			{
//				ImGui::BeginDisabled(!has_debug_tools);
//				ImGui::Checkbox("Highlight ID Conflicts", &io.ConfigDebugHighlightIdConflicts);
//				ImGui::EndDisabled();
//				ImGui::Checkbox("Assert on error recovery", &io.ConfigErrorRecoveryEnableAssert);
//				ImGui::TextDisabled("(see Demo->Configuration for details & more)");
//				ImGui::EndMenu();
//			}
//			ImGui::MenuItem("Debug Log", NULL, &demo_data->ShowDebugLog, has_debug_tools);
//			ImGui::MenuItem("ID Stack Tool", NULL, &demo_data->ShowIDStackTool, has_debug_tools);
//			bool is_debugger_present = io.ConfigDebugIsDebuggerPresent;
//			if (ImGui::MenuItem("Item Picker", NULL, false, has_debug_tools))// && is_debugger_present))
//				ImGui::DebugStartItemPicker();
//			if (!is_debugger_present)
//				ImGui::SetItemTooltip("Requires io.ConfigDebugIsDebuggerPresent=true to be set.\n\nWe otherwise disable some extra features to avoid casual users crashing the application.");
//			ImGui::MenuItem("Style Editor", NULL, &demo_data->ShowStyleEditor);
//			ImGui::MenuItem("About Dear ImGui", NULL, &demo_data->ShowAbout);

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::End();

	unsigned char* pYuv = m_yuvReader->read();
	int w = m_yuvReader->getWidth();
	int h = m_yuvReader->getHeight();
	libyuv::I420ToABGR(m_yuvReader->getY(), w, m_yuvReader->getU(), w / 2, m_yuvReader->getV(), w / 2,
		m_RGBBuffer->getBuf(), w * 4,
		w, h);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RGBBuffer->getBuf());


	ImGui::Begin("abc");

#if CursoScreenPos
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	draw_list->AddLine(ImVec2(pos.x, pos.y - 3.0f), ImVec2(pos.x, pos.y + 4.0f), IM_COL32(255, 0, 0, 255), 1.0f);
	draw_list->AddLine(ImVec2(pos.x - 3.0f, pos.y), ImVec2(pos.x + 4.0f, pos.y), IM_COL32(255, 0, 0, 255), 1.0f);
#endif

	//ImGui::SetWindowPos(ImVec2(0, 0));     // 窗口位置 (200, 150)

	//ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 0, 0, 1)); // 红色边框
	//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::Button("efgh");
	ImGui::SmallButton("ijkl");
	//ImGui::PopStyleVar();
	//ImGui::PopStyleColor();
	//ImGui::Button("Button2");	
	//ImGui::Button("Button3");	
	//ImGui::Button("Button4");
	//ImGui::Button("Button5");
	//ImGui::Button("Button6");
	ImGui::End();

	//ImGui::Begin("ButtonTestDis2");
	//ImGui::SetWindowPos(ImVec2(200, 0));     // 窗口位置 (200, 150)
	//ImGui::Button("Button1");
	//ImGui::Button("Button2");
	//ImGui::Button("Button3");
	//ImGui::Button("Button4");
	//ImGui::Button("Button5");
	//ImGui::Button("Button6");
	//ImGui::End();
	// 
	// 
	//ImGui::Begin("DrawImg");
	//ImGui::SetWindowPos(ImVec2(0, 0));     // 窗口位置 (200, 150)

	//ImGui::SetWindowSize(ImVec2(w, h));
	//ImGui::Image((void*)(intptr_t)m_textureId, ImVec2(w, h));
	//ImGui::End();

	//ImVec2 tex_size(w, h);
	//ImVec2 target_size(200.0f, 200.0f);
	//float scale = std::min(target_size.x / tex_size.x, target_size.y / tex_size.y);
	//ImVec2 display_size(tex_size.x * scale, tex_size.y * scale);

	//ImGui::Begin("DrawImg", nullptr, ImGuiWindowFlags_NoTitleBar);

	////ImVec2 window_pos = ImGui::GetWindowPos(); // 窗口左上角坐标（含标题栏）
	////ImVec2 content_pos = ImGui::GetCursorScreenPos(); // 内容区起始坐标
	////float title_bar_height = content_pos.y - window_pos.y;

	////ImGui::SetWindowPos(ImVec2(10, 10));
	//ImGui::SetWindowSize(ImVec2(target_size.x, target_size.y));

	//ImVec2 pos = ImGui::GetCursorScreenPos();
	//ImVec2 offset((target_size.x - display_size.x) * 0.5f, (target_size.y - display_size.y) * 0.5f);

	//
	//ImGui::SetCursorScreenPos(ImVec2(pos.x + offset.x, pos.y + offset.y));
	//ImGui::Image((void*)(intptr_t)m_textureId, display_size);
	//ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + target_size.y));
	//ImGui::End();

#if 0
	ImGui::Begin("DrawImg");
	ImGui::SetWindowPos(ImVec2(0, 0));     // 窗口位置 (200, 150)
	ImGui::SetWindowSize(ImVec2(w, h));
	ImGui::Image((void*)(intptr_t)m_textureId, ImVec2(w, h));
	ImGui::End();

	ImGui::Begin("Hello, world!");     
	ImGui::SetWindowPos(ImVec2(800, 0));
	ImGui::Button("Button");
	//ImGui::Text("This is some useful text.");
	ImGui::End();

	ImGui::Begin("Hello, world2");
	ImGui::SetWindowPos(ImVec2(1200, 0));
	ImGui::Button("Button2");
	//ImGui::Text("This is some useful text.");
	ImGui::End();

	ImVec2 tex_size(w, h);
	ImVec2 target_size(500.0f, 500.0f);
	float scale = std::min(target_size.x / tex_size.x, target_size.y / tex_size.y);
	ImVec2 display_size(tex_size.x * scale, tex_size.y * scale);

	ImGui::Begin("DrawImg", nullptr, ImGuiWindowFlags_NoTitleBar);

	//ImVec2 window_pos = ImGui::GetWindowPos(); // 窗口左上角坐标（含标题栏）
	//ImVec2 content_pos = ImGui::GetCursorScreenPos(); // 内容区起始坐标
	//float title_bar_height = content_pos.y - window_pos.y;

	ImGui::SetWindowPos(ImVec2(10, 0));
	ImGui::SetWindowSize(ImVec2(target_size.x, target_size.y));

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 offset((target_size.x - display_size.x) * 0.5f, (target_size.y - display_size.y) * 0.5f);

	ImGui::SetCursorScreenPos(ImVec2(pos.x + offset.x, pos.y + offset.y));
	ImGui::Image((void*)(intptr_t)m_textureId, display_size);
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + target_size.y));
	ImGui::End();
#endif

}

NS_HJ_END