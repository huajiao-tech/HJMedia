//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaNode.h"
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJNodeAEncoder : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeAEncoder>;
    //
    HJNodeAEncoder(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeAEncoder(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeAEncoder();
    
    virtual int init(const HJAudioInfo::Ptr& info);
    virtual void done() override;
    virtual int flush(const HJSeekInfo::Ptr info = nullptr) override;
public:
    virtual int proRun() override;
    virtual void run() override;

    const HJAudioInfo::Ptr getInfo() const {
        return m_info;
    }
private:
    HJAudioInfo::Ptr       m_info;
    HJBaseCodec::Ptr       m_coder;
    HJMediaFrame::Ptr       m_outFrame = nullptr;
};

NS_HJ_END

