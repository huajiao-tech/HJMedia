//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFileDialog.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJImguiUtils.h"
#include "HJJson.h"
#include "ImGuiFileDialog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJ_INSTANCE_IMP(HJFileDialog);
//
HJFileDialog::HJFileDialog()
{
}

HJFileDialog::~HJFileDialog()
{
}

int HJFileDialog::init(const std::string info)
{
	int res = HJ_OK;
	do
	{
		res = HJView::init(info);
		if (HJ_OK != res) {
			break;
		}
		//ImGuiFileDialog dialog;// = std::make_shared<ImGuiFileDialog>();
		//dialog.method_of_your_choice();

	} while (false);

	return res;
}

int HJFileDialog::draw()
{
	

	return HJ_OK;
}

NS_HJ_END