#pragma once

#include "HJPrerequisites.h"
#include "HJBaseVideoSR.h"
#include "HJMediaUtils.h"

#include <memory>



NS_HJ_BEGIN

class HJTransferMediaData;
class HJNCNNRealCUGANUtils;
class HJSPBuffer;

class HJSRNCNNRealCUGAN : public HJBaseVideoSR
{
public:
    HJ_DEFINE_CREATE(HJSRNCNNRealCUGAN);
    HJSRNCNNRealCUGAN();
    virtual ~HJSRNCNNRealCUGAN();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int process(std::shared_ptr<HJTransferMediaData> i_data, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret) override;

private:
    const unsigned char* priPadBuffer(const unsigned char* rgb, int width, int height);

    std::shared_ptr<HJSPBuffer> m_padBuffer = nullptr;
    std::unique_ptr<HJNCNNRealCUGANUtils> m_realcuganUtils;
    std::string m_inputName;
    std::string m_outputName;
    int m_modelScale = 2;
    std::shared_ptr<HJSPBuffer> m_inputRgb;
};

NS_HJ_END
