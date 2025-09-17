//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJNoticeCenter.h"
#include "HJExecutor.h"
#include "HJMediaUtils.h"
#include "HJTimelineHandler.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNoticeHandlerEx : public HJNoticeHandler
{
public:
    using Ptr = std::shared_ptr<HJNoticeHandlerEx>;

    template<typename FUNC>
    void addMediaFrameNotify(FUNC&& func) {
        addListener(HJBroadcast::EVENT_PLAYER_MEDIAFRAME, std::forward<FUNC>(func));
    }
    void delMediaFrameNotify() {
        delListener(HJBroadcast::EVENT_PLAYER_MEDIAFRAME);
    }
    int notifyMediaFrame(const HJMediaFrame::Ptr mavf) {
        return emitEvent(HJBroadcast::EVENT_PLAYER_MEDIAFRAME, mavf);;
    }
    bool hasMediaFrameNotify() {
        return hasListener(HJBroadcast::EVENT_PLAYER_MEDIAFRAME);
    }

    template<typename FUNC>
    void addSourceFrameNotify(FUNC&& func) {
        addListener(HJBroadcast::EVENT_PLAYER_SOURCEFRAME, std::forward<FUNC>(func));
    }
    void delSourceFrameNotify() {
        delListener(HJBroadcast::EVENT_PLAYER_SOURCEFRAME);
    }
    int notifySourceFrame(const HJMediaFrame::Ptr mavf) {
        return emitEvent(HJBroadcast::EVENT_PLAYER_SOURCEFRAME, mavf);;
    }
    bool hasSourceFrameNotify() {
        return hasListener(HJBroadcast::EVENT_PLAYER_SOURCEFRAME);
    }
};

class HJEnvironment : public HJNoticeHandlerEx
{
public:
	using Ptr = std::shared_ptr<HJEnvironment>;
	HJEnvironment();
	virtual ~HJEnvironment();
    //
	HJHWDevice::Ptr getHWDevice(HJDeviceType devType, int devID);
    
    HJExecutor::Ptr getExecutor(const std::string key);

    void setDevAttri(const HJDeviceType devType, int devID = 0) {
        m_devAttri.m_devType = devType;
        m_devAttri.m_devID = devID;
    }
    const HJDeviceAttribute& getDevAttri() const {
        return m_devAttri;
    }

    void setLowDelay(const bool lowDelay) {
        m_lowDelay = lowDelay;
    }
    const bool getLowDelay() const {
        return m_lowDelay;
    }
    const HJTimelineHandler::Ptr& getTimeline() {
        return m_timeline;
    }
    void setVDecThreads(const int threads) {
        m_vdecThreads = threads;
    }
    const int getVDecThreads() {
        return m_vdecThreads;
    }
    void setSpeed(const float speed) {
        m_speed = speed;
    }
    const float getSpeed() const {
        return m_speed;
    }
    void setVolume(const float volume) {
        m_volume = volume;
    }
    const float getVolume() const {
        return m_volume;
    }
    void setEnableMediaTypes(const int mediaTypes) {
        m_enableMediaTypes = mediaTypes;
    }
    const int getEnableMediaTypes() {
        return m_enableMediaTypes;
    }
protected:
	HJHWDeviceMap				m_hwDevices;
    HJExecutorMap               m_executors;
    HJDeviceAttribute           m_devAttri;
    bool                        m_lowDelay = true;
    HJTimelineHandler::Ptr      m_timeline = nullptr;
    int                         m_vdecThreads = 4;
    std::atomic<float>          m_speed = { 1.0f };
    std::atomic<float>          m_volume{1.0f};
    int                         m_enableMediaTypes{ HJMEDIA_TYPE_NONE };
};
NS_HJ_END
