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
#include "HJView.h"

struct GLFWwindow;

NS_HJ_BEGIN
//***********************************************************************************//
class HJWindow : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJWindow>;
	explicit HJWindow();
	virtual ~HJWindow();

	virtual int init(const HJSizei wsize = { 1280, 720 });
	virtual void done();
	virtual int draw();
	bool isClose();

	void setBGColor(const HJColor color) {
		m_bgColor = color;
	}
	void makeCurrent();
	void renderClear();
	void renderPresent();
	//
	void setWindowSize(const HJSizei size);
	const HJRecti getWindowRect();
	//
	void setIcon(const std::string& iconName);

	HJHND getHandle();
	void setInitBackends(const bool init) {
		m_initBackends = init;
	}
public:
	static const HJSizei gettPrimaryMonitorSize();
	static HJWindow* getWSelf(GLFWwindow* window);
private:
	static void onFramebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void onWindowRefreshCallback(GLFWwindow* window);
	static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void onCursorPositionCallback(GLFWwindow* window, double x, double y);
	//
	static bool changeTheme(GLFWwindow* window, const bool isDark);
private:
	HJSizei				m_wsize = { 1920, 1080 };
	GLFWwindow*				m_window = NULL;
	HJColor				m_bgColor = { 0.45f, 0.55f, 0.60f, 1.0f };
	HJRightClickMenu::Ptr	m_rightClickMenu = nullptr;
	bool					m_initBackends = true;
};

NS_HJ_END