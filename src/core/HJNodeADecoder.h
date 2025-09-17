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
class HJNodeADecoder : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeADecoder>;
    //
    HJNodeADecoder(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeADecoder(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeADecoder();
    
    virtual int init(const HJAudioInfo::Ptr& info);
    virtual void done() override;
    virtual int flush(const HJSeekInfo::Ptr info = nullptr) override;
public:
    virtual int proRun() override;
    virtual void run() override;
private:
    HJAudioInfo::Ptr       m_info;
    HJBaseCodec::Ptr       m_dec;
    HJMediaFrame::Ptr       m_outFrame = nullptr;
};

NS_HJ_END


