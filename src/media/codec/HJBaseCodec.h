//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

#define HJ_HAVE_CHECK_XTRADATA 1

NS_HJ_BEGIN
//***********************************************************************************//
class HJBaseCodec : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJBaseCodec>;
    HJBaseCodec() {};
    virtual ~HJBaseCodec();
    
    virtual int init(const HJStreamInfo::Ptr& info);
    virtual int getFrame(HJMediaFrame::Ptr& frame) { return HJ_OK; }
    virtual int run(const HJMediaFrame::Ptr frame) { return HJ_OK; }
    virtual int flush() {
        m_runState = HJRun_Ready;
        return HJ_OK;
    }
    virtual bool checkFrame(const HJMediaFrame::Ptr frame) { return true; }
    virtual int reset() { return HJ_OK; }
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
    
    virtual void addOutInfo(const HJStreamInfo::Ptr& info) {
        m_outInfos.emplace(info->getName(), info);
    }
    virtual void removeOutInfo(const std::string& key) {
        m_outInfos.erase(key);
    }
    virtual const HJStreamInfoMap& getOutInfos() const {
        return m_outInfos;
    }
    virtual const HJStreamInfo::Ptr getOutInfo() const {
        if (m_outInfos.size() > 0) {
            return m_outInfos.begin()->second;
        }
        return nullptr;
    }
    
    void setDeviceType(const HJDeviceType devType) {
        m_deviceType = devType;
    }
    const HJDeviceType getDeviceType() const {
        return m_deviceType;
    }
    void setDeviceID(int devID) {
        m_deviceID = devID;
    }
    const int getDeviceID() const {
        return m_deviceID;
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
    void setPopFrontFrame(const bool pop) {
        m_popFrontFrame = pop;
    }
    const bool getPopFrontFrame() const {
        return m_popFrontFrame;
    }

    HJTracker::Ptr getTracker(const std::string key) {
        auto it = m_trackers.find(key);
        if (m_trackers.end() != it) {
            return it->second;
        }
        return nullptr;
    }
    void addTracker(const std::string key, const HJTracker::Ptr& tracker) {
        m_trackers[key] = tracker;
        // to do

    }
    const int getError() {
        return m_error;
    }
public:
    static HJBaseCodec::Ptr createADecoder(int type = HJDEVICE_TYPE_NONE);
    static HJBaseCodec::Ptr createVDecoder(HJDeviceType type = HJDEVICE_TYPE_NONE);
    static HJBaseCodec::Ptr createDecoder(const HJMediaType mediaType, HJDeviceType type = HJDEVICE_TYPE_NONE);
    static HJBaseCodec::Ptr createAEncoder(int type = HJDEVICE_TYPE_NONE);
    static HJBaseCodec::Ptr createVEncoder(HJDeviceType type = HJDEVICE_TYPE_NONE);
    static HJBaseCodec::Ptr createEncoder(const HJMediaType mediaType, HJDeviceType type = HJDEVICE_TYPE_NONE);
protected:
    HJRunState              m_runState = HJRun_NONE;
    HJStreamInfo::Ptr       m_info;
    HJStreamInfoMap         m_outInfos;
    HJDeviceType            m_deviceType = HJDEVICE_TYPE_NONE;
    int                     m_deviceID = 0;
    HJRational              m_timeBase = { 0, 1 };
    int                     m_codecID = 0;
    std::string             m_codecName = "";
    bool                    m_isChanged = false;
    //options
    bool                    m_lowDelay = false;
    HJQuality               m_quality = HJQuality_Default;
    bool                    m_popFrontFrame = false;
    //other
    HJNipMuster::Ptr        m_nipMuster = nullptr;
    HJTrackerMap            m_trackers;
    int                     m_error = HJ_OK;
};
using HJBaseCodecMap = std::unordered_multimap<HJMediaType, HJBaseCodec::Ptr>;

NS_HJ_END

