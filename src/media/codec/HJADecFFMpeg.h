//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJBaseCodec.h"
#include "HJAudioConverter.h"
#include "HJAudioFifo.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJADecFFMpeg : public HJBaseCodec
{
public:
    using Ptr = std::shared_ptr<HJADecFFMpeg>;
    HJADecFFMpeg();
    virtual ~HJADecFFMpeg();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual int flush() override;
    virtual bool checkFrame(const HJMediaFrame::Ptr frame) override;
    virtual int reset() override;
    virtual void done() override;
private:
    int buildConverters();
protected:
    HJHND                   m_avctx = NULL;
    HJAudioConverter::Ptr   m_converter = nullptr;
    HJAudioAdopter::Ptr     m_adopter = nullptr;
    HJAudioFifo::Ptr        m_fifo = nullptr;
    HJMediaFrame::Ptr       m_storeFrame = nullptr;
};

NS_HJ_END


