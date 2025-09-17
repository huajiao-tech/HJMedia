//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJView.h"
#include <filesystem>

NS_HJ_BEGIN
//***********************************************************************************//
class HJFileDialogView : public HJView
{
public:
	using Ptr = std::shared_ptr<HJFileDialogView>;
	explicit HJFileDialogView();
	virtual ~HJFileDialogView();

	static HJFileDialogView& Instance();
	virtual int init(const std::string& title, const std::string& filter, bool isMultiselect = true, const std::string& startingDir = "");
	virtual int draw(const std::function<void(const std::vector<std::filesystem::path>&)>& cb);
	//
	static void setGLReady();
public:
	static const std::string KEY_FOLDER;
	static const std::string KEY_MEDIA_FILTER;
	static const std::string KEY_IMAGE_FILTER;
	static const std::string KEY_MUSIC_FILTER;
	static const std::string KEY_CC_FILTER;
protected:
	std::string		m_title = "";
	std::string		m_filter = HJFileDialogView::KEY_MEDIA_FILTER;
	bool			m_isMultiselect = true;
	std::string		m_startingDir = "";
};



NS_HJ_END