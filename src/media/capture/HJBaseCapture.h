//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJBaseCapture : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJBaseCapture>;
    HJBaseCapture() {};
    virtual ~HJBaseCapture();
    
    virtual int init(const HJStreamInfo::Ptr& info);
    virtual int getFrame(HJMediaFrame::Ptr& frame) { return HJ_OK; }
    virtual int run(const HJMediaFrame::Ptr frame) { return HJ_OK; }
    virtual void done() {}
    //
    const HJRunState getState() const {
        return m_runState;
    }
    const bool isEOF() const {
        return (HJRun_Done == m_runState);
    }
    const HJStreamInfo::Ptr& getInfo() const {
        return m_info;
    }
        
    void setCodecID(int codecID) {
        m_codecID = codecID;
    }
    const int getCodecID() const {
        return m_codecID;
    }
    void setCodecName(const std::string& name) {
        m_codecName = name;
    }
    const std::string& getCodecName() {
        return m_codecName;
    }
    void setLowDelay(const bool lowDelay) {
        m_lowDelay = lowDelay;
    }
    const bool getLowDelay() const {
        return m_lowDelay;
    }
public:
    static HJBaseCapture::Ptr createCapture(const HJMediaType mediaType, HJCaptureType type);
    static HJBaseCapture::Ptr createACapture(HJCaptureType type);
    static HJBaseCapture::Ptr createVCapture(HJCaptureType type); 
protected:
    HJRunState             m_runState = HJRun_NONE;
    HJStreamInfo::Ptr      m_info;
    HJRational             m_timeBase = { 0, 1 };
    int                    m_codecID = 0;
    std::string            m_codecName = "";
    //options
    bool                   m_lowDelay = false;
    HJQuality              m_quality = HJQuality_Default;
    int                    m_error = HJ_OK;
};

NS_HJ_END

