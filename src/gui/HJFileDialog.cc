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
#include "ImFileDialog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJ_INSTANCE_IMP(HJFileDialog);
//
HJFileDialog::HJFileDialog()
{
	m_name = HJMakeGlobalName("filedialog");
	m_dialog_id = m_name;
	m_isOpen = false;
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
		if (m_isOpen) {
			close();
		}
		m_dialog_id = info.empty() ? m_name : info;
		m_title.clear();
		m_filter.clear();
		m_starting_dir.clear();
		m_is_multiselect = true;
		m_on_complete = nullptr;
		m_results.clear();
		m_isOpen = false;

	} while (false);

	return res;
}

int HJFileDialog::open(const std::string& dialog_id, const std::string& title, const std::string& filter,
	bool is_multiselect, const std::string& starting_dir, HJFileDialogCallback on_complete)
{
	int res = HJ_OK;
	do
	{
		if (title.empty()) {
			res = HJErrInvalidParams;
			break;
		}
		if (m_isOpen) {
			close();
		}
		m_dialog_id = dialog_id.empty() ? m_name : dialog_id;
		m_title = title;
		m_filter = filter;
		m_starting_dir = starting_dir;
		m_is_multiselect = is_multiselect;
		m_on_complete = on_complete;
		m_results.clear();
		ifd::FileDialog::Instance().Open(m_dialog_id, m_title, m_filter, m_is_multiselect, m_starting_dir);
		m_isOpen = true;
	} while (false);

	return res;
}

void HJFileDialog::setOnComplete(HJFileDialogCallback on_complete)
{
	m_on_complete = on_complete;
}

void HJFileDialog::close()
{
	ifd::FileDialog::Instance().Close();
	m_isOpen = false;
}

int HJFileDialog::draw()
{
	if (!m_isOpen || m_dialog_id.empty()) {
		return HJ_OK;
	}
	if (!ifd::FileDialog::Instance().IsDone(m_dialog_id)) {
		return HJ_OK;
	}
	m_results.clear();
	if (ifd::FileDialog::Instance().HasResult()) {
		m_results = ifd::FileDialog::Instance().GetResults();
		if (m_on_complete) {
			m_on_complete(m_results);
		}
	}
	close();

	return HJ_OK;
}

NS_HJ_END
