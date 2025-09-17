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
class HJNodeVEncoder : public HJMediaNode
{
public:
    using Ptr = std::shared_ptr<HJNodeVEncoder>;
    //
    HJNodeVEncoder(const HJEnvironment::Ptr& env = nullptr, const HJScheduler::Ptr& scheduler = nullptr, const bool isAuto = true);
    HJNodeVEncoder(const HJEnvironment::Ptr& env, const HJExecutor::Ptr& executor, const bool isAuto = true);
    virtual ~HJNodeVEncoder();
    
    virtual int init(const HJVideoInfo::Ptr& info, const HJDeviceType devType = HJDEVICE_TYPE_NONE);
    virtual void done() override;
    virtual int flush(const HJSeekInfo::Ptr info = nullptr) override;
public:
    virtual int proRun() override;
    virtual void run() override;

    const HJVideoInfo::Ptr& getInfo() {
        return m_info;
    }
private:
    HJVideoInfo::Ptr       m_info;
    HJBaseCodec::Ptr       m_coder;
    HJMediaFrame::Ptr       m_outFrame = nullptr;
};

NS_HJ_END

