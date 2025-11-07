//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <iostream>
#include <fstream>
#include "HJXIOBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
//using HJFMode = std::ios::openmode;
using HJFStream = std::fstream;

typedef enum HJXFMode
{
	HJXF_MODE_CREATE = std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc, // | std::ios::_Noreplace,
	HJXF_MODE_RONLY = std::ios::binary | std::ios::in,
	HJXF_MODE_WONLY = std::ios::binary | std::ios::out,						// | std::ios::trunc, // | std::ios::_Noreplace,
	HJXF_MODE_RW = std::ios::binary | std::ios::in | std::ios::out			// | std::ios::app,
} HJXFMode;

class HJXIOFile : public HJXIOBase
{
public:
	using Ptr = std::shared_ptr<HJXIOFile>;
	HJXIOFile();
	virtual ~HJXIOFile();

	virtual int open(HJUrl::Ptr url) override;
	virtual void close() override;
	virtual int read(void* buffer, size_t cnt) override;
	virtual int write(const void* buffer, size_t cnt) override;
	virtual int seek(int64_t offset, int whence = std::ios::cur) override;
	virtual int flush() override;
	virtual int64_t tell() override;
	virtual int64_t size() override;
    virtual bool eof() override;
public:
    int checkState();
	static const std::map<int, int> XIO_FILE_MODE_MAPS;
	static int xioToFMode(int mode);
private:
	std::unique_ptr<HJFStream> m_file = nullptr;
	size_t						m_size = 0;
};

NS_HJ_END
