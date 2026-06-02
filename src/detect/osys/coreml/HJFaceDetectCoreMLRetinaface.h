#pragma once

#include "HJPrerequisites.h"
#include "HJBaseFaceDetect.h"
#include "HJMediaUtils.h"

#include <memory>

NS_HJ_BEGIN

class HJTransferMediaData;

class HJFaceDetectCoreMLRetinaface : public HJBaseFaceDetect
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectCoreMLRetinaface);
    HJFaceDetectCoreMLRetinaface();
    virtual ~HJFaceDetectCoreMLRetinaface();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret) override;

private:
    int ensureModelReady(int width, int height);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

NS_HJ_END
