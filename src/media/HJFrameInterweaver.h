//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJTrackInfo.h"

NS_HJ_BEGIN
#define HJ_INTERWEAVE_CACHE_MAX        30
#define HJ_INTERWEAVE_DELTA_MAX        3000    //ms
//***********************************************************************************//
class HJFrameInterweaver : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJFrameInterweaver>;
    HJFrameInterweaver();
    virtual ~HJFrameInterweaver();
    
    int init(const HJMediaInfo::Ptr& mediaInfo);
    int addFrame(const HJMediaFrame::Ptr& frame);
    HJMediaFrame::Ptr getFrame();
protected:
    virtual void removeTrackCache(const std::string& key) {
        auto it = m_trackCaches.find(key);
        if (it != m_trackCaches.end()) {
            m_trackCaches.erase(it);
        }
    }
    virtual HJMediaFrameCache::Ptr getTrackCache(const std::string& key = "") {
        if(key.empty()) {
            return nullptr;
        }
        auto it = m_trackCaches.find(key);
        if (it != m_trackCaches.end()) {
            return it->second;
        }
        return nullptr;
    }
    HJMediaFrameCache::Ptr checkTracks();
    int checkFrame(const HJMediaFrame::Ptr& frame, HJMediaFrameCache::Ptr trackCache);
private:
    HJMediaInfo::Ptr        m_mediaInfo;
    HJMediaFrameCachesMap   m_trackCaches;
    size_t                  m_inFrameNum = 0;
//    bool                    m_isWaitALL = true;
//    bool                    m_isAllReady = false;
};

//***********************************************************************************//
class HJAVInterweaver : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAVInterweaver>;
    HJAVInterweaver();
    virtual ~HJAVInterweaver();

    int init(const HJMediaInfo::Ptr& mediaInfo);
    void done();
    int addFrame(const HJMediaFrame::Ptr& frame);
    HJMediaFrame::Ptr getFrame();
protected:
    int checkFrame(const HJMediaFrame::Ptr& frame);
    const HJMediaFrame::Ptr popFrame();
    const HJMediaFrame::Ptr frontFrame();
protected:
    HJMediaInfo::Ptr        m_mediaInfo = nullptr;
    HJMediaFrameMap         m_frames;
    HJTrackInfoHolder       m_trackHolder;
    bool                    m_eof = false;
};

NS_HJ_END
