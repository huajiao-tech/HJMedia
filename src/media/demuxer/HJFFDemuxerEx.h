//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJFFDemuxer.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJFFDemuxerEx : public HJFFDemuxer
{
public:
    using Ptr = std::shared_ptr<HJFFDemuxerEx>;
    HJFFDemuxerEx();
    HJFFDemuxerEx(const HJMediaUrl::Ptr& mediaUrl);

    virtual int init() override;
    virtual int init(const HJMediaUrl::Ptr& mediaUrl) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
private:
    int checkFrame(const HJMediaFrame::Ptr& frame);
private:
    int64_t     m_gapFrameMax = 10;
};

NS_HJ_END
