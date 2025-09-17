//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJFFDemuxerEx.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJComplexDemuxer : public HJBaseDemuxer
{
public:
    using Ptr = std::shared_ptr<HJComplexDemuxer>;
    HJComplexDemuxer();
    virtual ~HJComplexDemuxer();
    
    int init(const HJMediaUrlVector& mediaUrls) override;
    virtual int seek(int64_t pos) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual void reset() override;

    virtual int64_t getDuration() override;
private:
    HJBaseDemuxer::Ptr getNextSource();
    void checkUrls();
protected:
    HJMediaUrlVector        m_mediaUrls;
    HJBaseDemuxerList       m_sources;
    HJBaseDemuxer::Ptr      m_curSource = nullptr;
    int64_t                 m_segOffset = 0;
};

NS_HJ_END
