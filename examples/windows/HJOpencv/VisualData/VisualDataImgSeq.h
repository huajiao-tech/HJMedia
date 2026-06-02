#pragma once

#include "VisualDataBase.h"

#include <string>
#include <vector>

NS_HJ_BEGIN

class VisualDataImgSeq : public VisualDataBase
{
public:
    HJ_DEFINE_CREATE(VisualDataImgSeq);
    VisualDataImgSeq() = default;
    virtual ~VisualDataImgSeq();

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual int restart() override;
    virtual std::shared_ptr<cv::Mat> read() override;

private:
    std::vector<std::string> m_imgSeq;
	int m_curIdx = 0;
};

NS_HJ_END
