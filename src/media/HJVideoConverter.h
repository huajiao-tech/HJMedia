//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJVideoConverterInfo : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJVideoConverterInfo>;
    
protected:
    
};

class HJVideoConverter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJVideoConverter>;
    HJVideoConverter(const HJVideoInfo::Ptr& info);
    virtual ~HJVideoConverter();
    
    int init(const HJVideoInfo::Ptr& info);
    int getFrame(HJMediaFrame::Ptr& frame);
    int addFrame(const HJMediaFrame::Ptr& frame);
    //
    HJMediaFrame::Ptr convert(const HJMediaFrame::Ptr& inFrame);
public:
    static const std::string FILTERS_DESC;
private:
    std::string formatFilterDesc();
private:
    HJHWFrameCtx::Ptr      m_hwFrameCtx = nullptr;
    HJVideoInfo::Ptr       m_outInfo = nullptr;
    HJVideoInfo::Ptr       m_inInfo = nullptr;
    HJHND                  m_avGraph = NULL;
    std::vector<HJHND>     m_inFilters;
    std::vector<HJHND>     m_outFilters;
    
    HJHND                  m_swr = NULL;
    HJBuffer::Ptr          m_buffer = nullptr;
};
using HJVideoConverterList = HJList<HJVideoConverter::Ptr>;

//***********************************************************************************//
class HJVSwsConverter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJVSwsConverter>;
    HJVSwsConverter(const HJVideoInfo::Ptr& outInfo);
    virtual ~HJVSwsConverter();

    int init(const HJVideoInfo::Ptr& inInfo);
    void done();
    HJMediaFrame::Ptr convert(const HJMediaFrame::Ptr& inFrame);
    HJMediaFrame::Ptr convert2(const HJMediaFrame::Ptr& inFrame);
    
    const HJVideoInfo::Ptr& getOutInfo() {
        return m_outInfo;
    }
    const HJVideoInfo::Ptr& getInInfo() {
        return m_inInfo;
    }
    void setCropMode(const HJVCropMode mode) {
        m_vcropMode = mode;
    }
    const HJVCropMode getCropMode() {
        return m_vcropMode;
    }
private:
    AVFrame* scaleFrame(const AVFrame* inAvf);
    AVFrame* rotateFrame(const AVFrame* inAvf);
    AVFrame* cropFrame(const AVFrame* inAvf);
private:
    HJVideoInfo::Ptr       m_outInfo = nullptr;
    HJVideoInfo::Ptr       m_inInfo = nullptr;
    SwsContext*             m_sws = NULL;
    HJVCropMode            m_vcropMode = HJ_VCROP_FIT;
    HJRectf                m_srsRect;
    HJRectf                m_dstRect;
    bool                    m_isRotate = false;
};

NS_HJ_END

