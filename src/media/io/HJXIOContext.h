//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJXIOBase.h"
#include "HJNotify.h"

typedef struct AVIOContext AVIOContext;
typedef struct AVIOInterruptCB AVIOInterruptCB;

NS_HJ_BEGIN
//***********************************************************************************//
class HJXIOContext : public HJXIOInterrupt, public HJXIOBase
{
public:
    using Ptr = std::shared_ptr<HJXIOContext>;
    HJXIOContext();
    virtual ~HJXIOContext();
    
    virtual int open(HJUrl::Ptr url) override;
    virtual int open(AVIOContext* avio);
    virtual void close() override;
    virtual int read(void* buffer, size_t cnt) override;
    virtual int write(const void* buffer, size_t cnt) override;
    virtual int seek(int64_t offset, int whence = 0) override;
    virtual int flush() override;
    virtual int64_t tell() override;
    virtual int64_t size() override;
    virtual bool eof() override;
    //
    virtual int getLine(void* buffer, size_t maxlen);
    virtual int getChompLine(void* buffer, size_t maxlen);
public:
    static const std::map<int, int>     XIO_AVIO_MODE_MAPS;
    static int xioToAvioMode(int mode);
protected:
    AVIOContext*        m_avio;
    size_t              m_size = 0;
};

//***********************************************************************************//
//class HJXIODynBufContext : public HJXIOBase
//{
//public:
//    using Ptr = std::shared_ptr<HJXIOContext>;
//    HJXIOContext();
//    virtual ~HJXIOContext();
//
//    virtual void onNotify(HJListener listener) { m_listener = listener; }
//    virtual void setQuit(const bool isQuit = true) {
//        m_isQuit = isQuit;
//    }
//private:
//    static int interruptCB(void *ctx);
//    static int interruptNetCB(void* ctx, int state);
//protected:
//    AVIOContext*        m_avio = NULL;
//    HJListener         m_listener = nullptr;
//    bool                m_isQuit = false;
//};


NS_HJ_END
