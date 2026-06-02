#include "VisualDataVideo.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "HJFLog.h"

NS_HJ_BEGIN

VisualDataVideo::~VisualDataVideo()
{
    done();
}

int VisualDataVideo::init(HJBaseParam::Ptr i_param)
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
int VisualDataVideo::restart()
{
    int i_err = HJ_OK;
    do 
    {
		m_cap = std::make_shared<cv::VideoCapture>(m_url);
		if (!m_cap->isOpened())
		{
			HJFLoge("open video failed {}", m_url);
			return HJErrInvalidParams;
		}
    } while (false);
    return i_err;
}
std::shared_ptr<cv::Mat> VisualDataVideo::read()
{
	std::shared_ptr<cv::Mat> frame = std::make_shared<cv::Mat>();
	if (!m_cap->read(*frame))
	{
		m_cap = nullptr;
		return nullptr;
	}
    return frame;
}

NS_HJ_END
