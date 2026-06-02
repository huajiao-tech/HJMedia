#pragma once

#include "VisualDataBase.h"

#include <string>

NS_HJ_BEGIN

class VisualDataImage : public VisualDataBase
{
public:
    HJ_DEFINE_CREATE(VisualDataImage);
    VisualDataImage() = default;
    virtual ~VisualDataImage();

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual int restart() override;
    virtual std::shared_ptr<cv::Mat> read() override;
};

NS_HJ_END
