//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJView.h"

//namespace IGFD {
//	class ImGuiFileDialog;
//}

NS_HJ_BEGIN
//***********************************************************************************//
class HJFileDialog : public HJView
{
public:
	using Ptr = std::shared_ptr<HJFileDialog>;
	explicit HJFileDialog();
	virtual ~HJFileDialog();

	static HJFileDialog& Instance();
	virtual int init(const std::string info) override;
	virtual int draw() override;
private:

protected:
	//std::shared_ptr<ImGuiFileDialog>	m_dialog;
};

NS_HJ_END