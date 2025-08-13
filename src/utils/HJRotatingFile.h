#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJRotatingFile : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJRotatingFile>;
	HJRotatingFile(const std::string& filename, size_t maxFileSize = 10 * 1024 * 1024, size_t maxFiles = 2);
	virtual ~HJRotatingFile();

	virtual int write(const std::string& msg) = 0;
	virtual void flush() = 0;

	static HJRotatingFile::Ptr create(const std::string& filename, size_t maxFileSize = 10 * 1024 * 1024, size_t maxFiles = 2);
protected:
	std::string m_filename{""};
	size_t		m_maxFileSize{ 10 * 1024 * 1024 };
	size_t		m_maxFiles{ 2 };
};
//***********************************************************************************//
NS_HJ_END