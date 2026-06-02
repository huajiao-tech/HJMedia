#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

NS_HJ_BEGIN

class HJRteGraphProc;
class HJOGEGLSurface;
class HJFaceuInfo;
class HJFacePointsMadeup;
class HJUIItemFaceuTool : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemFaceuTool);
    
	HJUIItemFaceuTool();
	virtual ~HJUIItemFaceuTool();

	virtual int run() override;
	virtual int renderEveryStart() override;
	virtual int renderEveryEnd() override;

private:
	void priBaseInfo();
	void priCreateFaceu();
	void priAdjustFaceu();
	void priDoneAll();
	void priFaceuOpen(const std::string &i_url);
	void priSave(const std::shared_ptr<HJFaceuInfo>& i_faceuInfo, const std::string& faceuPath, bool i_bUseBackUp);

	GLFWwindow* m_uiWindow = nullptr;
	std::shared_ptr<HJRteGraphProc> m_rteGraphProc = nullptr;
	std::shared_ptr<HJOGEGLSurface> m_surfacePtr = nullptr;
	static std::string s_imageUrl;
	static std::string s_faceuUrl;

	//bool m_bFaceu = false;
	std::weak_ptr<HJFaceuInfo> m_faceuInfoWtr;
	std::shared_ptr<HJFaceuInfo> m_createFaceuInfo = nullptr;
	double m_lastSaveTime = 0.0;
	char m_faceuPath[512]{0};
	bool m_bUseImgSeq = false;
	static std::string s_imageSeq;
	std::shared_ptr< HJFacePointsMadeup> m_facePointsMadeup = nullptr;
	bool m_openErrorPopup = false;
	int m_imgSeqIndex = 0;
	bool m_bUseFakePoints = false;
};
	
NS_HJ_END


