//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJBaseDemuxer.h"
#include "HJXIOContext.h"

typedef struct URLContext URLContext;

NS_HJ_BEGIN
//***********************************************************************************//
class HJHLSParser : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJHLSParser>;
    virtual ~HJHLSParser();

    virtual int init(const HJMediaUrl::Ptr& mediaUrl);

    const auto& getHLSMediaUrls() {
        return m_hlsMediaUrls;
    }

    void reset();
private:
    void writeHLSMediaUrls(int index, HJBuffer::Ptr buffer);
private:
    HJMediaUrl::Ptr                 m_mediaUrl{ nullptr };
    HJXIOContext::Ptr               m_xio{ nullptr };
    std::vector<HJMediaUrl::Ptr>    m_hlsMediaUrls;
};

NS_HJ_END
