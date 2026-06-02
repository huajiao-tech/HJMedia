#pragma once

#include "HJPrerequisites.h"
#include "HJBaseFaceDetect.h"
#include "HJMediaUtils.h"

#include <memory>

NS_HJ_BEGIN

class HJTransferMediaData;

class HJFaceDetectIOSVisionRect : public HJBaseFaceDetect
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectIOSVisionRect);
    HJFaceDetectIOSVisionRect();
    virtual ~HJFaceDetectIOSVisionRect();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret) override;

private:
    std::shared_ptr<HJSPBuffer> m_scaledNv12Buffer;
    std::shared_ptr<HJSPBuffer> m_scaledRgbBuffer;
    void* m_request = nullptr;
};

NS_HJ_END
