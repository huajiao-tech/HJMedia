//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJARenderBase : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJARenderBase>;
    using ARCallback = std::function<void(const HJMediaFrame::Ptr rawFrame)>;
    HJARenderBase() {};
    virtual ~HJARenderBase() {};
    
    virtual int init(const HJAudioInfo::Ptr& info, ARCallback cb = nullptr);
    virtual int start();
    virtual int pause();
    virtual int resume();
    virtual int reset();
    virtual int stop();
    virtual int write(const HJMediaFrame::Ptr rawFrame);
    //
    virtual void onARCallback(ARCallback cb) {
        m_callback = cb;
    }
    virtual bool isReady() {
        return (HJRun_Ready == m_runState || HJRun_Running == m_runState);
    }
    //virtual void setFlush(const bool flush) {
    //    if (flush) {
    //        if(HJRun_Stop == HJRUNSTATUS(m_runState)){
    //            m_runState = HJRun_Ready;
    //        }
    //        m_runState |= HJRun_FLUSH;
    //    } else {
    //        m_runState &= ~HJRun_FLUSH;
    //    }
    //}
    //virtual bool getFlush() {
    //    return m_runState & HJRun_FLUSH;
    //}

    virtual void setVolume(const float volume) {
        m_volume = volume;
    }
    const float getVolume() const {
        return m_volume;
    }
    virtual void setSpeed(const float speed) {
        m_speed = speed;
    }
    const float getSpeed() const {
        return m_speed;
    }
public:
    static const int AR_BUFFER_MAX = 4;
    //
    static HJARenderBase::Ptr createAudioRender();
protected:
    int                     m_runState = HJRun_NONE;
    HJAudioInfo::Ptr       m_info = nullptr;
    HJAudioInfo::Ptr       m_outInfo = nullptr;
    std::recursive_mutex    m_mutex;
    ARCallback              m_callback = nullptr;
    int                     m_frameCounter = 0;
    float                   m_volume = 1.0f;
    float                   m_speed = 1.0f;
};

NS_HJ_END
