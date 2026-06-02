#pragma once

#include "HJPrerequisites.h"
#include "HJBaseFaceDetect.h"
#include "HJMediaUtils.h"

namespace tnn
{
    class BlazeFaceDetector;
    class TNNSDKSample;
}

NS_HJ_BEGIN

class HJSPBuffer;
class HJTransferMediaData;

class HJFaceDetectTNNBlazeFace : public HJBaseFaceDetect
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectTNNBlazeFace);
    HJFaceDetectTNNBlazeFace();
    ~HJFaceDetectTNNBlazeFace() = default;

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret) override;

protected:
    std::shared_ptr<tnn::TNNSDKSample> initBlazeFace(HJBaseParam::Ptr params);
private: 

    

    std::shared_ptr<tnn::BlazeFaceDetector> m_detector = nullptr;

};

NS_HJ_END
