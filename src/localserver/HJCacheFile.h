//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJXIOBase.h"
#include "HJBlockFile.h"
#include "HJBlockManager.h"
#include "HJDownloader.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJCacheFile : public HJXIOBase
{
public:
	HJ_DECLARE_PUWTR(HJCacheFile);

    HJCacheFile();
    virtual ~HJCacheFile() noexcept;

	virtual int open(HJUrl::Ptr url) override;
	virtual void close() override;
	virtual int read(void* buffer, size_t cnt) override;
	virtual int write(const void* buffer, size_t cnt) override;
	virtual int seek(int64_t offset, int whence = std::ios::cur) override;
	virtual int flush() override;
	virtual int64_t tell() override;
	virtual int64_t size() override;
    virtual bool eof() override;
private:
    void getOptions();
private:
    std::string m_rid{""};
    std::string m_cacheDir{""};
    int m_preCacheSize{0};
};

NS_HJ_END