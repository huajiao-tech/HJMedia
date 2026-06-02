#pragma once

#include "HJPrerequisites.h"
#include "HJFaceDetectTNNBlazeFace.h"
#include "HJMediaUtils.h"

namespace tnn
{
    class FaceDetectAligner;
    class TNNSDKSample;
}

NS_HJ_BEGIN

class HJSPBuffer;
class HJTransferMediaData;

class HJFaceDetectTNNAligner : public HJFaceDetectTNNBlazeFace
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectTNNAligner);
    HJFaceDetectTNNAligner();
    ~HJFaceDetectTNNAligner() = default;

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret) override;
private: 

    std::shared_ptr<tnn::TNNSDKSample> priInitAlignSDK(HJBaseParam::Ptr params, int i_phase);

    std::shared_ptr<tnn::FaceDetectAligner> m_detector = nullptr;

};

NS_HJ_END
