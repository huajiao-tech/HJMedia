//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"
#include "HJBSFParser.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJVEncFFMpeg : public HJBaseCodec
{
public:
    using Ptr = std::shared_ptr<HJVEncFFMpeg>;
    HJVEncFFMpeg();
    virtual ~HJVEncFFMpeg();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual int flush() override;
protected:
    static std::string formatX264Params(const std::string preset, int framerate, int bitrate, int bframes = 3, int ref = 3);
    static std::string formatX265Params(const std::string preset, int framerate, int bitrate, int bframes = 3, int ref = 3);
private:
    HJHWFrameCtx::Ptr  m_hwFrameCtx = nullptr;
    HJHND              m_avctx = NULL;
    bool                m_swfFlag = false;
    HJBSFParser::Ptr   m_parser = nullptr;
    //HJBSFParser::Ptr   m_seiParser = nullptr;
};

NS_HJ_END
