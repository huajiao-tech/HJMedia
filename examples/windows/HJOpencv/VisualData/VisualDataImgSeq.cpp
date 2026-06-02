#include "VisualDataImgSeq.h"

#include <algorithm>
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "HJFLog.h"
#include "HJImgSeqInfo.h"

NS_HJ_BEGIN

VisualDataImgSeq::~VisualDataImgSeq()
{
    done();
}
int VisualDataImgSeq::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = VisualDataBase::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        HJImgSeqConfigInfo info;
        m_imgSeq = HJImgSeqParse::parseConfig(m_url, info);

        i_err = VisualDataBase::start();
        if (i_err < 0)
        {
            break;
		}
    } while (false);
    return i_err;
}
int VisualDataImgSeq::restart()
{
    int i_err = HJ_OK;
    do
    {
        m_curIdx = 0;
    } while (false);
    return i_err;
}
std::shared_ptr<cv::Mat> VisualDataImgSeq::read()
{
    if (m_imgSeq.empty())
    {
        return nullptr;
    }
    std::shared_ptr<cv::Mat> frame = std::make_shared<cv::Mat>();
    *frame = cv::imread(m_imgSeq[m_curIdx], cv::IMREAD_COLOR);
    m_curIdx++;
    if ((m_curIdx >= (int)m_imgSeq.size()) || frame->empty())
    {
        return nullptr;
    }
    return frame;
}

NS_HJ_END
