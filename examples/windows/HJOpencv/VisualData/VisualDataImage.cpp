#include "VisualDataImage.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "HJFLog.h"

NS_HJ_BEGIN

VisualDataImage::~VisualDataImage()
{
    done();
}
int VisualDataImage::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = VisualDataBase::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        i_err = VisualDataBase::start();
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
int VisualDataImage::restart()
{
    int i_err = HJ_OK;
    do
    {
    } while (false);
    return i_err;
}
std::shared_ptr<cv::Mat> VisualDataImage::read()
{
    std::shared_ptr<cv::Mat> frame = std::make_shared<cv::Mat>();
    *frame = cv::imread(m_url, cv::IMREAD_COLOR);
    if (frame->empty())
    {
        return nullptr;
    }
    return frame;
}

NS_HJ_END
