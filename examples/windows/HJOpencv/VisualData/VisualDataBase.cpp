#include "VisualDataBase.h"

#include "VisualDataCamera.h"
#include "VisualDataImage.h"
#include "VisualDataVideo.h"
#include "VisualDataImgSeq.h"

#include "HJFLog.h"
#include "HJThreadPool.h"

#include <thread>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "HJTransferMediaData.h"
#include "HJTime.h"

NS_HJ_BEGIN

std::string VisualDataBase::s_paramUrl = "paramUrl";
std::string VisualDataBase::s_paramFps = "paramFps";
std::string VisualDataBase::s_paramLoop = "paramLoop";
std::string VisualDataBase::s_paramOutputType = "paramOutputType";
std::string VisualDataBase::s_paramWidth = "paramWidth";
std::string VisualDataBase::s_paramHeight = "paramHeight";

VisualDataBase::~VisualDataBase()
{
    HJFLogi("~VisualDataBase");
}

void VisualDataBase::done()
{
	if (m_timerPool)
	{
		HJFLogi("~VisualDataBase  m_timerPool->stopTimer() enter");
		m_timerPool->stopTimer();
		HJFLogi("~VisualDataBase  m_timerPool->stopTimer() end");
		m_timerPool = nullptr;
		HJFLogi("~VisualDataBase  timepool is nullptr");
	}
}

std::shared_ptr<VisualDataBase> VisualDataBase::createVisualData(VisualDataType i_type)
{
    VisualDataBase::Ptr visualData = nullptr;
    switch (i_type)
    {
    case VisualDataType::VisualDataType_Video:
        visualData = std::make_shared<VisualDataVideo>();
        break;
    case VisualDataType::VisualDataType_Image:
        visualData = std::make_shared<VisualDataImage>();
        break;
    case VisualDataType::VisualDataType_ImgSeq:
        visualData = std::make_shared<VisualDataImgSeq>();
        break;
    case VisualDataType::VisualDataType_Camera:
        visualData = std::make_shared<VisualDataCamera>();
        break;
    default:
        break;
    }
    if (visualData)
    {
		visualData->setDataType(i_type);
    }
    HJFLogi("createVisualData {}", VisualDataType_to_string(i_type));
    return visualData;
}
int VisualDataBase::priTryRestart()
{
    int i_err = HJ_OK;
    if (m_bRestart)
    {
        i_err = restart();
        if (i_err < 0)
        {
            HJFLoge("restart i_err:{}", i_err);
            return i_err;
        }
        m_bRestart = false;
    }
    return i_err;
}
int VisualDataBase::priRun()
{
    int i_err = HJ_OK;
    do
    {
        int64_t currentTime = HJCurrentSteadyMS();
        i_err = priTryRestart();
        if (i_err < 0)
        {
            HJFLoge("priTryRestart i_err:{}", i_err);
            break;
        }
        std::shared_ptr<cv::Mat> frame = read();
		if (!frame && m_bLoop)
		{		
            m_bRestart = true;
			i_err = priTryRestart();
			if (i_err < 0)
			{
				HJFLoge("priTryRestart i_err:{}", i_err);
				break;
			}
		}
        if (frame)
        {
			HJTransferMediaData::Ptr data = priCvtFrame(frame);
			if (data)
			{
				HJ_CatchMapPlainSetVal(data, int64_t, "ImgDecodeElapse", (HJCurrentSteadyMS() - currentTime));
				m_asyncCache.enqueue(std::move(data));
			}
        }
    } while (false);
	return i_err;
}
std::shared_ptr<HJTransferMediaData> VisualDataBase::acquire()
{
    return m_asyncCache.acquire();
}
void VisualDataBase::recovery(std::shared_ptr<HJTransferMediaData> i_data)
{
    m_asyncCache.recovery(i_data);
}
int VisualDataBase::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        HJ_CatchMapPlainGetVal(i_param, std::string, s_paramUrl, m_url);
        HJ_CatchMapPlainGetVal(i_param, int, s_paramFps, m_fps);
		HJ_CatchMapPlainGetVal(i_param, bool, s_paramLoop, m_bLoop);
        HJ_CatchMapPlainGetVal(i_param, HJConvertDataType, s_paramOutputType, m_outputType);

        if (m_url.empty())
        {
            HJFLoge("url is empty");
			return HJErrInvalidParams;
        }
        if (m_outputType == HJConvertDataType_None)
        {
			HJFLoge("outputType is invalid");
			return HJErrInvalidParams;
        }

        
    } while (false);
    return i_err;
}
int VisualDataBase::start()
{
	int i_err = HJ_OK;
    do
    {
        m_timerPool = HJTimerThreadPool::Create();
        m_timerPool->startTimer(1000 / m_fps, [this]
            {
                return priRun();
            });
    } while (false);
    return i_err;
}
std::shared_ptr<HJTransferMediaData> VisualDataBase::priCvtFrame(const std::shared_ptr<cv::Mat>& i_frame)
{
    std::shared_ptr<HJTransferMediaData> transferData = nullptr;
    do
    {   
        if (!i_frame)
        {
            break;
        }

        cv::Mat rgba;
        if (i_frame->channels() == 3)
        {
            cv::cvtColor(*i_frame, rgba, cv::COLOR_BGR2RGBA);
        }
        else if (i_frame->channels() == 4)
        {
            cv::cvtColor(*i_frame, rgba, cv::COLOR_BGRA2RGBA);
        }
        else if (i_frame->channels() == 1)
        {
            cv::cvtColor(*i_frame, rgba, cv::COLOR_GRAY2RGBA);
        }
        else
        {
            return nullptr;
        }
        unsigned char* data[4] = { rgba.data, nullptr, nullptr, nullptr };
        int stride[4] = { static_cast<int>(rgba.step), 0, 0, 0 };
        transferData = HJTransferMediaData::create(HJConvertDataType_RGBA, data, stride, rgba.cols, rgba.rows, HJCurrentSteadyMS(), m_outputType);
    } while (false);
    return transferData;
}

NS_HJ_END
