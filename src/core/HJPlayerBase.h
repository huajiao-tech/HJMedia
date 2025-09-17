//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJLog.h"
#include "HJMediaInfo.h"
#include "HJMediaNode.h"
#include "HJExecutor.h"
#include "HJNoticeCenter.h"
#include "HJEnvironment.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJPlayerBase : public HJMediaInterface, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJPlayerBase>;
    using WPtr = std::weak_ptr<HJPlayerBase>;
    //
    HJPlayerBase(const HJScheduler::Ptr& scheduler = nullptr);
    virtual ~HJPlayerBase();
    
    virtual int init(const HJMediaUrl::Ptr& mediaUrl);
    virtual int init(const HJMediaUrlVector& mediaUrls);
    virtual void done() {}
    virtual int start();
    virtual int play();
    virtual int pause();
    virtual int resume();
    //
    virtual int setWindow(const std::any& window);
    virtual int seek(int64_t pos);
    virtual int seek(float progress);
    virtual int speed(float speed);
    virtual int64_t getCurrentPos();
    virtual float getProgress();
    virtual void setVolume(const float volume);
public:
    const HJScheduler::Ptr& getScheduler() const {
        return m_scheduler;
    }
protected:
    HJScheduler::Ptr       m_scheduler = nullptr;
    HJEnvironment::Ptr     m_env = nullptr;
    HJMediaUrl::Ptr         m_mediaUrl = nullptr;
    HJMediaUrlVector        m_mediaUrls;
    HJMediaInfo::Ptr        m_minfo = nullptr;
    HJProgressInfo::Ptr    m_progInfo = nullptr;
    std::recursive_mutex    m_mutex;
    //int64_t                 m_curPos = 0;
    //int64_t                 m_seekPos = 0;
    //float                   m_progress = 0.0f;
    float                   m_speed = 1.0;
};

NS_HJ_END

