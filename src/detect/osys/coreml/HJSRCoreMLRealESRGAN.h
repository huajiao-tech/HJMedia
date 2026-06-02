#pragma once

#include "HJPrerequisites.h"
#include "HJBaseVideoSR.h"
#include "HJMediaUtils.h"

#include <memory>

NS_HJ_BEGIN

class HJTransferMediaData;

class HJSRCoreMLRealESRGAN : public HJBaseVideoSR
{
public:
    HJ_DEFINE_CREATE(HJSRCoreMLRealESRGAN);
    HJSRCoreMLRealESRGAN();
    virtual ~HJSRCoreMLRealESRGAN();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int process(std::shared_ptr<HJTransferMediaData> i_data, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret) override;

private:
    int ensureModelReady(int width, int height);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

NS_HJ_END
