//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJAEncFFMpeg : public HJBaseCodec
{
public:
    using Ptr = std::shared_ptr<HJAEncFFMpeg>;
    HJAEncFFMpeg();
    virtual ~HJAEncFFMpeg();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual int flush() override;
private:
    HJHND                  m_avctx = NULL;
    int                     m_frameCounter = 0;
};

NS_HJ_END
