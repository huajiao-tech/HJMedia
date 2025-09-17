//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJView.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJImguiUtils.h"
#include "HJJson.h"
#include "HJFFUtils.h"
#include "HJException.h"

#define GL_CLAMP_TO_EDGE	0x812F
#define GL_BGRA				0x80E1

NS_HJ_BEGIN
//***********************************************************************************//
const float HJView::K_ICON_SIZE_W_DEFAULT = 26.0f;
const float HJView::K_ICON_SIZE_H_DEFAULT = 26.0f;
HJView::HJView()
{
	m_name = HJMakeGlobalName("HJView");
}

HJView::~HJView()
{

}

int HJView::init(const std::string info)
{
	int res = HJ_OK;
	do
	{
		m_info = info;

	} while (false);
	
	return res;
}

int HJView::draw()
{

	return HJ_OK;
}

int HJView::draw(const HJRectf& rect)
{
	return HJ_OK;
}

const size_t HJView::makeViewID()
{
    static size_t m_idCounter = 0;
    return m_idCounter++;
}

//***********************************************************************************//
HJAppMainMenu::HJAppMainMenu()
{
}

HJAppMainMenu::~HJAppMainMenu()
{
}

int HJAppMainMenu::init(const std::string info)
{
	int res = HJ_OK;
	do
	{
		res = HJView::init(info);
		if (HJ_OK != res) {
			break;
		}
		//m_resDir = "E:/MMGroup/SLMedia_a/jni/resource/icon";
		m_doc = std::make_shared<HJYJsonDocument>();
//		std::string cinfo = HJUtils::AnsiToUtf8(info);
//#if defined(HJ_OS_WINDOWS)
//		char ch = cinfo.back();
//		if ('\n' != ch && '\0'==ch) {
//			cinfo[cinfo.length() - 1] = '\n';
//		}
//#endif
		res = m_doc->init(info);
		if (HJ_OK != res) {
			HJFLoge("error, init main menu failed");
			break;
		}
	} while (false);

	return res;
}
/**
 * only support
 * 1��obj {};
 * 2��string;
 */
int HJAppMainMenu::draw()
{
	int res = HJ_OK;
	do
	{
		//ImGuiWindow* window = ImGui::GetCurrentWindow();
		//if (window)
		//	window->MenuBarImageButtonHeight = toolImageSize.y;

		if (ImGui::BeginMainMenuBar()) 
		{
			res = m_doc->forEach("menu", [&](const HJYJsonObject::Ptr& subObj) -> int {
					return drawSubMenu(subObj);
				});
			
			//Toolbar
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
			res = m_doc->forEach("toolbar", [&](const HJYJsonObject::Ptr& subObj) -> int {
					return drawSubButton(subObj);
				});
			//
			ImGui::EndMainMenuBar();
		}
	} while (false);

	return HJ_OK;
}

int HJAppMainMenu::drawSubMenu(std::shared_ptr<HJYJsonObject> obj)
{
	if (!obj->isObj()) {
		HJLogw("warning, not support obj");
		return HJErrNotSupport;
	}
	int res = HJ_OK;
	do
	{
		std::string titleName = obj->getString("title");
		if (ImGui::BeginMenu(titleName.c_str())) 
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.0f, 3.0f });

			res = obj->forEach("menu", [&](const HJYJsonObject::Ptr& subObj) -> int {
				if (subObj->isObj()) {
					drawSubMenu(subObj);
				}
				else if (subObj->isStr()) 
				{
					std::string subName = subObj->getString();
					if ("---" == subName) {
						ImGui::Separator();
					} else if (ImGui::MenuItem(subName.c_str())) {
						if (onMenuClick) {
							onMenuClick(HJUtf8ToAnsi(titleName), HJUtf8ToAnsi(subName));
						}
						//onMenuClick(HJUtf8ToAnsi(subName));
						//HJLogi("click sub obj, name:" + HJUtf8ToAnsi(subName));
					}
				}
				return HJ_OK;
				});
			ImGui::PopStyleVar();
			ImGui::EndMenu();
		}
	} while (false);

	return HJ_OK;
}

int HJAppMainMenu::drawSubButton(std::shared_ptr<HJYJsonObject> obj)
{
	if (!obj->isObj()) {
		HJLogw("warning, not support obj");
		return HJErrNotSupport;
	}
	int res = HJ_OK;
	do
	{
		m_vsize = { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
		//
		const ImGuiStyle& style = ImGui::GetStyle();
		float iconSize = ImGui::GetWindowSize().y - 2 * style.ItemSpacing.y - 2;
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.ItemSpacing.y);

		std::string iconName = obj->getString("icon");
		size_t key = HJUtilitys::hash(iconName);
		HJImageButton::Ptr button = nullptr; 
		if (m_buttons.end() != m_buttons.find(key)) {
			button = m_buttons.find(key)->second;
		}
		if (!button) {
			//std::string iconPath = HJContext::Instance().getResDir() + "/icon/" + iconName;
			std::string tips = obj->getString("tips");
			button = std::make_shared<HJImageButton>();
			res = button->init(iconName, tips, { iconSize, iconSize });
			button->onBtnClick = [&]() {
				HJLogi("button click");
			};
			m_buttons[key] = button;
		}
		if (button) {
			button->draw();
		}
	} while (false);

	return HJ_OK;
}

//***********************************************************************************//
HJRightClickMenu::HJRightClickMenu()
{

}
HJRightClickMenu::~HJRightClickMenu()
{

}

int HJRightClickMenu::init(const std::string info)
{
	int res = HJ_OK;
	do
	{
		res = HJView::init(info);
		if (HJ_OK != res) {
			break;
		}
	} while (false);

	return res;
}

int HJRightClickMenu::draw()
{
	int res = HJ_OK;
	//HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
	do
	{
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + (float)m_vpos.x, main_viewport->WorkPos.y + (float)m_vpos.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
		//
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_MenuBar;
		if (!ImGui::Begin(HJAnsiToUtf8("tips menu").c_str(), &m_isOpen, window_flags)) {
			ImGui::End(); //�۵�
			return HJ_OK;
		}
		//ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.35f);
		ImGui::PushItemWidth(ImGui::GetFontSize() * -12);

		// Menu Bar
		//if (ImGui::BeginMenuBar()) {

		//	if (ImGui::BeginMenu("Menu"))
		//	{

		//		ImGui::EndMenu();
		//	}
		//	ImGui::EndMenuBar();
		//}

		//
		if (ImGui::CollapsingHeader("Help"))
		{
		}
		ImGui::PopItemWidth();
		ImGui::End();
	} while (false);

	return HJ_OK;
}

int HJRightClickMenu::drawSubMenu()
{
	int res = HJ_OK;
	do
	{

	} while (false);

	return HJ_OK;
}

//***********************************************************************************//
HJOverlayView::HJOverlayView()
{
	m_name = HJMakeGlobalName("overlay");
}

HJOverlayView::~HJOverlayView()
{
}

int HJOverlayView::init(const std::string info)
{
	int res = HJ_OK;
	do
	{
		res = HJView::init(info);
		if (HJ_OK != res) {
			break;
		}
	} while (false);

	return res;
}


int HJOverlayView::draw()
{
	int res = HJ_OK;
	//HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
	do
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		if (m_locationType >= 0)
		{
			const float PAD = 10.0f;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = (m_locationType & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
			window_pos.y = (m_locationType & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
			window_pos_pivot.x = (m_locationType & 1) ? 1.0f : 0.0f;
			window_pos_pivot.y = (m_locationType & 2) ? 1.0f : 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		else if (m_locationType == UI_LOC_Center)
		{
			// Center window
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		//
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		if (ImGui::Begin(HJAnsiToUtf8(m_name).c_str(), &m_isOpen, window_flags))
		{
			ImGui::Text("App average %.3f ms / frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		}
		ImGui::End();
	} while (false);

	return HJ_OK;
}

int HJOverlayView::drawSubMenu()
{
	int res = HJ_OK;
	do
	{

	} while (false);

	return HJ_OK;
}

//***********************************************************************************//
HJPopupView::HJPopupView()
{
	m_name = "popup view";
}

HJPopupView::~HJPopupView()
{
}

int HJPopupView::init(const std::string info)
{
	int res = HJ_OK;
	do
	{
		res = HJView::init(info);
		if (HJ_OK != res) {
			break;
		}
	} while (false);

	return res;
}


int HJPopupView::draw()
{
	int res = HJ_OK;
	//HJFLogi("draw entry, cursor pos {},{}", m_cursorPos.x, m_cursorPos.y);
	do
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 vpos = m_useWorkArea ? viewport->WorkPos : viewport->Pos;
		ImVec2 vsize = m_useWorkArea ? viewport->WorkSize : viewport->Size;
		//ImGuiWindow* barWindow = ImGui::FindWindowByName("##MainMenuBar");
		//if (barWindow) {
		//	vpos.x += barWindow->Pos.x;
		//	vpos.y += barWindow->Pos.y + barWindow->Size.y;
		//	vsize.y -= barWindow->Pos.y + barWindow->Size.y;
		//}
		ImGui::SetNextWindowPos(vpos);
		ImGui::SetNextWindowSize(vsize);
		ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background
		if (ImGui::Begin(HJUtilitys::AnsiToUtf8(m_name).c_str(), &m_isOpen, window_flags))
		{
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Close")) {
                    m_isOpen = false;
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
	} while (false);

	return HJ_OK;
}

int HJPopupView::drawSubMenu()
{
	int res = HJ_OK;
	do
	{

	} while (false);

	return HJ_OK;
}

//***********************************************************************************//
const HJVec4f HJImageButton::ColourEnabledTint = { 1.00f, 1.00f, 1.00f, 1.00f };
const HJVec4f HJImageButton::ColourDisabledTint = { 0.54f, 0.54f, 0.54f, 1.00f };
const HJVec4f HJImageButton::ColourBG = { 0.00f, 0.00f, 0.00f, 0.00f };
const HJVec4f HJImageButton::ColourPressedBG = { 0.21f, 0.45f, 0.21f, 1.00f };
const HJVec4f HJImageButton::ColourClear = { 0.10f, 0.10f, 0.12f, 1.00f };
HJImageButton::HJImageButton()
{
	m_name = HJMakeGlobalName("imgbtn");
}

HJImageButton::~HJImageButton()
{

}

int HJImageButton::init(const std::string& iconName, const std::string& tips, const HJSizef size, const std::string& iconDir)
{
	int res = HJ_OK;
	do
	{
		m_iconName = iconDir + iconName;
		m_tips = tips;
		m_size = size;
		m_image = std::make_shared<HJImage>();
		res = m_image->init(m_iconName);
		if (HJ_OK != res) {
			return res;
		}

	} while (false);

	return res;
}

int HJImageButton::init(const std::string& iconName, const std::string& tips, const HJSizef size, HJRunnable click, const std::string& iconDir)
{
	int res = HJ_OK;
	do
	{
		m_iconName = iconDir + iconName;
		m_tips = tips;
		m_size = size;
		onBtnClick = click;
		m_image = std::make_shared<HJImage>();
		res = m_image->init(m_iconName);
		if (HJ_OK != res) {
			return res;
		}

	} while (false);

	return res;
}

int HJImageButton::draw()
{
	HJVec4f bgColor = m_showProps ? ColourPressedBG : ColourBG;
	//if (ImGui::ImageButton(ImTextureID(m_image->bind()), { m_size.w, m_size.h }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, 3, ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a), ImVec4(ColourEnabledTint.r, ColourEnabledTint.g, ColourEnabledTint.b, ColourEnabledTint.a)))
	if (ImGui::ImageButton(m_name.c_str(), ImTextureID(m_image->bind()), { m_size.w, m_size.h }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a), ImVec4(ColourEnabledTint.r, ColourEnabledTint.g, ColourEnabledTint.b, ColourEnabledTint.a))) 
	{
		if (onBtnClick) {
			onBtnClick();
		}
	}
	showToolTip(m_tips);

	return HJ_OK;
}

int HJImageButton::showToolTip(const std::string& tips)
{
	if (!ImGui::IsItemHovered() || tips.empty()) {
		return HJErrNotSupport;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 3.0f, 3.0f });

	ImGui::BeginTooltip();
	if (m_autoWrap)
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
	ImGui::TextUnformatted(tips.c_str());
	if (m_autoWrap)
		ImGui::PopTextWrapPos();
	ImGui::EndTooltip();

	ImGui::PopStyleVar();

	return HJ_OK;
}

//***********************************************************************************//
HJComplexImageButton::HJComplexImageButton()
{
	m_name = HJMakeGlobalName("cimgbtn");
}
HJComplexImageButton::~HJComplexImageButton()
{
	m_icons.clear();
	m_images.clear();
}

int HJComplexImageButton::init(const std::vector<std::pair<std::string, std::string>> icons, const HJSizef size, HJClickCallback onClick, const std::string& iconDir)
{
	int res = HJ_OK;
	do
	{
		for (auto& it : icons) {
			auto iconName = iconDir + it.second;
			auto img = std::make_shared<HJImage>();
			res = img->init(iconName);
			if (HJ_OK != res) {
				return res;
			}
			m_icons.emplace_back(std::make_pair(it.first, iconName));
			m_images[it.first] = img;
		}
		m_size = size;
		onBtnClick = onClick;
	} while (false);

	return res;
}

int HJComplexImageButton::draw()
{
	HJVec4f bgColor = m_showProps ? HJImageButton::ColourPressedBG : HJImageButton::ColourBG;
	const auto& icon = m_icons[m_showIdx % m_icons.size()];
	const auto& tips = icon.first;
	auto img = getImage(tips);
	if (!img) {
		return HJErrFatal;
	}
	if (ImGui::ImageButton(m_name.c_str(), ImTextureID(img->bind()), { m_size.w, m_size.h }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a), ImVec4(HJImageButton::ColourEnabledTint.r, HJImageButton::ColourEnabledTint.g, HJImageButton::ColourEnabledTint.b, HJImageButton::ColourEnabledTint.a)))
	//if (ImGui::ImageButton(ImTextureID(img->bind()), { m_size.w, m_size.h }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, 3, ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a), ImVec4(HJImageButton::ColourEnabledTint.r, HJImageButton::ColourEnabledTint.g, HJImageButton::ColourEnabledTint.b, HJImageButton::ColourEnabledTint.a)))
	{
		m_showIdx++;
		if (onBtnClick) {
			onBtnClick(tips);
		}
	}
	showToolTip(tips);

	return HJ_OK;
}

int HJComplexImageButton::showToolTip(const std::string& tips)
{
	if (!ImGui::IsItemHovered() || tips.empty()) {
		return HJErrNotSupport;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 3.0f, 3.0f });

	ImGui::BeginTooltip();
	if (m_autoWrap)
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
	ImGui::TextUnformatted(tips.c_str());
	if (m_autoWrap)
		ImGui::PopTextWrapPos();
	ImGui::EndTooltip();

	ImGui::PopStyleVar();

	return HJ_OK;
}

void HJComplexImageButton::reset()
{
	m_showIdx = 0;
}

//***********************************************************************************//
int HJButton::init(const std::string& name, HJRunnable click, int id)
{
	m_name = name;
	m_id = id;
    //m_tips = tips;
    onBtnClick = click;
    return HJ_OK;
}

int HJButton::draw()
{
    ImGui::PushID(m_id);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(3.f / 7.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(3.f / 7.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(3.f / 7.0f, 0.8f, 0.8f));
    if(ImGui::Button(m_name.c_str())) {
        if (onBtnClick) {
            onBtnClick();
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::PopID();
    
    //showToolTip(m_tips);

    return HJ_OK;
}

int HJButton::showToolTip(const std::string& tips)
{
    if (!ImGui::IsItemHovered() || tips.empty()) {
        return HJErrNotSupport;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 3.0f, 3.0f });

    ImGui::BeginTooltip();
    if (m_autoWrap)
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(tips.c_str());
    if (m_autoWrap)
        ImGui::PopTextWrapPos();
    ImGui::EndTooltip();

    ImGui::PopStyleVar();

    return HJ_OK;
}

//***********************************************************************************//
int HJImageView::init(const std::string& imgName)
{
	int res = HJ_OK;
	do
	{
		m_imgName = imgName;
		m_image = std::make_shared<HJImage>();
		res = m_image->init(m_imgName);
		if (HJ_OK != res) {
			return res;
		}
	} while (false);
	return res;
}

int HJImageView::draw()
{
	ImGui::Image(ImTextureID(m_image->bind()), { (float)m_image->getWidth(), (float)m_image->getHeight() });
	return HJ_OK;
}

//***********************************************************************************//
HJFrameView::~HJFrameView()
{
	done();
}

int HJFrameView::init()
{
	return HJ_OK;
}

void HJFrameView::done()
{
	unbind();
}

int HJFrameView::draw(const HJMediaFrame::Ptr frame)
{
	if (!frame) {
		return HJErrInvalidParams;
	}
	int res = HJ_OK;
	do
	{
		uint64_t texId = bind(frame);
		if (!texId) {
			res = HJErrFatal;
			break;
		}
		ImGui::Image(ImTextureID(texId), { (float)m_info->m_width, (float)m_info->m_height });
		//ImGui::Image(ImTextureID(texId), { (float)720.f, (float)1280.f });
	} while (false);

	return res;
}

int HJFrameView::createConverter(const HJVideoInfo::Ptr& vinfo)
{
	int res = HJ_OK;
	do
	{
		HJVideoInfo::Ptr videoInfo = std::make_shared<HJVideoInfo>(vinfo->m_width, vinfo->m_height);
		if (!videoInfo) {
			res = HJErrNewObj;
			break;
		}
		videoInfo->m_pixFmt = AV_PIX_FMT_RGBA; //AV_PIX_FMT_RGB24
		m_info = videoInfo;

		m_converter = std::make_shared<HJVSwsConverter>(m_info);
		if (!m_converter) {
			res = HJErrNewObj;
			break;
		}
		m_converter->setCropMode(HJ_VCROP_FILL);
	} while (false);

	return res;
}
HJMediaFrame::Ptr HJFrameView::convert(const HJMediaFrame::Ptr frame)
{
	int res = HJ_OK;
	HJMediaFrame::Ptr mavf = frame;
	do
	{
		const auto& videoInfo = frame->getVideoInfo();
		if (!videoInfo) {
			res = HJErrInvalidData;
			break;
		}
		if (!m_converter || m_converter->getOutInfo()->m_width != videoInfo->m_width || m_converter->getOutInfo()->m_height != videoInfo->m_height) {
		//if (!m_converter) {
			res = createConverter(videoInfo);
			if (HJ_OK != res) {
				break;
			}
		}
		if (videoInfo->m_width != m_info->m_width ||
			videoInfo->m_height != m_info->m_height ||
			videoInfo->m_pixFmt != m_info->m_pixFmt ||
			!HJ_IS_PRECISION_DEFAULT(videoInfo->m_rotate)) {
			mavf = m_converter->convert2(frame);
		}
	} while (false);

	return mavf;
}

uint64_t HJFrameView::bind(const HJMediaFrame::Ptr frame)
{
	int res = HJ_OK;
	do
	{
		if (m_texID  && (m_curFrameName == frame->getName())) {
			glBindTexture(GL_TEXTURE_2D, (GLuint)m_texID);
			return m_texID;
		}
		m_curFrameName = frame->getName();
		//
		if (!m_texID) 
		{
			GLuint tex = 0;
			glGenTextures(1, &tex);
			if (!tex) {
				return 0;
			}
			m_texID = tex;
			//
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		const auto mvf = convert(frame);
		if (!mvf) {
			return 0;
		}
		AVFrame* avf = (AVFrame*)mvf->getAVFrame();
		if (!avf) {
			return 0;
		}
		int fmt = 1;
		int stride = avf->linesize[0] * 8 / HJ_MAX(8, hj_get_bits_per_pixel(avf->format));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)stride);
		glBindTexture(GL_TEXTURE_2D, (GLuint)m_texID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, avf->width, avf->height, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, avf->data[0]);
		//glGenerateMipmap(GL_TEXTURE_2D);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	} while (false);

	return m_texID;
}
void HJFrameView::unbind()
{
	GLuint texID = (GLuint)(m_texID);
	if (0 != texID) {
		glDeleteTextures(1, &texID);
		texID = 0;
	}
}

//***********************************************************************************//
int HJLoadingView::init()
{
	m_name = HJMakeGlobalName("loadingview");
	return HJ_OK;
}

int HJLoadingView::draw()
{
	int res = HJ_OK;
	do
	{
		float indicator_radius = 20.0f;
		int circle_count = 12;
		float speed = 7.f;
		const ImVec4 main_color = ImVec4(144, 0, 0, 1.0);
		const ImVec4 backdrop_color = ImVec4(0, 255, 0, 1.0);
		//
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) {
			break;
		}
		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(m_name.c_str());

		const ImVec2 pos = window->DC.CursorPos;
		const float circle_radius = indicator_radius / 15.0f;
		const float updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
		const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, id)) {
			break;
		}
		const float t = g.Time;
		const auto degree_offset = 2.0f * IM_PI / circle_count;
		for (int i = 0; i < circle_count; ++i) {
			const auto x = updated_indicator_radius * std::sin(degree_offset * i);
			const auto y = updated_indicator_radius * std::cos(degree_offset * i);
			const auto growth = HJ_MAX(0.0f, std::sin(t * speed - i * degree_offset));
			ImVec4 color;
			color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
			color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
			color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
			color.w = 1.0f;
			window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x,
				pos.y + indicator_radius - y),
				circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
		}
	} while (false);

	return res;
}

//***********************************************************************************//
HJProgressView::HJProgressView()
{
	m_name = HJMakeGlobalName("progressview");
}

int HJProgressView::init()
{
	return HJ_OK;
}

bool HJProgressView::draw(int64_t* value, int64_t min, int64_t max)
{
	do
	{
		const ImGuiSliderFlags flags = 0;
		const char* format = "%lld ms";
		ImGuiDataType data_type = ImGuiDataType_S64;
		void* p_data = value;
		const void* p_min = &min;
		const void* p_max = &max;
		const ImU32 fill_col = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 184, 159, 1.0)); //ImGui::GetColorU32(ImGuiCol_Button);
		//
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		
		const char* label = m_name.c_str();
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const float w = window->Size.x - 4.0f * style.FramePadding.x; //ImGui::CalcItemWidth();

		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		const ImVec2 cursorPosDelta = ImVec2(w, 0.5f * label_size.y + style.FramePadding.y * 2.0f);
		const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + cursorPosDelta.x, window->DC.CursorPos.y + cursorPosDelta.y));
		const ImVec2 frame_bb_delta = ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f);
		const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + frame_bb_delta.x, frame_bb.Max.y + frame_bb_delta.y));

		const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
			return false;

		// Default format string when passing NULL
		//if (format == NULL)
		//	format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

		const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
		bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
		if (!temp_input_is_active)
		{
			// Tabbing or CTRL-clicking on Slider turns it into an input box
			const bool clicked = hovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, id);
			const bool make_active = (clicked || g.NavActivateId == id);
			if (make_active && clicked)
				ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
			if (make_active && temp_input_allowed)
				if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
					temp_input_is_active = true;

			if (make_active && !temp_input_is_active)
			{
				ImGui::SetActiveID(id, window);
				ImGui::SetFocusID(id, window);
				ImGui::FocusWindow(window);
				g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			}
		}

		//if (temp_input_is_active)
		//{
		//	// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
		//	const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
		//	return ImGui::TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
		//}

		// Draw frame
		const ImU32 frame_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		ImGui::RenderNavHighlight(frame_bb, id);
		ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

		// Slider behavior
		ImRect grab_bb;
		const bool value_changed = ImGui::SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
		if (value_changed)
			ImGui::MarkItemEdited(id);

		// Render grab
		if (grab_bb.Max.x > grab_bb.Min.x) {
			ImVec2 fill_bb_min = ImVec2(frame_bb.Min.x, grab_bb.Min.y);
			ImVec2 fill_bb_max = ImVec2(grab_bb.Min.x, grab_bb.Max.y);
			window->DrawList->AddRectFilled(fill_bb_min, fill_bb_max, fill_col);
			window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
		}

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		//char value_buf[64];
		//const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
		//if (g.LogEnabled)
		//	ImGui::LogSetNextTextDecoration("{", "}");
		//ImGui::RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

		//if (label_size.x > 0.0f)
		//	ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		//IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
		return value_changed;
	} while (false);

	return false;
}

//***********************************************************************************//
HJCombo::HJCombo()
{
	m_name = HJMakeGlobalName("progressview");
}

HJCombo::~HJCombo()
{
	if (m_comboItems) {
		delete m_comboItems;
		m_comboItems = NULL;
	}
}

int HJCombo::init(const std::vector<std::string> items, int currentIdx, HJComboCallback onClick)
{
	m_items = items;
	m_itemCurrentIdex = currentIdx;
	m_comboItems = new char* [m_items.size()];
	for (size_t i = 0; i < m_items.size(); i++) {
		m_comboItems[i] = (char *)m_items[i].c_str();
	}
	m_onClick = onClick;
	return HJ_OK;
}

int HJCombo::draw()
{
	int res = HJ_OK;
	do
	{
		static ImGuiComboFlags combo_flags = ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_WidthFitPreview;
		const char* combo_preview_value = m_comboItems[m_itemCurrentIdex];  // Pass in the preview value visible before opening the combo (it could be anything)
		if (ImGui::BeginCombo("speed", combo_preview_value, combo_flags))
		{
			for (int n = 0; n < m_items.size(); n++)
			{
				const bool is_selected = (m_itemCurrentIdex == n);
				if (ImGui::Selectable(m_comboItems[n], is_selected)) {
					m_itemCurrentIdex = n;
					if (m_onClick) {
						m_onClick(m_itemCurrentIdex);
					}
				}
				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	} while (false);

	return res;
}

//***********************************************************************************//
HJSlider::HJSlider(const std::string& tags, const float vmin, const float vmax, const float val, HJSliderCallback onClick)
{
	m_name = HJMakeGlobalName("slider");
	int res = init(tags, vmin, vmax, val, onClick);
	if (HJ_OK != res) {
		HJ_EXCEPT(HJException::ERR_INVALID_STATE, "create slider failed");
	}
}

HJSlider::~HJSlider()
{

}

int HJSlider::init(const std::string& tags, const float vmin, const float vmax, const float val, HJSliderCallback onClick)
{
	m_tags = tags;
	m_vmin = vmin;
	m_vmax = vmax;
	m_val = val;
	m_onClick = onClick;
	return HJ_OK;
}

int HJSlider::draw()
{
	static ImGuiSliderFlags flags = ImGuiSliderFlags_None;
	if (ImGui::SliderFloat(m_tags.c_str(), &m_val, 0.0f, 1.0f, "%.3f", flags)) {
		if (m_onClick) {
			m_onClick(m_val);
		}
	}
	return HJ_OK;
}

NS_HJ_END
