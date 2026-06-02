#pragma once

#include "VisualDataBase.h"

#include <string>

NS_HJ_BEGIN

class VisualDataVideo : public VisualDataBase
{
public:
    HJ_DEFINE_CREATE(VisualDataVideo);
    VisualDataVideo() = default;
    ~VisualDataVideo() override;

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual int restart() override;
    virtual std::shared_ptr<cv::Mat> read() override;
private:
    std::shared_ptr<cv::VideoCapture> m_cap = nullptr;
};

NS_HJ_END
