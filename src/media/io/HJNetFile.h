//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJXIOBase.h"

typedef struct URLContext URLContext;

NS_HJ_BEGIN
//***********************************************************************************//
class HJNetFile : public HJXIOInterrupt, public HJXIOBase
{
public:
	HJ_DECLARE_PUWTR(HJNetFile);
	HJNetFile();
	virtual ~HJNetFile();

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
    URLContext*			m_inner {nullptr};
    std::atomic<bool> 	m_eof = { false };
    int64_t             m_pos{0};
};

NS_HJ_END