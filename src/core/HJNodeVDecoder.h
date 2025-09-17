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
class HJNodeVDecoder : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeVDecoder>;
    //
    HJNodeVDecoder(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeVDecoder(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeVDecoder();
    
    virtual int init(const HJVideoInfo::Ptr& info, const HJDeviceType devType = HJDEVICE_TYPE_NONE);
    virtual void done() override;
    virtual int flush(const HJSeekInfo::Ptr info = nullptr) override;
public:
    virtual int proRun() override;
    virtual void run() override;
private:
    HJVideoInfo::Ptr       m_info;
    HJBaseCodec::Ptr       m_dec;
    HJMediaFrame::Ptr       m_outFrame = nullptr;
};

NS_HJ_END
