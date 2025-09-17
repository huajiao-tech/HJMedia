//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJFFMuxer.h"
#include "HJFrameInterweaver.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJInterleavedMuxer : public HJBaseMuxer
{
public:
    using Ptr = std::shared_ptr<HJInterleavedMuxer>;
    HJInterleavedMuxer();
    virtual ~HJInterleavedMuxer();
    
    virtual int init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts = nullptr) override;
    virtual int init(const std::string url, int mediaTypes = HJMEDIA_TYPE_AV, HJOptions::Ptr opts = nullptr) override;
    virtual int addFrame(const HJMediaFrame::Ptr& frame) override;
    virtual int writeFrame(const HJMediaFrame::Ptr& frame) override;
    virtual void done() override;
protected:
    HJBaseMuxer::Ptr        m_muxer = nullptr;
    HJAVInterweaver::Ptr    m_interweaver = nullptr;
};

NS_HJ_END
