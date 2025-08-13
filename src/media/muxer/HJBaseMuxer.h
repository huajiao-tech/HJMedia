//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJTicker.h"
#include "HJXIOBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJBaseCodec;
class HJTrackHolder
{
public:
    using Ptr = std::shared_ptr<HJTrackHolder>;

    void reset() {
        m_lastDTS = HJ_INT64_MIN;
        m_frameCnt = 0;
    }
public:       
    std::shared_ptr<HJBaseCodec>   m_codec = nullptr;
    HJHND					        m_st = NULL;            //AVStream
    int64_t                         m_lastDTS = HJ_INT64_MIN;
    size_t                          m_frameCnt = 0;
};
using HJTrackHolderMap = std::unordered_multimap<HJMediaType, HJTrackHolder::Ptr>;

class HJBaseMuxer : public HJXIOInterrupt, public virtual HJObject
{
public:
    using Ptr = std::shared_ptr<HJBaseMuxer>;
    //
    HJBaseMuxer();
    virtual ~HJBaseMuxer();

    virtual int init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts = nullptr);
    virtual int init(const std::string url, int mediaTypes = HJMEDIA_TYPE_AV, HJOptions::Ptr opts = nullptr);
    virtual int addFrame(const HJMediaFrame::Ptr& frame) { return  HJ_OK; }
    virtual int writeFrame(const HJMediaFrame::Ptr& frame) { return  HJ_OK; }
    virtual void done();
    //
    void setMediaInfo(const HJMediaInfo::Ptr mediaInfo) {
        m_mediaInfo = mediaInfo;
    }
    const HJMediaInfo::Ptr& getMediaInfo() {
        return m_mediaInfo;
    }
    void setIsLive(const bool isLive) {
        m_isLive = isLive;
    }
    bool getIsLive() {
        return m_isLive;
    }
    const size_t& getTotalFrameCnt() {
        return m_totalFrameCnt;
    }
    void setTimestampZero(bool isZero ) {
        m_timestampZero = isZero;
    }
    bool getTimestampZero() const {
        return m_timestampZero;
    }
public:
    static const std::string KEY_KEY_FRAME_CHECK;
protected:
    HJMediaInfo::Ptr        m_mediaInfo{nullptr};
    HJOptions::Ptr          m_opts{nullptr};
    std::string             m_url{""};
    int                     m_mediaTypes = HJMEDIA_TYPE_NONE;
    HJNipInterval           m_nip;
    bool                    m_isLive = false;
    size_t                  m_totalFrameCnt = 0;
    bool                    m_keyFrameCheck = true;
    bool                    m_timestampZero = false;
};

NS_HJ_END
