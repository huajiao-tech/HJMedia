#pragma once

#include "VisualDataBase.h"

NS_HJ_BEGIN

class VisualDataCamera : public VisualDataBase
{
public:
    HJ_DEFINE_CREATE(VisualDataCamera);
    VisualDataCamera() = default;
    ~VisualDataCamera() override;

	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual int restart() override;
	virtual std::shared_ptr<cv::Mat> read() override;
private:
	std::shared_ptr<cv::VideoCapture> m_cap = nullptr;
	int m_width = 0;
	int m_height = 0;
};

NS_HJ_END
