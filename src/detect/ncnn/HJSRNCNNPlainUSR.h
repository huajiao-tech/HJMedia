#pragma once

#include "HJPrerequisites.h"
#include "HJBaseVideoSR.h"
#include "HJMediaUtils.h"

#include <memory>

namespace ncnn
{
    class Net;
}

NS_HJ_BEGIN

class HJTransferMediaData;
class HJSPBuffer;

class HJSRNCNNPlainUSR : public HJBaseVideoSR
{
public:
    HJ_DEFINE_CREATE(HJSRNCNNPlainUSR);
    HJSRNCNNPlainUSR();
    virtual ~HJSRNCNNPlainUSR();

    virtual int init(HJBaseParam::Ptr params) override;
    virtual int process(std::shared_ptr<HJTransferMediaData> i_data, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret) override;

private:

};

NS_HJ_END
