//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJView.h"
#include <filesystem>
#include <functional>
#include <vector>

NS_HJ_BEGIN
//***********************************************************************************//
class HJFileDialog : public HJView
{
public:
	using Ptr = std::shared_ptr<HJFileDialog>;
	using HJFileDialogCallback = std::function<void(const std::vector<std::filesystem::path>&)>;
	explicit HJFileDialog();
	virtual ~HJFileDialog();

	static HJFileDialog& Instance();
	virtual int init(const std::string info) override;
	int open(const std::string& dialog_id, const std::string& title, const std::string& filter,
		bool is_multiselect = true, const std::string& starting_dir = "",
		HJFileDialogCallback on_complete = nullptr);
	void setOnComplete(HJFileDialogCallback on_complete);
	void close();
	virtual int draw() override;
	const std::vector<std::filesystem::path>& getResults() const {
		return m_results;
	}
private:
	std::string m_dialog_id = "";
	std::string m_title = "";
	std::string m_filter = "";
	std::string m_starting_dir = "";
	bool m_is_multiselect = true;
	HJFileDialogCallback m_on_complete = nullptr;
	std::vector<std::filesystem::path> m_results;

protected:
};

NS_HJ_END
