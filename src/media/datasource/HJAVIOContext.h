//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJXIOBase.h"

typedef struct AVIOContext AVIOContext;

NS_HJ_BEGIN
class HJDataSource;
//***********************************************************************************//
class HJAVIOContext : public HJXIOInterrupt, public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJAVIOContext);
    HJAVIOContext();
    virtual ~HJAVIOContext();
    
    int open(std::string url, std::string dir, std::string rid = "", int64_t timeout = 3000000);
    int close();

    AVIOContext* getAVIOContext() {
        return m_ioCtx;
    }
    const std::string getUrl() const {
        if(!m_hurl) {
            return "";
        }
        return m_hurl->getUrl();
    }
private:
    static int read_callback(void* opaque, uint8_t* buf, int buf_size);
    static int write_callback(void* opaque, const uint8_t* buf, int buf_size);
    static int64_t seek_callback(void* opaque, int64_t pos, int whence);
private:
    HJUrl::Ptr                      m_hurl{};
    HJBuffer::Ptr                   m_buffer{nullptr};
    AVIOContext*                    m_ioCtx{nullptr};
    std::shared_ptr<HJDataSource>   m_dataSource{nullptr};
};

NS_HJ_END