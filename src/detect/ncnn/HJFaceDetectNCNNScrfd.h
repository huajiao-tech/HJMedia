#pragma once

#include "HJPrerequisites.h"
#include "HJBaseFaceDetect.h"
#include "HJMediaUtils.h"

#include <memory>

namespace ncnn
{
    class Net;
}

NS_HJ_BEGIN

class HJTransferMediaData;
class HJNCNNScrfdUtils;

class HJFaceDetectNCNNScrfd : public HJBaseFaceDetect
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectNCNNScrfd);
    HJFaceDetectNCNNScrfd();
    virtual ~HJFaceDetectNCNNScrfd();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret) override;

private:
    std::unique_ptr<ncnn::Net> m_net;
    std::unique_ptr<HJNCNNScrfdUtils> m_scrfd;
};

NS_HJ_END
