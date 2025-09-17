//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"
#include "HJVideoConverter.h"
#include "HJH2645Parser.h"

typedef struct AVCodecContext AVCodecContext;

NS_HJ_BEGIN
//***********************************************************************************//
class HJVDecFFMpeg : public HJBaseCodec
{
public:
    using Ptr = std::shared_ptr<HJVDecFFMpeg>;
    HJVDecFFMpeg();
    virtual ~HJVDecFFMpeg();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual int flush() override;
    virtual bool checkFrame(const HJMediaFrame::Ptr frame) override;
    virtual int reset() override;
    virtual void done() override;
    //
    void setSwfFlag(const bool flag) {
        m_swfFlag = flag;
    }
    const bool getSwfFlag() const {
        return m_swfFlag;
    }
    
    int buildConverters();
protected:
    HJHND                   m_avctx = NULL;
    int                     m_hwPixFmt;
    bool                    m_swfFlag = false;
    HJVideoConverterList    m_videoConverters;
    HJH2645Parser::Ptr      m_parser = nullptr;
    HJMediaFrame::Ptr       m_storeFrame = nullptr;
private:
    bool                    m_bMediacodecDec = false;
    HJHND                   m_MediaCodecContext = NULL;
};

//***********************************************************************************//
class HJVDecFFMpegPlus : public HJVDecFFMpeg
{
public:
    HJ_DECLARE_PUWTR(HJVDecFFMpegPlus);
    HJVDecFFMpegPlus();

    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
private:
    HJMediaFrame::Ptr   m_firstFrame{ nullptr };
    bool                m_checkFirstFrameFlag{ true };
    int64_t             m_firstFrameDTS{ HJ_NOTS_VALUE };
};

NS_HJ_END

