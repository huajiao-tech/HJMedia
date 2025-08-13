//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJAudioConverter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAudioConverter>;
    HJAudioConverter();
    HJAudioConverter(const HJAudioInfo::Ptr& info);
    virtual ~HJAudioConverter();
    
    HJMediaFrame::Ptr convert(HJMediaFrame::Ptr&& inFrame);
    HJMediaFrame::Ptr channelMap(HJMediaFrame::Ptr&& inFrame, int channels);

    const auto& getDstInfo() {
        return m_dstInfo;
    }
private:
    HJAudioInfo::Ptr       m_dstInfo = nullptr;
    HJAudioInfo::Ptr       m_srcInfo = nullptr;
    HJHND                  m_swr = NULL;
    HJNipMuster::Ptr       m_nipMuster = nullptr;
    //HJBuffer::Ptr          m_buffer = nullptr;
    int64_t                 m_inSamplesCount = 0;
    int64_t                 m_outSamplesCount = 0;
};

class HJAudioAdopter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAudioAdopter>;
    HJAudioAdopter();
    HJAudioAdopter(const HJAudioInfo::Ptr& info);
    virtual ~HJAudioAdopter();
    
    int init();
    void done();
    int addFrame(HJMediaFrame::Ptr&& frame);
    HJMediaFrame::Ptr getFrame();
    HJMediaFrame::Ptr convert(HJMediaFrame::Ptr&& frame);
private:
    static const std::string FILTERS_DESC;

private:
	HJAudioInfo::Ptr                       m_dstInfo = nullptr;
	HJAudioInfo::Ptr                       m_srcInfo = nullptr;
	HJHND                                  m_avGraph = NULL;
	std::unordered_map<std::string, HJHND> m_inFilters;
	std::unordered_map<std::string, HJHND> m_outFilters;
    HJTimeBase                             m_timebase = {1, 44100};
    HJNipMuster::Ptr                       m_nipMuster = nullptr;
};

NS_HJ_END
