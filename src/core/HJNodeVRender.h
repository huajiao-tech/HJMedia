//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaNode.h"
#include "HJFFDemuxer.h"
#include "HJMediaFrame.h"
#include "HJVRenderBase.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNodeVRender : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeVRender>;
    //
    HJNodeVRender(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeVRender(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeVRender();
    
    virtual int init(const HJVideoInfo::Ptr& info);
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
public:
    virtual int proRun() override;
    virtual void run() override;
    //
    virtual int setWindow(const std::any& window);
private:
    int checkFrame(const HJMediaFrame::Ptr& rawFrame);
    int notifySeekInfo(const HJSeekInfo::Ptr info);
private:
    HJVideoInfo::Ptr           m_info = nullptr;
    HJVRenderBase::Ptr         m_render = nullptr;
    size_t                      m_renderCount = 0;
    HJMediaFrame::Ptr           m_rawFrame = nullptr;
    HJRenderState              m_renderState = HJRender_NONE;
    HJList<HJSeekInfo::Ptr>   m_seekInfos;
    int64_t                     m_waitTime = HJMediaNode::MTASK_DELAY_TIME_DEFAULT; //ms
    HJDataStati64::Ptr         m_clockDeltas = nullptr;
    int64_t                     m_clockDeltaValue = 0;
    std::atomic<bool>           m_preFlushFlag = { false };
};

NS_HJ_END
