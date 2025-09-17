//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaNode.h"
#include "HJFFDemuxer.h"
#include "HJMediaFrame.h"
#include "HJARenderBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNodeARender : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeARender>;
    //
    HJNodeARender(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeARender(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeARender();
    
    virtual int init(const HJAudioInfo::Ptr& info);
    virtual void done() override;
    virtual int play() override;
    virtual int pause() override;
    //virtual int resume() override;
    virtual int flush(const HJSeekInfo::Ptr info = nullptr) override;
    
    void setRenderState(const HJRenderState& state) {
        m_renderState = state;
    }
    const HJRenderState& getRenderState() const {
        return m_renderState;
    }
    virtual void setPreFlush(const bool flag);
private:
    virtual int proRun() override;
    virtual void run() override;
    int notifySeekInfo(const HJSeekInfo::Ptr info);
private:
    HJAudioInfo::Ptr           m_info;
    HJARenderBase::Ptr         m_render;
    size_t                      m_renderCount = 0;
    HJMediaFrame::Ptr           m_rawFrame = nullptr;
    HJRenderState              m_renderState = HJRender_NONE;
    //HJList<HJSeekInfo::Ptr>   m_seekInfos;
    int64_t                     m_waitTime = HJMediaNode::MTASK_DELAY_TIME_DEFAULT; //ms
    std::atomic<bool>           m_preFlushFlag = { false };
    int64_t                     m_startElapse = HJ_NOTS_VALUE;
};

NS_HJ_END

