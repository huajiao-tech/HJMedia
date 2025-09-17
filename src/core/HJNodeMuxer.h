//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaNode.h"
#include "HJFFMuxer.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNodeMuxer : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeMuxer>;
    //
    HJNodeMuxer(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeMuxer(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeMuxer();
    
    virtual int init(const HJMediaInfo::Ptr& mediaInfo);
    virtual void done() override;
    virtual void run() override;
public:
    virtual int proRun() override;

    const HJMediaInfo::Ptr getMediaInfo() {
        return m_mediaInfo;
    }
    HJMediaFrame::Ptr getPropertyFrame();
private:
    HJMediaInfo::Ptr        m_mediaInfo = nullptr;
    HJBaseMuxer::Ptr       m_muxer = nullptr;
    int                     m_vfCnt = 0;
    int                     m_afCnt = 0;
};
using HJNodeMuxerVector = std::vector<HJNodeMuxer::Ptr>;

NS_HJ_END


