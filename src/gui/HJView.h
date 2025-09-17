//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJLog.h"
#include "HJNotify.h"
#include "HJMediaUtils.h"
#include "HJImage.h"
 #include "HJContext.h"
#include "HJMediaFrame.h"
#include "HJVideoConverter.h"

NS_HJ_BEGIN
class HJYJsonObject;
class HJYJsonDocument;
//
class HJImageButton;
//***********************************************************************************//
class HJView : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJView>;
	explicit HJView();
	virtual ~HJView();

	virtual int init(const std::string info);
	virtual int draw();
	virtual int draw(const HJRectf& rect);

	bool isDone() {
		return !m_isOpen;
	}
	void setVPos(const HJPostionf pos) {
		m_vpos = pos;
	}
	const HJPostionf& getVPos() {
		return m_vpos;
	}
	void setVSize(const HJSizef size) {
		m_vsize = size;
	}
	const HJSizef& getVSize() {
		return m_vsize;
	}
public:
	static const float K_ICON_SIZE_W_DEFAULT;	//ImGui::CalcTextSize("A").x;
	static const float K_ICON_SIZE_H_DEFAULT;	//ImGui::GetTextLineHeightWithSpacing()
    
    static const size_t makeViewID();
protected:
	std::string		m_info = "";
	bool			m_isOpen = true;
	HJPostionf		m_vpos = { 0.0f, 0.0f };
	HJSizef		m_vsize = { 1280.f, 720.f };
	bool			m_useWorkArea = true;
};

//***********************************************************************************//
class HJAppMainMenu : public HJView
{
public:
	using Ptr = std::shared_ptr<HJAppMainMenu>;
	explicit HJAppMainMenu();
	virtual ~HJAppMainMenu();

	virtual int init(const std::string info) override;
	virtual int draw() override;
	//
	//
	using HJClickNotify = std::function<void(const std::string& title, const std::string& name)>;
	HJClickNotify onMenuClick;
private:
	int drawSubMenu(std::shared_ptr<HJYJsonObject> obj);
	int drawSubButton(std::shared_ptr<HJYJsonObject> obj);
protected:
	std::shared_ptr<HJYJsonDocument>	m_doc;
	std::unordered_map<size_t, std::shared_ptr<HJImageButton>> m_buttons;
};

//***********************************************************************************//
class HJRightClickMenu : public HJView
{
public:
	using Ptr = std::shared_ptr<HJRightClickMenu>;
	explicit HJRightClickMenu();
	virtual ~HJRightClickMenu();

	virtual int init(const std::string info) override;
	virtual int draw() override;
	//
	virtual int onMenuClick(const std::string& name) { return HJ_OK; }
private:
	int drawSubMenu();
protected:
	std::shared_ptr<HJYJsonDocument>	m_doc;
};

//***********************************************************************************//
class HJOverlayView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJOverlayView>;
	explicit HJOverlayView();
	virtual ~HJOverlayView();

	virtual int init(const std::string info) override;
	virtual int draw() override;
	//
	virtual int onMenuClick(const std::string& name) { return HJ_OK; }

	typedef enum UILocationType {
		UI_LOC_Custom = -1,
		UI_LOC_Center = -2,
		UI_LOC_Top_Left = 0,
		UI_LOC_Top_Right = 1,
		UI_LOC_Bottom_Left = 2,
		UI_LOC_Bottom_Right = 3
	} UILocationType;
private:
	int drawSubMenu();
protected:
	std::shared_ptr<HJYJsonDocument>	m_doc;
	UILocationType						m_locationType = UI_LOC_Bottom_Right;
};

//***********************************************************************************//
class HJPopupView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJPopupView>;
	explicit HJPopupView();
	virtual ~HJPopupView();

	virtual int init(const std::string info) override;
	virtual int draw() override;
	//
	virtual int onMenuClick(const std::string& name) { return HJ_OK; }
private:
	int drawSubMenu();
protected:
	std::shared_ptr<HJYJsonDocument>	m_doc;
};

//***********************************************************************************//
class HJImageButton: public HJView
{
public:
	using Ptr = std::shared_ptr<HJImageButton>;
	explicit HJImageButton();
	virtual ~HJImageButton();

	virtual int init(const std::string& iconName, const std::string& tips, const HJSizef size, const std::string& iconDir = HJContext::Instance().getIconDir());
	virtual int init(const std::string& iconName, const std::string& tips, const HJSizef size, HJRunnable click, const std::string& iconDir = HJContext::Instance().getIconDir());
	virtual int draw() override;
public:
	static const HJVec4f ColourEnabledTint;
	static const HJVec4f ColourDisabledTint;
	static const HJVec4f ColourBG;
	static const HJVec4f ColourPressedBG;
	static const HJVec4f ColourClear;
	//
	int showToolTip(const std::string& tips);
public:
	HJRunnable		onBtnClick;
protected:
	std::string		m_iconName = "";
	std::string		m_tips = "";
	HJImage::Ptr	m_image = nullptr;
	HJSizef		m_size = {0.f, 0.f};
	bool			m_showProps = false;
	bool			m_autoWrap = true;
};
//***********************************************************************************//
class HJComplexImageButton : public HJView
{
public:
	using Ptr = std::shared_ptr<HJComplexImageButton>;
	using HJClickCallback = std::function<void(const std::string)>;
	explicit HJComplexImageButton();
	virtual ~HJComplexImageButton();

	virtual int init(const std::vector<std::pair<std::string, std::string>> icons, const HJSizef size, HJClickCallback onClick, const std::string& iconDir = HJContext::Instance().getIconDir());
	virtual int draw() override;
	virtual void reset();
private:
	int showToolTip(const std::string& tips);
	HJImage::Ptr getImage(const std::string& key) {
		auto it = m_images.find(key);
		if (it != m_images.end()) {
			return it->second;
		}
		return nullptr;
	}
public:
	HJClickCallback onBtnClick;
protected:
	std::vector<std::pair<std::string, std::string>>	m_icons;
	std::map<std::string, HJImage::Ptr>	m_images;
	HJSizef		m_size = { 0.f, 0.f };
	bool			m_showProps = false;
	bool			m_autoWrap = true;
	size_t			m_showIdx = 0;
};
//***********************************************************************************//
class HJButton: public HJView
{
public:
    using Ptr = std::shared_ptr<HJButton>;
    
    virtual int init(const std::string& name, HJRunnable click, int id = 0);
    virtual int draw() override;
private:
    int showToolTip(const std::string& tips);
protected:
    int             m_id = 0;
    std::string     m_tips = "";
    HJRunnable     onBtnClick;
    bool            m_autoWrap = true;
};
//***********************************************************************************//
class HJImageView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJImageView>;

	virtual int init(const std::string& imgName);
	virtual int draw() override;

protected:
	std::string		m_imgName = "";
	HJImage::Ptr	m_image = nullptr;
};
//***********************************************************************************//
class HJFrameView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJFrameView>;
	virtual ~HJFrameView();

	virtual int init();
	virtual int draw(const HJMediaFrame::Ptr frame);
	virtual void done();
private:
	int createConverter(const HJVideoInfo::Ptr& vinfo);
	HJMediaFrame::Ptr convert(const HJMediaFrame::Ptr frame);
	uint64_t bind(const HJMediaFrame::Ptr frame);
	void unbind();
protected:
	HJVideoInfo::Ptr		m_info = nullptr;
	HJVSwsConverter::Ptr	m_converter = nullptr;
	uint64_t				m_texID = 0;
	std::string				m_curFrameName = "";
};

//***********************************************************************************//
class HJLoadingView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJLoadingView>;

	virtual int init();
	virtual int draw();
private:
	std::string     m_name = "";
};

//***********************************************************************************//
class HJProgressView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJProgressView>;
	HJProgressView();

	virtual int init();
	bool draw(int64_t* value, int64_t min, int64_t max);
private:
	std::string     m_name = "";
};

//***********************************************************************************//
class HJCombo : public HJView
{
public:
	using Ptr = std::shared_ptr<HJCombo>;
	using HJComboCallback = std::function<void(const int)>;
	HJCombo();
	virtual ~HJCombo();

	virtual int init(const std::vector<std::string> items, int currentIdx, HJComboCallback onClick);
	virtual int draw() override;
private:
	std::vector<std::string>	m_items;
	int							m_itemCurrentIdex = 0;
	char**						m_comboItems = NULL;
	HJComboCallback			m_onClick = nullptr;
};

//***********************************************************************************//
class HJSlider : public HJView
{
public:
	using Ptr = std::shared_ptr<HJSlider>;
	using HJSliderCallback = std::function<void(const float)>;
	HJSlider(const std::string& tags, const float vmin, const float vmax, const float val, HJSliderCallback onClick);
	virtual ~HJSlider();

	virtual int init(const std::string& tags, const float vmin, const float vmax, const float val, HJSliderCallback onClick);
	virtual int draw() override;
private:
	std::string			m_tags = "";
	float				m_vmin = 0.0f;
	float				m_vmax = 1.0f;
	float				m_val = 1.0f;
	HJSliderCallback	m_onClick = nullptr;
};

NS_HJ_END
