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
class HJNCNNRetinafaceUtils;

class HJFaceDetectNCNNRetinaface : public HJBaseFaceDetect
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectNCNNRetinaface);
    HJFaceDetectNCNNRetinaface();
    virtual ~HJFaceDetectNCNNRetinaface();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret) override;

private:
    std::unique_ptr<ncnn::Net> m_net;
    std::unique_ptr<HJNCNNRetinafaceUtils> m_retinaface;
    //float m_probThreshold = 0.8f;
    //float m_nmsThreshold = 0.4f;
    //int m_numThreads = 8;
};

NS_HJ_END
