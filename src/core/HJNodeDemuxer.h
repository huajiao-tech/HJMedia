//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaNode.h"
#include "HJComplexDemuxer.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNodeDemuxer : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeDemuxer>;
    //
    HJNodeDemuxer(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeDemuxer(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeDemuxer();
    
    virtual int init(const HJMediaUrl::Ptr& mediaUrl);
    virtual int init(const HJMediaUrlVector& mediaUrls);
    virtual void done() override;
    int seek(const HJSeekInfo::Ptr info);
private:
    virtual int proRun() override;
    virtual void run() override;
    virtual int deliver(HJMediaFrame::Ptr mediaFrame, const std::string& key = "") override;
//    virtual int onMessage(const HJMessage::Ptr msg) override;
//    int onSeek(const HJSeekInfo::Ptr info);
public:
    const HJMediaInfo::Ptr getMediaInfo() {
        if (m_source) {
            return m_source->getMediaInfo();
        }
        return nullptr;
    }
    const HJMediaInfo::Ptr getBestMediaInfo() {
        if(m_source) {
            const auto& minfo = m_source->getMediaInfo();
            auto subMInfo = minfo->getDefaultSubMInfo();
            return (nullptr != subMInfo) ? subMInfo : minfo;
        }
        return nullptr;
    }
    virtual int notifySourceFrame(const HJMediaFrame::Ptr mavf) {
        if (!mavf) {
            return HJ_OK;
        }
        if (m_env) {
            return m_env->notifySourceFrame(mavf);
        }
        return HJ_OK;
    }
    //
    static const std::string DEMUXER_KEY_SEEK;
private:
    HJBaseDemuxer::Ptr                          m_source;
    HJMediaFrame::Ptr                           m_esFrame;
    HJDemuxerState                             m_demuxerState = HJDemuxer_NONE;
};
using HJNodeDemuxerVector = std::vector<HJNodeDemuxer::Ptr>;

NS_HJ_END

