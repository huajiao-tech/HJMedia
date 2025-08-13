//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJBuffer.h"

typedef struct AVIOContext AVIOContext;
typedef struct AVIOInterruptCB AVIOInterruptCB;

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJXIOMode {
    HJ_XIO_NONE    = 0,
    HJ_XIO_READ    = 1,
    HJ_XIO_WRITE   = 2,
    HJ_XIO_RW      = HJ_XIO_READ | HJ_XIO_WRITE,
} HJXIOMode;

typedef enum HJSeekDir {
    HJ_SEEK_BEG = 0,               //std::ios::beg
    HJ_SEEK_CUR = 1,               //std::ios::cur
    HJ_SEEK_END = 2,               //std::ios::end
    HJ_SEEK_SIZE = 0x10000,
    HJ_SEEK_FORCE = 0x20000,
} HJSeekDir;

class HJUrl : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJUrl>;
	HJUrl(const std::string& url = "") 
		: m_url(HJUtilitys::AnsiToUtf8(url)) {
	}
	HJUrl(const std::string& url, const int mode)
		: m_url(HJUtilitys::AnsiToUtf8(url))
		, m_mode(mode) {
	}
	virtual ~HJUrl() { };

	virtual void operator=(const HJUrl::Ptr& other) {
		m_url = other->m_url;
		m_mode = other->m_mode;
	}
	virtual HJUrl::Ptr dup() {
		HJUrl::Ptr url = std::make_shared<HJUrl>();
		url = sharedFrom(this);
		return url;
	}

	const std::string& getUrl() const {
		return m_url;
	}
	void setUrl(const std::string& url) {
		m_url = url;
	}
	const int getMode() const {
		return m_mode;
	}
	void setMode(const int mode) {
		m_mode = mode;
	}
protected:
	std::string     m_url = "";
	int				m_mode = HJ_XIO_READ;
};

//***********************************************************************************//
class HJXIOBase : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJXIOBase>;
	HJXIOBase();
	virtual ~HJXIOBase();

	virtual int open(HJUrl::Ptr url) {
		m_url = url;
		return HJ_OK;
	};
	virtual void close() = 0;
	virtual int read(void* buffer, size_t cnt) = 0;
	virtual int write(const void* buffer, size_t cnt) = 0;
	virtual int seek(int64_t offset, int whence = 0) = 0;
	virtual int flush() = 0;
	virtual int64_t tell() = 0;
	virtual int64_t size() = 0;
    virtual bool eof() { return false; }
    //
    std::string getUrl() {
        if (m_url) {
            return m_url->getUrl();
        }
        return "";
    }
    virtual HJBuffer::Ptr readBlock(size_t cnt) {
        HJBuffer::Ptr block = std::make_shared<HJBuffer>(cnt);
        int recnt = read(block->data(), cnt);
        if (recnt <= 0) {
            return nullptr;
        }
        block->setSize(recnt);
        
        return block;
    }
    virtual int writeBlock(const HJBuffer::Ptr& block) {
        if(!block) {
            return HJErrInvalidParams;
        }
        return write(block->data(), block->size());
    }
protected:
	HJUrl::Ptr		m_url;
};

//***********************************************************************************//
class HJXIOInterrupt
{
public:
    using Ptr = std::shared_ptr<HJXIOInterrupt>;
    HJXIOInterrupt();
    virtual ~HJXIOInterrupt();
    
    virtual void onNotify(HJListener listener) { m_listener = listener; }
    virtual void setQuit(const bool isQuit = true) {
        m_isQuit = isQuit;
    }
    virtual bool getIsQuit() {
        return m_isQuit;
    }
    virtual int onInterruptNetNotify(const HJNotification::Ptr ntfy);
private:
    static int onInterruptCB(void *ctx);
    static int onInterruptNetCB(void* ctx, int state);
protected:
    AVIOInterruptCB*    m_interruptCB;
    HJListener         m_listener = nullptr;
    std::atomic<bool>   m_isQuit = {false};
};

NS_HJ_END
