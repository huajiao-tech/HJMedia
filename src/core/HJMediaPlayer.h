//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJPlayerBase.h"
#include "HJNotify.h"
#include "HJMediaInfo.h"
#include "HJGraphDec.h"
#include "HJNodeVRender.h"
#include "HJNodeARender.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJSeekLimiter : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJSeekLimiter>;
    HJSeekLimiter() = default;
    virtual ~HJSeekLimiter() = default;
    
    HJSeekInfo::Ptr checkInfo(const HJSeekInfo::Ptr info) {
        HJ_AUTO_LOCK(m_mutex);
        if (!m_seekInfo || ( ((info->getTime() - m_seekInfo->getTime()) >= SEEK_INTERVAL_MAX) && (m_seekInfo->getPos() != info->getPos()) )) {
            //HJLogi("pos:" + HJ2STR(info->getPos()));
            m_seekInfo = info;
            return info;
        }
        return nullptr;
    }
    void remove(const HJSeekInfo::Ptr info)
    {
        HJ_AUTO_LOCK(m_mutex);
        if (m_seekInfo == info) {
            m_seekInfo = nullptr;
        }
    }
public:
    static const int64_t SEEK_INTERVAL_MAX;
private:
//    std::map<size_t, HJSeekInfo::Ptr>  m_infos;
    HJSeekInfo::Ptr                    m_seekInfo;
    std::recursive_mutex                m_mutex;
};

class HJMediaPlayer : public HJPlayerBase
{
public:
    using Ptr = std::shared_ptr<HJMediaPlayer>;
    //
    HJMediaPlayer(const HJScheduler::Ptr& scheduler = nullptr);
    virtual ~HJMediaPlayer();
    
    virtual int init(const HJMediaUrl::Ptr& mediaUrl, const HJListener& listener = nullptr);
    virtual int init(const HJMediaUrlVector& mediaUrls, const HJListener& listener = nullptr);
    
    virtual int start() override;
    virtual int play() override;
    virtual int pause() override;
    //virtual int resume() override;
    //
    virtual int setWindow(const std::any& window) override;
    virtual int seek(int64_t pos) override;
    virtual int seek(float progress) override;
    virtual int speed(float speed) override;
    virtual int64_t getCurrentPos() override;
    virtual float getProgress() override;
    virtual void setVolume(const float volume) override;
    //
    virtual bool isReady();
    virtual bool isPause() { return m_isPause; }
    
    void setMediaFrameListener(const HJMediaFrameListener listener);
    void setSourceFrameListener(const HJMediaFrameListener listener);
protected:
    virtual void done() override;
    //
    int notify(const HJNotification::Ptr ntf) {
        if(m_listener) {
            return m_listener(ntf);
        }
        return HJ_OK;
    }
    
    int notifyMediaFrame(const HJMediaFrame::Ptr mavf) {
        if (m_mavfListener) {
            return m_mavfListener(mavf);
        }
        return HJ_OK;
    }
    int notifySourceFrame(const HJMediaFrame::Ptr mavf) {
        if (m_savfListener) {
            return m_savfListener(mavf);
        }
        return HJ_OK;
    }
    static const std::string PLAYER_KEY_SEEK;
private:
    std::atomic<HJRunState>    m_runState = { HJRun_NONE };
    bool                        m_isPause = false;
    HJListener                 m_listener = nullptr;
    HJMediaFrameListener        m_mavfListener = nullptr;
    HJMediaFrameListener        m_savfListener = nullptr;
    HJGraphDec::Ptr            m_graphDec = nullptr;
    HJNodeVRender::Ptr         m_videoRender = nullptr;
    HJNodeARender::Ptr         m_audioRender = nullptr;
    HJSeekLimiter::Ptr         m_seekLimiter = nullptr;
    HJExecutor::Ptr            m_renderExecutor = nullptr;
};

NS_HJ_END
